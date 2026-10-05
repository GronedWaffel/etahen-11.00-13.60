/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ETAHEN_PORT_PLUGIN_RUNTIME_H
#define ETAHEN_PORT_PLUGIN_RUNTIME_H
#include "port_plugin.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* A stale receipt is not authority to kill a process. Verify its current name. */
static inline int port_plugin_running(const char *identity,
                                      int (*process_name)(int, char *)) {
    char receipt[96], text[32] = {0}, name[32] = {0};
    snprintf(receipt, sizeof(receipt), "/system_tmp/%s.PID", identity);
    int fd = open(receipt, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, text, sizeof(text)-1);
    close(fd);
    char *end = NULL;
    long pid = n > 0 ? strtol(text, &end, 10) : -1;
    if (pid <= 1 || pid > 0x7fffffff || !end || *end ||
        process_name((int)pid, name) < 0 || strncmp(name, identity, sizeof(name))) {
        unlink(receipt);
        return -1;
    }
    return (int)pid;
}

static inline int port_plugin_load(const char *path, int output,
    int (*process_name)(int, char *),
    pid_t (*spawn)(const char *, int, uint8_t *, const char *)) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    struct stat st;
    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 64 ||
        (uint64_t)st.st_size > PORT_PLUGIN_MAX_SIZE) { close(fd); return 0; }
    size_t size = (size_t)st.st_size, done = 0;
    uint8_t *buf = (uint8_t *)malloc(size);
    if (!buf) { close(fd); return 0; }
    while (done < size) {
        ssize_t n = read(fd, buf+done, size-done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        done += (size_t)n;
    }
    close(fd);
    PortPlugin info;
    if (done != size || !port_plugin_parse(path, buf, size, size, &info)) {
        printf("Plugin rejected: invalid or truncated image\n");
        free(buf); return 0;
    }
    char lockpath[96], receipt[96];
    snprintf(lockpath, sizeof(lockpath), "/system_tmp/%s.plugin-lock", info.identity);
    snprintf(receipt, sizeof(receipt), "/system_tmp/%s.PID", info.identity);
    int lockfd = open(lockpath, O_RDWR | O_CREAT, 0600);
    if (lockfd < 0 || flock(lockfd, LOCK_EX | LOCK_NB)) {
        if (lockfd >= 0) close(lockfd);
        free(buf); return 0;
    }
    int pid = port_plugin_running(info.identity, process_name);
    int ok = pid > 1;
    if (!ok) {
        pid = spawn("/", output, buf + info.elf_offset, info.identity);
        ok = pid > 1;
        if (ok) {
            char text[32];
            int len = snprintf(text, sizeof(text), "%d", pid);
            fd = open(receipt, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd >= 0) {
                if (write(fd, text, (size_t)len) != len) unlink(receipt);
                close(fd);
            }
        }
    }
    printf("Plugin %s: %s (pid %d)\n", info.identity, ok ? "running" : "launch failed", pid);
    flock(lockfd, LOCK_UN); close(lockfd);
    free(buf);
    return ok;
}
#endif
