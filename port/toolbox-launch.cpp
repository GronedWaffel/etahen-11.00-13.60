// SPDX-License-Identifier: GPL-3.0-or-later
// Open the legacy settings route; do not reload etaHEN or install hooks.
#include <ps5/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
extern "C" int sceKernelLoadStartModule(const char*,size_t,const void*,uint32_t,const void*,int*);
extern "C" int sceUserServiceGetForegroundUser(int*);
extern "C" int sceUserServiceInitialize(const void*);
struct LaunchParam {uint32_t size;int userId;};
int main(){
 int module=sceKernelLoadStartModule("/system_ex/common_ex/lib/libSceShellUIUtil.sprx",0,nullptr,0,nullptr,nullptr);
 auto init=module<0?nullptr:(int(*)())kernel_dynlib_dlsym(getpid(),module,"sceShellUIUtilInitialize");
 auto launch=module<0?nullptr:(int(*)(const char*,LaunchParam*))kernel_dynlib_dlsym(getpid(),module,"sceShellUIUtilLaunchByUri");
 int initRc=init?init():-1;LaunchParam params{sizeof(LaunchParam),-1};
 sceUserServiceInitialize(nullptr);
 int userRc=sceUserServiceGetForegroundUser(&params.userId);
 int rc=init&&launch&&userRc==0?launch("pssettings:play?mode=settings&function=debug_settings_old",&params):-1;
 FILE* f=fopen("/data/etaHEN/toolbox-route-test.json","w");
 if(f){fprintf(f,"{\"module\":%d,\"init\":%d,\"user\":%d,\"launch\":%d,\"route\":\"debug_settings_old\"}\n",module,initRc,userRc,rc);fclose(f);}
 return rc?1:0;
}
