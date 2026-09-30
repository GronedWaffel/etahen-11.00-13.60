// SPDX-License-Identifier: GPL-3.0-or-later
#include "detour_port.h"
#include "relocate.h"
#include "../../include/port_publish.h"
#include <ps5/payload.h>
#include <ps5/kernel.h>
#include <ps5/mdbg.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
// The SDK's public syscall() declaration returns int, which truncates mmap
// addresses above 4 GiB. The initialized CRT gateway returns a full register.
extern "C" long __crt_syscall(long,...);
namespace {
constexpr size_t page=0x4000;
struct Hook {uint64_t address;unsigned char original[14],patch[14];void* trampoline;bool published;};
Hook hooks[128];size_t count=0,transactionStart=0;bool active=false,failed=false;
constexpr size_t slotSize=512,arenaSize=128*slotSize;
unsigned char* arena=nullptr;
bool externalPublication=false;
PortPatch publicationRecords[128];
bool selectedHook(size_t index){
#ifdef ETAHEN_PORT_PROBE_HOOKS
 return index<64&&(((uint64_t)ETAHEN_PORT_PROBE_HOOKS>>index)&1);
#else
 (void)index;return true;
#endif
}
volatile PortPublishRequest* publicationRequest(){return (volatile PortPublishRequest*)((char*)payload_get_args()+PORT_PUBLISH_OFFSET);}
bool publishExternal(bool restore){
 auto request=publicationRequest();unsigned records=0;
 for(size_t i=transactionStart;i<count;i++)if(restore?hooks[i].published:selectedHook(i)){
  auto& out=publicationRecords[records++];out.address=hooks[i].address;
  memcpy(out.before,restore?hooks[i].patch:hooks[i].original,14);
  memcpy(out.after,restore?hooks[i].original:hooks[i].patch,14);
 }
 if(!records)return true;
 request->count=records;request->records=(uintptr_t)publicationRecords;
 __atomic_thread_fence(__ATOMIC_RELEASE);request->state=1;
 for(int tick=0;tick<150;tick++){uint32_t state=request->state;if(state==2)return true;if(state==3)return false;usleep(100000);}
 return false;
}
struct AllocationCredentials {
 uint64_t auth=0;unsigned char caps[16];bool saved=false;
 AllocationCredentials(){auth=kernel_get_ucred_authid(getpid());if(!auth||kernel_get_ucred_caps(getpid(),caps))return;saved=true;unsigned char full[16];memset(full,0xff,16);kernel_set_ucred_authid(getpid(),0x4800000000010003ULL);kernel_set_ucred_caps(getpid(),full);}
 ~AllocationCredentials(){if(saved){kernel_set_ucred_caps(getpid(),caps);kernel_set_ucred_authid(getpid(),auth);}}
};
bool reserveArena(){
 if(arena)return true;
 AllocationCredentials credentials;
 void* p=(void*)__crt_syscall(SYS_mmap,0UL,arenaSize,(long)(PROT_READ|PROT_WRITE),(long)(MAP_PRIVATE|MAP_ANONYMOUS),-1L,0L);
 if(p==MAP_FAILED)return false;
 memset(p,0,arenaSize);
 if(kernel_mprotect(getpid(),(intptr_t)p,arenaSize,PROT_READ|PROT_WRITE|PROT_EXEC)){
  __crt_syscall(SYS_munmap,p,arenaSize);return false;
 }
 arena=(unsigned char*)p;return true;
}
}
bool PortReadCode(uint64_t address,void* bytes,size_t length){
 for(size_t off=0;off<length;){size_t n=0x1000-((address+off)&0xfff);if(n>length-off)n=length-off;
  if(kernel_proc_copyout(getpid(),address+off,(unsigned char*)bytes+off,n))return false;off+=n;}
 return true;
}
bool PortWriteCode(uint64_t address,const void* bytes,size_t length){
 unsigned char check[64];
 for(size_t off=0;off<length;){size_t n=0x1000-((address+off)&0xfff);if(n>sizeof(check))n=sizeof(check);if(n>length-off)n=length-off;
  // Do not use kernel_proc_copyin's physical-page fallback for executable
  // code; shared native-library pages must remain isolated by the VM layer.
  if(mdbg_copyin(getpid(),(const unsigned char*)bytes+off,address+off,n)||!PortReadCode(address+off,check,n)||memcmp(check,(const unsigned char*)bytes+off,n))return false;off+=n;}
 return true;
}
bool BeginPortDetours(){if(active||failed||!reserveArena())return false;transactionStart=count;active=true;return true;}
void EnablePortExternalPublication(){externalPublication=true;auto request=publicationRequest();request->state=0;request->magic=PORT_PUBLISH_MAGIC;}
void FinishPortExternalPublication(){if(externalPublication)publicationRequest()->state=4;}
bool PortDetoursReady(){return active&&!failed&&count>transactionStart;}
bool CommitPortDetours(){
 if(!active||failed){RollbackPortDetours();return false;}
 if(externalPublication){
  if(!publishExternal(false)){failed=true;active=false;return false;}
  for(size_t i=transactionStart;i<count;i++)hooks[i].published=selectedHook(i);
  active=false;return true;
 }
 for(size_t i=transactionStart;i<count;++i){unsigned char check[14];
  if(!PortReadCode(hooks[i].address,check,14)||memcmp(check,hooks[i].original,14)){RollbackPortDetours();return false;}
  hooks[i].published=true;
  if(!PortWriteCode(hooks[i].address,hooks[i].patch,14)){RollbackPortDetours();return false;}
 }
 active=false;return true;
}
bool RollbackPortDetours(){
 if(externalPublication){bool ok=publishExternal(true);failed=true;active=false;return ok;}
 bool ok=true;for(size_t i=count;i>transactionStart;--i)if(hooks[i-1].published&&!PortWriteCode(hooks[i-1].address,hooks[i-1].original,14))ok=false;
 // Keep published trampolines mapped: another thread may already be returning
 // through one. Refuse retries after a failed initialization in this process.
 failed=true;active=false;return ok;
}
void* PortDetourFunction(uint64_t address,void* replacement,void(*progress)(const char*)){
 if(!address||!replacement||failed||count==128)return nullptr;
 for(size_t i=0;i<count;++i)if(hooks[i].address==address){failed=true;return nullptr;}
 if(progress)progress("reading original code");
 unsigned char original[64];if(!PortReadCode(address,original,sizeof(original))){failed=true;return nullptr;}
 if(progress)progress("reserving trampoline slot");
 if(!reserveArena()){failed=true;return nullptr;}
 void* memory=arena+count*slotSize;
 if(progress)progress("relocating instructions");
 RelocatedCode code;if(!BuildTrampoline(original,sizeof(original),address,(uintptr_t)memory,&code)){failed=true;return nullptr;}
 if(progress)progress("populating trampoline memory");
 memcpy(memory,code.bytes,code.size);
 if(progress)progress("saving hook record");
 unsigned char patch[14]={0x68,0,0,0,0,0xc7,0x44,0x24,0x04,0,0,0,0,0xc3};uint64_t target=(uintptr_t)replacement;uint32_t low=target,high=target>>32;memcpy(patch+1,&low,4);memcpy(patch+9,&high,4);
 // Preserve the original before attempting a write, including a short write.
 Hook& hook=hooks[count++];hook.address=address;hook.trampoline=memory;memcpy(hook.original,original,14);memcpy(hook.patch,patch,14);
 if(!active){hook.published=true;if(!PortWriteCode(address,patch,sizeof(patch))){PortWriteCode(address,original,14);failed=true;return nullptr;}}
 return memory;
}
