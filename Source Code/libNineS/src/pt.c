/* Copyright (C) 2024 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <ps5/kernel.h>
#include <ps5/mdbg.h>
#include <ps5/klog.h>

#include "../include/pt.h"
#include "port_timing.h"
#include "port_diagnostic.h"


static int
sys_ptrace(int request, pid_t pid, caddr_t addr, int data) {
  pid_t mypid = getpid();
  uint8_t caps[16], fullcaps[16];
  memset(fullcaps,0xff,sizeof fullcaps);
  uint64_t authid;
  int ret;

  if(!(authid=kernel_get_ucred_authid(mypid))) {
    return -1;
  }
  if(kernel_get_ucred_caps(mypid,caps)) return -1;
  if(kernel_set_ucred_authid(mypid, 0x4800000000010003l)) {
    return -1;
  }
  if(kernel_set_ucred_caps(mypid,fullcaps)) {
    kernel_set_ucred_authid(mypid,authid);
    return -1;
  }

  ret = (int)syscall(SYS_ptrace, request, pid, addr, data);
  if(ret<0)port_diag("ptrace-error",(unsigned)request,ret,errno);

  int restore_caps=kernel_set_ucred_caps(mypid,caps);
  int restore_auth=kernel_set_ucred_authid(mypid,authid);
  if(restore_caps||restore_auth)return -1;

  return ret;
}


intptr_t
pt_resolve(pid_t pid, const char* nid) {
  intptr_t addr;

  if((addr=kernel_dynlib_resolve(pid, 0x1, nid))) {
    return addr;
  }

  return kernel_dynlib_resolve(pid, 0x2001, nid);
}


int
pt_attach(pid_t pid) {
  port_diag("attach-begin",pid,0,0);port_diag_flush();
  puts("ptrace attach request");
  if(sys_ptrace(PT_ATTACH, pid, 0, 0) == -1) {
    perror("ptrace attach");
    return -1;
  }
  puts("ptrace attach accepted; waiting for stop");
#if defined(ETAHEN_TOOLBOX_DIAGNOSTIC) || defined(ETAHEN_EXPERIMENTAL_DIAGNOSTICS)
  port_diag("attach-accepted",pid,0,0);
  int wait_status=0;
  int waited=waitpid(pid,&wait_status,0);
  port_diag("attach-wait",pid,waited,waited<0?errno:0);
  port_diag("attach-wait-status",pid,wait_status,0);
  if(waited==-1)return -1;
#else
  if(waitpid(pid, 0, 0) == -1) {
    return -1;
  }
#endif

  return 0;
}


int
pt_detach(pid_t pid, int sig) {
  port_diag("detach-begin",pid,sig,0);
  if(sys_ptrace(PT_DETACH, pid, 0, sig) == -1) {
    port_diag("detach-failed",pid,-1,errno);
    return -1;
  }
  port_diag("detach-ok",pid,0,0);port_diag_flush();

  return 0;
}


int
pt_step(int pid) {
  if(sys_ptrace(PT_STEP, pid, (caddr_t)1, 0)) {
    return -1;
  }

#if defined(ETAHEN_TOOLBOX_DIAGNOSTIC) || defined(ETAHEN_EXPERIMENTAL_DIAGNOSTICS)
  int wait_status=0;
  int waited=waitpid(pid,&wait_status,0);
  port_diag("step-wait",pid,waited,waited<0?errno:0);
  port_diag("step-wait-status",pid,wait_status,0);
  if(waited<0)return -1;
#else
  if(waitpid(pid, 0, 0) < 0) {
    return -1;
  }
#endif

  return 0;
}


int
pt_continue(pid_t pid, int sig) {
  if(sys_ptrace(PT_CONTINUE, pid, (caddr_t)1, sig) == -1) {
    return -1;
  }

  return 0;
}


int
pt_getint(pid_t pid, intptr_t addr) {
  return sys_ptrace(PT_READ_D, pid, (caddr_t)addr, 0);
}


int
pt_setint(pid_t pid, intptr_t addr, int val) {
  return sys_ptrace(PT_WRITE_D, pid, (caddr_t)addr, val);
}


int
pt_getregs(pid_t pid, struct reg *r) {
  return sys_ptrace(PT_GETREGS, pid, (caddr_t)r, 0);
}
int pt_getlwps(pid_t pid,int* tids,size_t capacity){
 int count=sys_ptrace(PT_GETNUMLWPS,pid,0,0);
 if(count<=0||(size_t)count>capacity)return -1;
 int got=sys_ptrace(PT_GETLWPLIST,pid,(caddr_t)tids,count);
 return got==count?got:-1;
}


int
pt_setregs(pid_t pid, const struct reg *r) {
  return sys_ptrace(PT_SETREGS, pid, (caddr_t)r, 0);
}


int
pt_copyin(pid_t pid, const void* buf, intptr_t addr, size_t len) {
#ifdef ETAHEN_PORT_1360
  const uint64_t write_started=port_timing_now();
  struct ptrace_io_desc iod = {
    .piod_op = PIOD_WRITE_D, .piod_offs = (void*)addr,
    .piod_addr = (void*)buf, .piod_len = len};
  int result=sys_ptrace(PT_IO,pid,(caddr_t)&iod,0);
  port_diag("copyin",addr,result,result?errno:0);
  port_diag("copyin-length",len,iod.piod_len,0);
  port_timing_end("write",pid,write_started,len);
  printf("ptrace memory write: %d, errno %d, length %zu/%zu\n",result,errno,iod.piod_len,len);
  if(result || iod.piod_len!=len) return -1;
  // Some SDK mdbg paths return success after a short transfer. Verify every
  // byte before allowing the injector to execute the remote image.
  const uint64_t verify_started=port_timing_now();
  unsigned char check[4096];
  for(size_t offset=0;offset<len;) {
    size_t count=0x1000-((addr+offset)&0xfff);if(count>len-offset)count=len-offset;
    if(kernel_proc_copyout(pid,addr+offset,check,count) ||
       memcmp(check,(const unsigned char*)buf+offset,count)) {
      port_diag("copyin-verify-failed",addr+offset,count,errno);
      errno=EIO;
      return -1;
    }
    offset+=count;
  }
  port_timing_end("verify",pid,verify_started,len);
  return 0;
#else
  struct ptrace_io_desc iod = {
    .piod_op = PIOD_WRITE_D,
    .piod_offs = (void*)addr,
    .piod_addr = (void*)buf,
    .piod_len = len};
  return sys_ptrace(PT_IO, pid, (caddr_t)&iod, 0);
#endif
}


int
pt_setchar(pid_t pid, intptr_t addr, char val) {
  return pt_copyin(pid, &val, addr, sizeof(val));
}


int
pt_setshort(pid_t pid, intptr_t addr, short val) {
  return pt_copyin(pid, &val, addr, sizeof(val));
}


int
pt_setlong(pid_t pid, intptr_t addr, long val) {
  return pt_copyin(pid, &val, addr, sizeof(val));
}


int
pt_copyout(pid_t pid, intptr_t addr, void* buf, size_t len) {
#ifdef ETAHEN_PORT_1360
  for(size_t off=0;off<len;){size_t count=0x1000-((addr+off)&0xfff);if(count>len-off)count=len-off;
    if(kernel_proc_copyout(pid,addr+off,(unsigned char*)buf+off,count))return -1;off+=count;}
  return 0;
#else
  struct ptrace_io_desc iod = {
    .piod_op = PIOD_READ_D,
    .piod_offs = (void*)addr,
    .piod_addr = buf,
    .piod_len = len};
  return sys_ptrace(PT_IO, pid, (caddr_t)&iod, 0);
#endif
}


char
pt_getchar(pid_t pid, intptr_t addr) {
  char val = 0;

  pt_copyout(pid, addr, &val, sizeof(val));

  return val;
}


short
pt_getshort(pid_t pid, intptr_t addr) {
  short val = 0;

  pt_copyout(pid, addr, &val, sizeof(val));

  return val;
}


long
pt_getlong(pid_t pid, intptr_t addr) {
  long val = 0;

  pt_copyout(pid, addr, &val, sizeof(val));

  return val;
}


long
pt_call(pid_t pid, intptr_t addr, ...) {
  struct reg jmp_reg;
  struct reg bak_reg;
  va_list ap;

  if(pt_getregs(pid, &bak_reg)) {
    return -1;
  }

  memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));
  jmp_reg.r_rip = addr;

  va_start(ap, addr);
  jmp_reg.r_rdi = va_arg(ap, uint64_t);
  jmp_reg.r_rsi = va_arg(ap, uint64_t);
  jmp_reg.r_rdx = va_arg(ap, uint64_t);
  jmp_reg.r_rcx = va_arg(ap, uint64_t);
  jmp_reg.r_r8  = va_arg(ap, uint64_t);
  jmp_reg.r_r9  = va_arg(ap, uint64_t);
  va_end(ap);

  if(pt_setregs(pid, &jmp_reg)) {
    return -1;
  }

  // single step until the function returns
  while(jmp_reg.r_rsp <= bak_reg.r_rsp) {
    if(pt_step(pid)) {
      return -1;
    }
    if(pt_getregs(pid, &jmp_reg)) {
      return -1;
    }
  }

  // restore registers
  if(pt_setregs(pid, &bak_reg)) {
    return -1;
  }

  return jmp_reg.r_rax;
}

long 
pt_call2(pid_t pid, intptr_t addr, ...)
{
  struct reg jmp_reg;
  struct reg bak_reg;
  va_list ap;

  if(pt_getregs(pid, &bak_reg)) {
    return -1;
  }

  memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));
  jmp_reg.r_rip = addr;
  // Preserve the interrupted thread's red zone and satisfy the SysV ABI.
  jmp_reg.r_rsp = ((bak_reg.r_rsp - 0x200) & ~15ULL) - 8;

  va_start(ap, addr);
  jmp_reg.r_rdi = va_arg(ap, uint64_t);
  jmp_reg.r_rsi = va_arg(ap, uint64_t);
  jmp_reg.r_rdx = va_arg(ap, uint64_t);
  jmp_reg.r_rcx = va_arg(ap, uint64_t);
  jmp_reg.r_r8  = va_arg(ap, uint64_t);
  jmp_reg.r_r9  = va_arg(ap, uint64_t);
  va_end(ap);

  if(pt_setregs(pid, &jmp_reg)) {
    return -1;
  }

  //
  // Continue until hit a breakpoint, that must be generated by the called function
  //
  if (pt_continue(pid, 0)) {
    pt_setregs(pid, &bak_reg);
    return -1;
  }
  int status = 0;
  pid_t stopped = 0;
  for (int i = 0; i < 2000 && stopped == 0; ++i) {
    stopped = waitpid(pid, &status, WNOHANG | WUNTRACED);
    if (!stopped) usleep(1000);
  }
  if (!stopped) {
    // Recover a timed-out call before the caller detaches.
    kill(pid, SIGSTOP);
    for (int i = 0; i < 1000 && stopped == 0; ++i) {
      stopped = waitpid(pid, &status, WNOHANG | WUNTRACED);
      if (!stopped) usleep(1000);
    }
  }
  long result = -1;
  if (stopped == pid && WIFSTOPPED(status)) {
    if (WSTOPSIG(status) == SIGTRAP && !pt_getregs(pid, &jmp_reg))
      result = jmp_reg.r_rax;
    if (pt_setregs(pid, &bak_reg)) result = -1;
  }
  return result;
}


long
pt_syscall(pid_t pid, int sysno, ...) {
  const uint64_t started=port_timing_now();
  printf("Remote syscall %d: resolve\n",sysno);
  intptr_t addr = pt_resolve(pid, "HoLVWNanBBc");
  struct reg jmp_reg;
  struct reg bak_reg;
  va_list ap;

  if(!addr) {
    return -1;
  } else {
    addr += 0xa;
  }
  uint8_t opcode[2];
  int read_result=pt_copyout(pid,addr,opcode,sizeof(opcode));
  printf("Syscall gate read %d, bytes %02x %02x\n",read_result,opcode[0],opcode[1]);
  if (read_result || opcode[0]!=0x0f || opcode[1]!=0x05) return -1;
  puts("Remote syscall instruction verified");

  if(pt_getregs(pid, &bak_reg)) {
    return -1;
  }

  memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));
  jmp_reg.r_rip = addr;
  jmp_reg.r_rax = sysno;

  va_start(ap, sysno);
  jmp_reg.r_rdi = va_arg(ap, uint64_t);
  jmp_reg.r_rsi = va_arg(ap, uint64_t);
  jmp_reg.r_rdx = va_arg(ap, uint64_t);
  jmp_reg.r_r10 = va_arg(ap, uint64_t);
  jmp_reg.r_r8  = va_arg(ap, uint64_t);
  jmp_reg.r_r9  = va_arg(ap, uint64_t);
  va_end(ap);

  if(pt_setregs(pid, &jmp_reg)) {
    return -1;
  }

  // Execute only the validated syscall instruction. Following a foreign
  // function's RET on the interrupted thread's stack is unnecessary.
  int failed=pt_step(pid) || pt_getregs(pid,&jmp_reg);
  printf("Remote syscall step result %d\n",failed);
  long result=failed || (jmp_reg.r_rflags & 1) ? -1 : jmp_reg.r_rax;
  if(pt_setregs(pid,&bak_reg)) return -1;
  port_timing_end("remote-syscall",pid,started,(uint64_t)sysno);
  return result;
}


intptr_t
pt_mmap(pid_t pid, intptr_t addr, size_t len, int prot, int flags,
	int fd, off_t off) {
  return pt_syscall(pid, SYS_mmap, addr, len, prot, flags, fd, off);
}


int
pt_msync(pid_t pid, intptr_t addr, size_t len, int flags) {
  return pt_syscall(pid, SYS_msync, addr, len, flags);
}


int
pt_munmap(pid_t pid, intptr_t addr, size_t len) {
  return pt_syscall(pid, SYS_munmap, addr, len);
}


int
pt_mprotect(pid_t pid, intptr_t addr, size_t len, int prot) {
  return pt_syscall(pid, SYS_mprotect, addr, len, prot);
}


int
pt_socket(pid_t pid, int domain, int type, int protocol) {
  return (int)pt_syscall(pid, SYS_socket, domain, type, protocol);
}


int
pt_setsockopt(pid_t pid, int fd, int level, int optname, intptr_t optval,
	      socklen_t optlen) {
  return (int)pt_syscall(pid, SYS_setsockopt, fd, level, optname, optval,
			 optlen, 0);
}


int
pt_close(pid_t pid, int fd) {
  return (int)pt_syscall(pid, SYS_close, fd);
}


int
pt_bind(pid_t pid, int sockfd, intptr_t addr, uint32_t addrlen) {
  return (int)pt_syscall(pid, SYS_bind, sockfd, addr, addrlen);
}


ssize_t
pt_recvmsg(pid_t pid, int fd, intptr_t msg, int flags) {
  return (int)pt_syscall(pid, SYS_recvmsg, fd, msg, flags);
}


int
pt_dup2(pid_t pid, int oldfd, int newfd) {
  return (int)pt_syscall(pid, SYS_dup2, oldfd, newfd);
}


int
pt_rdup(pid_t pid, pid_t other_pid, int fd) {
  return (int)pt_syscall(pid, 0x25b, other_pid, fd);
}


int
pt_pipe(pid_t pid, intptr_t pipefd) {
  intptr_t faddr = pt_resolve(pid, "-Jp7F+pXxNg");
  return (int)pt_call(pid, faddr, pipefd);
}


int
pt_errno(pid_t pid) {
  intptr_t faddr = pt_resolve(pid, "9BcDykPmo1I");
  intptr_t addr = pt_call(pid, faddr);
  return pt_getint(pid, addr);
}


intptr_t
pt_sceKernelGetProcParam(pid_t pid) {
  intptr_t faddr = pt_resolve(pid, "959qrazPIrg");

  return pt_call(pid, faddr);
}
