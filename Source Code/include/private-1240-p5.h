// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <stddef.h>
#define P5_MAGIC UINT64_C(0x124050354a524e4c)
#define P5_OFFSET 0x3500
#define P5_CAPACITY 256
#define P5_TARGET_PATH "/data/etaHEN/experimental-diagnostics/private-1240-p5-target.bin"
typedef struct P5Record {uint64_t sequence,ticks,value;int32_t error;char text[196];} P5Record;
typedef struct P5Journal {
 uint64_t magic,guard,next;
 uint32_t lock,dropped,ready,resources,toolbox_resources,root_requests,root_returns,root_failures,shortcuts;
 P5Record records[P5_CAPACITY];
} P5Journal;
typedef struct P5Pointer {uint64_t magic,address;} P5Pointer;
typedef struct P5Target {uint64_t magic;int32_t pid;uint32_t reserved;uint64_t address;} P5Target;
#ifdef __cplusplus
static_assert(P5_OFFSET+sizeof(P5Pointer)<=0x4000);
void P5Init();
void P5Event(const char*,uint64_t value=0,int error=0);
void P5Ready();
enum P5Counter {P5Resources,P5ToolboxResources,P5RootRequests,P5RootReturns,P5RootFailures,P5Shortcuts};
void P5Count(P5Counter);
#endif
