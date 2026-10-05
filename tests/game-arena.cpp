#include "../Source Code/include/port_game_arena.h"
#include <assert.h>
#include <windows.h>

static int query(int,uint64_t address,size_t) {
    MEMORY_BASIC_INFORMATION m{};
    assert(VirtualQuery((void*)address,&m,sizeof(m)));
    if(m.Protect==PAGE_EXECUTE_READ)return 5;
    if(m.Protect==PAGE_READWRITE)return 3;
    if(m.Protect==PAGE_EXECUTE_READWRITE)return 7;
    return 0;
}
static int split(int,uint64_t address,size_t size,int protection) {
    assert(size==0x4000 && protection==5);
    DWORD old;
    return VirtualProtect((void*)address,size,PAGE_EXECUTE_READ,&old)?0:-1;
}
int main() {
    auto region=(unsigned char*)VirtualAlloc(nullptr,0x8000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(region);
    uint64_t base=(uintptr_t)region;
    // A complete one-instruction counter test in the real split mapping:
    // lock inc qword [rip + disp32]; ret. The counter lives in the data half.
    unsigned char code[]={0xf0,0x48,0xff,0x05,0xf8,0x3f,0,0,0xc3};
    memcpy(region,code,sizeof(code));
    PortGameArenaProtection permissions;
    // Reproduce SDK metadata scope: a change on the first half affects the
    // entire unsplit VM entry. The old code accepted success and hooked it.
    auto wholeEntry=[](int,uint64_t address,size_t,int){
        DWORD old;return VirtualProtect((void*)address,0x8000,PAGE_EXECUTE_READ,&old)?0:-1;
    };
    assert(!PortSealGameArena(1,base,wholeEntry,query,permissions));
    assert(permissions.transition==0 && permissions.code==5 && permissions.data==5);
    DWORD old;assert(VirtualProtect(region,0x8000,PAGE_READWRITE,&old));
    // Real OS mprotect semantics preserve RW data while sealing executable code.
    assert(PortSealGameArena(1,base,split,query,permissions));
    assert(permissions.code==5 && permissions.data==3);
    FlushInstructionCache(GetCurrentProcess(),region,sizeof(code));
    ((void(*)())region)();((void(*)())region)();
    assert(*(uint64_t*)(region+0x4000)==2);
    // A rejected syscall is a hard stop even if queries happen to look right.
    auto rejected=[](int,uint64_t,size_t,int){return -1;};
    assert(!PortSealGameArena(1,base,rejected,query,permissions));
    auto wrongCode=[](int,uint64_t address,size_t size,int){
        DWORD previous;return VirtualProtect((void*)address,size,PAGE_EXECUTE_READWRITE,&previous)?0:-1;
    };
    assert(!PortSealGameArena(1,base,wrongCode,query,permissions));
    assert(permissions.code==7);
    assert(VirtualFree(region,0,MEM_RELEASE));
}
