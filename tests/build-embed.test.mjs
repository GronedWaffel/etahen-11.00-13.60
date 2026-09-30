import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync,writeFileSync,readFileSync,mkdirSync} from 'node:fs';
import path from 'node:path';
import {root,run} from '../scripts/toolchain.mjs';
test('changing only an incbin asset rebuilds the embedded bytes',()=>{
 mkdirSync(path.join(root,'build'),{recursive:true});
 const dir=mkdtempSync(path.join(root,'build/embed-test-'));
 const file=path.join(dir,'embed.S'),obj=path.join(dir,'embed.o');
 writeFileSync(file,'.section .data\n.incbin "asset.bin"\n');
 for(const value of ['FIRST-REVISION-123456789','NEWER-REVISION-987654321']){
  writeFileSync(path.join(dir,'asset.bin'),value);
  run(['cc','-target','x86_64-linux-none','-c',file,'-o',obj],{cwd:dir});
  assert.ok(readFileSync(obj).includes(Buffer.from(value)));
 }
 assert.ok(!readFileSync(obj).includes(Buffer.from('FIRST-REVISION-123456789')));
});
