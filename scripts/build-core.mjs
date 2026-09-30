import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdir,readdir,writeFile} from 'node:fs/promises';import path from 'node:path';
const out=path.join(root,'build/core');await mkdir(out,{recursive:true});
const targets=['libNidResolver','libhijacker','libNineS','libSelfDecryptor','libelfldr'];const result=[];
for(const target of targets){
 const dir=path.join(source,target,['libNineS','libelfldr'].includes(target)?'src':'source'),files=[];
 async function walk(d){for(const f of await readdir(d,{withFileTypes:true})){if(f.isDirectory()&&target==='libhijacker')await walk(path.join(d,f.name));else if(f.isFile()&&/\.(cpp|c|s)$/.test(f.name))files.push(path.join(d,f.name));}}
 await walk(dir);const objects=[];
 for(const f of files){const object=path.join(out,target+'-'+path.relative(dir,f).replaceAll(/[\\/]/g,'-')+'.o');const args=['cc',...(f.endsWith('.cpp')?cxx:[]),...common,'-I',path.join(source,'include'),'-I',path.join(source,target,'include'),'-I',path.join(source,'libNidResolver/include'),'-I',path.join(source,'libhijacker/source'),'-c',f,'-o',object];
  try{run(args);objects.push(object);console.log('Compiled',path.relative(source,f));}catch(e){await writeFile(path.join(out,'failure.txt'),e.message);console.error(e.message.slice(0,9000));process.exit(1);}
 }
 const library=path.join(out,target+'.a');run(['ar','rcs',library,...objects]);result.push({target,objects:objects.length,library});
}
await writeFile(path.join(out,'manifest.json'),JSON.stringify({sdk,result},null,2));console.log('All core libraries compiled. This is not yet a bootable etaHEN build.');
