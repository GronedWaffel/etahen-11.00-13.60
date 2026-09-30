import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';

test('value-less Toolbox buttons can reach their action handlers',()=>{
 const xml=readFileSync(new URL('../Source Code/shellui/assets/etaHEN_toolbox.xml',import.meta.url),'utf8');
 const hook=readFileSync(new URL('../Source Code/shellui/src/HookFunctions.cpp',import.meta.url),'utf8');
 const allowList=hook.match(/excludedIds\s*=\s*\{([\s\S]*?)\};/);
 assert.ok(allowList,'OnPress value-less action allow-list exists');
 const allowed=new Set([...allowList[1].matchAll(/"([^"]+)"/g)].map(m=>m[1]));
 const buttons=[...xml.matchAll(/<button\b[^>]*\bid="([^"]+)"[^>]*\/>/g)];
 assert.ok(buttons.length>0);
 for(const [element,id] of buttons){
  assert.ok(/\bvalue="[^"]+"/.test(element)||allowed.has(id),`${id} would be ignored by the empty-value gate`);
  assert.ok(hook.includes(`id == "${id}"`),`${id} has an action handler`);
 }
});
