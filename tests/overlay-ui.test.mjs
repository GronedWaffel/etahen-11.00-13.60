import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync, writeFileSync, mkdirSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import path from 'node:path';
import {root, source, zig} from '../scripts/toolchain.mjs';

test('overlay survives absent scenes and managed exceptions, and follows scene replacement', () => {
  const startup=readFileSync(path.join(source,'shellui/src/prx.cpp'),'utf8');
  assert.match(startup,/^\s*AppSystem_img = getDLLimage\(/m,'initialize the shared image; a local shadow leaves the overlay null');
  const implementation=readFileSync(path.join(source,'shellui/src/overlay-ui.cpp'),'utf8').replace(/^#include .*$/gm,'');
  const code=`
#include <cassert>
#include <cstring>
struct MonoObject {int id;};
using MonoClass=MonoObject;using MonoMethod=MonoObject;using MonoProperty=MonoObject;
static MonoObject klass{1},find{2},get{3},widgetFind{4},setter{5},sceneA{6},sceneB{7},rootA{8},rootB{9},label{10},exceptionObject{11};
static void* AppSystem_img=&klass;static void* pui_img=&klass;static void* Root_Domain=&klass;
static MonoObject* current=&sceneA;static bool missingClass=false,missingMethod=false,throwNow=false;
static int calls=0;
void P5Event(const char*,unsigned long long=0,int=0){}
MonoClass* mono_class_from_name(void*,const char*,const char*){return missingClass?nullptr:&klass;}
MonoMethod* mono_class_get_method_from_name(MonoClass*,const char* name,int){
 if(missingMethod)return nullptr;return !strcmp(name,"FindContainerSceneByPath")?&find:&widgetFind;
}
MonoProperty* mono_class_get_property_from_name(MonoClass*,const char* name){return !strcmp(name,"Text")?&setter:&get;}
MonoMethod* mono_property_get_get_method(MonoProperty* p){return missingMethod?nullptr:p;}
MonoMethod* mono_property_get_set_method(MonoProperty* p){return missingMethod?nullptr:p;}
void* mono_string_new(void*,const char* s){return (void*)s;}
MonoObject* mono_runtime_invoke(MonoMethod* m,MonoObject* instance,void** args,MonoObject** error){
 ++calls;assert(error);*error=throwNow?&exceptionObject:nullptr;if(throwNow)return nullptr;
 if(m==&find){assert(!instance&&args&&!strcmp((char*)args[0],"Game"));return current;}
 assert(instance); // Never invoke an instance property as a static method.
 if(m==&get)return instance==&sceneA?&rootA:&rootB;
 if(m==&widgetFind){assert(args&&args[0]);return &label;}
 assert(m==&setter&&instance==&label&&args&&!strcmp((char*)args[0],"60.0"));return nullptr;
}
${implementation}
int main(){
 assert(OverlayRoot()==&rootA);current=&sceneB;assert(OverlayRoot()==&rootB);
 current=nullptr;assert(!OverlayRoot());current=&sceneA;
 missingClass=true;assert(!OverlayRoot());assert(!OverlayFind(&rootA,"fps"));assert(!OverlayText(&label,"60.0"));missingClass=false;
 missingMethod=true;assert(!OverlayRoot());assert(!OverlayFind(&rootA,"fps"));assert(!OverlayText(&label,"60.0"));missingMethod=false;
 int before=calls;assert(!OverlayFind(nullptr,"fps"));assert(!OverlayText(nullptr,"60.0"));assert(calls==before);
 assert(OverlayFind(&rootA,"fps")==&label);assert(OverlayText(&label,"60.0"));
 throwNow=true;assert(!OverlayRoot());assert(!OverlayFind(&rootA,"fps"));assert(!OverlayText(&label,"60.0"));
 return 0;
}`;
  const dir=path.join(root,'build/overlay-ui-test');mkdirSync(dir,{recursive:true});
  const input=path.join(dir,'test.cpp'),exe=path.join(dir,'test.exe');writeFileSync(input,code);
  const c=spawnSync(zig,['c++','-std=c++17',input,'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
  assert.equal(c.status,0,c.stderr);
  const run=spawnSync(exe,[],{encoding:'utf8'});assert.equal(run.status,0,run.stderr);
});
