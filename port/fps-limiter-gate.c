// SPDX-License-Identifier: GPL-3.0-or-later
// Relocation-free code copied into the selected game's private RX mapping.
// No imports, TLS, file access, allocation or process-control operations.
#include "../Source Code/include/port_fps_limiter.h"
__attribute__((section(".text.gate"),noinline))
void FPS_SYSV fps_limiter_gate(FpsLimiterControl* c,unsigned lane) {
    if(lane>1)return;
    __atomic_fetch_add(&c->calls[lane],1,__ATOMIC_RELAXED);
    if(__atomic_load_n(&c->lane,__ATOMIC_RELAXED)!=lane+1)return;
    unsigned expected=0;
    if(!__atomic_compare_exchange_n(&c->busy,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    LimiterClock clock=(LimiterClock)(uintptr_t)c->clock_fn;
    LimiterSleep nap=(LimiterSleep)(uintptr_t)c->sleep_fn;
    LimiterTime t;
    if(!clock || !nap || clock(c->clock_id,&t) || t.sec<0 || t.nsec<0 || t.nsec>=1000000000)goto fail;
    uint64_t now=(uint64_t)t.sec*1000000000+(uint64_t)t.nsec;
    uint64_t lease=__atomic_load_n(&c->lease_until,__ATOMIC_ACQUIRE);
    // A dead controller/daemon cannot leave the game throttled indefinitely.
    if(now>=lease || lease-now>FPS_LIMITER_LEASE_NS){c->last_ns=0;goto done;}
    if(c->last_ns && now>=c->last_ns && now-c->last_ns<FPS_LIMITER_PERIOD_NS){
        const uint64_t deadline=c->last_ns+FPS_LIMITER_PERIOD_NS;
        // Bounded retries for interrupted sleep. Never accumulate catch-up debt.
        for(unsigned retry=0;retry<3 && now<deadline;++retry){
            lease=__atomic_load_n(&c->lease_until,__ATOMIC_ACQUIRE);
            if(now>=lease || lease-now>FPS_LIMITER_LEASE_NS){c->last_ns=0;goto done;}
            if(nap((unsigned)((deadline-now+999)/1000)))goto fail;
            __atomic_fetch_add(&c->waits,1,__ATOMIC_RELAXED);
            if(clock(c->clock_id,&t) || t.sec<0 || t.nsec<0 || t.nsec>=1000000000)goto fail;
            uint64_t next=(uint64_t)t.sec*1000000000+(uint64_t)t.nsec;
            if(next<now)goto fail;
            now=next;
        }
    }
    c->last_ns=now;
    goto done;
fail:
    c->last_ns=0;
    __atomic_fetch_add(&c->failures,1,__ATOMIC_RELAXED);
done:
    __atomic_store_n(&c->busy,0,__ATOMIC_RELEASE);
}
