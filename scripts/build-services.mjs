import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdir,readdir,readFile,writeFile,copyFile,access} from 'node:fs/promises';import path from 'node:path';
const requested=process.argv[2]||'fps_elf';if(!['fps_elf','daemon','util'].includes(requested))throw Error('Select fps_elf, daemon or util');
const out=path.join(root,'build',requested);await mkdir(out,{recursive:true});
const dirs=[requested+'/'+(requested==='fps_elf'?'src':'source'),'extern/tiny-json','extern/cJSON'];if(requested==='util')dirs.push('util/source/mc4decrypter','extern/pugixml-1.15');
const files=[];for(const dir of dirs)for(const f of await readdir(path.join(source,dir)))if(/\.(cpp|c|s)$/.test(f))files.push(path.join(source,dir,f));files.push(path.join(source,'lib/backtrace.cpp'));
if(requested==='daemon')await copyFile(path.join(root,'build/shellui/shellui.elf'),path.join(source,'daemon/assets/shellui.elf'));
const objects=requested==='fps_elf'?[]:[path.join(root,'build/core/libNineS-pt.c.o')];
for(const file of files){const object=path.join(out,path.relative(source,file).replaceAll(/[\\/]/g,'-')+'.o');
 try{run(['cc',...(file.endsWith('.cpp')?cxx:[]),...common,'-fexceptions','-I',path.join(source,'include'),'-I',path.join(source,requested,'include'),'-I',path.join(source,'libNidResolver/include'),'-c',file,'-o',object],{cwd:path.join(source,requested)});objects.push(object);console.log('Compiled',path.relative(source,file));}catch(e){await writeFile(path.join(out,'failure.txt'),e.message);console.error(e.message.slice(-9500));process.exit(1);}
}
const libraries={fps_elf:['kernel','SceGnmDriver'],daemon:['ScePad','SceSystemService','SceNotification','SceNet','SceRegMgr','SceSysmodule','SceUserService','SceNetCtl','SceSysCore','kernel_sys','SceAppInstUtil'],util:['SceLibcInternal','SceSystemService','SceNet','SceSysmodule','SceUserService','SceNetCtl','SceSysCore','ScePad','SceVideoOut','kernel_sys','SceAppInstUtil','SceHttp2','SceSsl']}[requested];
const local=['libhijacker','libNidResolver','libNineS','libSelfDecryptor','libelfldr'].map(n=>path.join(root,'build/core/'+n+'.a'));
const external=requested==='util'?['z','curl','wolfssl','minizip','microhttpd','psl','zstd','crypto','ssl']:[];
const elf=path.join(out,requested+'.elf');
try{run(['ld.lld','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objects,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'-L',path.join(root,'dependencies/lib'),'-L',path.join(source,'lib'),'--start-group',...local,...external.map(l=>'-l'+l),...['c++','c++abi','unwind','c'].map(l=>path.join(sdk,'target/lib/lib'+l+'.a')),'-ldl','--end-group','--no-as-needed',...libraries.map(l=>'-l'+l),'-lSceLibcInternal','-lSceNet','-o',elf]);}catch(e){await writeFile(path.join(out,'failure.txt'),e.message);console.error(e.message.slice(-9500));process.exit(1);}
if(requested==='fps_elf')await copyFile(elf,path.join(source,'daemon/assets/fps_elf.elf'));
console.log('Built component '+elf+'; hardware validation pending.');
