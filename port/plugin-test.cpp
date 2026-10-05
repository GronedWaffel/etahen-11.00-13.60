// SPDX-License-Identifier: GPL-3.0-or-later
// etaHEN SDK-compatible daemon fixtures, packaged with the official header.
// These fixtures never inject into games or change input, code or frame rates.
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
extern "C" int sceKernelSendNotificationRequest(int,void*,size_t,int);
extern "C" int sceKernelGetProcessName(int,char*);
extern "C" int sceSystemServiceGetAppIdOfRunningBigApp();
extern "C" int sceSystemServiceGetAppTitleId(int,char*);
#ifdef SYSTEM_PLUGIN_TEST
static const char *identity="SNPS00001";
#else
static const char *identity="SNPG00001";
#endif
static void report(const char *phase,const char *title="",bool popup=true){
 char name[64]={};sceKernelGetProcessName(getpid(),name);
 struct Notification {char prefix[45];char text[3075];} n{};
 snprintf(n.text,sizeof(n.text),"etaHEN .plugin %s v1.00: %s | PID %d | %.9s | %.31s",identity,phase,getpid(),title,name);
 if(popup)sceKernelSendNotificationRequest(0,&n,sizeof(n),0);
 mkdir("/data/etaHEN/plugin-tests",0755);
 char path[128];snprintf(path,sizeof(path),"/data/etaHEN/plugin-tests/%s.log",identity);
 struct stat st{};if(stat(path,&st)==0 && st.st_size>1024*1024){char old[140];snprintf(old,sizeof(old),"%s.old",path);rename(path,old);}
 FILE *f=fopen(path,"a");
 if(f){fprintf(f,"%lld %s\n",(long long)time(nullptr),n.text);fflush(f);fsync(fileno(f));fclose(f);}
}
int main(){
 report("daemon initialized");
#ifdef SYSTEM_PLUGIN_TEST
 for(;;){sleep(10);report("daemon alive","",false);}
#else
 char previous[32]={};int previous_app=-1;unsigned ticks=0;
 for(;;){
  int app=sceSystemServiceGetAppIdOfRunningBigApp();char title[32]={};
  if(app<0 || sceSystemServiceGetAppTitleId(app,title)!=0)app=-1;
  bool game=!strncmp(title,"PPSA",4)||!strncmp(title,"PPSB",4)||!strncmp(title,"CUSA",4)||!strncmp(title,"PCAS",4)||!strncmp(title,"PCJS",4);
  if(!game){title[0]=0;app=-1;}
  if(app!=previous_app || strcmp(title,previous)){
   if(previous_app>=0)report("game left",previous);
   if(app>=0)report("game detected",title);
   memcpy(previous,title,sizeof(previous));previous_app=app;
  }
  if(++ticks%10==0)report("daemon alive",title,false);
  sleep(1);
 }
#endif
}
