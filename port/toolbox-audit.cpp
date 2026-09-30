// SPDX-License-Identifier: GPL-3.0-or-later
// Runs on a temporary ShellUI thread. Resolves methods; installs no hooks.
#include <ps5/kernel.h>
#include <ps5/payload.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#ifdef ETAHEN_AUDIT_PREPARE_HOOKS
#include "detour_port.h"
static int unused_hook(){return 0;}
static char prepareStage[160];
static void prepare_progress(const char* stage){snprintf(prepareStage,sizeof(prepareStage),"%s",stage);}
#endif
#ifndef ETAHEN_AUDIT_PHASE
#define ETAHEN_AUDIT_PHASE 0
#endif
#ifndef ETAHEN_AUDIT_COMPILE_INDEX
#define ETAHEN_AUDIT_COMPILE_INDEX -1
#endif
#ifndef ETAHEN_AUDIT_WRITE_PROBE
#define ETAHEN_AUDIT_WRITE_PROBE 0
#endif
// Diagnostic threads use the supplied syscall gateway and do not need the
// SDK's process-wide credential/syscall-permission patches.
extern "C" int __wrap___patch_init(){return 0;}
struct Report {
 char *data;size_t used=0;
 void format(const char *fmt,...){if(used>=0x2fff)return;va_list ap;va_start(ap,fmt);int n=vsnprintf(data+used,0x3000-used,fmt,ap);va_end(ap);if(n>0)used+=(size_t)n<0x3000-used?(size_t)n:0x2fff-used;}
 void text(const char *s){format("%s",s);}
 void ch(char c){format("%c",c);}
 void quoted(const char *s){ch('"');if(s)for(;*s;++s){unsigned char c=*s;if(c=='"'||c=='\\')ch('\\');if(c>=32)ch(c);else format("\\u%04x",c);}ch('"');}
};
struct Check {const char *dll,*ns,*klass,*method;int argc;};
static const Check checks[]={
 {"Sce.PlayStation.PUI","Sce.PlayStation.PUI","Application","Update",0},
 {"Sce.Vsh.ShellUI.ReactNativeShellApp","ReactNative.Modules.ShellUI.HomeUI","OptionMenu","createJson",8},
 {"Sce.Vsh.LncUtilWrapper","Sce.Vsh.LncUtil","LncUtilWrapper","LaunchApp",4},
 {"Sce.Vsh.LncUtilWrapper","Sce.Vsh.LncUtil","LncUtilWrapper","KillAppWithReason",2},
 {"Sce.Vsh.ShellUI.AppSystem","Sce.Vsh.ShellUI.AppSystem","LayerManager","UpdateImposeStatusFlag",2},
 {"Sce.PlayStation.Core","Sce.PlayStation.Core.Input","GamePad","GetData",1},
 {"Sce.Vsh.ShellUI.Legacy","Sce.Vsh.ShellUI.Settings.CoreUI3","SettingsPlugin","CxmlUri",1},
 {"Sce.Vsh.ShellUI.Legacy","Sce.Vsh.ShellUI.Settings.CoreUI3","SettingPage","OnPressed",2},
 {"Sce.Vsh.ShellUI.Legacy","Sce.Vsh.ShellUI.Settings.CoreUI3","SettingPage","OnCreating",1},
 {"Sce.Vsh.ShellUI.Legacy","Sce.Vsh.ShellUI.Settings.CoreUI3","SettingsPlugin","GetString",1},
 {"Sce.Vsh.ShellUI.AppSystem","Sce.Vsh.ShellUI.AppSystem","BootHelper","Boot",3},
 {"Sce.Vsh.ShellUI.AppSystem","Sce.Vsh.ShellUI.AppSystem","BootHelper","Boot",2},
 {"Sce.Vsh.ShellUI.AppSystem","Sce.Vsh.ShellUI.AppSystem","PowerManager","Terminate",0},
 {"Sce.Vsh.ShellUI.AppSystem","Sce.Vsh.ShellUI.AppSystem","LayerManager","FindContainerSceneByPath",1},
 {"Sce.Vsh.ShellUI.CaptureMenu","Sce.Vsh.ShellUI.CaptureMenu","CaptureController","CaptureScreen",4},
 {"Sce.Vsh.ShellUI.CaptureMenu","Sce.Vsh.ShellUI.CaptureMenu","CaptureController","CaptureScreen",5},
 {"Sce.Vsh.ShellUI.CaptureMenu","Sce.Vsh.ShellUI.CaptureMenu","EventManager","OnShareButton",1},
 {"ReactNative.PUI","ReactNative.PlayStation.Security","JavaScriptBundleDecryptor","Decrypt",5},
 {"mscorlib","System.Reflection","RuntimeAssembly","GetManifestResourceStream",1},
 {"Sce.Vsh.UILib","Sce.Vsh.UILib","SystemSoftwareVersionInfo","set_DisplayVersion",1},
 {"Sce.PlayStation.Core","Sce.PlayStation.Core.Runtime","Diagnostics","CheckRunningOnMainThread",0}
};
struct Images {void *data[128];const char *names[128];unsigned count;};
static void *(*get_image)(void*);
static const char *(*image_name)(void*);
static void visit(void *assembly,void *ctx){auto x=(Images*)ctx;if(x->count>=128)return;void *img=get_image(assembly);const char *name=img?image_name(img):nullptr;if(name){x->data[x->count]=img;x->names[x->count++]=name;}}
int main(){
 // Return the report through the injector-owned argument page. ShellUI cannot
 // necessarily open the same filesystem paths as a standalone payload.
 char *report=(char*)payload_get_args()+0x400;
 memset(report,0,0x3000);
 Report out{report};
#ifdef ETAHEN_AUDIT_PREPARE_HOOKS
 if(!BeginPortDetours()){out.text("{\"error\":\"Cannot begin preparation\"}");return 7;}
#endif
 out.text("{\"stage\":\"main entered\",\"complete\":true}");
 if(ETAHEN_AUDIT_PHASE==0)return 0;
 out.used=0;
 uint32_t handle=0;if(kernel_dynlib_handle(getpid(),"libmonosgen-2.0.sprx",&handle)){out.text("{\"error\":\"Mono missing\"}");return 2;}
#define MONO(name,type) auto name=(type)kernel_dynlib_dlsym(getpid(),handle,#name); if(!name){out.format("{\"error\":\"Missing %s\"}",#name);return 3;}
 MONO(mono_get_root_domain,void*(*)());
 MONO(mono_thread_attach,void*(*)(void*));
 MONO(mono_thread_detach,void(*)(void*));
 MONO(mono_assembly_foreach,void(*)(void(*)(void*,void*),void*));
 MONO(mono_assembly_get_image,void*(*)(void*));
 MONO(mono_image_get_name,const char*(*)(void*));
 MONO(mono_class_from_name,void*(*)(void*,const char*,const char*));
 MONO(mono_class_get_method_from_name,void*(*)(void*,const char*,int));
 MONO(mono_compile_method,void*(*)(void*));
#ifdef ETAHEN_AUDIT_RESOURCES
 MONO(mono_image_get_table_info,const void*(*)(void*,int));
 MONO(mono_table_info_get_rows,int(*)(const void*));
 MONO(mono_metadata_decode_row_col,uint32_t(*)(const void*,int,unsigned));
 MONO(mono_metadata_string_heap,const char*(*)(void*,uint32_t));
 MONO(mono_class_get_methods,void*(*)(void*,void**));
 MONO(mono_method_get_name,const char*(*)(void*));
 MONO(mono_method_get_header,void*(*)(void*));
 MONO(mono_method_header_get_code,const unsigned char*(*)(void*,uint32_t*,uint32_t*));
 MONO(mono_metadata_free_mh,void(*)(void*));
#ifdef ETAHEN_AUDIT_RESOURCE_CALL
 MONO(mono_image_get_assembly,void*(*)(void*));
 MONO(mono_assembly_get_object,void*(*)(void*,void*));
 MONO(mono_string_new,void*(*)(void*,const char*));
 MONO(mono_runtime_invoke,void*(*)(void*,void*,void**,void**));
 MONO(mono_object_get_class,void*(*)(void*));
 MONO(mono_class_get_name,const char*(*)(void*));
 MONO(mono_object_unbox,void*(*)(void*));
 MONO(mono_gchandle_new,uint32_t(*)(void*,int));
 MONO(mono_gchandle_free,void(*)(uint32_t));
#endif
#endif
#ifdef ETAHEN_AUDIT_SIGNATURES
 // Public Mono metadata APIs; inspect signatures without invoking methods.
 MONO(mono_method_signature,void*(*)(void*));
 MONO(mono_signature_get_return_type,void*(*)(void*));
 MONO(mono_signature_get_params,void*(*)(void*,void**));
 MONO(mono_method_get_flags,uint32_t(*)(void*,uint32_t*));
 MONO(mono_type_get_name,char*(*)(void*));
 MONO(mono_class_from_mono_type,void*(*)(void*));
 MONO(mono_class_is_valuetype,int(*)(void*));
 MONO(mono_class_value_size,int32_t(*)(void*,uint32_t*));
 MONO(mono_class_get_fields,void*(*)(void*,void**));
 MONO(mono_field_get_name,const char*(*)(void*));
 MONO(mono_field_get_offset,uint32_t(*)(void*));
 MONO(mono_free,void(*)(void*));
#endif
 out.text("{\"stage\":\"exports resolved\",\"complete\":true}");
 if(ETAHEN_AUDIT_PHASE==1)return 0;
 out.used=0;
 void *domain=mono_get_root_domain();if(!domain){out.text("{\"error\":\"No root domain\"}");return 4;}
 out.text("{\"stage\":\"root domain resolved\",\"complete\":true}");
 if(ETAHEN_AUDIT_PHASE==2)return 0;
 out.used=0;out.text("{\"stage\":\"attaching Mono thread\",\"complete\":false}");
 void *thread=mono_thread_attach(domain);if(!thread){out.text("{\"error\":\"Attach failed\"}");return 5;}
 out.used=0;out.text("{\"stage\":\"Mono thread attached\",\"complete\":true}");
 if(ETAHEN_AUDIT_PHASE==3){mono_thread_detach(thread);return 0;}
 out.used=0;
 get_image=mono_assembly_get_image;image_name=mono_image_get_name;Images images{};mono_assembly_foreach(visit,&images);
#ifdef ETAHEN_AUDIT_RESOURCES
#ifdef ETAHEN_AUDIT_RESOURCE_CALL
 void* legacyImage=nullptr;void* coreImage=nullptr;
 for(unsigned j=0;j<images.count;++j){if(strstr(images.names[j],"Legacy"))legacyImage=images.data[j];if(strstr(images.names[j],"mscorlib"))coreImage=images.data[j];}
 if(!legacyImage||!coreImage){out.text("{\"error\":\"Missing image\"}");mono_thread_detach(thread);return 8;}
 void* assembly=mono_assembly_get_object(domain,mono_image_get_assembly(legacyImage));
 void* klass=mono_class_from_name(coreImage,"System.Reflection","RuntimeAssembly");
 void* method=klass?mono_class_get_method_from_name(klass,"GetManifestResourceStream",ETAHEN_AUDIT_RESOURCE_CALL):nullptr;
 if(!assembly||!method){out.text("{\"error\":\"Missing assembly object or method\"}");mono_thread_detach(thread);return 9;}
 void* name=mono_string_new(domain,"Sce.Vsh.ShellUI.Legacy.src.Sce.Vsh.ShellUI.Settings.Plugins.DebugSettings.data.debug_settings.xml");
 void* args1[]={name};void* args2[]={nullptr,name};void* exception=nullptr;
 out.text("{\"stage\":\"invoking installed resource method\",\"complete\":false}");
 void* stream=mono_runtime_invoke(method,assembly,ETAHEN_AUDIT_RESOURCE_CALL==1?args1:args2,&exception);
 out.used=0;out.format("{\"hooksInstalled\":false,\"invokedResourceMethod\":true,\"exception\":%s,\"streamClass\":",exception?"true":"false");
 out.quoted(stream?mono_class_get_name(mono_object_get_class(stream)):nullptr);
 if(stream&&!exception){
  uint32_t root=mono_gchandle_new(stream,1);void* streamClass=mono_object_get_class(stream);
  void* read=mono_class_get_method_from_name(streamClass,"ReadByte",0);char prefix[193]={0};unsigned used=0;
  for(;read&&used<192;++used){void* value=mono_runtime_invoke(read,stream,nullptr,&exception);if(exception||!value)break;int v=*(int*)mono_object_unbox(value);if(v<0)break;prefix[used]=(char)v;}
  out.text(",\"prefix\":");out.quoted(prefix);out.format(",\"readException\":%s",exception?"true":"false");mono_gchandle_free(root);
 }
 out.text(",\"complete\":true}");mono_thread_detach(thread);return 0;
#endif
 // ECMA-335 ManifestResource table 0x28, Name column 2. Metadata only;
 // do not compile, invoke or patch any target method in this diagnostic.
 out.text("{\"hooksInstalled\":false,\"resources\":[");unsigned found=0;
 for(unsigned j=0;j<images.count;++j){
  if(!strstr(images.names[j],"Legacy"))continue;
  const void* table=mono_image_get_table_info(images.data[j],0x28);
  int rows=table?mono_table_info_get_rows(table):0;
  for(int row=0;row<rows&&out.used<7000;++row){
   const char* name=mono_metadata_string_heap(images.data[j],mono_metadata_decode_row_col(table,row,2));
   if(!name||!strstr(name,".debug_settings.xml"))continue;
   if(found++)out.ch(',');out.quoted(name);
  }
 }
 out.text("],\"settingsMethods\":[");found=0;
 for(unsigned j=0;j<images.count;++j){
  if(!strstr(images.names[j],"Legacy"))continue;
  void* cls=mono_class_from_name(images.data[j],"Sce.Vsh.ShellUI.Settings.CoreUI3","SettingsPlugin");
  void* iter=nullptr;void* method;
  while(cls&&(method=mono_class_get_methods(cls,&iter))&&out.used<11000){if(found++)out.ch(',');out.quoted(mono_method_get_name(method));}
 }
 out.text("],\"il\":[");found=0;
 for(unsigned j=0;j<images.count;++j){
  bool legacy=strstr(images.names[j],"Legacy")!=nullptr;
  if(!legacy&&!strstr(images.names[j],"mscorlib"))continue;
  void* cls=mono_class_from_name(images.data[j],legacy?"Sce.Vsh.ShellUI.Settings.CoreUI3":"System.Reflection",legacy?"SettingsPlugin":"RuntimeAssembly");
  void* iter=nullptr;void* method;
  while(cls&&(method=mono_class_get_methods(cls,&iter))&&out.used<9000){
   const char* name=mono_method_get_name(method);
   if(legacy?(strcmp(name,"Load")&&strcmp(name,"Init")):!strstr(name,"GetManifestResourceStream"))continue;
   void* header=mono_method_get_header(method);uint32_t len=0,stack=0;
   const unsigned char* il=header?mono_method_header_get_code(header,&len,&stack):nullptr;
   if(found++)out.ch(',');out.text("{\"class\":");out.quoted(legacy?"SettingsPlugin":"RuntimeAssembly");out.text(",\"method\":");out.quoted(name);
   out.format(",\"length\":%u,\"bytes\":\"",len);
   for(uint32_t k=0;il&&k<len&&k<1024;++k)out.format("%02x",il[k]);out.text("\"}");
   if(header)mono_metadata_free_mh(header);
  }
 }
 out.text("],\"complete\":true}");mono_thread_detach(thread);return 0;
#endif
 out.format("{\"phase\":%d,\"compileIndex\":%d,\"hooksInstalled\":false,\"assemblyCount\":%u,\"checks\":[",ETAHEN_AUDIT_PHASE,ETAHEN_AUDIT_COMPILE_INDEX,images.count);
 for(unsigned i=0;i<sizeof(checks)/sizeof(*checks);++i){auto &c=checks[i];void *img=nullptr;
  for(unsigned j=0;j<images.count;++j)if(!strcmp(images.names[j],c.dll)||(strlen(images.names[j])==strlen(c.dll)+4&&!strncmp(images.names[j],c.dll,strlen(c.dll))&&!strcmp(images.names[j]+strlen(c.dll),".dll")))img=images.data[j];
  void *klass=img?mono_class_from_name(img,c.ns,c.klass):nullptr;void *method=klass?mono_class_get_method_from_name(klass,c.method,c.argc):nullptr;void *code=method&&ETAHEN_AUDIT_PHASE>=5&&(ETAHEN_AUDIT_COMPILE_INDEX<0||i==ETAHEN_AUDIT_COMPILE_INDEX)?mono_compile_method(method):nullptr;
  out.format("%s{\"dll\":\"%s\",\"class\":\"%s\",\"method\":\"%s\",\"argc\":%d,\"image\":%s,\"classFound\":%s,\"methodFound\":%s,\"resolved\":%s",i?",":"",c.dll,c.klass,c.method,c.argc,img?"true":"false",klass?"true":"false",method?"true":"false",code?"true":"false");
#ifdef ETAHEN_AUDIT_SIGNATURES
  if(method){
   uint32_t impl=0,flags=mono_method_get_flags(method,&impl);
   void* sig=mono_method_signature(method);void* type=sig?mono_signature_get_return_type(sig):nullptr;
   out.format(",\"static\":%s,\"returnType\":",flags&0x10?"true":"false");
   char* name=type?mono_type_get_name(type):nullptr;out.quoted(name);if(name)mono_free(name);
   out.text(",\"parameters\":[");void* iter=nullptr;void* param;unsigned n=0;
   while(sig&&(param=mono_signature_get_params(sig,&iter))&&n<32){if(n++)out.ch(',');char* p=mono_type_get_name(param);out.quoted(p);if(p)mono_free(p);}out.ch(']');
   if(!strcmp(c.method,"Boot")||!strcmp(c.method,"CaptureScreen")){
    out.text(",\"valueParameters\":[");iter=nullptr;n=0;unsigned index=0;
    while(sig&&(param=mono_signature_get_params(sig,&iter))&&index<32){
     void* cls=mono_class_from_mono_type(param);
     if(cls&&mono_class_is_valuetype(cls)){uint32_t alignment=0;int32_t size=mono_class_value_size(cls,&alignment);
      if(n++)out.ch(',');out.format("{\"index\":%u,\"size\":%d,\"alignment\":%u,\"fields\":[",index,size,alignment);
      void* fi=nullptr;void* field;unsigned fn=0;while((field=mono_class_get_fields(cls,&fi))&&fn<32){if(fn++)out.ch(',');out.text("{\"name\":");out.quoted(mono_field_get_name(field));out.format(",\"boxedOffset\":%u}",mono_field_get_offset(field));}out.text("]}");
     }++index;
    }out.ch(']');
   }
   if(type&&!strcmp(c.klass,"GamePad")&&!strcmp(c.method,"GetData")){
    void* retclass=mono_class_from_mono_type(type);uint32_t align=0;
    if(retclass){int32_t size=mono_class_value_size(retclass,&align);out.format(",\"returnSize\":%d,\"returnAlignment\":%u,\"fields\":[",size,align);
     iter=nullptr;void* field;n=0;
     while((field=mono_class_get_fields(retclass,&iter))&&n<64){if(n++)out.ch(',');out.text("{\"name\":");out.quoted(mono_field_get_name(field));out.format(",\"boxedOffset\":%u}",mono_field_get_offset(field));}out.ch(']');
    }
   }
  }
#endif
  if(code){
   out.format(",\"address\":\"0x%lx\"",(unsigned long)code);
   unsigned char bytes[32];bool readable=true;
   for(size_t off=0;off<sizeof(bytes);){size_t count=0x1000-(((uintptr_t)code+off)&0xfff);if(count>sizeof(bytes)-off)count=sizeof(bytes)-off;
    if(kernel_proc_copyout(getpid(),(intptr_t)code+off,bytes+off,count)){readable=false;break;}off+=count;}
   out.format(",\"readable\":%s",readable?"true":"false");
#ifdef ETAHEN_AUDIT_PREPARE_HOOKS
   void* trampoline=readable?PortDetourFunction((uint64_t)code,(void*)unused_hook,prepare_progress):nullptr;
   out.format(",\"trampolinePrepared\":%s,\"prepareStage\":\"%s\"",trampoline?"true":"false",prepareStage);
#endif
   if(readable&&ETAHEN_AUDIT_WRITE_PROBE&&i==0){
    out.text(",\"sameBytesWriteStarted\":true");
    int result=kernel_proc_copyin(getpid(),bytes,(intptr_t)code,8);
    unsigned char after[8]={0};bool equal=!kernel_proc_copyout(getpid(),(intptr_t)code,after,sizeof(after))&&!memcmp(bytes,after,sizeof(after));
    out.format(",\"sameBytesWriteResult\":%d,\"sameBytesUnchanged\":%s",result,equal?"true":"false");
   }
   if(readable){out.text(",\"prologue\":\"");for(auto byte:bytes)out.format("%02x",byte);out.ch('"');}
  }out.ch('}');
 }
 out.text("],\"native\":[");
#ifdef ETAHEN_AUDIT_PREPARE_HOOKS
 const char* modules[]={"libkernel_sys.sprx","libkernel_sys.sprx","libSceRegMgr.sprx"};
 const char* symbols[]={"read","ioctl","sceRegMgrGetInt"};
 for(unsigned i=0;i<3;++i){uint32_t h=0;uint64_t code=0;
  if(!kernel_dynlib_handle(getpid(),modules[i],&h))code=kernel_dynlib_dlsym(getpid(),h,symbols[i]);
  unsigned char bytes[32]={0};bool readable=code&&PortReadCode(code,bytes,sizeof(bytes));
  void* trampoline=readable?PortDetourFunction(code,(void*)unused_hook,prepare_progress):nullptr;
  out.format("%s{\"symbol\":\"%s\",\"address\":\"0x%lx\",\"trampolinePrepared\":%s,\"prepareStage\":\"%s\",\"prologue\":\"",i?",":"",symbols[i],code,trampoline?"true":"false",prepareStage);
  for(auto byte:bytes)out.format("%02x",byte);out.text("\"}");
 }
#endif
 out.text("],\"complete\":true}\n");
#ifdef ETAHEN_AUDIT_PREPARE_HOOKS
 RollbackPortDetours();
#endif
 mono_thread_detach(thread);return 0;
}
