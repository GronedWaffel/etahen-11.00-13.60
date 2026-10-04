// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <stdio.h>

// Boot-local acknowledgement, never a persistent log entry. The bootstrapper
// clears it before spawning services; consumers also bind it to both live PIDs.
static constexpr const char* PORT_STARTUP_PATH = "/system_tmp/etahen-experimental-startup";
enum class PortStartupStatus : uint32_t { ToolboxReady=1, ToolboxDisabled=2, Failed=3 };
struct PortStartupRecord {
    uint32_t magic=0x45544152, version=1;
    int32_t critical=0, shellui=0;
    PortStartupStatus status=PortStartupStatus::Failed;
    // 0: pending/obsolete record; 1: ready; -1: this startup failed or UI died.
    int evaluate(int liveCritical, int liveShellui) const {
        if(magic!=0x45544152 || version!=1 || critical<=0 || critical!=liveCritical) return 0;
        if(status==PortStartupStatus::Failed) return -1;
        if(status!=PortStartupStatus::ToolboxReady && status!=PortStartupStatus::ToolboxDisabled) return 0;
        if(shellui<=0 || liveShellui<=0 || shellui!=liveShellui) return -1;
        return 1;
    }
};
static_assert(sizeof(PortStartupRecord)==20, "startup record ABI");
static inline bool port_read_startup(FILE* file, PortStartupRecord& out) {
    PortStartupRecord record;
    if(!file || fread(&record,1,sizeof(record),file)!=sizeof(record) || fgetc(file)!=EOF || ferror(file)) return false;
    out=record;return true;
}
