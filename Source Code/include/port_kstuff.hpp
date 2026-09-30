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
// Injection worked with canonical INT3 tables on this port. A legacy "pause"
// wrote the same 0xffff tag, doing nothing; "resume" poisoned those tables.
// Do not alter them. Refuse an unexpected state rather than repair live hooks.
inline bool port_kstuff_injection_ready(){
 return port_read_kstuff_state().injectionReady(KERNEL_ADDRESS_DATA_BASE);
}
