import {spawnSync} from 'node:child_process';
import {root,common,run} from './toolchain.mjs';
import path from 'node:path';
run(['cc',...common,'-c',path.join(root,'port/abi-check.cpp'),'-o',path.join(root,'build/abi-check.o')]);
for(const args of [
 ['scripts/build-limiter-gate.mjs'],
 ['--test','tests/*.test.mjs'],
 ['scripts/build-core.mjs'],['scripts/build-shellui.mjs'],
 ['scripts/build-native-fps.mjs'],['scripts/build-bc-fps.mjs'],['scripts/build-services.mjs','daemon'],
 ['scripts/build-services.mjs','util'],['scripts/build-toolbox-card.mjs'],['scripts/build-private-watch.mjs'],['scripts/build-bootstrapper.mjs'],
 ['scripts/build-preflight.mjs'],['scripts/build-audit.mjs'],
 ['scripts/build-toolbox-launch.mjs'],
 ['scripts/build-limiter-plugin.mjs'],
]) {
 const result=spawnSync(process.execPath,args,{cwd:root,stdio:'inherit'});
 if(result.error||result.status!==0)throw result.error||Error('Build/check failed: '+args.join(' '));
}
console.log('Experimental ELF rebuilt. Build success does not establish hardware compatibility.');
