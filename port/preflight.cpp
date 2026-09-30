// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only prerequisite checks. This is NOT etaHEN and installs no hooks.
#include <ps5/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <initializer_list>
#include <sys/sysctl.h>
#include <sys/user.h>
#include "../Source Code/include/offsets.hpp"
#include "../Source Code/include/port_kstuff.hpp"

static bool pointer(uintptr_t p){return (p>>48)==0xffff && !(p&7);}
extern "C" int kernel_proc_getpaddr(int,unsigned long,unsigned long*,unsigned long*);
static int shellcore_pid=-1;
static int find_shellui(FILE *out){
 int mib[4]={CTL_KERN,KERN_PROC,KERN_PROC_PROC,0};size_t size=0;
 if(sysctl(mib,4,0,&size,0,0)||!size||size>16*1024*1024)return -1;
 auto data=(unsigned char*)malloc(size);if(!data)return -1;
 if(sysctl(mib,4,data,&size,0,0)){free(data);return -1;}int result=-1;
 fprintf(out,",\"processes\":[");unsigned count=0;
 for(size_t pos=0;pos+sizeof(kinfo_proc)<=size;){
  auto proc=(kinfo_proc*)(data+pos);if(proc->ki_structsize<sizeof(kinfo_proc)||proc->ki_structsize>size-pos)break;
  if(!strncmp(proc->ki_comm,"SceShellUI",sizeof(proc->ki_comm)))result=proc->ki_pid;
  if(!strncmp(proc->ki_comm,"SceShellCore",sizeof(proc->ki_comm)))shellcore_pid=proc->ki_pid;
  fprintf(out,"%s{\"pid\":%d,\"name\":\"",count++?",":"",proc->ki_pid);
  for(unsigned i=0;i<sizeof(proc->ki_comm)&&proc->ki_comm[i];++i){unsigned char c=proc->ki_comm[i];if(c=='"'||c=='\\')fputc('\\',out);if(c>=32&&c<127)fputc(c,out);}
  fputs("\"}",out);
  pos+=proc->ki_structsize;
 }fputc(']',out);free(data);return result;
}
int main(){
 alarm(15);const uint32_t fw=kernel_get_fw_version();
 FILE *out=fopen("/data/etahen-1360-preflight.json","w");if(!out)return 1;
 fprintf(out,"{\"diagnosticOnly\":true,\"revision\":\"post-injector-2\",\"firmware\":\"0x%08x\"",fw);
 if((fw&0xffff0000)!=0x13600000){fprintf(out,",\"error\":\"Expected firmware 13.60\"}\n");fclose(out);return 2;}
 const uintptr_t base=KERNEL_ADDRESS_DATA_BASE;
 const auto kstuff=port_read_kstuff_state();
 fprintf(out,",\"kstuffState\":%d,\"kstuffStateReadable\":%s,\"nativeSysent\":\"0x%016lx\",\"compatSysent\":\"0x%016lx\",\"cryptTags\":\"%04x/%04x\"",(int)kstuff.classify(base),kstuff.readable?"true":"false",kstuff.native,kstuff.compat,kstuff.xts,kstuff.hmac);
 const bool offsets_ok=base+offsets::allproc()==(uintptr_t)KERNEL_ADDRESS_ALLPROC&&base+offsets::root_vnode()==(uintptr_t)KERNEL_ADDRESS_ROOTVNODE&&base+offsets::security_flags()==(uintptr_t)KERNEL_ADDRESS_SECURITY_FLAGS&&base+offsets::qa_flags()==(uintptr_t)KERNEL_ADDRESS_QA_FLAGS&&base+offsets::utoken_flags()==(uintptr_t)KERNEL_ADDRESS_UTOKEN_FLAGS;
 const uintptr_t proc=kernel_get_proc(getpid());int observed=-1;uintptr_t vnode=0;
 const bool own_ok=pointer(proc)&&!kernel_copyout(proc+KERNEL_OFFSET_PROC_P_PID,&observed,sizeof observed)&&observed==getpid();
 const bool root_ok=!kernel_copyout(KERNEL_ADDRESS_ROOTVNODE,&vnode,sizeof vnode)&&pointer(vnode);
 fprintf(out,",\"offsetsMatchSdk\":%s,\"ownProcessRead\":%s,\"rootVnodeRead\":%s",offsets_ok?"true":"false",own_ok?"true":"false",root_ok?"true":"false");
 const int shell=offsets_ok&&own_ok&&root_ok?find_shellui(out):-1;
 uint32_t mono=0;bool mono_ok=shell>0&&!kernel_dynlib_handle(shell,"libmonosgen-2.0.sprx",&mono);
 fprintf(out,",\"nativePhysicalMappings\":[");unsigned mappingCount=0;
 const char* modules[]={"libkernel_sys.sprx","libkernel_sys.sprx","libSceRegMgr.sprx"};
 const char* names[]={"read","ioctl","sceRegMgrGetInt"};
 for(int target: {shell,shellcore_pid})for(unsigned i=0;i<3&&target>0;++i){
  uint32_t h=0;unsigned long address=0,physical=0,remaining=0;
  if(!kernel_dynlib_handle(target,modules[i],&h))address=kernel_dynlib_dlsym(target,h,names[i]);
  const bool translated=address&&!kernel_proc_getpaddr(target,address,&physical,&remaining);
  fprintf(out,"%s{\"pid\":%d,\"symbol\":\"%s\",\"address\":\"0x%lx\",\"physical\":\"0x%lx\",\"translated\":%s}",mappingCount++?",":"",target,names[i],address,physical,translated?"true":"false");
 }fprintf(out,"]");
 uint32_t kernelHandle=0;bool kernelOk=shell>0&&!kernel_dynlib_handle(shell,"libkernel_sys.sprx",&kernelHandle);
 fprintf(out,",\"registrySyscallExport\":\"0x%lx\",\"registrySyscallFixedHandle\":\"0x%lx\"",kernelOk?kernel_dynlib_dlsym(shell,kernelHandle,"__sys_regmgr_call"):0,shell>0?kernel_dynlib_dlsym(shell,0x2001,"__sys_regmgr_call"):0);
 fprintf(out,",\"shelluiFound\":%s,\"monoLibraryFound\":%s,\"monoExports\":{",shell>0?"true":"false",mono_ok?"true":"false");
 const char *symbols[]={"mono_get_root_domain","mono_assembly_foreach","mono_assembly_get_image","mono_class_from_name","mono_class_get_method_from_name","mono_compile_method"};
 for(unsigned i=0;i<sizeof(symbols)/sizeof(*symbols);i++)fprintf(out,"%s\"%s\":%s",i?",":"",symbols[i],mono_ok&&kernel_dynlib_dlsym(shell,mono,symbols[i])?"true":"false");
 fprintf(out,"},\"hooksInstalled\":false,\"kstuffLoaded\":false,\"complete\":true}\n");fclose(out);return offsets_ok&&own_ok&&root_ok&&mono_ok?0:3;
}
