// etaHEN/etaHEN-Plugins lib/make_plugin.py wire format, commit
// 6339554e4e3c92c0a655a37194c1152d6b82ad88. GPL-3.0-or-later.
import {inspectElf} from './verify-elf.mjs';
export function makePlugin(elf,titleId,version) {
 if(!/^[A-Za-z]{4}[0-9]{5}$/.test(titleId))throw Error('Plugin title ID must contain four letters and five digits');
 if(!/^[0-9]\.[0-9]{2}$/.test(version))throw Error('Plugin version must use x.xx');
 inspectElf(elf);
 return Buffer.concat([Buffer.from(`etaHEN_PLUGIN\0${titleId}\0${version}\0`,'ascii'),elf]);
}
