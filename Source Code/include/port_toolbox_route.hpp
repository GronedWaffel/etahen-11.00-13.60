// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string.h>
#include <ps5/kernel.h>
// OnionHEN settings-bundle profiles identify the legacy route from 11.00 onward.
// https://github.com/aydencharles/onionHEN/blob/b23ffe674b2de9f62fe634944c9230ff149d593a/source/include/onion/debug_settings_route_policy.hpp
// kernel_get_fw_version uses SDK encoding (e.g. 11.20 = 0x11200000).
inline const char* port_toolbox_uri(){
 return (kernel_get_fw_version()&0xffff0000u)>=0x11000000u?
  "pssettings:play?mode=settings&function=debug_settings_old":
  "pssettings:play?mode=settings&function=debug_settings";
}
#define ETAHEN_TOOLBOX_URI port_toolbox_uri()
#define ETAHEN_TOOLBOX_ROOT_URI "etaHEN?Toolbox"
inline bool port_toolbox_root_requested(const char* uri){
 return uri&&(!strcmp(uri,"pshome:gamehub?titleId=ETHN13600")||!strcmp(uri,ETAHEN_TOOLBOX_ROOT_URI)||!strcmp(uri,"pssettings:play?mode=settings&function=debug_settings_old&etahen_root=1"));
}
