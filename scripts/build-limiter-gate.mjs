import {root,zig} from './toolchain.mjs';
import {spawnSync} from 'node:child_process';
import {mkdirSync,readFileSync,writeFileSync} from 'node:fs';
import path from 'node:path';
const dir=path.join(root,'build/fps-limiter');mkdirSync(dir,{recursive:true});
function run(args){const r=spawnSync(zig,args,{cwd:root,encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')}});if(r.status!==0)throw Error(r.stderr||r.error?.message);}
for(const [name,stem] of [['gate','fps-limiter-gate'],['prx-gate','prx-load-gate']]){
const obj=path.join(dir,name+'.o'),elf=path.join(dir,name+'.elf'),bin=path.join(dir,name+'.bin');
run(['cc','-target','x86_64-linux-none','-O2','-fPIC','-ffreestanding','-fno-stack-protector','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-mno-red-zone','-mno-avx','-c','port/'+stem+'.c','-o',obj]);
// Standalone code, deliberately bypass the payload linker/syscall shim.
run(['ld.lld','-T','port/'+stem+'.ld',obj,'-o',elf]);
run(['objcopy','-O','binary','--only-section=.text',elf,bin]);
const bytes=readFileSync(bin);if(!bytes.length||bytes.length>4096)throw Error('Unexpected limiter code size');
writeFileSync(path.join(dir,name+'.h'),'// Generated; do not edit.\nstatic const unsigned char '+(name==='gate'?'limiter_gate_bytes':'prx_gate_bytes')+'[]={'+[...bytes].join(',')+'};\n');
console.log('Built',stem,bytes.length,'bytes');
}
