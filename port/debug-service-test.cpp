// SPDX-License-Identifier: GPL-3.0-or-later
// Integration test: asks the existing daemon to start PS5Debug, then repeats
// the same request to exercise its duplicate guard. Never injects directly.
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
struct Message { uint32_t magic=0xDEADBABE, command=0xDE8E6; int error=0; char json[4096]="{\"msg_1\":0}"; };
static int request() {
    int fd=socket(AF_UNIX,SOCK_STREAM,0); if(fd<0)return -100;
    timeval timeout{30,0}; setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    sockaddr_un addr{};addr.sun_family=AF_UNIX;
    strcpy(addr.sun_path,"/system_tmp/etaHEN_crit_service");
    if(connect(fd,(sockaddr*)&addr,SUN_LEN(&addr))){close(fd);return -101;}
    Message msg;
    for(size_t done=0;done<sizeof(msg);){ssize_t n=send(fd,(char*)&msg+done,sizeof(msg)-done,MSG_NOSIGNAL);if(n<=0){close(fd);return -102;}done+=n;}
    for(size_t done=0;done<sizeof(msg);){ssize_t n=recv(fd,(char*)&msg+done,sizeof(msg)-done,0);if(n<=0){close(fd);return -103;}done+=n;}
    close(fd);
    if(msg.magic!=0xDEADBABE||msg.command!=0x9000002)return -104;
    return msg.error;
}
int main(){
    alarm(65);
    FILE* f=fopen("/data/etaHEN/debug-service-test.json","w");if(!f)return 1;
    fprintf(f,"{\"pid\":%d,\"state\":\"running\"}\n",getpid());fflush(f);
    int first=request();int repeat=first==0?request():-999;
    rewind(f);fprintf(f,"{\"pid\":%d,\"state\":\"finished\",\"startResult\":%d,\"repeatResult\":%d}\n",getpid(),first,repeat);
    fflush(f);ftruncate(fileno(f),ftell(f));fclose(f);return first||repeat;
}
