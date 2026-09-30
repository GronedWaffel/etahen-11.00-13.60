import {root,source,sdk,common,run} from './toolchain.mjs';
import {mkdir,writeFile} from 'node:fs/promises';import path from 'node:path';
const dir=path.join(root,'build/spawn-test');await mkdir(dir,{recursive:true});
const file=path.join(root,'port/spawn-test.cpp');
function link(objs,out){run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objs,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','--as-needed','-lkernel_web','-lSceLibcInternal','-o',out]);}
const child=path.join(dir,'child.elf');
run(['cc',...common,'-DSPAWN_TEST_CHILD','-c',file,'-o',path.join(dir,'child.o')]);link([path.join(dir,'child.o')],child);
await writeFile(path.join(dir,'embed.S'),`.section .data\n.balign 16\n.global child_start\nchild_start:\n.incbin "${child.replaceAll('\\','/')}"\n`);
const objs=[];for(const [input,name,extra] of [[file,'main',[]],[path.join(dir,'embed.S'),'embed',[]],[path.join(source,'libelfldr/src/elfldr.c'),'elfldr',['-I',path.join(source,'libelfldr/include')]], [path.join(source,'libNineS/src/pt.c'),'pt',['-I',path.join(source,'libNineS/include')]]]){let o=path.join(dir,name+'.o');run(['cc',...common,...extra,'-c',input,'-o',o]);objs.push(o);}
link(objs,path.join(dir,'spawn-test.elf'));console.log('Built isolated service-spawn test');
