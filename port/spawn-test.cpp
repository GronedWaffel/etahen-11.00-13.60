// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#ifdef SPAWN_TEST_CHILD
static unsigned char zeroes[32768];
int main(){bool zero=true;for(auto c:zeroes)if(c)zero=false;
 FILE* f=fopen("/data/etahen-1360-spawn-child.json","w");if(!f)return 1;
 fprintf(f,"{\"pid\":%d,\"bssZero\":%s,\"ran\":true}",getpid(),zero?"true":"false");fclose(f);return zero?0:2;}
#else
extern "C" int elfldr_spawn(const char*,int,uint8_t*,const char*);
extern "C" unsigned char child_start[];
int main(){unlink("/data/etahen-1360-spawn-child.json");
 freopen("/data/etahen-1360-spawn.log","w",stdout);setvbuf(stdout,nullptr,_IONBF,0);puts("Starting isolated service-spawn test");
 int pid=elfldr_spawn("/",1,child_start,"etaHEN SpawnTest");printf("Spawn result %d\n",pid);return pid>0?0:1;}
#endif
