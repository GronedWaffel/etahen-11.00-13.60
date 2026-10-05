// SPDX-License-Identifier: GPL-3.0-or-later
#include "../Source Code/include/port_fps_limiter.h"
// Keep the copied gate independent of validation/libc headers and imports.
typedef struct {uint32_t enabled,state;int32_t handle,start_result;char path[256];} PrxSlot;
typedef struct {uint64_t load_fn;uint32_t busy,reserved;PrxSlot slots[8];} PrxControl;
typedef int (FPS_SYSV *LoadModule)(const char*,uint64_t,const void*,unsigned,const void*,int*);
__attribute__((section(".text.gate"),noinline))
void FPS_SYSV prx_load_gate(PrxControl* c,unsigned unused){
    (void)unused;unsigned expected=0;
    if(!c->load_fn||!__atomic_compare_exchange_n(&c->busy,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    for(unsigned i=0;i<8;++i){PrxSlot* slot=&c->slots[i];expected=0;
        if(!__atomic_load_n(&slot->enabled,__ATOMIC_ACQUIRE)||!__atomic_compare_exchange_n(&slot->state,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_RELAXED))continue;
        int result=0;
        int handle=((LoadModule)(uintptr_t)c->load_fn)(slot->path,0,0,0,0,&result);
        slot->handle=handle;slot->start_result=result;
        __atomic_store_n(&slot->state,handle>=0&&result>=0?2u:3u,__ATOMIC_RELEASE);
        break; // At most one initializer per game input call.
    }
    __atomic_store_n(&c->busy,0,__ATOMIC_RELEASE);
}
