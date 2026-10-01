// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only comparison of the cheat loader's module lookup and SDK lookup.
#include <ps5/kernel.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// Prefix of the existing libNineS module_info_t; no kernel structure overlays.
struct ModulePrefix { char filename[128]; unsigned long handle; unsigned char reserved[32]; unsigned long init,fini,eh,ehSize,frame,frameSize,base; };
extern "C" ModulePrefix* get_module_handle(int,const char*);
int main(){
 FILE* out=fopen("/data/etahen-cheat-module-probe.log","w");if(!out)return 1;
 setvbuf(out,nullptr,_IONBF,0);
 int mib[4]={CTL_KERN,KERN_PROC,KERN_PROC_PROC,0};size_t size=0;
 if(sysctl(mib,4,0,&size,0,0)||!size||size>16*1024*1024)return 2;
 auto bytes=(unsigned char*)malloc(size);if(!bytes)return 3;
 if(sysctl(mib,4,bytes,&size,0,0))return 4;
 for(size_t off=0;off+sizeof(kinfo_proc)<=size;){auto p=(kinfo_proc*)(bytes+off);
  if(p->ki_structsize<sizeof(kinfo_proc)||p->ki_structsize>size-off)break;
  if(!strcmp(p->ki_comm,"eboot.bin")){
   uint32_t handle=0;int rc=kernel_dynlib_handle(p->ki_pid,"eboot.bin",&handle);
   auto m=get_module_handle(p->ki_pid,"eboot.bin");
   fprintf(out,"pid=%d SDK status=%d handle=%u base=%#lx legacy=%s base=%#lx\n",p->ki_pid,rc,handle,rc?0:kernel_dynlib_mapbase_addr(p->ki_pid,handle),m?m->filename:"missing",m?m->base:0);
   if(m)free(m);
  }off+=p->ki_structsize;
 }free(bytes);fclose(out);return 0;
}
