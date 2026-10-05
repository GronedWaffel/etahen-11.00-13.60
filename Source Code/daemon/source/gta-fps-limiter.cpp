// SPDX-License-Identifier: GPL-3.0-or-later
// Private GTA V test plugin backend. All publication is serialized by jb_lock.
#include "kernel.hpp"
#include "port_kstuff.hpp"
#include "port_plugin_runtime.h"
#include "port_fps_limiter.h"
#include "experimental_trace.h"
#include "../../shellui/include/fps_counter_stub.h"
#include "../../shellui/include/fps_limiter_stub.h"
#include "../../../build/fps-limiter/gate.h"
#include <sys/mman.h>
#include <vector>
#include <string>
#include <time.h>
extern "C" {
#include "../../libNineS/include/pt.h"
int pt_getlwps(pid_t,int*,size_t);
int sceKernelGetProcessName(int,char*);
}
extern int get_game_pid();
extern void notify(bool,const char*,...);
namespace {
struct Hook {uint64_t target=0,stub=0;int original=-1;bool published=false;unsigned char before[64]{},patch[14]{};FpsLimiterStub code{};};
struct Session {int pid,app;uint64_t control=0,started=0,measure=0;bool failed=false,armed=false,announced=false;unsigned lane=0;};
std::vector<Session> sessions;
uint64_t mono(){timespec t{};return clock_gettime(CLOCK_MONOTONIC,&t)?0:uint64_t(t.tv_sec)*1000000000+t.tv_nsec;}
uint64_t resolve(int pid,const char* module,const char* nid){
    auto p=getProc(pid);if(!p)return 0;auto o=p->getSharedObject();if(!o)return 0;
    for(auto lib:o->getLibs()){
        if(!lib->getPath().contains(module))continue;
        auto m=lib->getMetaData();if(!m)continue;const auto& symbols=m->getSymbolTable();
        for(size_t i=0;i<symbols.length();++i){auto s=symbols[i];if(!s.exported())continue;auto n=s.name();
            if(n.length()>=11&&!memcmp(n.c_str(),nid,11)&&(n.length()==11||n[11]=='#'))return s.vaddr();}
    }return 0;
}
struct Stop {int pid;bool attached;Stop(int p):pid(p),attached(pt_attach(p)==0){}~Stop(){if(attached)pt_detach(pid,0);}bool resume(){if(!attached)return false;if(pt_detach(pid,0))return false;attached=false;return true;}};
bool install(Session& s,const char** stage){
    *stage="GTA limiter resolving VideoOut flip exports and kernel clock";
    Hook hooks[2];
    hooks[0].target=resolve(s.pid,"libSceVideoOut","j8xl+92A0q4"); // sceVideoOutSubmitEopFlip
    hooks[1].target=resolve(s.pid,"libSceVideoOut","U46NwOiJpys"); // sceVideoOutSubmitFlip
    if(hooks[1].target==hooks[0].target)hooks[1].target=0;
    FpsLimiterControl control{};
    control.clock_fn=resolve(s.pid,"libkernel","QBi7HCK03hw"); // sceKernelClockGettime
    control.sleep_fn=resolve(s.pid,"libkernel","1jfXLRVzisc"); // sceKernelUsleep
    control.clock_id=CLOCK_MONOTONIC;
    if((!hooks[0].target&&!hooks[1].target)||!control.clock_fn||!control.sleep_fn)return false;
    *stage="GTA limiter checking kstuff";if(!port_kstuff_injection_ready())return false;
    *stage="GTA limiter stopping target";Stop stop(s.pid);if(!stop.attached)return false;
    *stage="GTA limiter allocating private code and data";
    intptr_t base=pt_mmap(s.pid,0,0x8000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(base<=0 || base==(intptr_t)MAP_FAILED)return false;
    struct Mapping {int pid;intptr_t base;bool keep=false;~Mapping(){if(!keep)pt_munmap(pid,base,0x8000);}} mapping{s.pid,base};
    const uint64_t data=base+0x4000;
    *stage="GTA limiter preparing and relocating entries";
    int threads[2048],count=pt_getlwps(s.pid,threads,2048);if(count<=0)return false;
    for(unsigned lane=0;lane<2;++lane){auto& h=hooks[lane];if(!h.target)continue;
        h.stub=base+0x1000+lane*0x800;
        if(pt_copyout(s.pid,h.target,h.before,sizeof(h.before)))return false;
        if((h.before[0]==0xff&&h.before[1]==0x25)||h.before[0]==0xe9||h.before[0]==0xeb){*stage="GTA limiter refused existing or forwarded entry hook";return false;}
        h.original=kernel_get_vmem_protection(s.pid,h.target,14);if(h.original<0||!(h.original&PROT_EXEC))return false;
        if(!BuildFpsLimiterStub(h.before,sizeof(h.before),h.target,h.stub,data,base,lane,&h.code))return false;
        for(int i=0;i<count;++i){reg r{};if(pt_getregs(threads[i],&r))return false;if(r.r_rip>=h.target&&r.r_rip<h.target+h.code.stolen){*stage="GTA limiter deferred: thread in entry";return false;}}
        h.patch[0]=0xff;h.patch[1]=0x25;memcpy(h.patch+6,&h.stub,8);
        if(pt_copyin(s.pid,h.code.bytes,h.stub,h.code.size))return false;
    }
    *stage="GTA limiter writing helper and disabled control";
    if(pt_copyin(s.pid,limiter_gate_bytes,base,sizeof(limiter_gate_bytes))||pt_copyin(s.pid,&control,data,sizeof(control))||kernel_mprotect(s.pid,base,0x4000,PROT_READ|PROT_EXEC))return false;
    bool ok=true;unsigned char check[14];
    *stage="GTA limiter publishing verified private RX entries";
    for(auto& h:hooks){if(!h.target)continue;
        if(pt_copyout(s.pid,h.target,check,14)||memcmp(check,h.before,14)||kernel_mprotect(s.pid,h.target,14,h.original|PROT_READ|PROT_WRITE)){ok=false;break;}
        // A failed write may be partial. Keep code mapped until rollback verifies.
        h.published=true;mapping.keep=true;
        const int rx=FpsCounterEntryProtection(h.original,PROT_READ,PROT_WRITE);
        if(pt_copyin(s.pid,h.patch,h.target,14)||kernel_mprotect(s.pid,h.target,14,rx)||kernel_get_vmem_protection(s.pid,h.target,14)!=rx||pt_copyout(s.pid,h.target,check,14)||memcmp(check,h.patch,14)){ok=false;break;}
    }
    if(!ok){
        *stage="GTA limiter publication failed; restoring original entries";
        bool all=true;for(auto& h:hooks){if(!h.published)continue;bool restored=false;
            for(int retry=0;retry<2&&!restored;++retry){
                if(kernel_mprotect(s.pid,h.target,14,h.original|PROT_READ|PROT_WRITE))continue;
                int w=pt_copyin(s.pid,h.before,h.target,14);int p=kernel_mprotect(s.pid,h.target,14,h.original);
                restored=!w&&!p&&!pt_copyout(s.pid,h.target,check,14)&&!memcmp(check,h.before,14);
            }all=all&&restored;
        }
        mapping.keep=!all;if(!all)*stage="GTA limiter rollback incomplete; retained disabled helper";return false;
    }
    s.control=data;mapping.keep=true;
    *stage="GTA limiter resuming game";if(!stop.resume())return false;
    *stage="GTA limiter hooks installed; observing which flip path runs";return true;
}
bool field(Session& s,size_t offset,const void* value,size_t length){return kernel_proc_copyin(s.pid,value,s.control+offset,length)==0;}
}
// Only GTA V PPSA04264 can receive these hooks. Stopping the .plugin stops the
// lease; remote code bypasses pacing within one second even if this poll stalls.
void port_poll_gta_fps_limiter(const std::string& title,int app){
    if(title!=FPS_LIMITER_TITLE)return;
    const bool armed=port_plugin_running(FPS_LIMITER_PLUGIN_ID,sceKernelGetProcessName)>1;
    if(!armed && sessions.empty())return;
    const int pid=get_game_pid();if(pid<=1)return;const uint64_t now=mono();if(!now)return;
    Session* session=nullptr;for(auto& s:sessions)if(s.pid==pid&&s.app==app){session=&s;break;}
    if(!session){if(!armed||sessions.size()>=64)return;sessions.push_back(Session{pid,app,0,now});session=&sessions.back();}
    auto& s=*session;if(s.failed)return;
    if(!armed){if(s.armed&&s.control){uint64_t zero=0;field(s,offsetof(FpsLimiterControl,lease_until),&zero,8);notify(true,"GTA V 15 FPS test limiter off");}s.armed=false;s.announced=false;return;}
    if(!s.control){if(now-s.started<2000000000ull)return;const char* stage=nullptr;bool ok=install(s,&stage);
        experimental_event("fps-limiter",stage,pid,ok?0:errno);
        if(!ok){s.failed=true;notify(true,"GTA V limiter could not install; diagnostic saved. FPS display is separate.");return;}
        s.measure=now;
    }
    FpsLimiterControl observed{};if(kernel_proc_copyout(pid,s.control,&observed,sizeof(observed)))return;
    if(!s.lane){
        // Probe without delaying frames. Prefer EOP when it is actually used;
        // enabling just one lane avoids throttling nested submissions twice.
        if(now-s.measure<1000000000ull)return;
        if(observed.calls[0]>=2)s.lane=1;else if(observed.calls[1]>=2)s.lane=2;
        else {if(now-s.measure>15000000000ull&&!s.announced){notify(true,"GTA limiter waiting: no supported flip calls observed yet");s.announced=true;}return;}
        if(!field(s,offsetof(FpsLimiterControl,lane),&s.lane,4)){s.lane=0;return;}
        experimental_event("fps-limiter","selected observed VideoOut flip lane",s.lane,0);
    }
    if(observed.failures){uint64_t zero=0;field(s,offsetof(FpsLimiterControl,lease_until),&zero,8);s.failed=true;
        notify(true,"GTA limiter disabled: clock/sleep failed; diagnostic saved");experimental_event("fps-limiter","remote clock or sleep failure; stopped lease",observed.failures,0);return;}
    const uint64_t fresh=mono();if(!fresh)return;
    const uint64_t lease=fresh+FPS_LIMITER_LEASE_NS;if(!field(s,offsetof(FpsLimiterControl,lease_until),&lease,8))return;
    if(!s.armed){s.armed=true;s.announced=false;notify(true,"GTA V 15 FPS test limiter enabled. Stop the plugin to restore normal speed.");}
    if(!s.announced&&observed.waits){s.announced=true;experimental_event("fps-limiter","remote frame pacing is executing; verify displayed FPS",observed.waits,0);}
}
