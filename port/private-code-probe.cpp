// SPDX-License-Identifier: GPL-3.0-or-later
// Force VM-managed copy-on-write using identical bytes, never a physical write.
#include <ps5/kernel.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern "C" {
#include "../Source Code/libNineS/include/pt.h"
int kernel_proc_getpaddr(int,unsigned long,unsigned long*,unsigned long*);
}
static int find(const char* name){
 int mib[4]={CTL_KERN,KERN_PROC,KERN_PROC_PROC,0};size_t size=0;
 if(sysctl(mib,4,0,&size,0,0)||!size||size>16*1024*1024)return -1;
 auto buf=(unsigned char*)malloc(size);if(!buf)return -1;
 if(sysctl(mib,4,buf,&size,0,0)){free(buf);return -1;}int pid=-1;
 for(size_t off=0;off+sizeof(kinfo_proc)<=size;){auto p=(kinfo_proc*)(buf+off);
  if(p->ki_structsize<sizeof(kinfo_proc)||p->ki_structsize>size-off)break;
  if(!strncmp(p->ki_comm,name,sizeof(p->ki_comm))){pid=p->ki_pid;break;}off+=p->ki_structsize;
 }free(buf);return pid;
}
static unsigned long symbol(int pid,const char* module,const char* name){uint32_t h=0;return kernel_dynlib_handle(pid,module,&h)?0:kernel_dynlib_dlsym(pid,h,name);}
int main(){
 freopen("/data/etahen-1360-private-code.log","w",stdout);setvbuf(stdout,nullptr,_IONBF,0);
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000)return 1;
 const int ui=find("SceShellUI"),core=find("SceShellCore");if(ui<0||core<0)return 2;
 FILE* out=fopen("/data/etahen-1360-private-code.json","w");if(!out)return 3;
 fprintf(out,"{\"targetPid\":%d,\"referencePid\":%d,\"hooksInstalled\":false,\"checks\":[",ui,core);fflush(out);fsync(fileno(out));
 const char* modules[]={"libkernel_sys.sprx","libkernel_sys.sprx","libSceRegMgr.sprx"};
 const char* names[]={"read","ioctl","sceRegMgrGetInt"};bool all=true;
 for(unsigned i=0;i<3;++i){
  unsigned long addr=symbol(ui,modules[i],names[i]),other=symbol(core,modules[i],names[i]);
  unsigned long before=0,after=0,reference=0,referenceAfter=0,len=0;
  unsigned char original[14]={0},check[14]={0},otherBytes[14]={0};
  bool translated=addr&&other&&!kernel_proc_getpaddr(ui,addr,&before,&len)&&!kernel_proc_getpaddr(core,other,&reference,&len);
  int protection=translated?kernel_get_vmem_protection(ui,addr,14):-1;
  bool attached=false,copied=false,restored=false,unchanged=false,referenceUnchanged=false,detached=false;
  if(translated&&protection>=0&&!pt_copyout(ui,addr,original,14)&&!pt_copyout(core,other,otherBytes,14)&&!pt_attach(ui)){
   attached=true;
   if(!kernel_mprotect(ui,addr,14,protection|PROT_READ|PROT_WRITE))copied=!pt_copyin(ui,original,addr,14);
   restored=!kernel_mprotect(ui,addr,14,protection);
   if(!restored)restored=!kernel_mprotect(ui,addr,14,protection);
   unchanged=!pt_copyout(ui,addr,check,14)&&!memcmp(check,original,14);
   kernel_proc_getpaddr(ui,addr,&after,&len);kernel_proc_getpaddr(core,other,&referenceAfter,&len);
   referenceUnchanged=!pt_copyout(core,other,check,14)&&!memcmp(check,otherBytes,14)&&referenceAfter==reference;
   detached=!pt_detach(ui,0);
  }
  bool pass=attached&&copied&&restored&&unchanged&&referenceUnchanged&&detached&&after&&after!=referenceAfter;
  fprintf(out,"%s{\"symbol\":\"%s\",\"originalProtection\":%d,\"before\":\"0x%lx\",\"after\":\"0x%lx\",\"reference\":\"0x%lx\",\"copied\":%s,\"protectionRestored\":%s,\"bytesUnchanged\":%s,\"referenceUnchanged\":%s,\"detached\":%s,\"pass\":%s}",i?",":"",names[i],protection,before,after,reference,copied?"true":"false",restored?"true":"false",unchanged?"true":"false",referenceUnchanged?"true":"false",detached?"true":"false",pass?"true":"false");fflush(out);fsync(fileno(out));
  if(!pass){all=false;break;}
 }
 fprintf(out,"],\"complete\":true,\"pass\":%s}\n",all?"true":"false");fflush(out);fsync(fileno(out));fclose(out);return all?0:4;
}
