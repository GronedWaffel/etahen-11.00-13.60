#include <errno.h>
#include "../include/injector.h"
#include "ps5/klog.h"
#include <stddef.h>
#include "port_diagnostic.h"
#ifdef ETAHEN_PORT_1360
#define klog_puts puts
#define klog_printf printf
#endif

// Position-independent stager. Compiler/linker ordering cannot change its size.
// push rbp; mov rbp,rsp; sub rsp,16; pthread_create(&thread,0,entry,args); int3.
static const uint8_t thread_stager[] = {
  0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x10,0x49,0x89,0xf8,
  0x48,0x8d,0x7d,0xf8,0x31,0xf6,0x49,0x8b,0x50,0x08,
  0x49,0x8b,0x48,0x10,0x41,0xff,0x50,0x18,0xcc,0xc9,0xc3
};
_Static_assert(offsetof(SCEFunctions,elf_main)==8,"stager entry offset");
_Static_assert(offsetof(SCEFunctions,payload_args)==16,"stager args offset");
_Static_assert(offsetof(SCEFunctions,pthread_create_ptr)==24,"stager function offset");

int attached = false;
intptr_t remote_malloc = 0;
intptr_t remote_pthread_create = 0;
void* remote_pthread_join = NULL;
SCEFunctions sce_functions = {0};
intptr_t injector_result_address = 0;


int __attribute__((section(".stager_shellcode$1")))  stager(SCEFunctions* functions)
{
    pthread_t thread;
    functions->pthread_create_ptr(&thread, 0, (void *(*)(void *)) functions->elf_main, functions->payload_args);

    asm("int3");

    return 0;
}

//
// Just used to calculate stager size
//
int __attribute__((section(".stager_shellcode$2"))) stager_end()
{
    return 0;
}

//
// Poor man function size counter, temp stuff
//
uint32_t get_shellcode_size()
{
    return &stager_end - &stager;
}   


//
// Init all remote function pointers needed for injection
//
void init_remote_function_pointers(pid_t pid)
{
    if (!attached)
    {
        if (pt_attach(pid) < 0)
        {
            printf("Error attaching PID %d! aborting...\n", pid);
            return;
        }
    }

    char nid[12] = {0};
    //
    // Injector/loader specifics
    //
    nid_encode("malloc", nid);
    remote_malloc = pt_resolve(pid, nid);
    nid_encode("pthread_create", nid);
    remote_pthread_create = pt_resolve(pid, nid);
    nid_encode("nid_pthread_join", nid);
    remote_pthread_join = (void*) pt_resolve(pid, nid);
    port_diag("resolve-malloc",remote_malloc,0,0);
    port_diag("resolve-pthread-create",remote_pthread_create,0,0);

    //
    // Shellcode function pointers
    //
    nid_encode("sceKernelDebugOutText", nid);
    sce_functions.sceKernelDebugOutText = (void*) pt_resolve(pid, nid);
    sce_functions.pthread_create_ptr = (void*) remote_pthread_create;

}


int inject_elf(struct proc* proc, void* elf)
{   
    injector_result_address = 0;
    klog_puts("[+] Elevating injector...[+]");

    const uint64_t original_authid = kernel_get_ucred_authid(getpid());
    puts("Injector credentials read");
    if (!original_authid || kernel_set_ucred_authid(getpid(), PTRACE_AUTHID)) return false;
    puts("Injector credentials set; attaching");
    int status = false;
    uint64_t sce_ptr_mem;
    uint64_t shellcode_size = sizeof(thread_stager);

    if (pt_attach(proc->pid) < 0)
    {
        klog_printf("Error attaching into PID: %d\n", proc->pid);
        status = false;
        goto exit;
    }

    klog_printf("[+] Attached to %d! [+]\n", proc->pid);
    attached = true;

    init_remote_function_pointers(proc->pid);
    if (!remote_pthread_create) goto detach;

    klog_printf("[+] Loading ELF on %d...[+]\n", proc->pid);
    intptr_t entry = elfldr_load(proc->pid, (uint8_t*) elf);
    port_diag("ELF-load-entry",entry,entry<=0?-1:0,entry<=0?errno:0);

    if (entry <= 0)
    {
        klog_printf("[-] Failed to load ELF! [-]\n");
        goto detach;
    }

    intptr_t args = elfldr_payload_args(proc->pid);
    port_diag("ELF-payload-args",args,args<=0?-1:0,args<=0?errno:0);
    if (args <= 0) goto detach;
    if(pt_copyout(proc->pid,args+0x28,&injector_result_address,sizeof(injector_result_address)) ||
       !injector_result_address || pt_setint(proc->pid,injector_result_address,-1234567)) goto detach;
    klog_printf("[+] ELF entrypoint: %#02lx [+]\n[+] Payload Args: %#02lx [+]\n", entry, args);

    //  
    // Copy shellcode thread parameters & boot code
    //
    sce_functions.elf_main = (void*) entry;
    sce_functions.payload_args = (void*) args;


    uint64_t bootstrap = pt_mmap(proc->pid, 0, shellcode_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    
    if (!bootstrap || bootstrap == (uint64_t)MAP_FAILED)
    {
        klog_printf("Unable to allocate bootstrap code, injection aborted!\n");
        goto detach;
    } 

    //
    // Make it executable
    //
    if (kernel_mprotect(proc->pid, bootstrap, shellcode_size, PROT_EXEC|PROT_WRITE|PROT_READ) ||
        pt_copyin(proc->pid, thread_stager, bootstrap, shellcode_size)) goto detach;

    klog_printf("[+] Bootstrap code allocated at %#02lx [+]\n", bootstrap);
    //
    // Write the sce functions data
    //
    sce_ptr_mem = pt_mmap(proc->pid, 0, sizeof(sce_functions), PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (!sce_ptr_mem || sce_ptr_mem == (uint64_t)MAP_FAILED ||
        pt_copyin(proc->pid, &sce_functions, sce_ptr_mem, sizeof(SCEFunctions))) goto detach;

    klog_puts("[+] Triggering entrypoint... [+]");
    //
    // Call until hit a breakpoint
    //
    status = pt_call2(proc->pid, bootstrap, sce_ptr_mem, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL) == 0;
    port_diag("ELF-thread-stager",bootstrap,status,status?0:errno);

detach:
    if (pt_detach(proc->pid, 0)) status = false;
    attached = false;

    klog_puts("[+] ELF injection finished! [+]");

    klog_puts("[+] Detached [+]");
exit:
    if (kernel_set_ucred_authid(getpid(), original_authid)) status = false;
    return status;

}

//
// We can't stuck sceshellui for too long or the system will kill it's process, so we will load the library in a separated thread
//
module_info_t* load_remote_library(pid_t pid, const char* library_path, const char* library_name)
{
    if (!attached)
    {
        if (pt_attach(pid) < 0)
        {
            printf("load_remote_library: Failed to attach PID %d\n", pid);
            return NULL;
        }
    }

    // intptr_t library_str = pt_call(pid, 0, 0x100, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    intptr_t library_str = pt_call(pid, remote_malloc, strlen(library_name) + 1);
    mdbg_copyin(pid, library_path, library_str, strlen(library_path) + 1);

    //
    // Run the module loading in a separated thread
    //
    intptr_t sce_kernel_load_start_module = pt_resolve(pid, nid_sce_kernel_load_start_module);
    create_remote_thread(pid, sce_kernel_load_start_module, library_str);

    printf("sce_kernel_load_start_module: %#02lx\n", sce_kernel_load_start_module);
    //
    // Now we detach, sleep a little and attach again
    //
    pt_detach(pid, 0);

    int retries = 0;
    int max_retries = 100;
    module_info_t* module = NULL;

    while (retries <= max_retries)
    {
        module = get_module_handle(pid, library_name);
        if (!module)
        {
            usleep(500);
        } else
        {
            break;
        }
        retries++;
    }
    
    if (!module)
    {
        printf("Unable to load %s into PID %d!\n", library_name, pid);
    }

    pt_attach(pid);

    return module;
}


int create_remote_thread(pid_t pid, uintptr_t target_address, uintptr_t parameters)
{
    if (!attached)
    {
        if (pt_attach(pid) < 0)
        {
            printf("Unable to attach into the remote process!\n");
            return false;
        }
    }

    intptr_t pthread = pt_call(pid, remote_malloc, sizeof(pthread_t));
    if (!pthread)
    {
        printf("Unable to allocate memory for pthread pointer!\n");
        return false;
    }

    //
    // We don't have to wait (join), otherwise we would block the whole target
    //
    return pt_call(pid, remote_pthread_create, pthread, 0, target_address, parameters);
}




