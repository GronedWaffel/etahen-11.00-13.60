#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <elf.h>
#include <signal.h>

#include "../include/proc.h"
#include "../include/ucred.h"
#include "../include/injector.h"
#include "../include/notify.h"
#include "../include/server.h"

#include "ps5/mdbg.h"

#include <dlfcn.h>
extern int port_service_publication(int pid);
extern int port_game_publication(int pid);

// Game plugins do not publish ShellUI hooks or overwrite its observer target.
bool Inject_GamePlugin(int pid, uint8_t *elf) {
    if (pid <= 1 || !elf) return false;
    struct proc *target = get_proc_by_pid(pid);
    if (!target) return false;
    bool ok = inject_elf(target, elf);
    if(ok && port_game_publication(pid))ok=false;
    free(target);
    return ok;
}

bool Inject_Toolbox(int pid, uint8_t * elf)
{                                  
    if(pid < 0 || !elf){
        notify_send("Invalid ToolBox arguments");
        return false;
    } 
    bool success = true;
    printf("Resolving target process %d\n", pid);
    struct proc* target_proc = get_proc_by_pid(pid);
    printf("Target process resolved: %s\n", target_proc ? "yes" : "no");
    if (target_proc)
    {
        if (!(success = inject_elf(target_proc, elf)))
            notify_send("ELF failed to inject!");
        
        free(target_proc);
    }
    else{
        notify_send("unable to find shellui");
        return false;
    }

#ifdef ETAHEN_PORT_1360
    if(success&&port_service_publication(pid))return false;
#endif
    return success;
}
