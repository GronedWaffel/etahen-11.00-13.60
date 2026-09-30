import {root,source,sdk,common,cxx,run} from './toolchain.mjs';
import {mkdir,writeFile,readFile} from 'node:fs/promises';import path from 'node:path';import {createHash} from 'node:crypto';
const out=path.join(root,'build/audit');await mkdir(out,{recursive:true});
function compile(file,extra=[]){const obj=path.join(out,path.basename(file)+extra.length+'.o');run(['cc',...(file.endsWith('.cpp')?cxx:[]),...common,...extra,'-c',file,'-o',obj]);return obj;}
const shim=compile(path.join(root,'port/syscall-shim.S'));
function link(objects,elf,extra=[],kernel='kernel_web'){run(['ld.lld','--no-dependent-libraries','--wrap=syscall','--wrap=__syscall','--wrap=ptrace','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objects,shim,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',...extra,path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','--as-needed','-l'+kernel,'-lSceLibcInternal','-lSceNet','-o',elf]);}
const phase=Number(process.env.ETAHEN_AUDIT_PHASE||0);if(!Number.isInteger(phase)||phase<0||phase>5)throw Error('Invalid audit phase');
const index=Number(process.env.ETAHEN_AUDIT_COMPILE_INDEX??-1);if(!Number.isInteger(index)||index< -1||index>19)throw Error('Invalid compile index');
const writeProbe=process.env.ETAHEN_AUDIT_WRITE_PROBE==='1'?1:0;
const fullComponent=process.env.ETAHEN_AUDIT_FULL_COMPONENT==='1';
const audit=fullComponent?path.join(root,'build/shellui/shellui.elf'):path.join(out,'toolbox-audit.elf');
const aotLive=process.env.ETAHEN_AOT_LIVE_TEST==='1';
const hookTest=process.env.ETAHEN_HOOK_TEST==='1'||aotLive;
const disposable=process.env.ETAHEN_AUDIT_DISPOSABLE==='1';
const prepare=process.env.ETAHEN_AUDIT_PREPARE_HOOKS==='1';
const signatures=process.env.ETAHEN_AUDIT_SIGNATURES==='1';
const resources=process.env.ETAHEN_AUDIT_RESOURCES==='1';
const resourceCall=Number(process.env.ETAHEN_AUDIT_RESOURCE_CALL||0);
if(![0,1,2].includes(resourceCall))throw Error('Invalid resource overload');
if(resourceCall&&!resources)throw Error('Resource call requires resource audit mode');
if(resources&&(phase!==4||prepare||writeProbe||hookTest||fullComponent))throw Error('Resource audit requires phase 4 with no hooks or writes');
if(signatures&&(phase!==4||prepare||writeProbe||hookTest||fullComponent))throw Error('Signature audit requires phase 4 with no hooks or writes');
const auditObjects=hookTest?[
 compile(path.join(root,aotLive?'port/aot-live-test.cpp':'port/hook-test.cpp'),['-I',path.join(source,'shellui/include')]),...(aotLive?[]:[compile(path.join(root,'port/hook-test.S'))]),
 ...['detour_port.cpp','relocate.cpp','hde64.cpp'].map(n=>compile(path.join(source,'shellui/src',n),['-I',path.join(source,'shellui/include'),'-DHDE_DECODE_ONLY']))
]:[compile(path.join(root,'port/toolbox-audit.cpp'),['-DETAHEN_AUDIT_PHASE='+phase,'-DETAHEN_AUDIT_COMPILE_INDEX='+index,'-DETAHEN_AUDIT_WRITE_PROBE='+writeProbe,...(signatures?['-DETAHEN_AUDIT_SIGNATURES']:[]),...(resources?['-DETAHEN_AUDIT_RESOURCES']:[]),...(resourceCall?['-DETAHEN_AUDIT_RESOURCE_CALL='+resourceCall]:[]),...(prepare?['-DETAHEN_AUDIT_PREPARE_HOOKS','-I',path.join(source,'shellui/include')]:[])])];
if(prepare&&!hookTest)for(const n of ['detour_port.cpp','relocate.cpp','hde64.cpp'])auditObjects.push(compile(path.join(source,'shellui/src',n),['-I',path.join(source,'shellui/include'),'-DHDE_DECODE_ONLY']));
if(!fullComponent)link(auditObjects,audit,['--wrap=__patch_init'],disposable?'kernel_web':'kernel_sys');
const auditId=createHash('sha256').update(await readFile(audit)).update(await readFile(path.join(root,'build/core/libNineS.a'))).update(await readFile(path.join(root,'port/audit-loader.cpp'))).digest('hex');
await writeFile(path.join(out,'audit-run.json'),JSON.stringify({id:auditId,phase,index,hookTest,disposable,prepare,writeProbe,fullComponent,aotLive,signatures,resources,resourceCall}));
link([compile(path.join(root,'port/audit-resume.cpp'))],path.join(out,'audit-resume.elf'));
const embed=path.join(out,'embed.S');await writeFile(embed,`.section .data\n.balign 16\n.global audit_start\naudit_start:\n.incbin "${audit.replaceAll('\\','/')}"\n`);
link([compile(path.join(root,'port/audit-loader.cpp'),['-DETAHEN_AUDIT_RUN_ID="'+auditId+'"',...(disposable?['-DETAHEN_INJECT_TEST=1']:[])]),compile(embed)],path.join(out,'toolbox-audit-loader.elf'),[path.join(root,'build/core/libNineS.a')]);console.log(fullComponent?'Built loader for the selected full ShellUI component.':aotLive?'Built temporary live system-method probe with rollback.':'Built diagnostic loader; no system-method hooks.');
const testThread=path.join(out,'inject-test-thread.elf');link([compile(path.join(root,'port/inject-test-thread.cpp'))],testThread);
link([compile(path.join(root,'port/inject-test-host.cpp'))],path.join(out,'inject-test-host.elf'));
const testEmbed=path.join(out,'test-embed.S');await writeFile(testEmbed,`.section .data\n.balign 16\n.global audit_start\naudit_start:\n.incbin "${testThread.replaceAll('\\','/')}"\n`);
link([compile(path.join(root,'port/audit-loader.cpp'),['-DETAHEN_INJECT_TEST=1']),compile(testEmbed)],path.join(out,'inject-test-loader.elf'),[path.join(root,'build/core/libNineS.a')]);
