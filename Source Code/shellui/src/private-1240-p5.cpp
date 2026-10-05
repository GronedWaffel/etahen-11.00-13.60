// SPDX-License-Identifier: GPL-3.0-or-later
#include "private-1240-p5.h"
#include <errno.h>
#include <ps5/payload.h>
static P5Journal journal;
static uint64_t ticks(){unsigned lo,hi;__asm__ volatile("rdtsc":"=a"(lo),"=d"(hi));return ((uint64_t)hi<<32)|lo;}
void P5Init(){
 journal.magic=P5_MAGIC;
 auto* p=(P5Pointer*)((char*)payload_get_args()+P5_OFFSET);
 p->address=(uintptr_t)&journal;__atomic_store_n(&p->magic,P5_MAGIC,__ATOMIC_RELEASE);
}
void P5Event(const char* text,uint64_t value,int error){
 // No system calls, allocation, disk/network I/O, retry or wait on target threads.
 int saved=errno;
 if(__atomic_exchange_n(&journal.lock,1,__ATOMIC_ACQUIRE)){
  __atomic_add_fetch(&journal.dropped,1,__ATOMIC_RELAXED);errno=saved;return;
 }
 __atomic_add_fetch(&journal.guard,1,__ATOMIC_ACQ_REL);
 uint64_t seq=journal.next+1;auto& r=journal.records[(seq-1)%P5_CAPACITY];
 r.sequence=seq;r.ticks=ticks();r.value=value;r.error=error;
 unsigned i=0;for(;text&&text[i]&&i+1<sizeof r.text;i++)r.text[i]=text[i];r.text[i]=0;
 journal.next=seq;
 __atomic_add_fetch(&journal.guard,1,__ATOMIC_RELEASE);
 __atomic_store_n(&journal.lock,0,__ATOMIC_RELEASE);errno=saved;
}
void P5Ready(){__atomic_store_n(&journal.ready,1,__ATOMIC_RELEASE);P5Event("Toolbox readiness marker written");}
void P5Count(P5Counter counter){
 uint32_t* p=nullptr;
 switch(counter){case P5Resources:p=&journal.resources;break;case P5ToolboxResources:p=&journal.toolbox_resources;break;case P5RootRequests:p=&journal.root_requests;break;case P5RootReturns:p=&journal.root_returns;break;case P5RootFailures:p=&journal.root_failures;break;case P5Shortcuts:p=&journal.shortcuts;break;}
 if(p)__atomic_add_fetch(p,1,__ATOMIC_RELAXED);
}
