import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {pathToFileURL} from 'node:url';
export function inspectElf(b){
 if(b.length<64||b.readUInt32LE(0)!==0x464c457f||b[4]!==2||b[5]!==1||b.readUInt16LE(18)!==62)throw Error('Expected little-endian x86-64 ELF');
 const off=Number(b.readBigUInt64LE(32)),size=b.readUInt16LE(54),count=b.readUInt16LE(56),entry=Number(b.readBigUInt64LE(24));
 if(size!==56||!count||off+count*size>b.length)throw Error('Invalid program-header table');
 const segments=[];let executableEntry=false;
 for(let i=0;i<count;i++){const p=off+i*size;if(b.readUInt32LE(p)!==1)continue;const flags=b.readUInt32LE(p+4),offset=Number(b.readBigUInt64LE(p+8)),address=Number(b.readBigUInt64LE(p+16)),filesz=Number(b.readBigUInt64LE(p+32)),memsz=Number(b.readBigUInt64LE(p+40));
  if(address%0x4000||offset%0x4000)throw Error('Loadable segment is not 16 KiB aligned');
  if(filesz>memsz||offset+filesz>b.length)throw Error('Invalid segment bounds');
  if((flags&1)&&entry>=address&&entry<address+memsz)executableEntry=true;
  segments.push({address,offset,filesz,memsz,flags});
 }
 if(!executableEntry)throw Error('Entrypoint outside executable segments');
 return {bytes:b.length,sha256:createHash('sha256').update(b).digest('hex'),segments};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){for(const f of process.argv.slice(2))console.log(JSON.stringify({file:f,...inspectElf(await readFile(f))}));}
