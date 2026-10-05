import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdirSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import path from 'node:path';
import {root,zig} from '../scripts/toolchain.mjs';
test('plugin images reject truncation, incompatible ELF, malformed headers and path aliases share identity',()=>{
 const dir=path.join(root,'build/plugin-test');mkdirSync(dir,{recursive:true});
 const exe=path.join(dir,process.platform==='win32'?'test.exe':'test');
 const c=spawnSync(zig,['c++','-std=c++17',path.join(root,'tests/plugin.cpp'),'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
 assert.equal(c.status,0,c.stderr||c.error?.message);
 const r=spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr||r.error?.message);
});
