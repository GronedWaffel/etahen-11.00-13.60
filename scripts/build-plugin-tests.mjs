import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdirSync,readFileSync,writeFileSync} from 'node:fs';import path from 'node:path';
import {makePlugin} from './plugin-format.mjs';
const out=path.join(root,'build/plugin-fixtures');mkdirSync(out,{recursive:true});
for(const kind of ['system','game']){
 const object=path.join(out,kind+'.o'),elf=path.join(out,kind+'-lifecycle-test.elf');
 run(['cc',...cxx,...common,...(kind==='system'?['-DSYSTEM_PLUGIN_TEST']:[]),'-c',path.join(root,'port/plugin-test.cpp'),'-o',object]);
 run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),object,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','--no-as-needed','-lkernel_sys','-lSceLibcInternal','-lSceSystemService','-o',elf]);
 const plugin=elf.replace(/\.elf$/,'.plugin');
 writeFileSync(plugin,makePlugin(readFileSync(elf),kind==='system'?'SNPS00001':'SNPG00001','1.00'));
 console.log(plugin);
}
