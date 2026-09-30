import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const manifest=JSON.parse(await readFile(path.join(root,'BUILD-INPUTS.json'),'utf8'));
let failed=false;
for(const entry of manifest.inputs){
 try{const b=await readFile(path.join(root,entry.path));if(b.length!==entry.bytes||createHash('sha256').update(b).digest('hex')!==entry.sha256)throw Error('does not match the tested input');console.log('OK '+entry.path);}
 catch(e){failed=true;console.error(entry.path+': '+(e.code==='ENOENT'?'missing; see BUILDING.md':e.message));}
}
if(failed)process.exitCode=1;
