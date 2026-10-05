import test from 'node:test';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';import path from 'node:path';import {readFileSync,writeFileSync} from 'node:fs';
import {root,source,zig} from '../scripts/toolchain.mjs';
test('PRX validates module containers and actual load gate handles success, failure, Stop and reentry',()=>{
 const env={...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')};
 // The full build generates both gates before tests.
 const exe=path.join(root,'build/prx-test.exe');const c=spawnSync(zig,['c++','-std=c++17',path.join(root,'tests/prx.cpp'),'-o',exe],{encoding:'utf8',env});assert.equal(c.status,0,c.stderr);
 const r=spawnSync(exe,[path.join(root,'build/fps-limiter/prx-gate.bin')],{encoding:'utf8',env});assert.equal(r.status,0,r.stdout+r.stderr);
 const ui=readFileSync(path.join(source,'shellui/src/HookFunctions.cpp'),'utf8');assert.doesNotMatch(ui,/int pid = .*\.PRX/,'PRX state must never become a synthetic process ID');
});
test('manual plugin Start works without a game while automatic Start remains title-scoped',()=>{
 let implementation=readFileSync(path.join(source,'daemon/source/game_plugins.cpp'),'utf8');
 implementation=implementation.slice(implementation.indexOf('static bool load_locked('),implementation.indexOf('bool port_load_game_plugin('));
 implementation=implementation.replaceAll('open(','fake_open(').replaceAll('fstat(','fake_fstat(').replaceAll('read(','fake_read(').replaceAll('close(','fake_close(').replaceAll('struct stat','FakeStat');
 const code=`#include <cassert>\n#include <vector>\n#include <string>\n#include <cstring>\n#include <cerrno>\n#include <cstdint>\n#include "${path.join(source,'include/port_game_plugin.hpp').replaceAll('\\','/')}"\n
using ssize_t=long long;struct FakeStat {unsigned st_mode=0;long long st_size=128;};
#define O_RDONLY 0
#define O_NOFOLLOW 0
#define STDOUT_FILENO 1
#define S_ISREG(x) true
int fake_open(const char*,int){return 3;}int fake_fstat(int,FakeStat*){return 0;}int fake_close(int){return 0;}
ssize_t fake_read(int,void* p,size_t n){memset(p,0,n);return n;}
static bool running=false,change=false;static int queries=0,starts=0;static std::string title="CUSA12345";
bool Get_Running_App_TID(std::string& t,int& app){++queries;t=change&&queries==2?"PPSA12345":title;app=10;return running;}
bool port_request_prx(const std::string&,bool,bool){return true;}
int sceKernelGetProcessName(int,char*){return 0;}int elfldr_spawn(const char*,int,uint8_t*,const char*){return 42;}
int port_plugin_load(const char*,int,int(*)(int,char*),int(*)(const char*,int,uint8_t*,const char*)){++starts;return 1;}
void experimental_event(const char*,const char*,long long,int){}
// File validation is independently covered by plugin.cpp; isolate launch policy.
#define port_plugin_parse(path,data,available,total,out) 1
${implementation}
int main(){const std::string path="/data/etaHEN/game_plugins/CUSA12345/test.plugin";
assert(load_locked(path,false)&&starts==1&&queries==0);
assert(!load_locked(path,true)&&starts==1);
running=true;title="PPSA12345";assert(!load_locked(path,true));
title="CUSA12345";queries=0;change=true;assert(!load_locked(path,true));
queries=0;change=false;assert(load_locked(path,true)&&starts==2);
assert(!load_locked("/data/etaHEN/game_plugins/CUSA12345/../bad.plugin",false));}
`;
 const cpp=path.join(root,'build/plugin-start-test.cpp'),exe=path.join(root,'build/plugin-start-test.exe');writeFileSync(cpp,code);
 const env={...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')};const c=spawnSync(zig,['c++','-std=c++17',cpp,'-o',exe],{encoding:'utf8',env});assert.equal(c.status,0,c.stderr);const r=spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stdout+r.stderr);
});
