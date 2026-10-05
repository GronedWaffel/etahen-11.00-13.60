import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdir,readdir,readFile,writeFile} from 'node:fs/promises';import path from 'node:path';import {createHash} from 'node:crypto';
const trace=process.argv.includes('--shellui-trace');
const out=path.join(root,trace?'build/shellui-trace/shellui':'build/shellui');await mkdir(out,{recursive:true});
const probeHooks=process.env.ETAHEN_PORT_PROBE_HOOKS;
if(probeHooks&&!/^(0x[0-9a-f]+|[0-9]+)$/i.test(probeHooks))throw Error('Invalid diagnostic hook mask');
const assets=path.join(source,'shellui/assets'),key=Buffer.from('U0lTVFIwX0lfU0VFX1lPVQ==');
for(const name of ['etaHEN_toolbox','etaHEN_Lite']){const b=await readFile(path.join(assets,name+'.xml'));for(let i=0;i<b.length;i++)b[i]^=key[i%key.length];await writeFile(path.join(assets,name+'.sxml'),b);}
const files=[];for(const dir of ['shellui/src','extern/tiny-json','extern/cJSON'])for(const f of await readdir(path.join(source,dir)))if(/\.(cpp|c|s)$/.test(f))files.push(path.join(source,dir,f));files.push(path.join(source,'lib/backtrace.cpp'));
const objects=[];for(const file of files){const object=path.join(out,path.basename(file)+'.o');
 const args=['cc',...(file.endsWith('.cpp')?cxx:[]),...common,...(trace?['-DETAHEN_SHELLUI_TRACE=1']:[]),...(probeHooks?['-DETAHEN_PORT_PROBE_HOOKS='+probeHooks]:[]),...(process.env.ETAHEN_PORT_PREPARE_ONLY==='1'?['-DETAHEN_PORT_PREPARE_ONLY']:[]),'-fexceptions','-I',path.join(source,'fps_native/include'),'-I',path.join(source,'include'),'-I',path.join(source,'shellui/include'),'-I',path.join(source,'libNidResolver/include'),'-c',file,'-o',object];
 try{run(args,{cwd:path.join(source,'shellui')});objects.push(object);console.log('Compiled',path.relative(source,file));}catch(e){await writeFile(path.join(out,'failure.txt'),e.message);console.error(e.message.slice(-10000));process.exit(1);}
}
const elf=path.join(out,'shellui.elf'),libs=['ScePad','SceRegMgr','SceSystemService','SceNet','SceSysmodule','SceUserService','SceNetCtl','SceSysCore','kernel_sys','SceAppInstUtil'];
try{run(['ld.lld','--wrap=__patch_init','--no-dependent-libraries','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objects,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'-L',path.join(source,'lib'),'--start-group',path.join(root,'build/core/libhijacker.a'),path.join(root,'build/core/libNidResolver.a'),path.join(sdk,'target/lib/libc++.a'),path.join(sdk,'target/lib/libc++abi.a'),path.join(sdk,'target/lib/libunwind.a'),path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','--no-as-needed',...libs.map(l=>'-l'+l),'-lSceLibcInternal','-o',elf]);}catch(e){await writeFile(path.join(out,'failure.txt'),e.message);console.error(e.message.slice(-10000));process.exit(1);}
const b=await readFile(elf);await writeFile(path.join(out,'manifest.json'),JSON.stringify({file:'shellui.elf',bytes:b.length,sha256:createHash('sha256').update(b).digest('hex'),hardwareValidated:false,probeHooks:probeHooks||null,prepareOnly:process.env.ETAHEN_PORT_PREPARE_ONLY==='1'},null,2));console.log('ShellUI component built; not a complete etaHEN payload.');
