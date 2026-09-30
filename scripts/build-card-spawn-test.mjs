import {root,sdk,common,cxx,run} from './toolchain.mjs';import path from 'node:path';import {writeFile} from 'node:fs/promises';
const embed=path.join(root,'build/card-spawn-embed.S'),main=path.join(root,'build/card-spawn-test.o');
await writeFile(embed,`.section .data\n.balign 16\n.global child_start\nchild_start:\n.incbin "${path.join(root,'build/toolbox-card-install.elf').replaceAll('\\','/')}"\n`);
run(['cc',...cxx,...common,'-c',path.join(root,'port/card-spawn-test.cpp'),'-o',main]);run(['cc',...common,'-c',embed,'-o',embed+'.o']);
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),main,embed+'.o',path.join(root,'build/core/libelfldr-elfldr.c.o'),path.join(root,'build/core/libNineS-pt.c.o'),path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','-lkernel_web','-lSceLibcInternal','-o',path.join(root,'build/card-spawn-test.elf')]);
console.log('Built embedded card-helper spawn test; no etaHEN service reload.');
