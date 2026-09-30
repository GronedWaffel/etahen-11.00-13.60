#include "relocate.h"
#include "hde64.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
static uint64_t qword(const unsigned char* p){uint64_t v;memcpy(&v,p,8);return v;}
int main(int argc,char** argv){
 unsigned char code[64];RelocatedCode out;const uint64_t src=0x800000000,dst=src+0x10000000;
 if(argc==2){memset(code,0x90,sizeof(code));size_t len=strlen(argv[1])/2;assert(len<=sizeof(code));for(size_t i=0;i<len;i++){char byte[3]={argv[1][i*2],argv[1][i*2+1],0};code[i]=(unsigned char)strtoul(byte,nullptr,16);}if(!BuildTrampoline(code,sizeof(code),src,0x200000000,&out)){for(size_t i=0;i<16;){hde64s h;hde64_disasm(code+i,&h);fprintf(stderr,"offset %zu length %u op %x flags %x rex %x mod %u reg %u rm %u\n",i,h.len,h.opcode,h.flags,h.rex,h.modrm_mod,h.modrm_reg,h.modrm_rm);if(!h.len)break;i+=h.len;}return 1;}return 0;}
 memset(code,0x90,sizeof(code));assert(BuildTrampoline(code,sizeof(code),src,dst,&out));assert(out.stolen==14&&out.size==28);assert(qword(out.bytes+20)==src+14);
 const unsigned char rip[]={0x80,0x3d,0x78,0x56,0x34,0x12,0};memcpy(code,rip,sizeof(rip));assert(BuildTrampoline(code,sizeof(code),src,dst,&out));int32_t d;memcpy(&d,out.bytes+2,4);assert(dst+7+d==src+7+0x12345678);
 assert(BuildTrampoline(code,sizeof(code),src,src+0x100000000,&out)); assert(qword(out.bytes+12)==src+7+0x12345678);
 memset(code,0x90,sizeof(code));code[0]=0xe8;int32_t disp=0x12345;memcpy(code+1,&disp,4);assert(BuildTrampoline(code,sizeof(code),src,dst,&out));assert(out.bytes[0]==0xff&&out.bytes[1]==0x15);assert(qword(out.bytes+8)==src+5+disp);
 memset(code,0x90,sizeof(code));code[0]=0x75;code[1]=0x30;assert(BuildTrampoline(code,sizeof(code),src,dst,&out));assert(out.bytes[0]==0x74&&out.bytes[1]==14);assert(qword(out.bytes+8)==src+2+0x30);
 code[0]=0xeb;code[1]=2;assert(BuildTrampoline(code,sizeof(code),src,dst,&out));assert(qword(out.bytes+6)==dst+16);
 code[0]=0xe2;assert(!BuildTrampoline(code,sizeof(code),src,dst,&out));
#ifdef _WIN32
 auto origin=(unsigned char*)VirtualAlloc((void*)src,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 auto relocated=(unsigned char*)VirtualAlloc((void*)0x200000000,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 assert(origin&&(uint64_t)origin==src&&relocated);
 memset(origin,0x90,64);origin[0]=0x8b;origin[1]=0x05;int32_t displacement=128-6;memcpy(origin+2,&displacement,4);origin[14]=0xc3;int expected=123;memcpy(origin+128,&expected,4);
 assert(BuildTrampoline(origin,64,(uint64_t)origin,(uint64_t)relocated,&out));memcpy(relocated,out.bytes,out.size);
 DWORD old;assert(VirtualProtect(origin,4096,PAGE_EXECUTE_READ,&old));assert(VirtualProtect(relocated,4096,PAGE_EXECUTE_READ,&old));
 assert(((int(*)())origin)()==123);assert(((int(*)())relocated)()==123);
 VirtualFree(origin,0,MEM_RELEASE);VirtualFree(relocated,0,MEM_RELEASE);
#endif
 puts("relocation tests passed");
}
