import {test} from 'node:test';import assert from 'node:assert/strict';import {inspectElf} from '../scripts/verify-elf.mjs';
function fixture(){const b=Buffer.alloc(0x4010);b.writeUInt32LE(0x464c457f);b[4]=2;b[5]=1;b.writeUInt16LE(62,18);b.writeBigUInt64LE(64n,32);b.writeUInt16LE(56,54);b.writeUInt16LE(1,56);b.writeUInt32LE(1,64);b.writeUInt32LE(5,68);b.writeBigUInt64LE(0x4000n,72);b.writeBigUInt64LE(16n,96);b.writeBigUInt64LE(16n,104);return b;}
test('accepts valid page-aligned x86-64 payload',()=>assert.equal(inspectElf(fixture()).segments.length,1));
test('rejects the unaligned segment regression',()=>{const b=fixture();b.writeBigUInt64LE(8n,80);assert.throws(()=>inspectElf(b),/aligned/);});
test('rejects truncated segment content',()=>{const b=fixture();b.writeBigUInt64LE(100n,96);b.writeBigUInt64LE(100n,104);assert.throws(()=>inspectElf(b),/bounds/);});
test('rejects non-executable entrypoint',()=>{const b=fixture();b.writeUInt32LE(6,68);assert.throws(()=>inspectElf(b),/Entrypoint/);});
test('rejects wrong architecture and truncated ELF headers',()=>{assert.throws(()=>inspectElf(Buffer.alloc(20)));const b=fixture();b.writeUInt16LE(183,18);assert.throws(()=>inspectElf(b),/x86-64/);});
