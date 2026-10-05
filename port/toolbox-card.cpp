// SPDX-License-Identifier: GPL-3.0-or-later
// A separate, removable home-screen deep-link tile with transactional ordering.
#include <ps5/kernel.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#ifndef ETAHEN_CARD_REMOVE
#include "toolbox-card-order.hpp"
#endif
#include "../Source Code/include/port_toolbox_route.hpp"
#include "../Source Code/include/port_firmware.h"
#include "../Source Code/include/experimental_trace.h"
static int (*initialize)();
static int (*install)(const char*,const char*,void*);
static int (*installAll)(void*);
static int (*uninstall)(const char*,void*,void*);
extern "C" const unsigned char card_icon[],card_icon_end[];
static constexpr const char* title="ETHN13600";
static constexpr const char* directory="/user/app/ETHN13600";
static constexpr const char* owner="/user/app/ETHN13600/.etahen-toolbox-card";
static constexpr const char* receipt="/user/app/ETHN13600/.etahen-toolbox-registered";
// Retain the original ownership/receipt identity to reuse existing cards without rescanning.
static constexpr const char marker[]="etaHEN unofficial 13.60 Toolbox card v1\n";
#ifdef ETAHEN_CARD_REMOVE
static constexpr const char* result_path="/data/etaHEN/toolbox-card-remove.json";
#else
static constexpr const char* result_path="/data/etaHEN/toolbox-card-install.json";
#endif
static constexpr const char param[]=
 "{\"applicationCategoryType\":0,\"titleId\":\"ETHN13600\","
 "\"deeplinkUri\":\"" ETAHEN_TOOLBOX_ROOT_URI "\","
 "\"localizedParameters\":{\"defaultLanguage\":\"en-US\",\"en-US\":{\"titleName\":\"etaHEN Toolbox\"}}}";
static bool owned(){char bytes[sizeof(marker)]={0};FILE* f=fopen(owner,"rb");if(!f)return false;size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);return n==sizeof(marker)-1&&!memcmp(bytes,marker,n);}
static bool equals_file(const char* path,const void* data,size_t length){
 FILE* f=fopen(path,"rb");if(!f)return false;unsigned char bytes[4096];const auto* expected=(const unsigned char*)data;bool equal=true;
 while(length){size_t want=length<sizeof bytes?length:sizeof bytes;size_t n=fread(bytes,1,want,f);if(n!=want||memcmp(bytes,expected,want)){equal=false;break;}length-=want;expected+=want;}
 if(equal)equal=fgetc(f)==EOF&&!ferror(f);fclose(f);return equal;
}
static int write_file(const char* path,const void* bytes,size_t length){
 FILE* f=fopen(path,"wb");if(!f)return -errno;bool ok=fwrite(bytes,1,length,f)==length;
 if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;return ok?0:-EIO;
}
static int auth(uint64_t value){return kernel_set_ucred_authid(getpid(),value)||kernel_get_ucred_authid(getpid())!=value?-EPERM:0;}
static void checkpoint(const char* stage){experimental_event("toolbox-card",stage,0,0);FILE* f=fopen("/data/etaHEN/toolbox-card-entered.log","a");if(f){fprintf(f,"pid=%d %s\n",getpid(),stage);fflush(f);fsync(fileno(f));fclose(f);}}
int main(){
 const char* stage="firmware";int rc=0;uint32_t fw=0;size_t length=sizeof fw;
 uint64_t saved=0;bool changed=false;struct stat st{};
 checkpoint("entered");
 {FILE* f=fopen(result_path,"w");if(f){fprintf(f,"{\"pid\":%d,\"state\":\"starting\"}\n",getpid());fclose(f);}}
 if(sysctlbyname("kern.sdk_version",&fw,&length,nullptr,0)||length!=sizeof fw||!snipers_firmware_profile(fw)){rc=-ENOTSUP;goto done;}
 {char detail[160];snprintf(detail,sizeof detail,"profile=%s firmware=0x%08x route=%s",snipers_firmware_profile(fw)->name,fw,ETAHEN_TOOLBOX_URI);checkpoint(detail);}
 stage="symbols";
 // Resolve the services linked by this helper from its inherited scope.
 // Loading their modules again can hang; never guess firmware offsets or reload them.
 checkpoint("resolving inherited AppInstUtil");
 initialize=(int(*)())dlsym(RTLD_DEFAULT,"sceAppInstUtilInitialize");
 install=(int(*)(const char*,const char*,void*))dlsym(RTLD_DEFAULT,"sceAppInstUtilAppInstallTitleDir");
 uninstall=(int(*)(const char*,void*,void*))dlsym(RTLD_DEFAULT,"sceAppInstUtilAppUnInstall");
 installAll=(int(*)(void*))dlsym(RTLD_DEFAULT,"sceAppInstUtilAppInstallAll");
 if(!install)install=(int(*)(const char*,const char*,void*))dlsym(RTLD_DEFAULT,"Wudg3Xe3heE");
 {char detail[192];uint32_t handle=0;int found=kernel_dynlib_handle(getpid(),"libSceAppInstUtil.sprx",&handle);
 if(!found){if(!initialize)initialize=(int(*)())kernel_dynlib_dlsym(getpid(),handle,"sceAppInstUtilInitialize");
 if(!install)install=(int(*)(const char*,const char*,void*))kernel_dynlib_dlsym(getpid(),handle,"sceAppInstUtilAppInstallTitleDir");
 if(!install)install=(int(*)(const char*,const char*,void*))kernel_dynlib_resolve(getpid(),handle,"Wudg3Xe3heE");
 if(!installAll)installAll=(int(*)(void*))kernel_dynlib_dlsym(getpid(),handle,"sceAppInstUtilAppInstallAll");
 if(!uninstall)uninstall=(int(*)(const char*,void*,void*))kernel_dynlib_dlsym(getpid(),handle,"sceAppInstUtilAppUnInstall");}
 snprintf(detail,sizeof detail,"symbols init=%d install=%d scan=%d uninstall=%d moduleResult=%d handle=%u",!!initialize,!!install,!!installAll,!!uninstall,found,handle);checkpoint(detail);}
 #ifdef ETAHEN_CARD_REMOVE
 if(!initialize||!uninstall){rc=-ENOSYS;goto done;}
#else
 if(!initialize||(!install&&!installAll)){rc=-ENOSYS;goto done;}
#endif
 stage="ownership";
 if(lstat(directory,&st)==0){if(!S_ISDIR(st.st_mode)||!owned()){rc=-EEXIST;goto done;}}
 else if(errno!=ENOENT){rc=-errno;goto done;}
#ifdef ETAHEN_CARD_REMOVE
 else {rc=-ENOENT;goto done;}
#else
 else {if(mkdir(directory,0755)){rc=-errno;goto done;}rc=write_file(owner,marker,sizeof(marker)-1);if(rc)goto done;}
 if(equals_file(receipt,marker,sizeof(marker)-1)&&
    equals_file("/user/app/ETHN13600/sce_sys/param.json",param,sizeof(param)-1)&&
    equals_file("/user/app/ETHN13600/sce_sys/icon0.png",card_icon,card_icon_end-card_icon)){
  stage="already-installed";checkpoint(stage);goto done;
 }
 stage="assets";
 unlink(receipt);
 if(mkdir("/user/app/ETHN13600/sce_sys",0755)&&errno!=EEXIST){rc=-errno;goto done;}
 rc=write_file("/user/app/ETHN13600/sce_sys/param.json",param,sizeof(param)-1);if(rc)goto done;
 rc=write_file("/user/app/ETHN13600/sce_sys/icon0.png",card_icon,card_icon_end-card_icon);if(rc)goto done;
#endif
 stage="auth";saved=kernel_get_ucred_authid(getpid());if(!saved||saved==UINT64_MAX){rc=-EPERM;goto done;}
 changed=true;rc=auth(0x4801000000000013ULL);if(rc)goto done;
 stage="initialize";rc=initialize();if(rc)goto done;
#ifdef ETAHEN_CARD_REMOVE
 stage="uninstall-auth";rc=auth(0x3800000000000010ULL);if(rc)goto done;
 stage="uninstall";rc=uninstall(title,nullptr,nullptr);if(rc)goto done;
 // Only known files in our owned tile; never recursively remove a computed path.
 unlink("/user/app/ETHN13600/sce_sys/param.json");unlink("/user/app/ETHN13600/sce_sys/icon0.png");
 rmdir("/user/app/ETHN13600/sce_sys");unlink(receipt);unlink(owner);rmdir(directory);
#else
 stage=install?"register":"registration-scan";checkpoint(stage);
 rc=install?install(title,"/user/app/",nullptr):installAll(nullptr);
 if(!rc){stage="registered";rc=write_file(receipt,marker,sizeof(marker)-1);}
#endif
done:
#ifndef ETAHEN_CARD_REMOVE
 if(!rc && (strcmp(stage,"registered")==0 || strcmp(stage,"already-installed")==0)){
   stage="card-order";rc=pin_toolbox_card();checkpoint(rc?"card order could not be set":"card order saved; no dashboard refresh requested");
 }
#endif
 bool restored=!changed;
 if(changed)for(int i=0;i<3&&!restored;++i)restored=auth(saved)==0;
 experimental_event("toolbox-card",stage,rc,0);
 experimental_event("toolbox-card","authorization restored",restored?0:-EPERM,0);
 FILE* f=fopen(result_path,"w");
 if(f){fprintf(f,"{\"pid\":%d,\"state\":\"finished\",\"titleId\":\"%s\",\"stage\":\"%s\",\"result\":%d,\"authRestored\":%s}\n",getpid(),title,stage,rc,restored?"true":"false");fclose(f);}
 return rc||!restored?1:0;
}
