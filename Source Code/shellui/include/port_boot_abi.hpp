// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string.h>
enum class PortBootAbi { Unsupported, TwoArguments, StringArgument, NullableInt64 };
inline PortBootAbi port_boot_abi(unsigned count, bool instance, const char* result,
 const char* first, const char* second, const char* third, int size=0, unsigned alignment=0){
 if(instance||!result||strcmp(result,"System.Boolean")||!first||strcmp(first,"System.String")||!second||strcmp(second,"System.Int32"))return PortBootAbi::Unsupported;
 if(count==2)return PortBootAbi::TwoArguments;
 if(count!=3||!third)return PortBootAbi::Unsupported;
 if(!strcmp(third,"System.String"))return PortBootAbi::StringArgument;
 if((!strcmp(third,"System.Nullable<System.Int64>")||!strcmp(third,"System.Nullable`1<System.Int64>"))&&size==16&&alignment==8)return PortBootAbi::NullableInt64;
 return PortBootAbi::Unsupported;
}
