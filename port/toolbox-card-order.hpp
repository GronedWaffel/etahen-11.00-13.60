// SPDX-License-Identifier: GPL-3.0-or-later
// Adjust only our registered card, in one SQLite transaction. No raw DB edits,
// category changes, rescans, UI restarts or changes to any Sony application.
#pragma once
#include "../Source Code/include/sqlite3.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

static int toolbox_card_order(sqlite3 *db, int *old_priority, int *new_priority) {
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT t.sortPriority,s.sortPriority FROM tbl_contentinfo t "
        "JOIN tbl_contentinfo s ON s.titleId='NPXS40047' "
        "WHERE t.titleId='ETHN13600' AND t.titleName='etaHEN Toolbox' "
        "AND t.viewCategory='game' AND s.viewCategory='game'", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return rc;
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW || sqlite3_column_type(stmt,0) != SQLITE_INTEGER ||
        sqlite3_column_type(stmt,1) != SQLITE_INTEGER) {
        sqlite3_finalize(stmt); return SQLITE_NOTFOUND;
    }
    *old_priority = sqlite3_column_int(stmt,0);
    int store = sqlite3_column_int(stmt,1);
    sqlite3_finalize(stmt);
    if (store <= 1 || store >= 100) return SQLITE_RANGE;
    *new_priority = store - 1;
    return SQLITE_OK;
}

static int pin_toolbox_card() {
    sqlite3 *db = nullptr;
    int rc = sqlite3_open_v2("/system_data/priv/mms/app.db", &db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (rc != SQLITE_OK) { if(db)sqlite3_close(db); return -EIO; }
    sqlite3_busy_timeout(db, 1000);
    rc = sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) { sqlite3_close(db); return -EBUSY; }
    int before=0, after=0;
    rc = toolbox_card_order(db, &before, &after);
    if (rc == SQLITE_OK && before != after) {
        // Save exactly the value we own before the first modification. Removing
        // the card remains the normal rollback; this also supports manual restore.
        const char *backup="/data/etaHEN/toolbox-card-order-original.json";
        if (access(backup,F_OK) != 0) {
            FILE *f=fopen(backup,"wx");
            if (!f) rc=SQLITE_IOERR;
            else {
                bool ok=fprintf(f,"{\"titleId\":\"ETHN13600\",\"sortPriority\":%d}\n",before)>0;
                if(fflush(f)||fsync(fileno(f)))ok=false;
                if(fclose(f))ok=false;
                if(!ok){unlink(backup);rc=SQLITE_IOERR;}
            }
        }
        sqlite3_stmt *stmt=nullptr;
        if(rc==SQLITE_OK)rc=sqlite3_prepare_v2(db,
            "UPDATE tbl_contentinfo SET sortPriority=? WHERE titleId='ETHN13600' AND sortPriority=?",
            -1,&stmt,nullptr);
        if(rc==SQLITE_OK){sqlite3_bind_int(stmt,1,after);sqlite3_bind_int(stmt,2,before);
            rc=sqlite3_step(stmt)==SQLITE_DONE && sqlite3_changes(db)==1?SQLITE_OK:SQLITE_ERROR;}
        if(stmt)sqlite3_finalize(stmt);
        int verify=0,wanted=0;
        if(rc==SQLITE_OK && (toolbox_card_order(db,&verify,&wanted)!=SQLITE_OK || verify!=after))rc=SQLITE_ERROR;
    }
    if(rc==SQLITE_OK)rc=sqlite3_exec(db,"COMMIT",nullptr,nullptr,nullptr);
    if(rc!=SQLITE_OK)sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);
    sqlite3_close(db);
    return rc==SQLITE_OK?0:-EIO;
}
