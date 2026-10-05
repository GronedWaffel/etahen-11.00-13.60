// SPDX-License-Identifier: GPL-3.0-or-later
// Native PRX modules load inside their matching game, once per process.
#include "kernel.hpp"
#include "port_kstuff.hpp"
#include "port_prx.h"
#include "port_game_plugin.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "experimental_trace.h"
#include "../../shellui/include/fps_counter_stub.h"
#include "../../shellui/include/fps_limiter_stub.h"
#include "../../../build/fps-limiter/prx-gate.h"
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
struct Session {int pid,app;uint64_t control=0,started=0;bool failed=false;std::string paths[PORT_PRX_MAX];unsigned reported[PORT_PRX_MAX]{};bool waiting_notified=false;};
struct Request {std::string path,title,identity;bool enabled;};
std::vector<Session> sessions;
std::vector<Request> requests;
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
    *stage="PRX loader resolving game pad exports and module loader";
    Hook hooks[2];
    hooks[0].target=resolve(s.pid,"libScePad","YndgXqQVV7c"); // scePadReadState
    hooks[1].target=resolve(s.pid,"libScePad","q1cHNfGycLI"); // scePadRead
    if(hooks[1].target==hooks[0].target)hooks[1].target=0;
    PrxControl control{};
    control.load_fn=resolve(s.pid,"libkernel","wzvqT4UqKX8"); // sceKernelLoadStartModule
    if((!hooks[0].target&&!hooks[1].target)||!control.load_fn)return false;
    *stage="PRX loader checking kstuff";if(!port_kstuff_injection_ready())return false;
    *stage="PRX loader stopping target";Stop stop(s.pid);if(!stop.attached)return false;
    *stage="PRX loader allocating private code and data";
    intptr_t base=pt_mmap(s.pid,0,0x8000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(base<=0 || base==(intptr_t)MAP_FAILED)return false;
    struct Mapping {int pid;intptr_t base;bool keep=false;~Mapping(){if(!keep)pt_munmap(pid,base,0x8000);}} mapping{s.pid,base};
    const uint64_t data=base+0x4000;
    *stage="PRX loader preparing and relocating entries";
    int threads[2048],count=pt_getlwps(s.pid,threads,2048);if(count<=0)return false;
    for(unsigned lane=0;lane<2;++lane){auto& h=hooks[lane];if(!h.target)continue;
        h.stub=base+0x1000+lane*0x800;
        if(pt_copyout(s.pid,h.target,h.before,sizeof(h.before)))return false;
        if((h.before[0]==0xff&&h.before[1]==0x25)||h.before[0]==0xe9||h.before[0]==0xeb){*stage="PRX loader refused existing or forwarded entry hook";return false;}
        h.original=kernel_get_vmem_protection(s.pid,h.target,14);if(h.original<0||!(h.original&PROT_EXEC))return false;
        if(!BuildFpsLimiterStub(h.before,sizeof(h.before),h.target,h.stub,data,base,lane,&h.code))return false;
        for(int i=0;i<count;++i){reg r{};if(pt_getregs(threads[i],&r))return false;if(r.r_rip>=h.target&&r.r_rip<h.target+h.code.stolen){*stage="PRX loader deferred: thread in entry";return false;}}
        h.patch[0]=0xff;h.patch[1]=0x25;memcpy(h.patch+6,&h.stub,8);
        if(pt_copyin(s.pid,h.code.bytes,h.stub,h.code.size))return false;
    }
    *stage="PRX loader writing helper and disabled control";
    if(pt_copyin(s.pid,prx_gate_bytes,base,sizeof(prx_gate_bytes))||pt_copyin(s.pid,&control,data,sizeof(control))||kernel_mprotect(s.pid,base,0x4000,PROT_READ|PROT_EXEC))return false;
    bool ok=true;unsigned char check[14];
    *stage="PRX loader publishing verified private RX entries";
    for(auto& h:hooks){if(!h.target)continue;
        if(pt_copyout(s.pid,h.target,check,14)||memcmp(check,h.before,14)||kernel_mprotect(s.pid,h.target,14,h.original|PROT_READ|PROT_WRITE)){ok=false;break;}
        // A failed write may be partial. Keep code mapped until rollback verifies.
        h.published=true;mapping.keep=true;
        const int rx=FpsCounterEntryProtection(h.original,PROT_READ,PROT_WRITE);
        if(pt_copyin(s.pid,h.patch,h.target,14)||kernel_mprotect(s.pid,h.target,14,rx)||kernel_get_vmem_protection(s.pid,h.target,14)!=rx||pt_copyout(s.pid,h.target,check,14)||memcmp(check,h.patch,14)){ok=false;break;}
    }
    if(!ok){
        *stage="PRX loader publication failed; restoring original entries";
        bool all=true;for(auto& h:hooks){if(!h.published)continue;bool restored=false;
            for(int retry=0;retry<2&&!restored;++retry){
                if(kernel_mprotect(s.pid,h.target,14,h.original|PROT_READ|PROT_WRITE))continue;
                int w=pt_copyin(s.pid,h.before,h.target,14);int p=kernel_mprotect(s.pid,h.target,14,h.original);
                restored=!w&&!p&&!pt_copyout(s.pid,h.target,check,14)&&!memcmp(check,h.before,14);
            }all=all&&restored;
        }
        mapping.keep=!all;if(!all)*stage="PRX loader rollback incomplete; retained disabled helper";return false;
    }
    s.control=data;mapping.keep=true;
    *stage="PRX loader resuming game";if(!stop.resume())return false;
    *stage="PRX loader module hooks installed; waiting for game input calls";return true;
}

bool validate(const std::string& path,const std::string& title){
    int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW);if(fd<0)return false;
    struct stat st{};unsigned char prefix[16384];
    bool regular=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_size>=64&&st.st_size<=PORT_PLUGIN_MAX_SIZE;
    ssize_t n=regular?read(fd,prefix,sizeof(prefix)):-1;close(fd);int platform=0;
    if(n<0||!port_prx_parse(prefix,(size_t)n,(size_t)st.st_size,&platform))return false;
    bool native=title.rfind("PPS",0)==0;
    return !platform||(native?platform==5:platform==4);
}
std::string marker(const std::string& identity){return "/system_tmp/"+identity+".PRX";}
}
// Caller holds jb_lock. A Start before launching the game arms the request.
bool port_request_prx(const std::string& path,bool enabled,bool automatic){
    std::string title;if(!port_game_plugin_path(path,&title)||!port_prx_suffix(path.c_str()))return false;
    Request* r=nullptr;for(auto& item:requests)if(item.path==path){r=&item;break;}
    if(automatic&&r&&!r->enabled)return true; // Respect manual Stop this boot.
    if(enabled&&!validate(path,title))return false;
    if(!r){if(requests.size()>=64)return false;char identity[16];port_plugin_identity(path.c_str(),identity);
        requests.push_back({path,title,identity,enabled});r=&requests.back();}
    r->enabled=enabled;
    if(enabled){int fd=open(marker(r->identity).c_str(),O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0){r->enabled=false;return false;}close(fd);}
    else unlink(marker(r->identity).c_str());
    experimental_event("game-prx",enabled?"module armed for matching title":"future loads disarmed; close game to unload resident module",0,0);
    return true;
}
void port_poll_prx(const std::string& title,int app){
    bool any=false;for(const auto& r:requests)if(r.enabled&&r.title==title){any=true;break;}
    if(!any&&sessions.empty())return;
    int pid=get_game_pid();if(pid<=1)return;uint64_t now=mono();if(!now)return;
    Session* current=nullptr;for(auto& s:sessions)if(s.pid==pid&&s.app==app){current=&s;break;}
    if(!current){if(!any||sessions.size()>=64)return;sessions.push_back(Session{pid,app,0,now});current=&sessions.back();}
    auto& s=*current;if(s.failed)return;
    if(!s.control){if(now-s.started<2000000000ull)return;const char* stage=nullptr;bool ok=install(s,&stage);
        experimental_event("game-prx",stage,pid,ok?0:errno);
        if(!ok){s.failed=true;notify(true,"Game PRX loader unavailable; diagnostic stage saved");return;}
    }
    for(auto& r:requests){if(r.title!=title)continue;
        int slot=-1;for(unsigned i=0;i<PORT_PRX_MAX;i++)if(s.paths[i]==r.path){slot=i;break;}
        if(slot<0&&r.enabled){
            for(unsigned i=0;i<PORT_PRX_MAX;i++)if(s.paths[i].empty()){slot=i;break;}
            if(slot<0){experimental_event("game-prx","per-game module limit reached",PORT_PRX_MAX,0);continue;}
            if(!validate(r.path,title)){r.enabled=false;unlink(marker(r.identity).c_str());notify(true,"PRX rejected: file changed, invalid module, or wrong platform");continue;}
            // Fill the path while disabled. Only the final enable write publishes.
            PrxSlot value{};value.handle=-1;snprintf(value.path,sizeof(value.path),"%s",r.path.c_str());
            if(kernel_proc_copyin(pid,&value,s.control+offsetof(PrxControl,slots)+slot*sizeof(PrxSlot),sizeof(value)))continue;
            s.paths[slot]=r.path;
        }
        if(slot<0)continue;
        const uint64_t at=s.control+offsetof(PrxControl,slots)+slot*sizeof(PrxSlot);
        uint32_t enabled=r.enabled?1:0;if(kernel_proc_copyin(pid,&enabled,at+offsetof(PrxSlot,enabled),sizeof(enabled)))continue;
        PrxSlot observed{};if(kernel_proc_copyout(pid,at,&observed,sizeof(observed)))continue;
        if(observed.state==0&&r.enabled&&now-s.started>30000000000ull&&!s.waiting_notified){
            s.waiting_notified=true;notify(true,"PRX is armed but has not reached the game input callback yet");
            experimental_event("game-prx","module still pending game callback after 30 seconds",pid,0);
        }
        if(observed.state!=s.reported[slot]){
            s.reported[slot]=observed.state;char message[320];const char* file=strrchr(r.path.c_str(),'/');file=file?file+1:r.path.c_str();
            snprintf(message,sizeof(message),"PRX %s state=%u module=0x%x start=0x%x",file,observed.state,observed.handle,observed.start_result);
            experimental_event("game-prx",message,pid,0);
            if(observed.state==2)notify(true,"Game module loaded: %s",file);
            if(observed.state==3)notify(true,"PRX %s failed: module 0x%x, start 0x%x. Check game version and dependencies.",file,observed.handle,observed.start_result);
        }
    }
}
