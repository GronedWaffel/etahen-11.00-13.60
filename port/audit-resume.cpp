// SPDX-License-Identifier: GPL-3.0-or-later
// Recovery for the temporary kstuff pause performed by audit-loader only.
#include <ps5/kernel.h>
#include <stdio.h>
#include <unistd.h>
int main(){
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000)return 1;
 FILE *j=fopen("/data/etahen-1360-audit-resume.txt","r");if(!j)return 2;
 unsigned a=0,b=0;int n=fscanf(j,"%x %x",&a,&b);fclose(j);
 if(n!=2||(a!=0xffff&&a!=0xdeb7)||(b!=0xffff&&b!=0xdeb7))return 3;
 intptr_t native=KERNEL_ADDRESS_DATA_BASE+0xDDD8F8+14,compat=KERNEL_ADDRESS_DATA_BASE+0xDDDA70+14;
 unsigned ca=(unsigned short)kernel_getshort(native),cb=(unsigned short)kernel_getshort(compat);
 if((ca!=0xffff&&ca!=a)||(cb!=0xffff&&cb!=b))return 4;
 if(kernel_setshort(native,a)||kernel_setshort(compat,b))return 5;
 unlink("/data/etahen-1360-audit-resume.txt");
 FILE *o=fopen("/data/etahen-1360-audit-recovered.txt","w");if(o){fputs("Original kstuff state restored\n",o);fclose(o);}return 0;
}
