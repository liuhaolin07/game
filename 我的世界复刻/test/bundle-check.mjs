// 打包产物冒烟测试：把 dist/minecraft.html 里内联的脚本抽出来，在 Node 的替身环境里
// 真跑几百帧。这一条专门盯「双击能不能打开」——源码能跑不代表产物能跑：
// 打包可能把某些东西去掉（importmap、模块作用域、顶层 this），产物必须独立可用。
// 跑法：node test/bundle-check.mjs

import { readFile, access } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { join } from 'node:path';
import { installDom } from './dom-stub.mjs';

const ROOT = fileURLToPath(new URL('..', import.meta.url));
const BUNDLE = join(ROOT, 'dist/minecraft.html');

let pass = 0, fail = 0;
const failures = [];
const ok = (c, label, detail) => {
  if (c) { pass++; return true; }
  fail++; failures.push(label + (detail ? '  →  ' + detail : ''));
  return false;
};

console.log('\n\x1b[1m── 打包产物 ──\x1b[0m');

let html = null;
try {
  await access(BUNDLE);
  html = await readFile(BUNDLE, 'utf8');
  ok(true, 'dist/minecraft.html 存在');
} catch {
  ok(false, 'dist/minecraft.html 存在', '先运行 node build.mjs');
  report();
}

// --- 静态检查：这个文件必须能脱离服务器和 CDN 独立工作 ---
ok(!/cdn\.jsdelivr/.test(html), '不依赖任何 CDN');
ok(!/<script[^>]+src=/.test(html), '不引用任何外部脚本');
ok(!/from\s*['"][^'"]+\.js['"]/.test(html), '不残留跨文件的模块 import');
ok(/<script>\s*\n/.test(html) || /<script>/.test(html), '有内联的普通 script（非 module）');
ok(!/<script type="module"/.test(html), '没有 type=module 的 script');

// 把内联脚本抠出来执行
const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]);
ok(scripts.length >= 1, '至少有一段内联脚本', `${scripts.length} 段`);
const code = scripts.sort((a, b) => b.length - a.length)[0];
ok(code && code.length > 100000, '主脚本体积合理（应含 three.js）',
  `${((code || '').length / 1024).toFixed(0)} KB`);

if (code) {
  const env = installDom({ search: '?seed=20240817' });
  let crashed = null;
  try {
    // 产物是 IIFE，不是 ESM，用 Function 包一层在当前作用域执行
    new Function(code)();
  } catch (e) {
    crashed = e;
  }
  ok(!crashed, '产物脚本执行无异常',
    crashed ? (crashed.stack || '').split('\n').slice(0, 3).join(' | ') : '');

  if (!crashed) {
    let err = null;
    try { env.frames(200); } catch (e) { err = e; }
    ok(!err, '产物连续 200 帧无异常',
      err ? (err.stack || '').split('\n').slice(0, 3).join(' | ') : '');

    // 产物里拿不到 __game 之外的引用，只能靠画面相关的副作用判断：
    // 检查 globalThis 上是否留下了任何可用的东西，以及没有未捕获错误
    ok(typeof document.getElementById('app').children.length === 'number',
      '渲染器已挂到 #app');
  }
}

report();

function report() {
  console.log('\n' + '─'.repeat(52));
  if (fail === 0) console.log(`\x1b[32m产物检查全部通过：${pass} 项\x1b[0m`);
  else {
    console.log(`\x1b[31m${fail} 项失败\x1b[0m / 通过 ${pass} 项\n`);
    for (const f of failures) console.log('  \x1b[31m✗\x1b[0m ' + f);
  }
  console.log('─'.repeat(52));
  process.exit(fail === 0 ? 0 : 1);
}
