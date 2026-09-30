// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string.h>
// 13.60's React Native debug page does not consume the legacy Toolbox XML.
#ifdef ETAHEN_PORT_1360
#define ETAHEN_TOOLBOX_URI "pssettings:play?mode=settings&function=debug_settings_old"
#define ETAHEN_TOOLBOX_ROOT_URI ETAHEN_TOOLBOX_URI "&etahen_root=1"
#else
#define ETAHEN_TOOLBOX_URI "pssettings:play?mode=settings&function=debug_settings"
#define ETAHEN_TOOLBOX_ROOT_URI ETAHEN_TOOLBOX_URI
#endif
inline bool port_toolbox_root_requested(const char* uri){
 return uri&&(!strcmp(uri,ETAHEN_TOOLBOX_ROOT_URI)||!strcmp(uri,"etaHEN?Toolbox"));
}
