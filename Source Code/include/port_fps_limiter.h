// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#define FPS_LIMITER_PLUGIN_ID "SNPL00015"
#define FPS_LIMITER_TITLE "PPSA04264"
#define FPS_LIMITER_PERIOD_NS 66666667ull
#define FPS_LIMITER_LEASE_NS 1000000000ull
#if defined(_WIN32) && defined(__clang__)
#define FPS_SYSV __attribute__((sysv_abi))
#else
#define FPS_SYSV
#endif
typedef struct { int64_t sec, nsec; } LimiterTime;
typedef int (FPS_SYSV *LimiterClock)(int,LimiterTime*);
typedef int (FPS_SYSV *LimiterSleep)(unsigned);
typedef struct {
    uint64_t lease_until; // Written only by the daemon, expires on Stop/failure.
    uint64_t clock_fn, sleep_fn;
    uint64_t last_ns;
    uint64_t calls[2];
    uint64_t waits, failures;
    uint32_t lane, busy;
    int32_t clock_id;
    uint32_t reserved;
} FpsLimiterControl;
#ifdef __cplusplus
extern "C" {
#endif
void FPS_SYSV fps_limiter_gate(FpsLimiterControl*,unsigned);
#ifdef __cplusplus
}
#endif
