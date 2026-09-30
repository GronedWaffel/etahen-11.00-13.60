// SPDX-License-Identifier: GPL-3.0-or-later
// Temporarily forward Application.Update unchanged, count calls, then restore.
#include <ps5/kernel.h>
#include <ps5/payload.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "detour_port.h"
extern "C" int __wrap___patch_init(){return 0;}
static void* (*get_image)(void*);
static const char* (*image_name)(void*);
static void* pui=nullptr;
static void(*original)(void*);
static unsigned calls=0;
static void forward_update(void* self){__atomic_fetch_add(&calls,1,__ATOMIC_RELAXED);original(self);}
static void visit(void* assembly,void*){void* img=get_image(assembly);if(img){const char* name=image_name(img);if(name&&(!strcmp(name,"Sce.PlayStation.PUI")||!strcmp(name,"Sce.PlayStation.PUI.dll")))pui=img;}}
int main(){
 char* report=(char*)payload_get_args()+0x400;strcpy(report,"{\"stage\":\"live AOT probe entered\"}");
 EnablePortExternalPublication();if(!BeginPortDetours())return 1;
 uint32_t handle=0;if(kernel_dynlib_handle(getpid(),"libmonosgen-2.0.sprx",&handle))return 2;
#define MONO(name,type) auto name=(type)kernel_dynlib_dlsym(getpid(),handle,#name);if(!name)return 3;
 MONO(mono_get_root_domain,void*(*)());MONO(mono_thread_attach,void*(*)(void*));MONO(mono_thread_detach,void(*)(void*));
 MONO(mono_assembly_get_image,void*(*)(void*));MONO(mono_image_get_name,const char*(*)(void*));
 MONO(mono_assembly_foreach,void(*)(void(*)(void*,void*),void*));
 MONO(mono_class_from_name,void*(*)(void*,const char*,const char*));MONO(mono_class_get_method_from_name,void*(*)(void*,const char*,int));
 MONO(mono_compile_method,void*(*)(void*));
 void* domain=mono_get_root_domain();if(!domain)return 4;void* thread=mono_thread_attach(domain);if(!thread)return 5;
 get_image=mono_assembly_get_image;image_name=mono_image_get_name;mono_assembly_foreach(visit,nullptr);
 void* klass=pui?mono_class_from_name(pui,"Sce.PlayStation.PUI","Application"):nullptr;
 void* method=klass?mono_class_get_method_from_name(klass,"Update",0):nullptr;void* code=method?mono_compile_method(method):nullptr;
 original=code?(void(*)(void*))PortDetourFunction((uintptr_t)code,(void*)forward_update):nullptr;
 bool committed=original&&CommitPortDetours();
 snprintf(report,0x3000,"{\"stage\":\"live AOT hook %s\"}",committed?"active":"not installed");
 if(committed)for(int i=0;i<20&&__atomic_load_n(&calls,__ATOMIC_RELAXED)<30;i++)usleep(50000);
 unsigned observed=__atomic_load_n(&calls,__ATOMIC_RELAXED);bool restored=RollbackPortDetours();
 mono_thread_detach(thread);
 snprintf(report,0x3000,"{\"systemMethod\":\"Application.Update\",\"forwardedCalls\":%u,\"committed\":%s,\"restored\":%s,\"pass\":%s}",observed,committed?"true":"false",restored?"true":"false",committed&&observed&&restored?"true":"false");
 FinishPortExternalPublication();return committed&&observed&&restored?0:6;
}
