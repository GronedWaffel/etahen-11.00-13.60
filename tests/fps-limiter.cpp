#include "../Source Code/include/port_fps_limiter.h"
#include "fps_limiter_stub.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
static uint64_t now=1000000000, slept=0;static int clockError=0,sleepError=0,sleeps=0;
static int FPS_SYSV fakeClock(int id,LimiterTime* t){assert(id==4);t->sec=now/1000000000;t->nsec=now%1000000000;return clockError;}
static int FPS_SYSV fakeSleep(unsigned us){assert(us<=66667);++sleeps;slept+=us;now+=(uint64_t)us*1000;return sleepError;}
int main(int argc,char** argv){
    assert(argc==2);std::ifstream f(argv[1],std::ios::binary);assert(f);std::vector<unsigned char> code((std::istreambuf_iterator<char>(f)),{});assert(!code.empty());
#ifdef _WIN32
    auto memory=(unsigned char*)VirtualAlloc(nullptr,16384,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(memory);
    auto entry=memory,stub=memory+4096,gate=memory+8192;
    auto c=(FpsLimiterControl*)(memory+12288);
    c->clock_fn=(uintptr_t)fakeClock;c->sleep_fn=(uintptr_t)fakeSleep;c->clock_id=4;
    c->lane=1;c->lease_until=now+FPS_LIMITER_LEASE_NS;
    memcpy(gate,code.data(),code.size());
    DWORD old;assert(VirtualProtect(gate,4096,PAGE_EXECUTE_READ,&old));
    using Gate=void(FPS_SYSV *)(FpsLimiterControl*,unsigned);auto invoke=(Gate)gate;
    invoke(c,0);assert(sleeps==0&&c->calls[0]==1);
    now+=10000000;invoke(c,0);assert(sleeps==1&&slept==56667&&c->last_ns==now);
    auto before=now;invoke(c,1);assert(now==before&&c->calls[1]==1); // No nested double cap.
    c->lease_until=0;invoke(c,0);assert(now==before&&c->last_ns==0); // Stop.
    c->lease_until=now+FPS_LIMITER_LEASE_NS+1;invoke(c,0);assert(now==before); // Corrupt lease.
    c->lease_until=now+FPS_LIMITER_LEASE_NS;invoke(c,0);assert(now==before); // Restart, no old debt.
    now+=200000000;invoke(c,0);assert(sleeps==1); // Slow frame, no artificial extra delay.
    clockError=1;invoke(c,0);assert(c->failures==1&&c->busy==0);clockError=0;
    invoke(c,0);sleepError=1;invoke(c,0);assert(c->failures==2&&c->busy==0);sleepError=0;
    c->busy=1;before=now;invoke(c,0);assert(now==before);c->busy=0;
    c->lease_until=0;
    // Run the real generated wrapper with six register arguments and two stack
    // arguments. The helper uses/clobbers argument registers before restoration.
    const unsigned char original[]={0x48,0x89,0xf8,0x48,0x01,0xf0,0x48,0x01,0xd0,0x48,0x01,0xc8,0x4c,0x01,0xc0,0x4c,0x01,0xc8,0x48,0x03,0x44,0x24,0x08,0x48,0x03,0x44,0x24,0x10,0xc3};
    memset(entry,0x90,64);memcpy(entry,original,sizeof(original));FpsLimiterStub hook{};
    assert(BuildFpsLimiterStub(entry,64,(uintptr_t)entry,(uintptr_t)stub,(uintptr_t)c,(uintptr_t)gate,0,&hook));
    memcpy(stub,hook.bytes,hook.size);entry[0]=0xff;entry[1]=0x25;memset(entry+2,0,4);auto dest=(uint64_t)stub;memcpy(entry+6,&dest,8);
    assert(VirtualProtect(memory,8192,PAGE_EXECUTE_READ,&old));
    using Function=uint64_t(FPS_SYSV *)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
    auto calls=c->calls[0];assert(((Function)entry)(1,2,4,8,16,32,64,128)==255);assert(c->calls[0]==calls+1);
    c->lease_until=now+FPS_LIMITER_LEASE_NS;assert(((Function)entry)(2,3,5,7,11,13,17,19)==77);
    assert(((Function)entry)(1,2,4,8,16,32,64,128)==255);assert(c->waits>1);
    VirtualFree(memory,0,MEM_RELEASE);
#endif
    puts("limiter code, bounded pacing, Stop/restart and argument preservation passed");
}
