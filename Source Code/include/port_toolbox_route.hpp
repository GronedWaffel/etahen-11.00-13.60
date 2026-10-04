// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string.h>
#include <ps5/kernel.h>
// Preserve upstream routing below 13.xx and the working 13.60 legacy page.
// The untested intermediate firmware routes remain community-test candidates.
inline const char* port_toolbox_uri(){
 return (kernel_get_fw_version()&0xffff0000u)>=0x13000000u?
  "pssettings:play?mode=settings&function=debug_settings_old":
  "pssettings:play?mode=settings&function=debug_settings";
}
#define ETAHEN_TOOLBOX_URI port_toolbox_uri()
#define ETAHEN_TOOLBOX_ROOT_URI "etaHEN?Toolbox"
inline bool port_toolbox_root_requested(const char* uri){
 return uri&&(!strcmp(uri,ETAHEN_TOOLBOX_ROOT_URI)||!strcmp(uri,"pssettings:play?mode=settings&function=debug_settings_old&etahen_root=1"));
}
