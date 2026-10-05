import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import path from 'node:path';
import {root,source,zig} from '../scripts/toolchain.mjs';

test('remote arenas reject whole-entry RX and execute counters with split RX/RW pages',()=>{
 const exe=path.join(root,'build/game-arena-test.exe');
 const env={...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')};
 const build=spawnSync(zig,['c++','-std=c++20',path.join(root,'tests/game-arena.cpp'),'-o',exe],{encoding:'utf8',env});
 assert.equal(build.status,0,build.stderr);
 const result=spawnSync(exe,[],{encoding:'utf8',env});
 assert.equal(result.status,0,result.stdout+result.stderr);
});

test('every new game hook validates separate code/data permissions before publication',()=>{
 for(const file of ['bc-fps-counter.cpp','gta-fps-limiter.cpp','game-prx.cpp']){
  const code=readFileSync(path.join(source,'daemon/source',file),'utf8');
  assert.match(code,/PortSealGameArena\([^;]+pt_mprotect,kernel_get_vmem_protection/);
  assert.doesNotMatch(code,/kernel_mprotect\([^;]+(?:base|stub),0x4000/);
  assert.ok(code.indexOf('PortSealGameArena(')<code.indexOf('mapping.keep=true'),file);
  assert.match(code,/arena mprotect=%d code=%d data=%d/);
 }
});
