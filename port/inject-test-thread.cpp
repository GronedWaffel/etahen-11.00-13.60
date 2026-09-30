// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <unistd.h>
#include <ps5/kernel.h>
static unsigned long must_be_zero[256];
int main(){
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000)return 2;
 bool bss=true;for(auto x:must_be_zero)if(x)bss=false;
 FILE *o=fopen("/data/etahen-1360-test-thread.json","w");if(!o)return 1;
 fprintf(o,"{\"pid\":%d,\"bssZero\":%s,\"threadRan\":true}\n",getpid(),bss?"true":"false");fclose(o);return 0;
}
