import {spawnSync} from 'node:child_process';
import {root,common,run} from './toolchain.mjs';
import path from 'node:path';
run(['cc',...common,'-c',path.join(root,'port/abi-check.cpp'),'-o',path.join(root,'build/abi-check.o')]);
for(const args of [
 ['--test','tests/elf.test.mjs','tests/build-embed.test.mjs','tests/relocate.test.mjs','tests/mono-abi.test.mjs','tests/toolbox-buttons.test.mjs'],
 ['scripts/build-core.mjs'],['scripts/build-shellui.mjs'],
 ['scripts/build-services.mjs','fps_elf'],['scripts/build-services.mjs','daemon'],
 ['scripts/build-services.mjs','util'],['scripts/build-toolbox-card.mjs'],['scripts/build-bootstrapper.mjs'],
 ['scripts/build-preflight.mjs'],['scripts/build-audit.mjs'],
 ['scripts/build-toolbox-launch.mjs'],
]) {
 const result=spawnSync(process.execPath,args,{cwd:root,stdio:'inherit'});
 if(result.error||result.status!==0)throw result.error||Error('Build/check failed: '+args.join(' '));
}
console.log('Experimental ELF rebuilt. Build success does not establish hardware compatibility.');
