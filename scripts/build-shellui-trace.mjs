// Separate ShellUI trace candidate; preserve prior diagnostic and r3 outputs.
import {root,source,common,run} from './toolchain.mjs';
import {mkdir,readFile,writeFile,access} from 'node:fs/promises';
import {createHash} from 'node:crypto';import {spawnSync} from 'node:child_process';import path from 'node:path';
const out=path.join(root,'build/shellui-trace');await mkdir(out,{recursive:true});
const digest=async f=>createHash('sha256').update(await readFile(path.join(root,f))).digest('hex');
const pinned=['build/etaHEN-13.60-experimental.elf','build/shellui/shellui.elf','Source Code/daemon/assets/shellui.elf','build/util/util.elf','Source Code/bootstrapper/assets/kstuff.elf'];
const prior='build/toolbox-diagnostic/etaHEN-13.60-toolbox-diagnostic.elf';
try{await access(path.join(root,prior));pinned.push(prior);}catch(e){if(e.code!=='ENOENT')throw e;}
const before=Object.fromEntries(await Promise.all(pinned.map(async f=>[f,await digest(f)])));
if(before[pinned[1]]!==before[pinned[2]])throw Error('Embedded ShellUI differs; run the baseline component build first');
for(const name of ['pt.c','port_publish.c','port_diagnostic.c'])run(['cc',...common,'-DETAHEN_TOOLBOX_DIAGNOSTIC=1','-I',path.join(source,'include'),'-c',path.join(source,'libNineS/src',name),'-o',path.join(out,name+'.o')]);
for(const args of [['scripts/build-shellui.mjs','--shellui-trace'],['scripts/build-services.mjs','daemon','--shellui-trace'],['scripts/build-bootstrapper.mjs','--shellui-trace']]){
 const r=spawnSync(process.execPath,args,{cwd:root,stdio:'inherit',windowsHide:true});if(r.error||r.status)throw r.error||Error('Candidate build failed');
}
for(const file of pinned)if(await digest(file)!==before[file])throw Error('Preserved input changed: '+file);
await writeFile(path.join(out,'preserved-inputs.json'),JSON.stringify(before,null,2)+'\n');
console.log('ShellUI trace built separately; r3, previous diagnostic, kstuff and utility preserved.');
