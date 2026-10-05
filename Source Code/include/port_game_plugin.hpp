// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "port_plugin.h"
#include "port_prx.h"
#include <string>

static inline bool port_game_title(const std::string &id) {
    if (id.size()!=9) return false;
    const std::string prefix=id.substr(0,4);
    if(prefix!="PPSA" && prefix!="PPSB" && prefix!="CUSA" && prefix!="PCAS" &&
       prefix!="PCJS" && prefix!="PCKS" && prefix!="CUHJ" && prefix!="SCUS") return false;
    for(size_t i=4;i<9;i++)if(id[i]<'0'||id[i]>'9')return false;
    return true;
}
static inline bool port_game_plugin_path(const std::string &path, std::string *title) {
    const std::string prefix="/data/etaHEN/game_plugins/";
    if(path.compare(0,prefix.size(),prefix)!=0 || path.size()>240) return false;
    const std::string rest=path.substr(prefix.size());
    if(rest.size()<15 || rest[9]!='/' || !port_game_title(rest.substr(0,9)))return false;
    const std::string name=rest.substr(10);
    if(name.find('/')!=std::string::npos || name.find('\\')!=std::string::npos ||
       name.find("..")!=std::string::npos || name[0]=='.' || (!port_plugin_suffix(name.c_str(),".plugin") && !port_prx_suffix(name.c_str())))return false;
    if(title)*title=rest.substr(0,9);
    return true;
}
