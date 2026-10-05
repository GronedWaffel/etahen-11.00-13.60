#pragma once
#include "experimental_trace.h"
static inline bool p5_append(const char *text,size_t length){
    bool persisted=false;int saved=errno,lock=-1,fd=-1;
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
    persisted=length==0&&fsync(fd)==0;
done:
    if(fd>=0)close(fd);
    if(lock>=0)close(lock);
    errno=saved;return persisted;
}

static inline bool p5Persist(const char* component,const char* event){
 int saved=errno;struct timespec ts={0};clock_gettime(CLOCK_MONOTONIC,&ts);
 char line[768];int n=snprintf(line,sizeof line,"schema=1 build=%s utc=%lld mono_us=%llu pid=%d component=%s result=0 errno=0 event=%s\n",EXPERIMENTAL_DIAGNOSTIC_BUILD,(long long)time(NULL),(unsigned long long)ts.tv_sec*1000000+ts.tv_nsec/1000,getpid(),component,event);
 bool ok=n>0&&n<(int)sizeof line&&p5_append(line,(size_t)n);errno=saved;return ok;
}
