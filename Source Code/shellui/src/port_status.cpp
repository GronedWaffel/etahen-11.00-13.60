// SPDX-License-Identifier: GPL-3.0-or-later
#include <ps5/payload.h>
#include <stdio.h>
#include <string.h>
static bool enabled=false;
void PortStartStatus(){enabled=true;}
void PortStopStatus(){enabled=false;}
void PortStatus(const char* text){
 if(!enabled)return;
 char escaped[2048];size_t used=0;
 for(;*text&&used+3<sizeof(escaped);++text){
  if(*text=='"'||*text=='\\')escaped[used++]='\\';
  if((unsigned char)*text>=32)escaped[used++]=*text;
 }
 escaped[used]=0;
 snprintf((char*)payload_get_args()+0x400,0x3000,"{\"stage\":\"%s\"}",escaped);
}
