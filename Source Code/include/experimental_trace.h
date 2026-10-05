// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#ifndef EXPERIMENTAL_TRACE_ROOT
#define EXPERIMENTAL_TRACE_ROOT "/data/etaHEN/experimental-diagnostics"
#endif
#ifndef EXPERIMENTAL_TRACE_PARENT
#define EXPERIMENTAL_TRACE_PARENT "/data/etaHEN"
#endif
#define EXPERIMENTAL_DIAGNOSTIC_BUILD "ex-diag-20261005.unified-dev6"
#define EXPERIMENTAL_TRACE_LIMIT (1024*1024)
// Safe stage boundaries only: never while a target is ptrace-stopped.
// Logging failure or lock contention must not stop startup.
static inline void experimental_append(const char *text,size_t length){
    int saved=errno,lock=-1,fd=-1;
    struct stat st;
    mkdir(EXPERIMENTAL_TRACE_PARENT,0777);
    mkdir(EXPERIMENTAL_TRACE_ROOT,0700);
    if(lstat(EXPERIMENTAL_TRACE_ROOT,&st)||!S_ISDIR(st.st_mode))goto done;
    lock=open(EXPERIMENTAL_TRACE_ROOT "/.lock",O_WRONLY|O_CREAT|O_NOFOLLOW|O_NONBLOCK,0600);
    if(lock<0||fstat(lock,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||flock(lock,LOCK_EX|LOCK_NB))goto done;
    fd=open(EXPERIMENTAL_TRACE_ROOT "/trace.log",O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW|O_NONBLOCK,0600);
    if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1)goto done;
    if(st.st_size+(off_t)length>EXPERIMENTAL_TRACE_LIMIT){
        close(fd);fd=-1;
        unlink(EXPERIMENTAL_TRACE_ROOT "/trace.log.3");
        rename(EXPERIMENTAL_TRACE_ROOT "/trace.log.2",EXPERIMENTAL_TRACE_ROOT "/trace.log.3");
        rename(EXPERIMENTAL_TRACE_ROOT "/trace.log.1",EXPERIMENTAL_TRACE_ROOT "/trace.log.2");
        if(rename(EXPERIMENTAL_TRACE_ROOT "/trace.log",EXPERIMENTAL_TRACE_ROOT "/trace.log.1"))goto done;
        fd=open(EXPERIMENTAL_TRACE_ROOT "/trace.log",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
        if(fd<0)goto done;
    }
    while(length){ssize_t n=write(fd,text,length);if(n<0&&errno==EINTR)continue;if(n<=0)break;text+=n;length-=(size_t)n;}
    fsync(fd);
done:
    if(fd>=0)close(fd);
    if(lock>=0)close(lock);
    errno=saved;
}
static inline void experimental_event(const char *component,const char *event,long long result,int error){
    int saved=errno;struct timespec ts={0};clock_gettime(CLOCK_MONOTONIC,&ts);
    char line[768];int n=snprintf(line,sizeof line,"schema=1 build=%s utc=%lld mono_us=%llu pid=%d component=%s result=%lld errno=%d event=%s\n",
        EXPERIMENTAL_DIAGNOSTIC_BUILD,(long long)time(NULL),(unsigned long long)ts.tv_sec*1000000+ts.tv_nsec/1000,
        getpid(),component,result,error,event);
    if(n>0)experimental_append(line,(size_t)n<sizeof line?(size_t)n:sizeof line-1);
    errno=saved;
}
