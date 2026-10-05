// SPDX-License-Identifier: GPL-3.0-or-later
// Native .plugin controller; unified dev7 owns bounded game hook publication.
#include "../Source Code/include/port_fps_limiter.h"
#include <stdio.h>
#include <unistd.h>
extern "C" int sceKernelSendNotificationRequest(int,void*,size_t,int);
int main(){
    struct Notification {char prefix[45];char text[3075];} n{};
    snprintf(n.text,sizeof(n.text),"GTA V 15 FPS test limiter armed (requires unified dev7). Waiting for GTA V; Stop this plugin to disable.");
    sceKernelSendNotificationRequest(0,&n,sizeof(n),0);
    // etaHEN's receipt + matching live process identity form the enable signal.
    // No stale marker can enable pacing after Stop, process exit, or reboot.
    for(;;)sleep(60);
}
