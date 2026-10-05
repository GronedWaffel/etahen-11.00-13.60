// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "relocate.h"
#include <string.h>
#include <limits.h>
// FF 25 jumps read an inline pointer: their entry page must remain readable.
// Preserve unrelated protection bits, but never leave publication write access.
constexpr int FpsCounterEntryProtection(int original,int read,int write) {
    return (original|read)&~write;
}
struct FpsCounterStub {unsigned char bytes[272];size_t size,stolen;};
inline bool BuildFpsCounterStub(const unsigned char* original,size_t available,
    uint64_t source,uint64_t destination,uint64_t counter,FpsCounterStub* out) {
    if(!out)return false;
    const int64_t distance=(int64_t)counter-(int64_t)(destination+9);
    if(distance<INT32_MIN || distance>INT32_MAX)return false;
    RelocatedCode code{};
    if(!BuildTrampoline(original,available,source,destination+10,&code))return false;
    // Preserve flags and all argument registers, count exactly one flip call.
    const unsigned char prefix[]={0x9c,0xf0,0x48,0xff,0x05,0,0,0,0,0x9d};
    memcpy(out->bytes,prefix,sizeof(prefix));int32_t delta=(int32_t)distance;
    memcpy(out->bytes+5,&delta,4);memcpy(out->bytes+10,code.bytes,code.size);
    out->size=10+code.size;out->stolen=code.stolen;return true;
}
