// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "relocate.h"
#include <string.h>
struct FpsLimiterStub { unsigned char bytes[512];size_t size,stolen; };
inline bool BuildFpsLimiterStub(const unsigned char* before,size_t available,
    uint64_t target,uint64_t stub,uint64_t control,uint64_t gate,unsigned lane,FpsLimiterStub* out){
    if(!out || lane>1)return false;
    // Save the complete incoming integer argument state and flags. Align a
    // private FXSAVE area for the helper call, then tail-jump with the original
    // stack (including stack arguments) into relocated code.
    const unsigned char save[]={0x9c,0x50,0x51,0x52,0x56,0x57,0x41,0x50,0x41,0x51,0x41,0x52,0x41,0x53,0x55,
        0x48,0x89,0xe5,0x48,0x83,0xe4,0xf0,0x48,0x81,0xec,0,2,0,0,0x48,0x0f,0xae,0x04,0x24};
    size_t n=0;memcpy(out->bytes,save,sizeof(save));n+=sizeof(save);
    out->bytes[n++]=0x48;out->bytes[n++]=0xbf;memcpy(out->bytes+n,&control,8);n+=8;
    out->bytes[n++]=0xbe;memcpy(out->bytes+n,&lane,4);n+=4;
    out->bytes[n++]=0x48;out->bytes[n++]=0xb8;memcpy(out->bytes+n,&gate,8);n+=8;
    const unsigned char restore[]={0xff,0xd0,0x48,0x0f,0xae,0x0c,0x24,0x48,0x89,0xec,0x5d,
        0x41,0x5b,0x41,0x5a,0x41,0x59,0x41,0x58,0x5f,0x5e,0x5a,0x59,0x58,0x9d};
    memcpy(out->bytes+n,restore,sizeof(restore));n+=sizeof(restore);
    RelocatedCode code{};if(!BuildTrampoline(before,available,target,stub+n,&code))return false;
    if(n+code.size>sizeof(out->bytes))return false;
    memcpy(out->bytes+n,code.bytes,code.size);out->size=n+code.size;out->stolen=code.stolen;return true;
}
