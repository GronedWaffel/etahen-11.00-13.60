
import {root,source,sdk,common,cxx,run} from './toolchain.mjs';import path from 'node:path';
const obj=path.join(root,'build/private-watch.o'),elf=path.join(root,'build/private-watch.elf');
run(['cc',...cxx,...common,'-I',path.join(source,'include'),'-c',path.join(root,'port/private-1240-watch-p5.cpp'),'-o',obj]);
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),obj,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','-lkernel_web','-lSceLibcInternal','-o',elf]);
