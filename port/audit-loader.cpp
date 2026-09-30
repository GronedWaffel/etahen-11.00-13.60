// SPDX-License-Identifier: GPL-3.0-or-later
#include <ps5/kernel.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern "C" bool Inject_Toolbox(int pid,unsigned char *elf);
extern "C" unsigned char audit_start[];
extern "C" intptr_t injector_result_address;
#ifndef ETAHEN_AUDIT_RUN_ID
#define ETAHEN_AUDIT_RUN_ID "unspecified"
#endif
#ifdef ETAHEN_INJECT_TEST
#define AUDIT_TARGET "etaHEN AuditHost"
#else
#define AUDIT_TARGET "SceShellUI"
#endif
int main(){
 FILE *initial=fopen("/data/etahen-1360-audit-loader.json","w");if(initial){fprintf(initial,"{\"runId\":\"%s\",\"state\":\"starting\"}\n",ETAHEN_AUDIT_RUN_ID);fclose(initial);}
 unlink("/data/etahen-1360-toolbox-audit.json");
 FILE *stage=fopen("/data/etahen-1360-audit-stage.txt","w");
 if(stage){fputs("Audit loader entered\n",stage);fflush(stage);}
 freopen("/data/etahen-1360-injector.log","w",stdout);setvbuf(stdout,nullptr,_IONBF,0);
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000)return 1;
 int mib[4]={CTL_KERN,KERN_PROC,KERN_PROC_PROC,0};size_t size=0;if(sysctl(mib,4,0,&size,0,0)||!size||size>16*1024*1024)return 2;
#ifdef ETAHEN_INJECT_TEST
 char host_pid_text[32]={0};FILE *hostfile=fopen("/data/etahen-1360-test-host.txt","r");if(!hostfile)return 9;
 fread(host_pid_text,1,sizeof(host_pid_text)-1,hostfile);fclose(hostfile);int hostpid=atoi(host_pid_text);if(hostpid<=0)return 9;
#endif
 auto buf=(unsigned char*)malloc(size);if(!buf)return 3;if(sysctl(mib,4,buf,&size,0,0)){free(buf);return 4;}
 int pid=-1;for(size_t off=0;off+sizeof(kinfo_proc)<=size;){auto p=(kinfo_proc*)(buf+off);if(p->ki_structsize<sizeof(kinfo_proc)||p->ki_structsize>size-off)break;
#ifdef ETAHEN_INJECT_TEST
 if(p->ki_pid==hostpid&&!strncmp(p->ki_comm,"payload.elf",sizeof(p->ki_comm)))
#else
 if(!strncmp(p->ki_comm,AUDIT_TARGET,sizeof(p->ki_comm)))
#endif
 {pid=p->ki_pid;break;}off+=p->ki_structsize;}free(buf);
 if(stage){fprintf(stage,"ShellUI pid %d; injecting audit\n",pid);fclose(stage);}
 // Match etaHEN's Toolbox startup: kstuff hooks must be paused for ptrace.
 const intptr_t native=KERNEL_ADDRESS_DATA_BASE+0xDDD8F8+14;
 const intptr_t compat=KERNEL_ADDRESS_DATA_BASE+0xDDDA70+14;
 const uint16_t old_native=kernel_getshort(native),old_compat=kernel_getshort(compat);
 printf("kstuff markers %04x/%04x\n",old_native,old_compat);
 if((old_native!=0xdeb7&&old_native!=0xffff)||(old_compat!=0xdeb7&&old_compat!=0xffff))return 6;
 FILE *journal=fopen("/data/etahen-1360-audit-resume.txt","w");if(!journal)return 8;
 fprintf(journal,"%04x %04x\n",old_native,old_compat);fclose(journal);
 if(kernel_setshort(native,0xffff)||kernel_setshort(compat,0xffff)){kernel_setshort(native,old_native);kernel_setshort(compat,old_compat);return 7;}
 bool ok=pid>0&&Inject_Toolbox(pid,audit_start);
 kernel_setshort(native,old_native);kernel_setshort(compat,old_compat);
 unlink("/data/etahen-1360-audit-resume.txt");
 int result=-1234567;
 if(ok&&injector_result_address)for(int i=0;i<50&&result==-1234567;++i){usleep(100000);if(kernel_proc_copyout(pid,injector_result_address,&result,sizeof(result)))break;}
 if(ok){
  char report[0x3001]={0};bool copied=true;
  for(size_t off=0;off<0x3000;off+=0x400)if(kernel_proc_copyout(pid,injector_result_address+0x100+off,report+off,0x400)){copied=false;break;}
  FILE *file=copied&&report[0]=='{'?fopen("/data/etahen-1360-toolbox-audit.json","w"):nullptr;
  if(file){fputs(report,file);fclose(file);}
 }
 FILE *out=fopen("/data/etahen-1360-audit-loader.json","w");if(out){fprintf(out,"{\"runId\":\"%s\",\"state\":\"finished\",\"targetPid\":%d,\"targetFound\":%s,\"threadStarted\":%s,\"exitCode\":%d}\n",ETAHEN_AUDIT_RUN_ID,pid,pid>0?"true":"false",ok?"true":"false",result);fclose(out);}return ok?0:5;
}
