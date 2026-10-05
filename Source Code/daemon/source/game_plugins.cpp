// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../include/port_game_plugin.hpp"
#include "../../include/experimental_trace.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <string>
#include <set>
#include <pthread.h>

extern bool Get_Running_App_TID(std::string &,int &);
extern int get_game_pid();
extern "C" bool Inject_GamePlugin(int, uint8_t *);
extern pthread_mutex_t jb_lock;
extern "C" int sceKernelGetProcessName(int,char *);

static int session_pid=-1,session_app=-1;
static std::string session_title;
static std::set<std::string> attempted;
// Called under jb_lock: injection uses shared libNineS scratch state.
static bool load_locked(const std::string &path) {
    std::string title,current;int app=-1;
    if(!port_game_plugin_path(path,&title) || !Get_Running_App_TID(current,app) || current!=title)return false;
    int pid=get_game_pid();char name[32]={};
    if(pid<=1 || sceKernelGetProcessName(pid,name)<0)return false;
    if(pid!=session_pid || app!=session_app || current!=session_title){
        session_pid=pid;session_app=app;session_title=current;attempted.clear();
    }
    // Never retry an uncertain in-process injection in the same game session.
    if(attempted.count(path))return false;
    struct stat st{};
    int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW);
    if(fd<0)return false;
    if(fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<64 || st.st_size>PORT_PLUGIN_MAX_SIZE){close(fd);return false;}
    std::vector<uint8_t> image((size_t)st.st_size);size_t done=0;
    while(done<image.size()){
        ssize_t n=read(fd,image.data()+done,image.size()-done);
        if(n<0 && errno==EINTR)continue;
        if(n<=0)break;
        done+=(size_t)n;
    }
    close(fd);PortPlugin info{};
    if(done!=image.size() || !port_plugin_parse(path.c_str(),image.data(),done,done,&info))return false;
    // Revalidate the foreground game after file I/O, immediately before attaching.
    int check=-1;std::string check_title;
    if(!Get_Running_App_TID(check_title,check) || check!=app || check_title!=title || get_game_pid()!=pid)return false;
    attempted.insert(path);
    experimental_event("game-plugin","dispatching game ELF",pid,0);
    bool ok=Inject_GamePlugin(pid,image.data());
    experimental_event("game-plugin",ok?"game ELF dispatched; plugin owns initialization":"game ELF dispatch failed; restart game before retry",pid,ok?0:errno);
    return ok;
}
bool port_load_game_plugin(const std::string &path) {
    pthread_mutex_lock(&jb_lock);
    bool ok=load_locked(path);
    pthread_mutex_unlock(&jb_lock);
    return ok;
}
// Existing game monitor already owns jb_lock and calls this once per poll.
void port_poll_game_plugins(const std::string &title,int app) {
    if(!port_game_title(title))return;
    if(app!=session_app || title!=session_title){attempted.clear();session_pid=-1;}
    std::string directory="/data/etaHEN/game_plugins/"+title;
    DIR *dir=opendir(directory.c_str());if(!dir)return;
    dirent *entry;
    while((entry=readdir(dir))){
        if(!port_plugin_suffix(entry->d_name,".elf"))continue;
        std::string path=directory+"/"+entry->d_name;
        if(access((path+".auto_start").c_str(),F_OK)==0 && !attempted.count(path))load_locked(path);
    }
    closedir(dir);
}
void port_reset_game_plugins() {
    session_pid=-1;session_app=-1;session_title.clear();attempted.clear();
}
