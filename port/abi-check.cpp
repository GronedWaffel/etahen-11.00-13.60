// SPDX-License-Identifier: GPL-3.0-or-later
#include <machine/param.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <ps5/payload.h>
static_assert(PAGE_SIZE==0x4000,"PS5 requires 16 KiB pages");
static_assert(sizeof(wchar_t)==2,"PS5 SDK uses 16-bit wchar_t");
static_assert(sizeof(void*)==8 && sizeof(long)==8,"PS5 LP64 ABI");
static_assert(sizeof(payload_args_t)==48,"SDK payload argument ABI");
static_assert(offsetof(payload_args_t,sys_dynlib_dlsym)==0,"SDK entry argument");
static_assert(sizeof(ptrace_io_desc)==32,"ptrace I/O descriptor ABI");
