// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <ps5/kernel.h>
#include <stdint.h>
#include "port_kstuff_state.hpp"
inline PortKstuffSnapshot port_read_kstuff_state(){
 PortKstuffSnapshot s;const auto base=KERNEL_ADDRESS_DATA_BASE;
 if((kernel_get_fw_version()&0xffff0000)!=0x13600000)return s;
 s.readable=!kernel_copyout(base+0xDDD8F8+8,&s.native,sizeof(s.native))&&
  !kernel_copyout(base+0xDDDA70+8,&s.compat,sizeof(s.compat))&&
  !kernel_copyout(base+0x2DCDE70+11*8+2*8+6,&s.xts,sizeof(s.xts))&&
  !kernel_copyout(base+0x2DCDE70+11*8+9*8+6,&s.hmac,sizeof(s.hmac));
 return s;
}
// Public kstuff-lite v1.11 offsets/13_60.h and installation markers.
// A process name alone cannot establish that kernel hooks were installed.
inline bool port_kstuff_hooks_installed(){
 return port_read_kstuff_state().classify(KERNEL_ADDRESS_DATA_BASE)==PortKstuffState::Installed;
}
struct PortKstuffPause {
 intptr_t native=KERNEL_ADDRESS_DATA_BASE+0xDDD8F8+14,compat=KERNEL_ADDRESS_DATA_BASE+0xDDDA70+14;
 uint16_t savedNative=0,savedCompat=0;bool changed=false;
 PortKstuffPause(){
  if(!port_kstuff_hooks_installed())return;
  savedNative=kernel_getshort(native);savedCompat=kernel_getshort(compat);
  if((savedNative!=0xffff&&savedNative!=0xdeb7)||(savedCompat!=0xffff&&savedCompat!=0xdeb7))return;
  changed=true;kernel_setshort(native,0xffff);kernel_setshort(compat,0xffff);
 }
 ~PortKstuffPause(){if(changed){kernel_setshort(native,savedNative);kernel_setshort(compat,savedCompat);}}
};
