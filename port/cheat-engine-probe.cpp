// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the real parser/toggle implementation on an explicitly allocated
// diagnostic scratch page. Refuses pages without the host's probe marker.
#include <stdarg.h>
#include "../Source Code/util/source/CheatManager.cpp"
extern "C" {
#include "../Source Code/libNineS/include/pt.h"
}
extern "C" void etaHEN_log(const char* fmt,...){va_list ap;va_start(ap,fmt);vprintf(fmt,ap);puts("");va_end(ap);}
extern "C" void notify(bool,const char*,...){}
extern "C" int pt_attach_proc(pid_t pid){return pt_attach(pid);}
extern "C" int pt_detach_proc(pid_t pid,int sig){return pt_detach(pid,sig);}
int main(){
 freopen("/data/etahen-cheat-engine-probe.log","w",stdout);setvbuf(stdout,nullptr,_IONBF,0);
 FILE* f=fopen("/data/etahen-cheat-engine-probe.cfg","r");int pid=0;unsigned long addr=0;
 if(!f||fscanf(f,"%d %lx",&pid,&addr)!=2)return 1;fclose(f);
 unsigned long marker=0;uint32_t handle=0;
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000||pid<=0||addr%0x4000||kernel_dynlib_handle(pid,"eboot.bin",&handle)||kernel_proc_copyout(pid,addr,&marker,8)||marker!=0x50524f4245434854ULL)return 2;
 unsigned long base=kernel_dynlib_mapbase_addr(pid,handle),offset=addr+8-base;
 if(!base||addr<=base)return 3;
 char json[512],xml[768];snprintf(json,sizeof json,"{\"name\":\"Scratch test\",\"process\":\"eboot.bin\",\"mods\":[{\"name\":\"Probe\",\"memory\":[{\"offset\":\"%lx\",\"on\":\"11223344\",\"off\":\"00000000\"}]}]}",offset);
 f=fopen("/data/etahen-cheat-engine-probe.json","w");if(!f)return 4;fputs(json,f);fclose(f);
 snprintf(xml,sizeof xml,"<Trainer Game=\"Scratch test\" Process=\"eboot.bin\" Moder=\"test\"><Cheat Text=\"Probe\"><Cheatline><Offset>%lx</Offset><ValueOn>11-22-33-44</ValueOn><ValueOff>00-00-00-00</ValueOff></Cheatline></Cheat></Trainer>",offset);
 bool all=true;
 for(int format=0;format<2;format++){
  currentGameCheat=format?CheatManager::CheatManagerFormats::ParseXMLCheat(xml,nullptr):CheatManager::CheatManagerFormats::ParseJSONCheat("/data/etahen-cheat-engine-probe.json",nullptr);
  if(!currentGameCheat||currentGameCheat->cheats.size()!=1)return 5;
  bool relative=!currentGameCheat->cheats[0].mods[0].absolute;std::string name;uint32_t value=0;
  bool enabled=relative&&CheatManager::ToggleCheat(pid,"scratch-only",0,name);
  bool on=!kernel_proc_copyout(pid,addr+8,&value,4)&&value==0x44332211;
  bool disabled=enabled&&CheatManager::ToggleCheat(pid,"scratch-only",0,name);
  bool off=!kernel_proc_copyout(pid,addr+8,&value,4)&&value==0;
  printf("RESULT format=%s relative=%d enabled=%d on=%d disabled=%d off=%d\n",format?"SHN/MC4 XML":"JSON",relative,enabled,on,disabled,off);
  all &= relative&&enabled&&on&&disabled&&off;delete currentGameCheat;currentGameCheat=nullptr;if(!all)break;
 }
 printf("COMPLETE pass=%d\n",all);return all?0:6;
}
