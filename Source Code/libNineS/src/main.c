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
