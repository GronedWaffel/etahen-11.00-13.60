// SPDX-License-Identifier: GPL-3.0-or-later
#include "port_publish.h"
#include "port_stage.hpp"
#include "../include/pt.h"
#include <ps5/kernel.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
extern intptr_t injector_result_address;
extern int pt_getlwps(pid_t,int*,size_t);
static int write_code(int pid,uint64_t address,const unsigned char* bytes){
 // Never fall back to physical writes for executable mappings: a native
 // library page may be shared with other processes. PT_IO must either obtain
 // a writable target mapping through the VM subsystem or fail closed.
 const int protection=kernel_get_vmem_protection(pid,address,14);
 if(protection<0)return -1;
 if(kernel_mprotect(pid,address,14,protection|PROT_READ|PROT_WRITE))return -1;
 const int written=pt_copyin(pid,bytes,address,14);
 int restored=kernel_mprotect(pid,address,14,protection);
 if(restored)restored=kernel_mprotect(pid,address,14,protection);
 unsigned char check[14];
 return written||restored||pt_copyout(pid,address,check,sizeof(check))||memcmp(check,bytes,sizeof(check))?-1:0;
}
static int apply_stopped(int pid,const PortPublishRequest* request,intptr_t request_address){
 if(!request->count||request->count>128||!request->records)return -1;
 PortPatch patches[128];
 if(pt_copyout(pid,request->records,patches,request->count*sizeof(PortPatch)))return -1;
 for(unsigned i=0;i<request->count;i++){
  if(!patches[i].address||patches[i].address>UINT64_MAX-14)return -1;
  for(unsigned j=0;j<i;j++)if(patches[i].address<patches[j].address+14&&patches[j].address<patches[i].address+14)return -1;
 }
 for(int attempt=0;attempt<20;attempt++){
  port_stage("publisher","before attaching to stop target threads");
  if(pt_attach(pid))return -1;
  port_stage("publisher","target stopped; checking thread instruction pointers");
  int tids[2048],n=pt_getlwps(pid,tids,2048),busy=0;
  if(n<=0){pt_detach(pid,0);return -1;}
  for(int t=0;t<n&&!busy;t++){struct reg regs;if(pt_getregs(tids[t],&regs)){busy=-1;break;}
   for(unsigned i=0;i<request->count;i++)if(regs.r_rip>=patches[i].address&&regs.r_rip<patches[i].address+14){busy=1;break;}}
  if(busy){pt_detach(pid,0);if(busy<0)return -1;usleep(1000);continue;}
  unsigned done=0;int ok=1;
  port_stage("publisher","checking original hook bytes");
  for(unsigned i=0;i<request->count;i++){unsigned char current[14];
   if(pt_copyout(pid,patches[i].address,current,14)||memcmp(current,patches[i].before,14)){ok=0;break;}}
  if(ok)port_stage("publisher","writing verified hook batch");
  if(ok)for(unsigned i=0;i<request->count;i++){done=i+1;
   if(write_code(pid,patches[i].address,patches[i].after)){ok=0;break;}}
  uint32_t acknowledged=2;
  if(ok&&pt_copyin(pid,&acknowledged,request_address+offsetof(PortPublishRequest,state),sizeof(acknowledged)))ok=0;
  if(!ok)for(unsigned i=done;i>0;i--)if(write_code(pid,patches[i-1].address,patches[i-1].before))puts("Hook rollback write failed");
  port_stage("publisher",ok?"hook batch acknowledged; resuming target":"publication failed; attempted restoration before resume");
  int detached=pt_detach(pid,0);
  port_stage("publisher",detached?"detach failed":"target resumed");
  printf("Stopped-process publication: %u hooks, %d threads, result %d\n",request->count,n,ok&&!detached);
  return ok&&!detached?0:-1;
 }
 return -1;
}
int port_service_publication(int pid){
 // The result integer is at args+0x300; the private handshake is at +0x3400.
 intptr_t address=injector_result_address+PORT_PUBLISH_OFFSET-0x300;
 char previous[192]={0};
 port_stage("publisher","observing injected initializer");
 for(int tick=0;tick<150;tick++){
  PortPublishRequest request={0};int result=-1234567;
  if(pt_copyout(pid,injector_result_address,&result,sizeof(result)))return -1;
  // Preserve the child's most recent stage from its shared argument page.
  // This stays in the injector process, outside ShellUI's filesystem sandbox.
  char stage[192]={0};
  if(!pt_copyout(pid,injector_result_address+0x100,stage,sizeof(stage)-1)&&stage[0]&&strcmp(stage,previous)){
   port_stage("shellui-observed",stage);memcpy(previous,stage,sizeof(stage));
  }
  if(result!=-1234567)return 0;
  if(pt_copyout(pid,address,&request,sizeof(request)))return -1;
  if(request.magic==PORT_PUBLISH_MAGIC){
   if(request.state==4)return 0;
   if(request.state==1){
    uint32_t state=apply_stopped(pid,&request,address)?3:2;
    // The initializer only reads this aligned state word while waiting.
    if(state==3&&kernel_proc_copyin(pid,&state,address+offsetof(PortPublishRequest,state),sizeof(state)))return -1;
    if(state==3)return -1;
   }
  }
  usleep(100000);
 }
 puts("Payload initialization did not finish within the observation window");return 0;
}
