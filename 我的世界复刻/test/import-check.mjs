// 模块图检查：把每个模块都真加载一遍，确认没有循环依赖、拼错的路径、
// 引用了不存在的导出。three 用桩件，DOM 用最小替身。
// 跑法：node --import ./test/register.mjs test/import-check.mjs

import { readdir, readFile } from 'node:fs/promises';
import { join, relative, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const ROOT = fileURLToPath(new URL('..', import.meta.url));
const SRC = join(ROOT, 'src');

// ---------- 最小 DOM 替身 ----------
// 2D 上下文做成「任何方法调用都返回一个可继续链式调用的代理」，
// 这样 createRadialGradient().addColorStop() 这类链式写法不会炸。
let ctx2d;
{
  const handler = {
    get(_, k) {
      if (k === 'createImageData' || k === 'getImageData') {
        return (x, y, w, h) => ({ data: new Uint8ClampedArray(w * h * 4), width: w, height: h });
      }
      if (k === 'measureText') return () => ({ width: 8 });
      if (k === 'canvas') return { width: 256, height: 256 };
      if (k === Symbol.toPrimitive) return () => 0;
      return () => ctx2d;
    },
    set() { return true; },
    apply() { return ctx2d; },
  };
  ctx2d = new Proxy(function () {}, handler);
}
function makeEl(tag) {
  const el = {
    tagName: String(tag).toUpperCase(), style: { display: '' }, dataset: {},
    children: [], className: '', textContent: '', innerHTML: '', width: 16, height: 16,
    classList: { add() {}, remove() {}, toggle() {}, contains: () => false },
    getContext: () => ctx2d,
    appendChild(c) { this.children.push(c); return c; },
    addEventListener() {}, removeEventListener() {},
    setAttribute() {}, getAttribute: () => null,
    querySelector: () => null, querySelectorAll: () => [],
    focus() {},
  };
  return el;
}
const store = new Map();
globalThis.document = {
  readyState: 'complete',
  createElement: makeEl,
  createElementNS: makeEl,
  getElementById: (id) => (id ? makeEl('div') : null),
  querySelector: () => null,
  querySelectorAll: () => [],
  addEventListener() {},
  body: makeEl('body'),
};
globalThis.window = globalThis;
globalThis.localStorage = {
  getItem: (k) => (store.has(k) ? store.get(k) : null),
  setItem: (k, v) => store.set(k, String(v)),
  removeItem: (k) => store.delete(k),
};
globalThis.location = { search: '', href: 'http://localhost:8080/' };
globalThis.devicePixelRatio = 1;
globalThis.innerWidth = 1280;
globalThis.innerHeight = 720;
globalThis.performance = globalThis.performance || { now: () => Date.now() };
globalThis.requestAnimationFrame = () => 0;
globalThis.AudioContext = undefined;
globalThis.addEventListener = () => {};
globalThis.removeEventListener = () => {};
globalThis.WebGLRenderingContext = function () {};

// ---------- 收集所有源文件 ----------
async function walk(dir, out = []) {
  for (const e of await readdir(dir, { withFileTypes: true })) {
    const p = join(dir, e.name);
    if (e.isDirectory()) await walk(p, out);
    else if (e.name.endsWith('.js')) out.push(p);
  }
  return out;
}

const files = (await walk(SRC)).sort();
console.log(`\x1b[1m模块图检查（${files.length} 个文件）\x1b[0m\n`);

let bad = 0;

// 1) 静态解析：找出 import 的源文件是否存在、导出的名字是否真的存在
const exportsOf = new Map();
for (const f of files) {
  const src = await readFile(f, 'utf8');
  const names = new Set();
  for (const m of src.matchAll(/^export\s+(?:async\s+)?(?:function|class)\s+([A-Za-z0-9_$]+)/gm)) names.add(m[1]);
  for (const m of src.matchAll(/^export\s+(?:const|let|var)\s+([A-Za-z0-9_$]+)/gm)) names.add(m[1]);
  for (const m of src.matchAll(/^export\s*\{([^}]+)\}/gm)) {
    for (const part of m[1].split(',')) {
      const n = part.trim().split(/\s+as\s+/).pop().trim();
      if (n) names.add(n);
    }
  }
  if (/^export\s+default/m.test(src)) names.add('default');
  exportsOf.set(f, names);
}

console.log('  解析 import / export：');
for (const f of files) {
  const src = await readFile(f, 'utf8');
  const rel = relative(ROOT, f).replace(/\\/g, '/');
  const errs = [];
  for (const m of src.matchAll(/import\s+([^'"]*?)\s*from\s*['"]([^'"]+)['"]/g)) {
    const clause = m[1].trim();
    const spec = m[2];
    if (spec === 'three') continue;
    if (!spec.startsWith('.')) { errs.push(`裸模块 ${spec}`); continue; }
    const target = join(dirname(f), spec);
    try {
      await readFile(target);
    } catch {
      errs.push(`找不到 ${spec}`);
      continue;
    }
    const want = new Set();
    const braces = clause.match(/\{([^}]*)\}/);
    if (braces) {
      for (const p of braces[1].split(',')) {
        const n = p.trim().split(/\s+as\s+/)[0].trim();
        if (n) want.add(n);
      }
    }
    const have = exportsOf.get(target);
    if (have) {
      for (const n of want) if (!have.has(n)) errs.push(`${spec} 未导出 ${n}`);
    }
  }
  if (errs.length) { bad += errs.length; console.log(`  \x1b[31m✗\x1b[0m ${rel}\n      ${errs.join('\n      ')}`); }
}
console.log(bad ? '' : '  \x1b[32m✓\x1b[0m 全部 import 路径与导出名有效');

// 2) 真正动态加载一遍
console.log('\n  实际加载模块：');
for (const f of files) {
  const rel = relative(ROOT, f).replace(/\\/g, '/');
  if (rel.endsWith('main.js')) { console.log(`  \x1b[33m-\x1b[0m ${rel}（会立即启动主循环，跳过）`); continue; }
  try {
    const m = await import(pathToFileURL(f).href);
    console.log(`  \x1b[32m✓\x1b[0m ${rel}  (${Object.keys(m).length} 个导出)`);
  } catch (e) {
    bad++;
    console.log(`  \x1b[31m✗\x1b[0m ${rel}\n      ${e.message.split('\n')[0]}`);
  }
}

console.log('\n' + '─'.repeat(52));
console.log(bad === 0 ? '\x1b[32m模块图检查通过\x1b[0m\n' : `\x1b[31m${bad} 处问题\x1b[0m\n`);
process.exit(bad === 0 ? 0 : 1);
