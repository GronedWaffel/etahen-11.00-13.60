// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#ifdef ETAHEN_PORT_1360
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
// Persist stage boundaries before proceeding to a potentially failing action.
// Each entry includes the process, so independently started services are clear.
static inline void port_stage(const char* component,const char* stage){
 int fd=open("/data/etaHEN/startup-1360-port.log",O_WRONLY|O_CREAT|O_APPEND,0666);
 if(fd<0)return;
 char line[256];int n=snprintf(line,sizeof(line),"%s pid=%d: %s\n",component,getpid(),stage);
 if(n>0){size_t len=(size_t)n<sizeof(line)?(size_t)n:sizeof(line)-1;
  for(size_t done=0;done<len;){ssize_t written=write(fd,line+done,len-done);if(written<=0)break;done+=(size_t)written;}
  fsync(fd);
 }
 close(fd);
}
#else
static inline void port_stage(const char*,const char*){}
#endif
