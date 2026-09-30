// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the same embedded helper/spawn path as the full bootstrap,
// without reloading the running etaHEN services or hooks.
#include <stdint.h>
#include <stdio.h>
extern "C" int elfldr_spawn(const char*,int,uint8_t*,const char*);
extern "C" unsigned char child_start[];
int main(){
 FILE* log=fopen("/data/etaHEN/card-spawn-test.log","w");if(!log)return 1;
 int pid=elfldr_spawn("/",fileno(log),child_start,"etaHEN Card Setup");
 fprintf(log,"Card helper spawn pid=%d\n",pid);fclose(log);
 return pid>0?0:2;
}
