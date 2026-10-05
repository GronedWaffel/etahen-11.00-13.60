#include "../Source Code/include/port_prx.h"
#include "../Source Code/include/port_fps_limiter.h"
#include <assert.h>
#include <fstream>
#include <vector>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
static void u64(unsigned char* p,uint64_t v){memcpy(p,&v,8);}
static int calls=0,nextHandle=4,nextStart=0;
using Gate=void(FPS_SYSV *)(PrxControl*,unsigned);
static Gate gate=nullptr;static PrxControl* current=nullptr;
static int FPS_SYSV load(const char* path,uint64_t argc,const void* argv,unsigned flags,const void* opt,int* result){
 assert(path&&strstr(path,"/data/etaHEN/game_plugins/")==path);assert(!argc&&!argv&&!flags&&!opt);
 ++calls;gate(current,0); // Loader reentry must not start a second module.
 *result=nextStart;return nextHandle;
}
int main(int argc,char** argv){
 unsigned char module[512]{};memcpy(module,"\177ELF\2\1\1",7);
 module[16]=0x18;module[17]=0xfe;module[18]=62;module[52]=64;module[54]=56;module[56]=2;u64(module+32,64);
 module[64]=1;module[68]=5;u64(module+64+32,512);u64(module+64+40,512);module[120]=2;
 int platform=-1;assert(port_prx_parse(module,512,512,&platform)&&platform==0);
 module[16]=3;module[17]=0;assert(!port_prx_parse(module,512,512,&platform));module[16]=0x18;module[17]=0xfe;
 module[18]=40;assert(!port_prx_parse(module,512,512,&platform));module[18]=62;
 module[120]=0;assert(!port_prx_parse(module,512,512,&platform));module[120]=2;
 u64(module+32,UINT64_MAX);assert(!port_prx_parse(module,512,512,&platform));u64(module+32,64);
 for(unsigned n=0;n<176;n++)assert(!port_prx_parse(module,n,512,&platform));
 unsigned char self[1024]{};memcpy(self,"\x4f\x15\x3d\x1d",4);u64(self+16,1024);self[24]=1;
 u64(self+40,576);u64(self+48,448);memcpy(self+64,module,512);
 assert(port_prx_parse(self,1024,1024,&platform)&&platform==4);
 memcpy(self,"\x54\x14\xf5\xee",4);assert(port_prx_parse(self,1024,1024,&platform)&&platform==5);
 self[24]=255;assert(!port_prx_parse(self,1024,1024,&platform));self[24]=1;
 u64(self+48,449);assert(!port_prx_parse(self,1024,1024,&platform));u64(self+48,448);
 assert(argc==2);std::ifstream file(argv[1],std::ios::binary);assert(file);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),{});assert(!bytes.empty());
#ifdef _WIN32
 void* code=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(code);memcpy(code,bytes.data(),bytes.size());DWORD old;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));
 gate=(Gate)code;PrxControl c{};current=&c;c.load_fn=(uintptr_t)load;
 strcpy(c.slots[0].path,"/data/etaHEN/game_plugins/CUSA12345/test.prx");
 gate(&c,0);assert(!calls);c.slots[0].enabled=1;
 gate(&c,0);assert(calls==1&&c.slots[0].state==2&&c.slots[0].handle==4&&!c.busy);
 gate(&c,0);assert(calls==1); // No repeated initialization.
 c.slots[0].enabled=0;gate(&c,0);assert(calls==1); // Stop never guesses an unload.
 strcpy(c.slots[1].path,c.slots[0].path);c.slots[1].enabled=1;nextHandle=-123;
 gate(&c,0);assert(calls==2&&c.slots[1].state==3&&c.slots[1].handle==-123);
 gate(&c,0);assert(calls==2);
 strcpy(c.slots[2].path,c.slots[0].path);c.slots[2].enabled=1;nextHandle=5;nextStart=-7;
 gate(&c,0);assert(calls==3&&c.slots[2].state==3&&c.slots[2].start_result==-7);
 VirtualFree(code,0,MEM_RELEASE);
#endif
}
