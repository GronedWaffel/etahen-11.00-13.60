// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "port_plugin.h"
#define PORT_PRX_MAX 8
static inline int port_prx_suffix(const char* path){return port_plugin_suffix(path,".prx")||port_plugin_suffix(path,".sprx");}
// Validate a native dynamic module, not an ELF executable renamed .prx.
// A SELF may contain compressed/encrypted segments; its declared container
// ranges are checked here, while the console loader validates/authenticates it.
// platform: 0=plain ELF (loader decides), 4=Orbis SELF, 5=Prospero SELF.
static inline int port_prx_parse(const void* data,size_t available,size_t total,int* platform){
    if(!data||!platform||available>total||total<64||total>PORT_PLUGIN_MAX_SIZE)return 0;
    const unsigned char* p=(const unsigned char*)data;size_t off=0;*platform=0;
    if(available<64)return 0;
    if(!memcmp(p,"\x4f\x15\x3d\x1d",4)||!memcmp(p,"\x54\x14\xf5\xee",4)){
        *platform=p[0]==0x4f?4:5;
        unsigned count=port_plugin_u16(p+24);off=32+(size_t)count*32;
        if(!count||count>128||off>total-64||off+64>available||port_plugin_u64(p+16)!=total)return 0;
        for(unsigned i=0;i<count;i++){const unsigned char* s=p+32+i*32;uint64_t at=port_plugin_u64(s+8),len=port_plugin_u64(s+16);if(at>total||len>total-at)return 0;}
    }
    const unsigned char* e=p+off;
    if(memcmp(e,"\177ELF\2\1\1",7)||port_plugin_u16(e+18)!=62||port_plugin_u16(e+52)!=64||port_plugin_u16(e+54)!=56)return 0;
    unsigned type=port_plugin_u16(e+16),count=port_plugin_u16(e+56);
    if(type!=0xfe18 || !count || count>128)return 0;
    uint64_t ph=port_plugin_u64(e+32);
    if(ph<64||ph>total-off||count*56ull>total-off-ph||ph>available-off||count*56ull>available-off-ph)return 0;
    bool exec=false,dynamic=false;
    for(unsigned i=0;i<count;i++){const unsigned char* s=e+ph+i*56;
        uint64_t type32=(uint64_t)s[0]|((uint64_t)s[1]<<8)|((uint64_t)s[2]<<16)|((uint64_t)s[3]<<24);
        uint64_t at=port_plugin_u64(s+8),len=port_plugin_u64(s+32),mem=port_plugin_u64(s+40),va=port_plugin_u64(s+16);
        if(type32==1){if(len>mem||mem>UINT64_MAX-va)return 0;if(!off&&(at>total||len>total-at))return 0;if(s[4]&1)exec=true;}
        if(type32==2||type32==0x61000000)dynamic=true;
    }
    return exec&&dynamic;
}
typedef struct {uint32_t enabled,state;int32_t handle,start_result;char path[256];} PrxSlot;
typedef struct {uint64_t load_fn;uint32_t busy,reserved;PrxSlot slots[PORT_PRX_MAX];} PrxControl;
