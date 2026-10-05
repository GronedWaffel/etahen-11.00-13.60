// SPDX-License-Identifier: GPL-3.0-or-later
// Harmless lifecycle fixture: no game patches, controller hooks or frame delays.
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
extern "C" int sceKernelSendNotificationRequest(int,void*,size_t,int);
extern "C" int sceKernelGetProcessName(int,char*);
extern "C" int sceKernelGetAppInfo(int,void*);
struct AppInfo {uint32_t app;uint64_t unknown;uint32_t type;char title[10];char tail[60];};
static void report(const char *phase){
 char name[64]={};AppInfo app{};int name_rc=sceKernelGetProcessName(getpid(),name);
 int app_rc=sceKernelGetAppInfo(getpid(),&app);app.title[9]=0;
 struct Notification {char prefix[45];char text[3075];} n{};
#ifdef SYSTEM_PLUGIN_TEST
 const char *kind="System";
#else
 const char *kind="Game";
#endif
 snprintf(n.text,sizeof(n.text),"etaHEN %s plugin test: %s | PID %d | %.9s | %.31s",kind,phase,getpid(),app_rc?"unknown":app.title,name_rc?"unknown":name);
 sceKernelSendNotificationRequest(0,&n,sizeof(n),0);
 mkdir("/data/etaHEN/plugin-tests",0755);
 FILE *f=fopen("/data/etaHEN/plugin-tests/lifecycle.log","a");
 if(f){fprintf(f,"%s\n",n.text);fflush(f);fsync(fileno(f));fclose(f);}
}
int main(){
 report("initialized");
#ifdef SYSTEM_PLUGIN_TEST
 // Remains alive so start/stop and duplicate prevention are observable.
 for(;;)sleep(60);
#endif
 return 0;
}
