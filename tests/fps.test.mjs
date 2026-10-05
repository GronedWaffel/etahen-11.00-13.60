import test from 'node:test';import assert from 'node:assert/strict';
import {mkdirSync} from 'node:fs';import {spawnSync} from 'node:child_process';import path from 'node:path';
import {root,source,zig} from '../scripts/toolchain.mjs';
test('FPS selection and lock-free UI cache reject stale, future, torn and invalid readings',()=>{
 const out=path.join(root,'build/fps-test');mkdirSync(out,{recursive:true});const exe=path.join(out,'fps-test.exe');
 const c=spawnSync(zig,['c++','-std=c++17','-I',path.join(source,'fps_native/include'),path.join(root,'tests/fps.cpp'),path.join(source,'fps_native/source/fps_formula.cpp'),'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
 assert.equal(c.status,0,c.stderr);const r=spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
});
