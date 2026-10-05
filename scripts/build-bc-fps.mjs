import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdirSync,copyFileSync} from 'node:fs';import path from 'node:path';
const out=path.join(root,'build/fps-bc');mkdirSync(out,{recursive:true});
const files=[path.join(root,'port/fps-bc.cpp'),...['detour_port.cpp','relocate.cpp','hde64.cpp'].map(n=>path.join(source,'shellui/src',n)),path.join(source,'fps_native/source/fps_publish.cpp')];
const objects=[];
for(const file of files){const obj=path.join(out,path.basename(file)+'.o');
 run(['cc',...cxx,...common,'-DONION_SYSTEM_TMP_FPS_SAMPLE="/system_tmp/etahen_fps_bc.sample"','-I',path.join(source,'include'),'-I',path.join(source,'shellui/include'),'-I',path.join(source,'fps_native/include'),'-c',file,'-o',obj]);objects.push(obj);}
const elf=path.join(out,'fps-bc.elf');
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objects,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',...['c++','c++abi','unwind','c'].map(n=>path.join(sdk,'target/lib/lib'+n+'.a')),'-ldl','--end-group','--no-as-needed','-lkernel_sys','-lSceLibcInternal','-lSceSystemService','-lSceGnmDriver','-o',elf]);
copyFileSync(elf,path.join(source,'daemon/assets/fps_elf.elf'));
console.log('PS4 FPS component built; game validation pending.');
