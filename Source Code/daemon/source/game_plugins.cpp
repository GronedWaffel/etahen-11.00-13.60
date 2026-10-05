// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../include/port_game_plugin.hpp"
#include "../../include/port_plugin_runtime.h"
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
extern "C" pid_t elfldr_spawn(const char *,int,uint8_t *,const char *);
extern pthread_mutex_t jb_lock;
extern "C" int sceKernelGetProcessName(int,char *);

static int session_pid=-1,session_app=-1;
static std::string session_title;
static std::set<std::string> attempted;
// Called under jb_lock: the daemon loader shares its libelfldr scratch state.
static bool load_locked(const std::string &path,bool automatic=false) {
    if(port_prx_suffix(path.c_str())){extern bool port_request_prx(const std::string&,bool,bool);return port_request_prx(path,true,automatic);}
    std::string title,current;int app=-1;
    if(!port_game_plugin_path(path,&title))return false;
    // Manual Start launches the watcher even on the dashboard. Automatic starts
    // remain title-scoped; plugins own validation before touching a game.
    if(automatic && (!Get_Running_App_TID(current,app) || current!=title))return false;
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
    if(automatic){int check=-1;std::string check_title;
        if(!Get_Running_App_TID(check_title,check) || check!=app || check_title!=title)return false;}
    experimental_event("game-plugin","starting plugin watcher; game need not be open for manual Start",app,0);
    bool ok=port_plugin_load(path.c_str(),STDOUT_FILENO,sceKernelGetProcessName,elfldr_spawn)!=0;
    experimental_event("game-plugin",ok?"plugin process started; plugin owns game-specific initialization":"plugin process launch failed",app,ok?0:errno);
    return ok;
}
bool port_load_game_plugin(const std::string &path,bool enabled) {
    pthread_mutex_lock(&jb_lock);
    extern bool port_request_prx(const std::string&,bool,bool);
    bool ok=port_prx_suffix(path.c_str())?port_request_prx(path,enabled,false):(enabled&&load_locked(path));
    pthread_mutex_unlock(&jb_lock);
    return ok;
}
// Existing game monitor already owns jb_lock and calls this once per poll.
void port_poll_game_plugins(const std::string &title,int app) {
    if(!port_game_title(title))return;
    if(app!=session_app || title!=session_title){attempted.clear();session_pid=-1;session_app=app;session_title=title;}
    std::string directory="/data/etaHEN/game_plugins/"+title;
    DIR *dir=opendir(directory.c_str());if(!dir)return;
    dirent *entry;
    while((entry=readdir(dir))){
        if(!port_plugin_suffix(entry->d_name,".plugin")&&!port_prx_suffix(entry->d_name))continue;
        std::string path=directory+"/"+entry->d_name;
        if(access((path+".auto_start").c_str(),F_OK)==0 && attempted.insert(path).second)load_locked(path,true);
    }
    closedir(dir);
}
void port_reset_game_plugins() {
    session_pid=-1;session_app=-1;session_title.clear();attempted.clear();
}
