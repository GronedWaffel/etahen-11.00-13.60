// SPDX-License-Identifier: GPL-3.0-or-later
// Diagnostic candidate only. Never open/write/fsync a file while the target
// is stopped. Events use static storage, then flush after detach or before attach.
#include "port_diagnostic.h"
#ifdef ETAHEN_TOOLBOX_DIAGNOSTIC
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
typedef struct {const char *operation;unsigned long long us,address;long long result;int error;} Event;
static Event events[8192];
static unsigned used,dropped;
void port_diag(const char *operation,unsigned long long address,long long result,int error){
 int saved=errno;struct timespec now={0};clock_gettime(CLOCK_MONOTONIC,&now);
 if(used<sizeof(events)/sizeof(events[0]))events[used++]=(Event){operation,(unsigned long long)now.tv_sec*1000000+now.tv_nsec/1000,address,result,error};else dropped++;
 errno=saved;
}
static int write_all(int fd,const char *data,size_t size){
 while(size){ssize_t n=write(fd,data,size);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;data+=n;size-=(size_t)n;}return 0;
}
void port_diag_flush(void){
 int saved=errno;
 int fd=open("/data/etaHEN/toolbox-diagnostic.log",O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW,0600);
 if(fd>=0){
  char line[256];int failed=0;
  for(unsigned i=0;i<used;i++){const Event *e=&events[i];
   int n=snprintf(line,sizeof line,"pid=%d mono_us=%llu op=%s arg=0x%llx result=%lld errno=%d\n",getpid(),e->us,e->operation,e->address,e->result,e->error);
   if(n<=0||n>=(int)sizeof line||write_all(fd,line,(size_t)n)){failed=1;break;}
  }
  if(dropped){int n=snprintf(line,sizeof line,"pid=%d dropped=%u\n",getpid(),dropped);if(n>0)write_all(fd,line,(size_t)n);}
  if(!failed&&fsync(fd)==0){used=0;dropped=0;}close(fd);
 }
 errno=saved;
}
#endif
