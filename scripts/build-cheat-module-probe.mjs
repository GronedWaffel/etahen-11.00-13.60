import {root,source,sdk,common,cxx,run} from './toolchain.mjs';import path from 'node:path';
const object=path.join(root,'build/cheat-module-probe.o');
run(['cc',...cxx,...common,'-I',path.join(source,'include'),'-c',path.join(root,'port/cheat-module-probe.cpp'),'-o',object]);
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),object,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(root,'build/core/libNineS.a'),path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','-lkernel_web','-lSceLibcInternal','-o',path.join(root,'build/cheat-module-probe.elf')]);
