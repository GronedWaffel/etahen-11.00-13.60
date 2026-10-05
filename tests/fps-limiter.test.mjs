import test from 'node:test';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';import path from 'node:path';
import {root,source,zig} from '../scripts/toolchain.mjs';
test('actual remote limiter code bounds pacing, expires on Stop and preserves game arguments',()=>{
 const env={...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')};
 const exe=path.join(root,'build/fps-limiter/test.exe');
 const c=spawnSync(zig,['c++','-std=c++20','-DHDE_DECODE_ONLY','-I',path.join(source,'shellui/include'),path.join(root,'tests/fps-limiter.cpp'),path.join(source,'shellui/src/relocate.cpp'),path.join(source,'shellui/src/hde64.cpp'),'-o',exe],{encoding:'utf8',env});assert.equal(c.status,0,c.stderr);
 const r=spawnSync(exe,[path.join(root,'build/fps-limiter/gate.bin')],{encoding:'utf8',env});assert.equal(r.status,0,r.stdout+r.stderr);
});
