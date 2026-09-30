#include "port_mono_abi.hpp"
#include "../../include/port_kstuff_state.hpp"
#define ETAHEN_PORT_1360 1
#include "../../include/port_toolbox_route.hpp"
#include "../../include/port_process_match.hpp"
#include <assert.h>
#include <initializer_list>
// Exercise both eight-byte ABI slots, including an absent value with nonzero
// payload bits. A pointer-shaped third argument loses the second slot.
static PortNullableInt64 captured;
__attribute__((sysv_abi,noinline)) static bool original(void* uri,int option,PortNullableInt64 action){
 assert(uri==(void*)0x1234&&option==7);captured=action;return action.hasValue!=0;
}
__attribute__((sysv_abi,noinline)) static bool forward(void* uri,int option,PortNullableInt64 action){
 return original(uri,option,action);
}
int main(){
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
 const uint64_t base=UINT64_C(0xffffffff88000000);
 PortKstuffSnapshot state{base+0x1B6E50,base+0x1AE5C0,0xffff,0xffff,true};
 assert(state.classify(base)==PortKstuffState::Unmodified);
 state.readable=false;assert(state.classify(base)==PortKstuffState::Unknown);state.readable=true;
 state.native=base+0x4000000;state.compat=base+0x5000000;state.xts=state.hmac=0xdeb7;
 assert(state.classify(base)==PortKstuffState::Installed); // paused sysent tags
 state.native=(state.native&UINT64_C(0x0000ffffffffffff))|UINT64_C(0xdeb7000000000000);
 state.compat=(state.compat&UINT64_C(0x0000ffffffffffff))|UINT64_C(0xdeb7000000000000);
 assert(state.classify(base)==PortKstuffState::Installed); // active sysent tags
 state.hmac=0xffff;assert(state.classify(base)==PortKstuffState::Unknown);
 state.native=0;assert(state.classify(base)==PortKstuffState::Unknown);
 for(uint8_t present: {uint8_t(0),uint8_t(1)}){
  PortNullableInt64 action={present,{0},INT64_C(0x123456789abcdef)};
  assert(forward((void*)0x1234,7,action)==bool(present));
  assert(captured.hasValue==present&&captured.value==action.value);
 }
}
