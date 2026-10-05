// SPDX-License-Identifier: GPL-3.0-or-later
// GNM export selection follows OnionHEN/LightningMods fps_bc.cpp (b23ffe674).
// Publication uses etaHEN's stopped-thread, COW-write and relocation checks.
#include "kernel.hpp"
#include "port_kstuff.hpp"
#include "port_game_arena.h"
#include "experimental_trace.h"
#include "../../shellui/include/fps_counter_stub.h"
#include "../../fps_native/include/onion/fps_types.h"
#include <ps5/kernel.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <string>
extern "C" {
#include "../../libNineS/include/pt.h"
int pt_getlwps(pid_t,int*,size_t);
}
extern int get_game_pid();
extern void notify(bool,const char*,...);
namespace {
struct Counter {int pid;int app;uint64_t address=0,target=0,stub=0,before=0,stamp=0;bool failed=false;int original_protection=-1,installed_protection=-1;PortGameArenaProtection arena{};};
std::vector<Counter> counters;
uint64_t clock_ns(clockid_t id){timespec t{};return clock_gettime(id,&t)?0:uint64_t(t.tv_sec)*1000000000+t.tv_nsec;}
uint64_t resolve_flip(int pid) {
    auto proc=::getProc(pid);if(!proc)return 0;
    auto obj=proc->getSharedObject();if(!obj)return 0;
    for(auto lib:obj->getLibs()) {
        auto path=lib->getPath();if(!path.contains("GnmDriver")&&!path.contains("gnmdriver"))continue;
        auto meta=lib->getMetaData();if(!meta)continue;
        const auto& symbols=meta->getSymbolTable();
        // Only flip submissions, not generic command-buffer submission counts.
        for(const char* nid:{"Ga6r7H6Y0RI","xbxNatawohc"})for(size_t i=0;i<symbols.length();++i){
            auto symbol=symbols[i];if(!symbol.exported())continue;auto name=symbol.name();
            if(name.length()>=11 && !memcmp(name.c_str(),nid,11) && (name.length()==11||name[11]=='#'))return symbol.vaddr();
        }
    }
    return 0;
}
struct Stop {
    int pid;bool attached=false;
    explicit Stop(int value):pid(value),attached(pt_attach(value)==0){}
    bool resume(){if(!attached)return false;bool ok=pt_detach(pid,0)==0;if(ok)attached=false;return ok;}
    ~Stop(){if(attached)pt_detach(pid,0);}
};
bool install(Counter& record,const char** stage) {
    *stage="PS4 FPS resolve flip export";
    uint64_t target=resolve_flip(record.pid);if(!target)return false;
    if(!port_kstuff_injection_ready()){*stage="PS4 FPS kstuff unavailable";return false;}
    *stage="PS4 FPS attach";Stop stopped(record.pid);if(!stopped.attached)return false;
    unsigned char before[64]{};
    *stage="PS4 FPS read prologue";
    if(pt_copyout(record.pid,target,before,sizeof(before)))return false;
    if(before[0]==0xff && before[1]==0x25){*stage="PS4 FPS existing hook refused";return false;}
    *stage="PS4 FPS allocate counter and trampoline";
    auto allocation=pt_mmap(record.pid,0,0x8000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(allocation<=0 || allocation==(intptr_t)MAP_FAILED)return false;
    struct Mapping {
        int pid;intptr_t address;bool keep=false;
        ~Mapping(){if(!keep)pt_munmap(pid,address,0x8000);}
    } mapping{record.pid,allocation};
    uint64_t stub=allocation,counter=stub+0x4000;
    FpsCounterStub code{};
    *stage="PS4 FPS relocate prologue";
    if(!BuildFpsCounterStub(before,sizeof(before),target,stub,counter,&code))return false;
    *stage="PS4 FPS check stopped instruction pointers";
    int threads[2048],count=pt_getlwps(record.pid,threads,2048);if(count<=0)return false;
    for(int i=0;i<count;++i){reg registers{};if(pt_getregs(threads[i],&registers))return false;
        if(registers.r_rip>=target && registers.r_rip<target+code.stolen)return false;}
    *stage="PS4 FPS write trampoline";uint64_t zero=0;
    if(pt_copyin(record.pid,&zero,counter,8)||pt_copyin(record.pid,code.bytes,stub,code.size))return false;
    *stage="PS4 FPS split RX code / RW data using target mprotect";
    if(!PortSealGameArena(record.pid,stub,pt_mprotect,kernel_get_vmem_protection,record.arena))return false;
    unsigned char patch[14]={0xff,0x25,0,0,0,0};memcpy(patch+6,&stub,8);
    unsigned char check[14];
    *stage="PS4 FPS recheck original entry";
    if(pt_copyout(record.pid,target,check,14)||memcmp(check,before,14))return false;
    *stage="PS4 FPS read entry protection";
    const int protection=kernel_get_vmem_protection(record.pid,target,14);
    if(protection<0 || !(protection&PROT_EXEC))return false;
    record.original_protection=protection;
    const int installed_protection=FpsCounterEntryProtection(protection,PROT_READ,PROT_WRITE);
    *stage="PS4 FPS publish COW entry";
    if(kernel_mprotect(record.pid,target,14,protection|PROT_READ|PROT_WRITE))return false;
    mapping.keep=true; // Until rollback is verified, the entry may reference it.
    bool ok=pt_copyin(record.pid,patch,target,14)==0;
    // The FF 25 entry reads its target from target+6. Restoring an XO page
    // here causes SYSTEM_XO_VIOLATION on the first flip. Keep this COW page RX.
    if(kernel_mprotect(record.pid,target,14,installed_protection))ok=false;
    record.installed_protection=kernel_get_vmem_protection(record.pid,target,14);
    if(record.installed_protection!=installed_protection)ok=false;
    if(pt_copyout(record.pid,target,check,14)||memcmp(check,patch,14))ok=false;
    if(!ok){
        *stage="PS4 FPS entry write failed; rollback";
        bool restored=false;
        for(int attempt=0;attempt<2&&!restored;++attempt){
            kernel_mprotect(record.pid,target,14,protection|PROT_READ|PROT_WRITE);
            int written=pt_copyin(record.pid,before,target,14);
            int protected_again=kernel_mprotect(record.pid,target,14,protection);
            restored=!written&&!protected_again&&!pt_copyout(record.pid,target,check,14)&&!memcmp(check,before,14);
        }
        if(!restored)*stage="PS4 FPS rollback could not be verified";
        else mapping.keep=false;
        return false;
    }
    record.target=target;record.stub=stub;record.address=counter;
    *stage="PS4 FPS resume";if(!stopped.resume())return false;
    record.stamp=0;
    *stage="PS4 FPS remote flip counter installed";return true;
}
void publish(const OnionFpsSample& sample){
    static int fd=-1;static uint32_t seq=0;
    if(fd<0){fd=open("/system_tmp/etahen_fps_bc.sample",O_CREAT|O_RDWR,0644);if(fd<0)return;if(ftruncate(fd,sizeof(sample))){close(fd);fd=-1;return;}}
    OnionFpsSample value=sample;value.seq=(seq+=2)-1;
    if(pwrite(fd,&value,sizeof(value),0)==sizeof(value))pwrite(fd,&seq,sizeof(seq),offsetof(OnionFpsSample,seq));
}
}
// Called by the existing 250ms game monitor under jb_lock. No game thread creation.
void port_poll_bc_fps(const std::string& title,int app) {
    int pid=get_game_pid();if(pid<=1)return;
    Counter* active=nullptr;
    for(auto& counter:counters)if(counter.pid==pid&&counter.app==app){active=&counter;break;}
    if(!active){if(counters.size()>=64)return;counters.push_back(Counter{pid,app,0,0,0,0,clock_ns(CLOCK_MONOTONIC)});active=&counters.back();}
    auto& counter=*active;if(counter.failed)return;
    if(!counter.address){
        if(clock_ns(CLOCK_MONOTONIC)-counter.stamp<2000000000ull)return;
        const char* stage=nullptr;errno=0;bool ok=install(counter,&stage);
        experimental_event("fps-bc",stage,pid,ok?0:errno);
        // install() has detached before journal I/O; do not log while stopped.
        char detail[256];snprintf(detail,sizeof(detail),
            "PS4 FPS entry permissions original=%d installed=%d target=0x%llx stub=0x%llx",
            counter.original_protection,counter.installed_protection,
            (unsigned long long)counter.target,(unsigned long long)counter.stub);
        experimental_event("fps-bc",detail,pid,0);
        snprintf(detail,sizeof(detail),"arena mprotect=%d code=%d data=%d (expected 0/5/3)",counter.arena.transition,counter.arena.code,counter.arena.data);
        experimental_event("fps-bc",detail,pid,0);
        if(!ok){counter.failed=true;notify(true,"PS4 FPS counter unavailable; diagnostic stage saved.");return;}
    }
    uint64_t frames=0,now=clock_ns(CLOCK_MONOTONIC);
    OnionFpsSample sample{};sample.magic=ONION_FPS_MAGIC;sample.pid=pid;sample.source=ONION_FPS_SRC_BC;
    sample.unix_ns=clock_ns(CLOCK_REALTIME);snprintf(sample.title_id,sizeof(sample.title_id),"%s",title.c_str());
    if(!kernel_proc_copyout(pid,counter.address,&frames,sizeof(frames))){
        if(counter.stamp && now>counter.stamp && now-counter.stamp<2000000000ull && frames>=counter.before){
            float fps=(frames-counter.before)*1000000000.0/(now-counter.stamp);
            if(fps>=1 && fps<=240){sample.valid=1;sample.fps=fps;}
        }
        counter.before=frames;counter.stamp=now;
    }
    publish(sample);
}
