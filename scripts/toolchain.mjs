import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {mkdirSync,readFileSync,writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {inspectElf} from './verify-elf.mjs';
export const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
export const source=path.join(root,'Source Code');
export const sdk=path.resolve(process.env.PS5_PAYLOAD_SDK||path.join(root,'../ps-neighbourhood/tools/ps5-sdk-0.43/ps5-payload-sdk'));
export const zig=path.resolve(process.env.ZIG||path.join(root,'../ps-neighbourhood/tools/zig-x86_64-windows-0.14.1/zig.exe'));
export const common=['-target','x86_64-linux-none','-U__linux__','-D__FreeBSD__=11','-D__PS5__','-D__SCE__','-fshort-wchar','-DPS5','-DPPR','-DETAHEN_PORT_1360=1','-Wno-date-time','-isystem',path.join(sdk,'target/include'),'-march=znver2','-fPIC','-fno-stack-protector','-fno-plt','-femulated-tls','-O1','-fno-exceptions'];
export const cxx=['-nostdinc++','-I',path.join(sdk,'target/include/c++/v1'),'-std=c++20','-D_Bool=bool'];
export function run(args,options={}) {
 mkdirSync(path.join(root,'build/zig-cache'),{recursive:true});
 args=[...args];
 if(args[0]==='cc'&&args.includes('-c')) {
  const index=args.indexOf('-c')+1,file=args[index];
  const input=readFileSync(file,'utf8');
  const assets=[...input.replaceAll('\\"','"').matchAll(/\.incbin\s+"([^"]+)"/g)];
  if(assets.length) {
   const hash=createHash('sha256').update(input);
   for(const [,asset] of assets)hash.update(readFileSync(path.resolve(options.cwd||source,asset)));
   const digest=hash.digest('hex'),directory=path.join(root,'build/embedded-sources');
   mkdirSync(directory,{recursive:true});
   const generated=path.join(directory,digest+path.extname(file));
   writeFileSync(generated,input+'\n/* Embedded asset content: '+digest+' */\n');
   args[index]=generated;args.push('-I',path.dirname(file));
  }
 }
 if(args[0]==='ld.lld'&&!args.includes('--eh-frame-hdr'))args.splice(1,0,'--eh-frame-hdr');
 if(args[0]==='ld.lld'&&!args.includes('--wrap=syscall')) {
  const shim=path.join(root,'build/syscall-shim.o');
  run(['cc',...common,'-c',path.join(root,'port/syscall-shim.S'),'-o',shim]);
  args.splice(1,0,'--wrap=syscall','--wrap=__syscall','--wrap=ptrace',shim);
 }
 const r=spawnSync(zig,args,{cwd:source,encoding:'utf8',maxBuffer:16*1024**2,env:{...process.env,SOURCE_DATE_EPOCH:'1790640000',ZIG_GLOBAL_CACHE_DIR:path.join(root,'build/zig-cache')},...options});
 if(r.status!==0)throw Error((r.stdout||'')+(r.stderr||'')+(r.error?.message||''));
 if(args[0]==='ld.lld'&&args.includes('-o'))inspectElf(readFileSync(args[args.indexOf('-o')+1]));
 return r;
}
