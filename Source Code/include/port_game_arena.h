// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <stddef.h>

struct PortGameArenaProtection {
    int transition=-1,code=-1,data=-1;
};

// The SDK's kernel_mprotect edits entire VM entries, without splitting them or
// updating page tables. Code and counters initially share one RW mmap entry.
// Use the target's real mprotect syscall to split that entry and apply RX to
// only the first 16 KiB. Refuse publication if the platform rejects it; never
// fall back to a metadata-only permission change or to an RWX arena.
template<class Protect,class Query>
bool PortSealGameArena(int pid,uint64_t base,Protect protect,Query query,
                      PortGameArenaProtection& observed) {
    observed.transition=protect(pid,base,size_t(0x4000),5); // READ | EXEC
    observed.code=query(pid,base,size_t(0x4000));
    observed.data=query(pid,base+0x4000,size_t(0x4000));
    return observed.transition==0 && observed.code==5 && observed.data==3;
}
