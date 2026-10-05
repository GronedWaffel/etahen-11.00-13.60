import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdirSync,readdirSync} from 'node:fs';import path from 'node:path';
const out=path.join(root,'build/fps-native');mkdirSync(out,{recursive:true});
const base=path.join(source,'fps_native'),objects=[];
for(const file of readdirSync(path.join(base,'source')).filter(f=>f.endsWith('.cpp'))){
 const obj=path.join(out,file+'.o');
 run(['cc',...cxx,...common,'-I',path.join(base,'include'),'-c',path.join(base,'source',file),'-o',obj]);objects.push(obj);
}
run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objects,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'-L',path.join(source,'lib'),'--start-group',...['c++','c++abi','unwind','c'].map(n=>path.join(sdk,'target/lib/lib'+n+'.a')),'-ldl','--end-group','--no-as-needed','-lkernel_sys','-lSceLibcInternal','-lSceSystemService','-lSceSysCore','-o',path.join(out,'fps-native.elf')]);
console.log('Native PS5 FPS sampler built; game validation pending.');
