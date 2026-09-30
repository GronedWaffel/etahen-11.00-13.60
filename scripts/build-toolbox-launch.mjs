import {root,sdk,common,cxx,run} from './toolchain.mjs';import path from 'node:path';
const object=path.join(root,'build/toolbox-launch.o');
run(['cc',...cxx,...common,'-c',path.join(root,'port/toolbox-launch.cpp'),'-o',object]);
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),object,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','-lkernel_web','-lSceLibcInternal','-lSceUserService','-o',path.join(root,'build/toolbox-launch.elf')]);
console.log('Built legacy Toolbox route launcher; no services or hooks.');
