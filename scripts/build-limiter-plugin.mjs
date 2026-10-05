import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdirSync,readFileSync,writeFileSync} from 'node:fs';import path from 'node:path';
import {makePlugin} from './plugin-format.mjs';
const out=path.join(root,'build/fps-limiter');mkdirSync(out,{recursive:true});
const object=path.join(out,'plugin.o'),elf=path.join(out,'gta-v-15fps-test.elf');
run(['cc',...cxx,...common,'-c',path.join(root,'port/plugin-fps-limiter.cpp'),'-o',object]);
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),object,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','--no-as-needed','-lkernel_sys','-lSceLibcInternal','-o',elf]);
writeFileSync(elf.replace(/\.elf$/,'.plugin'),makePlugin(readFileSync(elf),'SNPL00015','1.00'));
console.log('Built native GTA V 15 FPS test .plugin controller');
