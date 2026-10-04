// SPDX-License-Identifier: GPL-3.0-or-later
// Diagnostic candidate only. Never open/write/fsync a file while the target
// is stopped. Events use static storage, then flush after detach or before attach.
#include "port_diagnostic.h"
#if defined(ETAHEN_TOOLBOX_DIAGNOSTIC) || defined(ETAHEN_EXPERIMENTAL_DIAGNOSTICS)
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "experimental_trace.h"
typedef struct {const char *operation;unsigned long long us,address;long long result;int error;} Event;
static Event events[1024];
static int busy;
static unsigned used,dropped;
void port_diag(const char *operation,unsigned long long address,long long result,int error){
 int saved=errno;if(__atomic_exchange_n(&busy,1,__ATOMIC_ACQUIRE)){__atomic_fetch_add(&dropped,1,__ATOMIC_RELAXED);return;}struct timespec now={0};clock_gettime(CLOCK_MONOTONIC,&now);
 if(used<sizeof(events)/sizeof(events[0]))events[used++]=(Event){operation,(unsigned long long)now.tv_sec*1000000+now.tv_nsec/1000,address,result,error};else __atomic_fetch_add(&dropped,1,__ATOMIC_RELAXED);
 __atomic_store_n(&busy,0,__ATOMIC_RELEASE);errno=saved;
}
static int write_all(int fd,const char *data,size_t size){
 while(size){ssize_t n=write(fd,data,size);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;data+=n;size-=(size_t)n;}return 0;
}
void port_diag_flush(void){
 int saved=errno;
 if(__atomic_exchange_n(&busy,1,__ATOMIC_ACQUIRE))return;
 static char batch[240*1024];size_t length=0;
 for(unsigned i=0;i<used;i++){const Event *e=&events[i];
  int n=snprintf(batch+length,sizeof(batch)-length,"schema=1 build=%s utc=%lld pid=%d mono_us=%llu component=injector event=%s arg=0x%llx result=%lld errno=%d\n",EXPERIMENTAL_DIAGNOSTIC_BUILD,(long long)time(NULL),getpid(),e->us,e->operation,e->address,e->result,e->error);
  if(n<=0||(size_t)n>=sizeof(batch)-length)break;length+=(size_t)n;
 }
 unsigned missed=__atomic_exchange_n(&dropped,0,__ATOMIC_RELAXED);
 if(missed){int n=snprintf(batch+length,sizeof(batch)-length,"component=injector dropped=%u\n",missed);if(n>0&&(size_t)n<sizeof(batch)-length)length+=(size_t)n;}
 if(length)experimental_append(batch,length);
 used=0;__atomic_store_n(&busy,0,__ATOMIC_RELEASE);errno=saved;
}
#endif
