import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, mkdirSync, writeFileSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { root, zig } from '../scripts/toolchain.mjs';

test('actual CheatMemory initializes relative addressing even on poisoned heap memory', () => {
  const header = readFileSync(path.join(root, 'Source Code/util/include/CheatManager.hpp'), 'utf8');
  const structure = header.match(/struct CheatMemory\r?\n\{[\s\S]*?\r?\n\};/)?.[0];
  assert.ok(structure);
  const dir = path.join(root, 'build/cheat-address-test'); mkdirSync(dir, { recursive: true });
  const source = path.join(dir, 'test.cpp'), executable = path.join(dir, process.platform === 'win32' ? 'test.exe' : 'test');
  writeFileSync(source, '#include <vector>\n#include <cstdint>\n#include <cstring>\n#include <new>\nusing ByteArray=std::vector<uint8_t>;\n' + structure + '\nint main(){alignas(CheatMemory) unsigned char raw[sizeof(CheatMemory)];memset(raw,0xa5,sizeof raw);auto p=new(raw) CheatMemory;bool valid=!p->absolute&&!p->codeCaveReloc&&p->section==0&&p->Offset==0;p->~CheatMemory();return valid?0:1;}\n');
  const compile = spawnSync(zig, ['c++', '-std=c++17', source, '-o', executable], { encoding: 'utf8', env: { ...process.env, ZIG_GLOBAL_CACHE_DIR: path.join(root, 'build/zig-cache') } });
  assert.equal(compile.status, 0, compile.stderr || compile.error?.message);
  const result = spawnSync(executable, [], { encoding: 'utf8' });
  assert.equal(result.status, 0, result.stderr || result.error?.message);
});
