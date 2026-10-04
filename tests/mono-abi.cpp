#include "port_mono_abi.hpp"
#include "port_boot_abi.hpp"
#include "../../include/port_kstuff_state.hpp"
#define ETAHEN_PORT_1360 1
#include "../../include/port_toolbox_route.hpp"
#include "../../include/port_process_match.hpp"
#include <assert.h>
#include <initializer_list>
static unsigned test_firmware=0x13600000u;
extern "C" unsigned kernel_get_fw_version(void){return test_firmware;}
// Exercise both eight-byte ABI slots, including an absent value with nonzero
// payload bits. A pointer-shaped third argument loses the second slot.
static PortNullableInt64 captured;
static uint32_t expected_option=7;
__attribute__((sysv_abi,noinline)) static bool original(void* uri,int option,PortNullableInt64 action){
 assert(uri==(void*)0x1234&&static_cast<uint32_t>(option)==expected_option);captured=action;return action.hasValue!=0;
}
__attribute__((sysv_abi,noinline)) static bool forward(void* uri,int option,PortNullableInt64 action){
 return original(uri,option,action);
}
int main(){
 // Captured PS5 13.60 metadata: Boot(String, BootHelper.Option, Nullable<Int64>).
 // Option has a UInt32 backing field (ECMA field signature 06 09), read from the console metadata.
 const char* option="Sce.Vsh.ShellUI.AppSystem.BootHelper.Option";
 const bool enum32=port_boot_integer32_enum(true,"System.Int32",4,4,false);
 assert(enum32);
 assert(port_boot_abi(3,false,"System.Boolean","System.String",option,"System.Nullable<System.Int64>",16,8,enum32)==PortBootAbi::NullableInt64);
 assert(port_boot_abi(3,false,"System.Boolean","System.String",option,"System.Nullable<System.Int64>",16,8)==PortBootAbi::Unsupported);
 assert(!port_boot_integer32_enum(false,"System.Int32",4,4,false));
 assert(!port_boot_integer32_enum(true,"System.Int64",8,8,false));
 const bool enumU32=port_boot_integer32_enum(true,"System.UInt32",4,4,false);
 assert(enumU32);
 assert(port_boot_abi(3,false,"System.Boolean","System.String",option,"System.Nullable<System.Int64>",16,8,enumU32)==PortBootAbi::NullableInt64);
 assert(port_boot_abi(3,false,"System.Boolean","System.String","System.UInt32","System.Nullable<System.Int64>",16,8)==PortBootAbi::NullableInt64);
 assert(!port_boot_integer32_enum(true,"System.UInt32",8,4,false));
 assert(!port_boot_integer32_enum(true,"System.UInt32",4,8,false));
 assert(!port_boot_integer32_enum(true,"System.UInt32",4,4,true));
 assert(!port_boot_integer32_enum(true,"System.Single",4,4,false));
 assert(!port_boot_integer32_enum(true,"System.Int32",4,4,true));
 assert(!port_boot_integer32_enum(true,"System.Int32",8,4,false));
 assert(!port_boot_integer32_enum(true,"System.Int32",4,8,false));
 assert(!port_boot_integer32_enum(true,nullptr,4,4,false));
 assert(port_boot_abi(2,false,"System.Boolean","System.String","System.Int32",nullptr)==PortBootAbi::TwoArguments);
 assert(port_boot_abi(3,false,"System.Boolean","System.String",option,"System.String",0,0,enum32)==PortBootAbi::StringArgument);
 assert(port_boot_abi(3,true,"System.Boolean","System.String",option,"System.Nullable<System.Int64>",16,8,enum32)==PortBootAbi::Unsupported);
 assert(port_boot_abi(3,false,"System.Boolean","System.String",option,"System.Nullable<System.Int64>",8,8,enum32)==PortBootAbi::Unsupported);
 const char url_name[]="etaHEN-13.60-242b53c63ddf.elf";
 assert(!port_other_process_matches(98,98,url_name,sizeof(url_name),"etaHEN"));
 assert(port_other_process_matches(99,98,url_name,sizeof(url_name),"etaHEN"));
 const char service[]="etaHEN Critical services";
 assert(port_other_process_matches(94,98,service,sizeof(service),"etaHEN"));
 assert(!port_other_process_matches(0,98,service,sizeof(service),"etaHEN"));
 assert(!port_other_process_matches(99,98,"payload.elf",12,"etaHEN"));
 assert(!port_other_process_matches(99,98,"elfldr.elf",11,"etaHEN"));
 const char bounded[]={'e','t','a','H','E','N','X'};
 assert(port_other_process_matches(99,98,bounded,6,"etaHEN"));
 assert(!port_other_process_matches(99,98,bounded,5,"etaHEN"));
 assert(!port_toolbox_root_requested(nullptr));
 assert(!port_toolbox_root_requested(ETAHEN_TOOLBOX_URI));
 assert(!port_toolbox_root_requested("etaHEN?Cheats"));
 assert(!port_toolbox_root_requested("pssettings:play?mode=settings&function=debug_settings"));
 assert(port_toolbox_root_requested(ETAHEN_TOOLBOX_ROOT_URI));
 assert(port_toolbox_root_requested("etaHEN?Toolbox"));
 test_firmware=0x12000000u;assert(!strcmp(ETAHEN_TOOLBOX_URI,"pssettings:play?mode=settings&function=debug_settings"));
 test_firmware=0x13000000u;assert(!strcmp(ETAHEN_TOOLBOX_URI,"pssettings:play?mode=settings&function=debug_settings_old"));
 const uint64_t base=UINT64_C(0xffffffff88000000);
 PortKstuffSnapshot state{base+0x1B6E50,base+0x1AE5C0,0xffff,0xffff,true};
 state.expectedNative=state.native;state.expectedCompat=state.compat;
 assert(state.classify(base)==PortKstuffState::Unmodified);
 assert(state.injectionReady(base)); // no_kstuff mode remains usable
 state.readable=false;assert(state.classify(base)==PortKstuffState::Unknown);assert(!state.injectionReady(base));state.readable=true;
 state.native=base+0x4000000;state.compat=base+0x5000000;state.xts=state.hmac=0xdeb7;
 assert(state.classify(base)==PortKstuffState::Installed); // active INT3 tables
 assert(state.injectionReady(base));
 state.native=(state.native&UINT64_C(0x0000ffffffffffff))|UINT64_C(0xdeb7000000000000);
 assert(!state.injectionReady(base)); // one legacy-poisoned table is enough to refuse
 state.compat=(state.compat&UINT64_C(0x0000ffffffffffff))|UINT64_C(0xdeb7000000000000);
 assert(state.classify(base)==PortKstuffState::Installed); // installed but legacy-poisoned
 assert(!state.injectionReady(base));
 state.hmac=0xffff;assert(state.classify(base)==PortKstuffState::Unknown);
 state.native=0;assert(state.classify(base)==PortKstuffState::Unknown);
 for(uint32_t bits:{0u,7u,0x80000000u,0xffffffffu}){
 expected_option=bits;int option;memcpy(&option,&bits,sizeof(option));
 for(uint8_t present: {uint8_t(0),uint8_t(1)}){
  PortNullableInt64 action={present,{0},INT64_C(0x123456789abcdef)};
  assert(forward((void*)0x1234,option,action)==bool(present));
  assert(captured.hasValue==present&&captured.value==action.value);
 }
}
}
