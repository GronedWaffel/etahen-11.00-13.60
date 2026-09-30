// SPDX-License-Identifier: GPL-3.0-or-later
#include <ps5/payload.h>
#include <ps5/kernel.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "detour_port.h"
extern "C" int __wrap___patch_init(){return 0;}
extern "C" { int port_probe_value=7; int port_probe(int); }
static int(*original)(int);
static char* report;
static char lastStage[160];
static void progress(const char* stage){snprintf(lastStage,sizeof(lastStage),"%s",stage);snprintf(report,0x3000,"{\"stage\":\"%s\"}",stage);}
struct Credentials {
 uint64_t auth;unsigned char caps[16];bool ready=false;
 Credentials(){auth=kernel_get_ucred_authid(getpid());if(!auth||kernel_get_ucred_caps(getpid(),caps))return;unsigned char full[16];memset(full,0xff,16);ready=true;kernel_set_ucred_authid(getpid(),0x4800000000010003ULL);kernel_set_ucred_caps(getpid(),full);}
 ~Credentials(){if(ready){kernel_set_ucred_caps(getpid(),caps);kernel_set_ucred_authid(getpid(),auth);}}
};
static int replacement(int x){return original(x)+100;}
int main(){
 report=(char*)payload_get_args()+0x400;memset(report,0,0x3000);
 Credentials credentials;
 EnablePortExternalPublication();
 strcpy(report,"{\"stage\":\"hook test started\"}");
 int(*volatile invoke)(int)=port_probe;int before=invoke(5);
 snprintf(report,0x3000,"{\"stage\":\"baseline returned\",\"value\":%d}",before);
 if(!BeginPortDetours()){strcpy(report,"{\"error\":\"begin\"}");return 1;}
 strcpy(report,"{\"stage\":\"preparing trampoline\"}");
 original=(int(*)(int))PortDetourFunction((uint64_t)port_probe,(void*)replacement,progress);
 strcpy(report,"{\"stage\":\"trampoline prepared\"}");
 int staged=invoke(5);
 if(!original){RollbackPortDetours();snprintf(report,0x3000,"{\"error\":\"prepare\",\"lastStage\":\"%s\"}",lastStage);return 2;}
 strcpy(report,"{\"stage\":\"committing hook\"}");
 if(!CommitPortDetours()){strcpy(report,"{\"error\":\"commit\"}");return 3;}
 strcpy(report,"{\"stage\":\"calling hooked test function\"}");
 int hooked=invoke(5);bool restored=RollbackPortDetours();int after=invoke(5);
 bool pass=before==12&&staged==12&&hooked==112&&restored&&after==12;
 snprintf(report,0x3000,"{\"before\":%d,\"staged\":%d,\"hooked\":%d,\"restored\":%s,\"after\":%d,\"pass\":%s}",before,staged,hooked,restored?"true":"false",after,pass?"true":"false");return pass?0:4;
}
