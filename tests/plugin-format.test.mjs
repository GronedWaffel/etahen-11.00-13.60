import test from 'node:test';
import assert from 'node:assert/strict';
import {makePlugin} from '../scripts/plugin-format.mjs';
function elf(){
 const b=Buffer.alloc(0x4001);b.set([0x7f,69,76,70,2,1,1]);b.writeUInt16LE(3,16);b.writeUInt16LE(62,18);
 b.writeBigUInt64LE(64n,32);b.writeUInt16LE(64,52);b.writeUInt16LE(56,54);b.writeUInt16LE(1,56);
 b.writeUInt32LE(1,64);b.writeUInt32LE(5,68);b.writeBigUInt64LE(0x4000n,72);b.writeBigUInt64LE(1n,96);b.writeBigUInt64LE(1n,104);b[0x4000]=0xc3;return b;
}
test('etaHEN plugin uses official 29-byte header and preserves its executable body',()=>{
 const input=elf(),p=makePlugin(input,'SNPS00001','1.00');
 assert.equal(p.subarray(0,29).toString('hex'),'65746148454e5f504c5547494e00534e5053303030303100312e303000');
 assert.deepEqual(p.subarray(29),input);
 assert.equal(makePlugin(input,'Abcd12345','9.99').subarray(14,24).toString(),'Abcd12345\0');
 for(const version of ['1.0','10.00','1.a0','1.00\0'])assert.throws(()=>makePlugin(input,'SNPS00001',version),/version/);
 for(const id of ['SNPS0000','SNPS000001','SNP/00001'])assert.throws(()=>makePlugin(input,id,'1.00'),/title ID/);
 assert.throws(()=>makePlugin(Buffer.alloc(128),'SNPS00001','1.00'),/ELF/);
});
