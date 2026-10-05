import {root,source,sdk,common,cxx,run} from './toolchain.mjs';import path from 'node:path';import {writeFile,readFile} from 'node:fs/promises';import {createHash} from 'node:crypto';
const embed=path.join(root,'build/toolbox-card-icon.S');
await writeFile(embed,`.section .rodata\n.balign 16\n.global card_icon,card_icon_end\ncard_icon:\n.incbin "${path.join(source,'ETAHEN.png').replaceAll('\\','/')}"\ncard_icon_end:\n`);
const icon=embed+'.o';run(['cc',...common,'-c',embed,'-o',icon]);
const manifest=[];
for(const remove of [true,false]){
 const name='toolbox-card-'+(remove?'remove':'install');const object=path.join(root,'build',name+'.o'),elf=path.join(root,'build',name+'.elf');
 run(['cc',...cxx,...common,...(remove?['-DETAHEN_CARD_REMOVE']:[]),'-c',path.join(root,'port/toolbox-card.cpp'),'-o',object]);
 run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),object,icon,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(root,'dependencies/lib/libsqlite3.a'),path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','-lkernel_web','-lSceLibcInternal','--no-as-needed','-lSceSystemService','-lSceUserService','-lSceIpmi','-lSceAppInstUtil','-o',elf]);
 const b=await readFile(elf);manifest.push({file:name+'.elf',bytes:b.length,sha256:createHash('sha256').update(b).digest('hex')});
}
await writeFile(path.join(root,'build/toolbox-card-manifest.json'),JSON.stringify({titleId:'ETHN13600',hardwareValidated:false,files:manifest},null,2));console.log(manifest);


