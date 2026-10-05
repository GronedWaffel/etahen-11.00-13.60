// SPDX-License-Identifier: GPL-3.0-or-later
// etaHEN GNM counter using the same relocated, stopped-process publication as
// Toolbox hooks. Rendering only increments a counter; it performs no file I/O.
#include "../Source Code/shellui/include/detour_port.h"
#include "../Source Code/include/private-1240-p5.h"
#include <onion/fps_publish.hpp>
#include <onion/log.h>
#include <atomic>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <string.h>

using Submit=int(*)(uint32_t,uint32_t,uint32_t**,uint32_t*,uint32_t**,uint32_t*,uint32_t,uint32_t,uint32_t,uint32_t);
static Submit original;
static std::atomic<uint64_t> frames{0};
static std::atomic<bool> enabled{false};
static char title[16];
extern "C" int sceSystemServiceGetAppIdOfRunningBigApp();
extern "C" int sceSystemServiceGetAppTitleId(int,char*);
void P5Event(const char *stage,uint64_t value,int error) {LOG_INFO("BC FPS %s value=%llu error=%d",stage,(unsigned long long)value,error);}
static int count_flip(uint32_t w,uint32_t c,uint32_t**d,uint32_t*ds,uint32_t**cc,uint32_t*cs,uint32_t vo,uint32_t b,uint32_t m,uint32_t a) {
 int rc=original(w,c,d,ds,cc,cs,vo,b,m,a);
 if(rc==0 && enabled.load(std::memory_order_relaxed))frames.fetch_add(1,std::memory_order_relaxed);
 return rc;
}
static double now(){timespec t{};clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static void *publish_samples(void*) {
 uint64_t before=frames.load();double start=now();
 for(;;){
  bool on=access("/system_tmp/fps_enabled",F_OK)==0;enabled.store(on,std::memory_order_relaxed);
  usleep(500000);
  uint64_t end=frames.load();double stop=now(),elapsed=stop-start;
  OnionFpsSample sample{};sample.pid=getpid();sample.unix_ns=onion_fps_realtime_ns();
  strncpy(sample.title_id,title,sizeof(sample.title_id)-1);sample.source=ONION_FPS_SRC_BC;
  if(on && elapsed>0 && elapsed<2){float value=(float)((end-before)/elapsed);if(value>=1 && value<=240){sample.valid=1;sample.fps=value;}}
  onion::fps::publish(sample);before=end;start=stop;
 }
 return nullptr;
}
int main(){
 int app=sceSystemServiceGetAppIdOfRunningBigApp();
 char current[32]={};if(app<0 || sceSystemServiceGetAppTitleId(app,current))return 1;
 if(strncmp(current,"CUSA",4) && strncmp(current,"PCAS",4) && strncmp(current,"PCJS",4) && strncmp(current,"PCKS",4) && strncmp(current,"CUHJ",4) && strncmp(current,"SCUS",4))return 1;
 strncpy(title,current,sizeof(title)-1);
 void *target=dlsym(RTLD_DEFAULT,"sceGnmSubmitAndFlipCommandBuffersForWorkload");
 if(!target){LOG_ERROR("BC FPS submit symbol unavailable");return 1;}
 EnablePortExternalPublication();
 if(!BeginPortDetours())return 1;
 original=(Submit)PortDetourFunction((uintptr_t)target,(void*)count_flip);
 if(!original || !CommitPortDetours())return 1;
 pthread_t worker;
 if(pthread_create(&worker,nullptr,publish_samples,nullptr)){FinishPortExternalPublication();return 1;}
 pthread_detach(worker);FinishPortExternalPublication();
 LOG_INFO("BC FPS initialized for %.9s",title);
 return 0;
}
