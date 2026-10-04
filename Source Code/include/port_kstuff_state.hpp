// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
enum class PortKstuffState { Unmodified, Installed, Unknown };
struct PortKstuffSnapshot {
 uint64_t native=0,compat=0;uint16_t xts=0,hmac=0;bool readable=false;
 uint64_t expectedNative=0,expectedCompat=0;
 // kstuff-lite v1.11 hooks selected entries in copied tables with INT3.
 // Its table pointers are canonical even while the hooks are active.
 // 0xdeb7/0xffff pointer poisoning belongs to the legacy implementation.
 bool injectionReady(uint64_t base)const{
  const auto state=classify(base);
  return state==PortKstuffState::Unmodified ||
   (state==PortKstuffState::Installed && (native>>48)==0xffff && (compat>>48)==0xffff);
 }
 PortKstuffState classify(uint64_t base)const{
  if(!readable||(base>>48)!=0xffff||(expectedNative>>48)!=0xffff||(expectedCompat>>48)!=0xffff)return PortKstuffState::Unknown;
  const auto nativeTag=native>>48,compatTag=compat>>48;
  if((nativeTag!=0xffff&&nativeTag!=0xdeb7)||(compatTag!=0xffff&&compatTag!=0xdeb7))return PortKstuffState::Unknown;
  const bool nativeOriginal=(native|UINT64_C(0xffff000000000000))==expectedNative;
  const bool compatOriginal=(compat|UINT64_C(0xffff000000000000))==expectedCompat;
  if(nativeOriginal&&compatOriginal&&nativeTag==0xffff&&compatTag==0xffff&&xts==0xffff&&hmac==0xffff)return PortKstuffState::Unmodified;
  if(!nativeOriginal&&!compatOriginal&&xts==0xdeb7&&hmac==0xdeb7)return PortKstuffState::Installed;
  return PortKstuffState::Unknown;
 }
};
