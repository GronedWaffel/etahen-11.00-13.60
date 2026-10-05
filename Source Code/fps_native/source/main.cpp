// SPDX-License-Identifier: GPL-3.0-or-later
#include "runtime.hpp"
#include "../../include/port_firmware.h"
#include "../../include/port_startup_state.hpp"
#include "../../include/freebsd-helper.h"
#include <onion/fps_formula.hpp>
#include <ps5/kernel.h>
#include <sys/sysctl.h>
#include <sys/file.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <cstring>
extern "C" int sceSystemServiceGetAppIdOfRunningBigApp();
extern "C" int sceSystemServiceGetAppTitleId(int,char*);
extern "C" int _sceApplicationGetAppId(int,int*);
extern "C" int sceKernelGetProcessName(int,char*);
std::atomic<bool> g_stack_shutting_down{false};
void *vsync_fps_sampler_thread(void*) noexcept;
void *fps_sampler_thread(void*) noexcept;
bool Get_Running_App_TID(std::string &title,int &app) {
 app=sceSystemServiceGetAppIdOfRunningBigApp();char id[32]={};
 if(app<0 || sceSystemServiceGetAppTitleId(app,id))return false;
 title=id;
 return onion::fps::is_ps5_native_title(id);
}
bool onion_proc_is_alive(pid_t pid) {char name[32]={};return pid>1&&sceKernelGetProcessName(pid,name)>=0;}
bool fps_startup_ready() {
 FILE *f=fopen(PORT_STARTUP_PATH,"rb");PortStartupRecord record;
 bool ok=port_read_startup(f,record);if(f)fclose(f);
 if(!ok || record.evaluate(record.critical,record.shellui)!=1)return false;
 char critical[32]={},shell[32]={};
 return sceKernelGetProcessName(record.critical,critical)>=0 &&
        sceKernelGetProcessName(record.shellui,shell)>=0 &&
        !strncmp(critical,"etaHEN Critical",14) && !strcmp(shell,"SceShellUI");
}
pid_t fps_game_pid(int app) {
 int mib[]={CTL_KERN,KERN_PROC,KERN_PROC_PROC};size_t size=0;
 if(app<0 || sysctl(mib,3,nullptr,&size,nullptr,0) || !size || size>16*1024*1024)return -1;
 std::vector<unsigned char> data(size+4096);size=data.size();
 if(sysctl(mib,3,data.data(),&size,nullptr,0))return -1;
 const size_t minimum=offsetof(kinfo_proc,ki_pid)+sizeof(pid_t);
 for(size_t off=0;off+minimum<=size;){
  const kinfo_proc *p=(const kinfo_proc*)(data.data()+off);
  if(p->ki_structsize<(int)minimum || (size_t)p->ki_structsize>size-off)break;
  int found=-1;if(p->ki_pid>1 && _sceApplicationGetAppId(p->ki_pid,&found)==0 && found==app)return p->ki_pid;
  off+=p->ki_structsize;
 }
 return -1;
}
int main() {
 if(!snipers_firmware_profile(kernel_get_fw_version()))return 1;
 int lock=open("/system_tmp/etahen_fps_native.lock",O_CREAT|O_RDWR,0600);
 if(lock<0 || flock(lock,LOCK_EX|LOCK_NB))return 1;
 pthread_t vsync;
 if(pthread_create(&vsync,nullptr,vsync_fps_sampler_thread,nullptr))return 1;
 fps_sampler_thread(nullptr);
 g_stack_shutting_down.store(true);pthread_join(vsync,nullptr);close(lock);
 return 0;
}
