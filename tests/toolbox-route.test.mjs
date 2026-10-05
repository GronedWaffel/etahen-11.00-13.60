import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import path from 'node:path';
import {root,source,zig} from '../scripts/toolchain.mjs';

test('dashboard URI reaches Toolbox through both Boot ABIs across the firmware route boundary',()=>{
 const hooks=readFileSync(path.join(source,'shellui/src/HookFunctions.cpp'),'utf8');
 const start=hooks.indexOf('  template<typename Argument> bool port_boot_dispatch');
 const end=hooks.indexOf('  GamePadData GetData_hook',start);
 assert.ok(start>0&&end>start);
 const code=`
#include <cassert>
#include <string>
#include <stdint.h>
static uint32_t firmware;
#include "port_toolbox_route.hpp"
#include "port_firmware.h"
unsigned kernel_get_fw_version(){return firmware;}
#define ETAHEN_PORT_1360 1
using MonoString=std::string;
using BootActionArgument=long long;
static void* Root_Domain=nullptr;
static bool cheats_shortcut_activated,cheats_shortcut_activated_not_open;
static bool game_shortcut_activated,game_shortcut_activated_media;
static struct {bool lite_mode;} global_conf;
static std::string Mono_to_String(MonoString* s){return s?*s:"";}
static MonoString* mono_string_new(void*,const char* s){static MonoString text;text=s;return &text;}
static bool handle_uri_boot_common(MonoString*,int){return false;}
static void notify(const char*){assert(false);}
static std::string seen;static int seen_opt;static long long seen_arg;static MonoString* seen_string;
static bool original2(MonoString* s,int opt){seen=*s;seen_opt=opt;return true;}
static bool original3(MonoString* s,int opt,long long arg){seen_arg=arg;return original2(s,opt);}
static bool originalString(MonoString* s,int opt,MonoString* arg){seen_string=arg;return original2(s,opt);}
static auto boot_orig=original3;static auto boot_orig_string=originalString;static auto boot_orig_2=original2;
${hooks.slice(start,end)}
static void dirty(){cheats_shortcut_activated=cheats_shortcut_activated_not_open=game_shortcut_activated=game_shortcut_activated_media=true;}
static void clean(){assert(!cheats_shortcut_activated&&!cheats_shortcut_activated_not_open&&!game_shortcut_activated&&!game_shortcut_activated_media);}
int main(){
 const uint32_t cases[]={0x07000000,0x10600001,0x11000000,0x11200005,0x12020000,0x13200000,0x13600000};
 for(auto fw:cases){firmware=fw;assert(snipers_firmware_profile(fw));
  const char* expected=fw<0x11000000?"pssettings:play?mode=settings&function=debug_settings":"pssettings:play?mode=settings&function=debug_settings_old";
  assert(std::string(port_toolbox_uri())==expected);
  for(bool lite:{false,true}){global_conf.lite_mode=lite;
   for(const char* uri:{"etaHEN?Toolbox","pssettings:play?mode=settings&function=debug_settings_old&etahen_root=1"}){
    MonoString input=uri;dirty();assert(uri_boot_hook_2(&input,-2147483647));clean();assert(seen==expected&&seen_opt==-2147483647);
    dirty();assert(uri_boot_hook(&input,7,0x123456789abcdefLL));clean();assert(seen==expected&&seen_opt==7&&seen_arg==0x123456789abcdefLL);
    MonoString title="title";dirty();assert(uri_boot_hook_string(&input,8,&title));clean();assert(seen==expected&&seen_string==&title);
   }
  }
 }
 for(uint32_t unsupported:{0x09050000u,0x11400000u,0x13990000u,0u})assert(!snipers_firmware_profile(unsupported));
 MonoString unrelated="pssettings:play?mode=settings&function=network";dirty();assert(uri_boot_hook_2(&unrelated,42));assert(seen==unrelated&&seen_opt==42&&game_shortcut_activated);
 assert(!port_toolbox_root_requested(nullptr));assert(!port_toolbox_root_requested("etaHEN?ToolboxExtra"));
}
`;
 const dir=path.join(root,'build/toolbox-route-test');mkdirSync(dir,{recursive:true});
 const input=path.join(dir,'route.cpp'),exe=path.join(dir,'route.exe');writeFileSync(input,code);
 const build=spawnSync(zig,['c++','-std=c++20','-I',path.join(root,'tests/include'),'-I',path.join(source,'include'),input,'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
 assert.equal(build.status,0,build.stderr);const run=spawnSync(exe,[],{encoding:'utf8'});assert.equal(run.status,0,run.stdout+run.stderr);
});
