// SPDX-License-Identifier: GPL-3.0-or-later
#include "port_fps_cache.hpp"
#include "private-1240-p5.h"
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

static PortFpsCache fps_cache;
static uint64_t time_ns(clockid_t clock) {
    timespec t{};
    return clock_gettime(clock,&t)?0:uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);
}
static bool read_record(const char* path,OnionFpsSample& out) {
    // Never map an external file into ShellUI: truncation could cause SIGBUS.
    int fd=open(path,O_RDONLY|O_NONBLOCK);
    if(fd<0)return false;
    struct stat st{};bool ok=false;
    if(!fstat(fd,&st) && S_ISREG(st.st_mode) && st.st_size>=sizeof(out)) {
        OnionFpsSample a{},b{};
        for(int attempt=0;attempt<2;++attempt) {
            if(pread(fd,&a,sizeof(a),0)!=sizeof(a) || pread(fd,&b,sizeof(b),0)!=sizeof(b))break;
            if(!(a.seq&1) && !memcmp(&a,&b,sizeof(a))){out=a;ok=true;break;}
        }
    }
    close(fd);return ok;
}
static void* reader(void*) {
    P5Event("FPS background reader started; no app-service queries");
    unsigned previous=~0u;
    for(;;) {
        OnionFpsSample native{},bc{};
        read_record("/system_tmp/etahen_fps_native.sample",native);
        read_record("/system_tmp/etahen_fps_bc.sample",bc);
        uint64_t real=time_ns(CLOCK_REALTIME),mono=time_ns(CLOCK_MONOTONIC)/1000000;
        auto sample=port_fps_choose(native,bc,real);
        if(mono)fps_cache.publish(sample,real,mono);else fps_cache.clear();
        unsigned available=fps_cache.read(mono)?1:0;
        if(available!=previous){P5Event("FPS cached reading available",available);previous=available;}
        usleep(250000);
    }
    return nullptr;
}
void StartFpsUiReader() {
    pthread_t worker;
    int error=pthread_create(&worker,nullptr,reader,nullptr);
    if(!error)pthread_detach(worker);
    else P5Event("FPS background reader unavailable",0,error);
}
void ReadCachedFps(char* output,size_t size) {
    uint64_t mono=time_ns(CLOCK_MONOTONIC)/1000000;
    unsigned tenths=mono?fps_cache.read(mono):0;
    if(tenths)snprintf(output,size,"%u.%u",tenths/10,tenths%10);
    else snprintf(output,size,"--");
}
