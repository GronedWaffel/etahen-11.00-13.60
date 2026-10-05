import test from 'node:test';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';import path from 'node:path';import {mkdirSync} from 'node:fs';
import {root,source,zig} from '../scripts/toolchain.mjs';
test('publication diagnostics preserve rollback and never flush while ShellUI is stopped',()=>{
 const out=path.join(root,'build/toolbox-diagnostic');mkdirSync(out,{recursive:true});const exe=path.join(out,'publication-test.exe');
 const compile=spawnSync(zig,['cc','-std=c11','-I',path.join(root,'tests'),'-I',path.join(source,'include'),path.join(root,'tests/publication.c'),'-o',exe],{encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});
 assert.equal(compile.status,0,compile.stderr);const run=spawnSync(exe,[],{encoding:'utf8'});assert.equal(run.status,0,run.stdout+run.stderr);
});
