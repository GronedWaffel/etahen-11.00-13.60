import test from 'node:test';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';import path from 'node:path';import {writeFileSync,readFileSync} from 'node:fs';
import {root,source,zig} from '../scripts/toolchain.mjs';
test('native relocator preserves branches and RIP-relative references',()=>{
 const exe=path.join(root,'build/relocate-test.exe');
 const r=spawnSync(zig,['c++','-std=c++20','-DHDE_DECODE_ONLY','-I',path.join(source,'shellui/include'),path.join(root,'tests/relocate.cpp'),path.join(source,'shellui/src/relocate.cpp'),path.join(source,'shellui/src/hde64.cpp'),'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
 assert.equal(r.status,0,r.stderr);const check=spawnSync(exe,[],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});assert.equal(check.status,0,check.stdout+check.stderr);
 const captured=JSON.parse(readFileSync(path.join(root,'tests/fixtures/ps5-1360-prologues.json'),'utf8'));
 for(const method of captured){const result=spawnSync(exe,[method.prologue],{encoding:'utf8'});assert.equal(result.status,0,method.name+': '+result.stderr);}
});
