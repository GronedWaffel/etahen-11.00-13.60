// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <unistd.h>
#include <ps5/kernel.h>
extern "C" int sceKernelSetProcessName(const char*);
int main(){
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000)return 1;
 FILE *o=fopen("/data/etahen-1360-test-host.txt","w");if(o){fprintf(o,"%d\n",getpid());fclose(o);}
 for(int i=0;i<120;++i)sleep(1);
 return 0;
}
