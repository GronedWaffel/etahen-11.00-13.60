// Diagnostic candidate: retains r3's ShellUI, utility, kstuff and startup waits.
import {root,source,common,run} from './toolchain.mjs';
import {mkdir,readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import path from 'node:path';
const out=path.join(root,'build/startup-profile');await mkdir(out,{recursive:true});
const digest=async f=>createHash('sha256').update(await readFile(path.join(root,f))).digest('hex');
const original=await digest('build/etaHEN-13.60-experimental.elf');
if(original!=='aa166a43347d10a31f7efa5ca26ec32f512c65e8910466bfc8033537337b87b2')throw Error('Expected the preserved r3 build');
if(await digest('Source Code/daemon/assets/shellui.elf')!==await digest('build/shellui/shellui.elf'))throw Error('Embedded ShellUI differs from the existing built component');
for(const [file,name] of [['libNineS/src/pt.c','pt.c'],['libNineS/src/port_publish.c','port_publish.c'],['libelfldr/src/elfldr.c','elfldr.c']]){
 run(['cc',...common,'-DETAHEN_STARTUP_PROFILE=1','-I',path.join(source,'include'),'-I',path.join(source,'libelfldr/include'),'-c',path.join(source,file),'-o',path.join(out,name+'.o')]);
}
for(const args of [['scripts/build-services.mjs','daemon','--startup-profile'],['scripts/build-bootstrapper.mjs','--startup-profile']]){
 const r=spawnSync(process.execPath,args,{cwd:root,stdio:'inherit',windowsHide:true});
 if(r.error||r.status!==0)throw r.error||Error('Profile build failed');
}
if(await digest('build/etaHEN-13.60-experimental.elf')!==original)throw Error('Original r3 changed');
console.log('Profile candidate built separately; original r3 SHA-256 unchanged.');
