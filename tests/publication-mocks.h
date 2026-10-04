#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "port_publish.h"
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
struct reg {uint64_t r_rip;};
static int stopped,busy_once,fail_write,write_calls,fail_detach,flushes,events_count;
static unsigned char memory[65536];
static const char *operations[8192];
intptr_t injector_result_address=0x300;
static void port_diag(const char *operation,unsigned long long address,long long result,int error){
 (void)address;(void)result;(void)error;assert(events_count<8192);operations[events_count++]=operation;
}
static void port_diag_flush(void){assert(!stopped);flushes++;}
static void port_stage(const char *component,const char *stage){(void)component;(void)stage;assert(!stopped);}
static int pt_copyout(int pid,intptr_t address,void *bytes,size_t size){(void)pid;assert(address>=0&&(size_t)address+size<=sizeof memory);memcpy(bytes,memory+address,size);return 0;}
static int pt_copyin(int pid,const void *bytes,intptr_t address,size_t size){(void)pid;assert(stopped);assert(address>=0&&(size_t)address+size<=sizeof memory);if(++write_calls==fail_write){errno=EIO;return -1;}memcpy(memory+address,bytes,size);return 0;}
static int kernel_proc_copyin(int pid,const void *bytes,intptr_t address,size_t size){(void)pid;memcpy(memory+address,bytes,size);return 0;}
static int kernel_get_vmem_protection(int pid,uint64_t address,size_t size){(void)pid;(void)address;(void)size;assert(stopped);return PROT_READ|PROT_EXEC;}
static int kernel_mprotect(int pid,uint64_t address,size_t size,int protection){(void)pid;(void)address;(void)size;(void)protection;assert(stopped);return 0;}
static int pt_attach(int pid){(void)pid;assert(!stopped);stopped=1;return 0;}
static int pt_detach(int pid,int sig){(void)pid;(void)sig;assert(stopped);if(fail_detach)return -1;stopped=0;port_diag_flush();return 0;}
int pt_getlwps(pid_t pid,int *tids,size_t capacity){(void)pid;assert(capacity>0);tids[0]=44;return 1;}
static int pt_getregs(int tid,struct reg *regs){(void)tid;regs->r_rip=busy_once?0x2000:0x5000;busy_once=0;return 0;}
static int mock_usleep(unsigned n){(void)n;return 0;}
#define usleep mock_usleep
