// SPDX-License-Identifier: GPL-3.0-or-later
// Independent, bounded observer. Never attaches, stops or writes to ShellUI.
#include <ps5/kernel.h>
#include "../Source Code/include/port_firmware.h"
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../Source Code/include/port_stage.hpp"
#include "../Source Code/include/private-1240-p5.h"
#include "../Source Code/include/private-1240-p5-persist.h"
static int shellPid(){
 int mib[4]={CTL_KERN,KERN_PROC,KERN_PROC_PROC,0};size_t length=0;
 if(sysctl(mib,4,nullptr,&length,nullptr,0)||!length||length>16*1024*1024)return -1;
 auto* bytes=(unsigned char*)malloc(length);if(!bytes)return -1;
 int found=-1;
 if(!sysctl(mib,4,bytes,&length,nullptr,0))for(size_t off=0;off+sizeof(kinfo_proc)<=length;){
  auto* p=(kinfo_proc*)(bytes+off);size_t size=p->ki_structsize;
  if(size<sizeof(kinfo_proc)||size>length-off)break;
  if(!strncmp(p->ki_comm,"SceShellUI",sizeof p->ki_comm)){found=p->ki_pid;break;}off+=size;
 }
 free(bytes);return found;
}
static bool readTarget(P5Target* target){
 int fd=open(P5_TARGET_PATH,O_RDONLY|O_NOFOLLOW);if(fd<0)return false;
 struct stat st{};bool ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_nlink==1&&st.st_size==sizeof(*target);
 if(ok)ok=read(fd,target,sizeof(*target))==sizeof(*target);close(fd);
 return ok&&target->magic==P5_MAGIC&&target->reserved==0&&target->pid>0&&target->address>=0x10000&&target->address<UINT64_C(0x0000800000000000)-sizeof(P5Pointer);
}
static void cardAudit(const char* when){
 const char* files[]={"/user/app/ETHN13600/.etahen-toolbox-card","/user/app/ETHN13600/.etahen-toolbox-registered","/user/app/ETHN13600/sce_sys/param.json","/user/app/ETHN13600/sce_sys/icon0.png","/data/etaHEN/toolbox-card-install.json"};
 for(const char* name:files){struct stat st{};int rc=lstat(name,&st),err=rc?errno:0;char line[320];
  snprintf(line,sizeof line,"%s path=%s rc=%d errno=%d bytes=%lld mode=%o",when,name,rc,err,rc?0:(long long)st.st_size,rc?0:(unsigned)st.st_mode);port_stage("p5-card",line);
 }
}
int main(){
 const unsigned fw=kernel_get_fw_version()&0xffff0000;
 if(!snipers_firmware_profile(fw))return 2;
 port_stage("p5-watch","observer started; six-minute limit; raw TSC timestamps are not microseconds");
 P5Target target{};bool got=false;
 for(int tick=0;tick<600;tick++){if(readTarget(&target)){got=true;break;}usleep(100000);}
 if(!got){port_stage("p5-watch","no target published within 60 seconds");return 3;}
 char text[480];snprintf(text,sizeof text,"target pid=%d observer pid=%d",target.pid,getpid());port_stage("p5-watch",text);
 static P5Journal snapshot;uint64_t last=0,missed=0,lastTicks=0;uint32_t drops=0;bool ready=false;int absent=0,readErrors=0;
 uint32_t previousCounters[7]={};
 // 50 Hz during the first 15 seconds, then 5 Hz. Process inventory at 2 Hz.
 for(unsigned elapsed=0,tick=0;elapsed<360000;tick++){
  unsigned delay=elapsed<15000?20:200;
  if(tick%(elapsed<15000?25:3)==0){
   int current=shellPid();
   if(current>0&&current!=target.pid){snprintf(text,sizeof text,"ShellUI restarted old=%d new=%d ready=%d lastSequence=%llu lastTsc=%llu missed=%llu writerDrops=%u",target.pid,current,ready,(unsigned long long)last,(unsigned long long)lastTicks,(unsigned long long)missed,drops);port_stage("p5-watch",text);cardAudit("after restart");return 4;}
   if(current<=0){if(++absent==1)port_stage("p5-watch","ShellUI query missing or unavailable");if(absent>=20){port_stage("p5-watch","ShellUI unavailable; stopping");return 5;}}
   else absent=0;
  }
  P5Pointer pointer{};int rc=kernel_proc_copyout(target.pid,target.address,&pointer,sizeof pointer);int err=rc?errno:0;
  if(!rc&&pointer.magic==P5_MAGIC&&pointer.address>=0x10000&&pointer.address<UINT64_C(0x0000800000000000)-sizeof(snapshot)){
   rc=kernel_proc_copyout(target.pid,pointer.address,&snapshot,sizeof snapshot);err=rc?errno:0;
   uint64_t guard=0;
   if(!rc&&snapshot.magic==P5_MAGIC&&!(snapshot.guard&1)&&!kernel_proc_copyout(target.pid,pointer.address+offsetof(P5Journal,guard),&guard,sizeof guard)&&guard==snapshot.guard){
    readErrors=0;
    uint64_t first=snapshot.next>P5_CAPACITY?snapshot.next-P5_CAPACITY+1:1;
    if(first>last+1){missed+=first-last-1;snprintf(text,sizeof text,"overwritten records=%llu total=%llu",(unsigned long long)(first-last-1),(unsigned long long)missed);port_stage("p5-gap",text);last=first-1;}
    // Each record is persisted outside ShellUI; the writer never waits for this.
    for(uint64_t seq=last+1;seq<=snapshot.next;seq++){
     auto& r=snapshot.records[(seq-1)%P5_CAPACITY];if(r.sequence!=seq){port_stage("p5-gap","sequence mismatch; snapshot rejected");break;}
     r.text[sizeof r.text-1]=0;
     snprintf(text,sizeof text,"seq=%llu tsc=%llu value=0x%llx errno=%d %s",(unsigned long long)r.sequence,(unsigned long long)r.ticks,(unsigned long long)r.value,r.error,r.text);
     if(!p5Persist("p5-event",text))break; // Retry outside ShellUI; never claim an unsaved record.
     last=seq;lastTicks=r.ticks;
    }
    if(snapshot.dropped!=drops){drops=snapshot.dropped;snprintf(text,sizeof text,"writer contention drops=%u",drops);port_stage("p5-gap",text);}
    if(snapshot.ready&&!ready){ready=true;port_stage("p5-watch","Toolbox readiness independently observed");cardAudit("at readiness");}
    uint32_t counters[]={snapshot.ready,snapshot.resources,snapshot.toolbox_resources,snapshot.root_requests,snapshot.root_returns,snapshot.root_failures,snapshot.shortcuts};
    if(memcmp(counters,previousCounters,sizeof counters)){snprintf(text,sizeof text,"ready=%u resources=%u toolboxResources=%u rootRequests=%u rootReturns=%u rootFailures=%u shortcuts=%u",counters[0],counters[1],counters[2],counters[3],counters[4],counters[5],counters[6]);port_stage("p5-ui",text);memcpy(previousCounters,counters,sizeof counters);}
   }
  }
  if(rc&&++readErrors==1)port_result("p5-watch","journal read failed",rc,err);
  if(elapsed%10000<delay){snprintf(text,sizeof text,"heartbeat pid=%d ready=%d lastSequence=%llu missed=%llu writerDrops=%u",target.pid,ready,(unsigned long long)last,(unsigned long long)missed,drops);port_stage("p5-watch",text);}
  if(elapsed==0||elapsed==30000)cardAudit(elapsed==0?"initial":"after 30 seconds");
  usleep(delay*1000);elapsed+=delay;
 }
 cardAudit("final");port_stage("p5-watch","six-minute observation completed");return 0;
}
