// Separate diagnostic: preserve r3, its ShellUI, kstuff, utility and startup waits.
import {root,source,common,run} from './toolchain.mjs';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';import {spawnSync} from 'node:child_process';import path from 'node:path';
const out=path.join(root,'build/toolbox-diagnostic');await mkdir(out,{recursive:true});
const digest=async f=>createHash('sha256').update(await readFile(path.join(root,f))).digest('hex');
const pinned=['build/etaHEN-13.60-experimental.elf','build/shellui/shellui.elf','Source Code/daemon/assets/shellui.elf','build/util/util.elf','Source Code/bootstrapper/assets/kstuff.elf'];
const before=Object.fromEntries(await Promise.all(pinned.map(async f=>[f,await digest(f)])));
if(before[pinned[0]]!=='aa166a43347d10a31f7efa5ca26ec32f512c65e8910466bfc8033537337b87b2')throw Error('Preserved r3 does not match');
if(before[pinned[1]]!==before[pinned[2]])throw Error('Embedded ShellUI differs');
for(const name of ['pt.c','port_publish.c','port_diagnostic.c'])run(['cc',...common,'-DETAHEN_TOOLBOX_DIAGNOSTIC=1','-I',path.join(source,'include'),'-c',path.join(source,'libNineS/src',name),'-o',path.join(out,name+'.o')]);
for(const args of [['scripts/build-services.mjs','daemon','--toolbox-diagnostic'],['scripts/build-bootstrapper.mjs','--toolbox-diagnostic']]){
 const r=spawnSync(process.execPath,args,{cwd:root,stdio:'inherit',windowsHide:true});if(r.error||r.status)throw r.error||Error('Candidate build failed');
}
for(const file of pinned)if(await digest(file)!==before[file])throw Error('Preserved input changed: '+file);
await writeFile(path.join(out,'preserved-inputs.json'),JSON.stringify(before,null,2)+'\n');
console.log('Toolbox diagnostic built separately; r3 and embedded runtime components preserved.');
