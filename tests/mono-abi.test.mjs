import test from 'node:test';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';import path from 'node:path';
import {root,source,zig} from '../scripts/toolchain.mjs';
test('13.60 optional Int64 forwarding and kstuff state classification',()=>{
 const exe=path.join(root,'build/mono-abi-test.exe');
 const build=spawnSync(zig,['c++','-std=c++20','-I',path.join(root,'tests/include'),'-I',path.join(source,'shellui/include'),path.join(root,'tests/mono-abi.cpp'),'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
 assert.equal(build.status,0,build.stderr);const run=spawnSync(exe,[],{encoding:'utf8'});assert.equal(run.status,0,run.stdout+run.stderr);
});
