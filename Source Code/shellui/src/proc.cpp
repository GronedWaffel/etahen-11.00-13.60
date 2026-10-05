#include "private-1240-p5.h"
#include <errno.h>
extern "C"{
#include "../include/proc.h"
}
struct proc* find_proc_by_name(const char* proc_name)
{

    uint64_t next = 0;
    kernel_copyout(KERNEL_ADDRESS_ALLPROC, &next, sizeof(uint64_t));
    struct proc* proc = (struct proc*) malloc(sizeof(struct proc));
    do
    {
        kernel_copyout(next, (void*) proc, sizeof(struct proc));
        if (!strcmp(proc->p_comm, proc_name))
            return proc;

        kernel_copyout(next, &next, sizeof(uint64_t));

    } while (next);

    free(proc);
    return NULL;
}

struct proc* get_proc_by_title_id(const char* title_id)
{
    uint64_t next = 0;
    kernel_copyout(KERNEL_ADDRESS_ALLPROC, &next, sizeof(uint64_t));
    struct proc* proc = (struct proc*) malloc(sizeof(struct proc));
    do
    {
        kernel_copyout(next, (void*) proc, sizeof(struct proc));
        if (!strcmp(proc->title_id, title_id))
            return proc;

        kernel_copyout(next, &next, sizeof(uint64_t));

    } while (next);

    free(proc);
    return NULL;
}

void list_all_proc_and_pid()
{

    uint64_t next = 0;
    kernel_copyout(KERNEL_ADDRESS_ALLPROC, &next, sizeof(uint64_t));
    struct proc* proc = (struct proc*) malloc(sizeof(struct proc));
    struct vmspace vmspace;

    do
    {
        kernel_copyout(next, (void*) proc, sizeof(struct proc));

        kernel_copyout((intptr_t) proc->p_vmspace, (void*) &vmspace, sizeof(vmspace));

        printf("%s - %d\n", proc->p_comm, proc->pid);

        kernel_copyout(next, &next, sizeof(uint64_t));

    } while (next);

    free(proc);
}

struct proc* get_proc_by_pid(pid_t pid)
{
    uintptr_t next = 0;

    kernel_copyout(KERNEL_ADDRESS_ALLPROC, &next, sizeof(uintptr_t));
    struct proc* proc =  (struct proc*) malloc(sizeof(struct proc));
    do
    {
        kernel_copyout(next, proc, sizeof(struct proc));

        if (proc->pid == pid)
            return proc;

        kernel_copyout(next, &next, sizeof(uint64_t));

    } while (next);

    free(proc);
    return NULL;
}


//
// List process modules by using the sys_dynlib_get_info_ex syscall
//
void list_proc_modules(struct proc* proc)
{
    size_t num_handles = 0;
    syscall(SYS_dl_get_list, proc->pid, NULL, 0, &num_handles);
    
    if (num_handles)
    {
        uintptr_t* handles = (uintptr_t*) calloc(num_handles, sizeof(uintptr_t));
        syscall(SYS_dl_get_list, proc->pid, handles, num_handles, &num_handles);

        for (int i = 0; i < num_handles; ++i)
        {
            module_info_t mod_info;
            bzero(&mod_info, sizeof(mod_info));

            syscall(SYS_dl_get_info_2, proc->pid, 1, handles[i], &mod_info);

            printf("%s - ", mod_info.filename);
            printf("%#02lx\n", mod_info.init);
        }
        
        free(handles);
    }
}


module_info_t* get_module_info(pid_t pid, const char* module_name)
{
    size_t num_handles = 0;
    {P5Event("before module count"); int p5_rc=syscall(SYS_dl_get_list, pid, NULL, 0, &num_handles); P5Event("module count result",p5_rc,p5_rc?errno:0);}
    
    if (num_handles)
    {
        uintptr_t* handles = (uintptr_t*) calloc(num_handles, sizeof(uintptr_t));
        {P5Event("before module list"); int p5_rc=syscall(SYS_dl_get_list, pid, handles, num_handles, &num_handles); P5Event("module list result",p5_rc,p5_rc?errno:0);}

        module_info_t* mod_info = (module_info_t*) malloc(sizeof(module_info_t));
        
        for (int i = 0; i < num_handles; ++i)
        {
            bzero(mod_info, sizeof(module_info_t));
            {int p5_rc=syscall(SYS_dl_get_info_2, pid, 1, handles[i], mod_info); if(p5_rc)P5Event("module info failure",p5_rc,errno);}
            if (!strcmp(mod_info->filename, module_name))
            {
                return mod_info;
            }
        }
        
        free(handles);
        free(mod_info);
    }

    return NULL;
}


// SDK v0.43 kernel_dynlib_handle reads the kernel's module list; it does
// not require replacing ShellUI's auth ID with debugger credentials. Never
// fall back to privileged SYS_dl_get_list on the live UI process.
int get_module_handle(pid_t pid, const char* module_name)
{
    if (!module_name || !*module_name) { errno=EINVAL; return 0; }
    unsigned int handle=0;
    errno=0;
    const int rc=kernel_dynlib_handle(pid,module_name,&handle);
    const int error=rc ? (errno ? errno : EIO) : (handle ? 0 : ENOENT);
    P5Event("p6 SDK module lookup result",handle,error);
    if(rc || !handle){errno=error;return 0;}
    return (int)handle;
}
