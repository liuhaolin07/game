// 打包脚本：把多文件源码 + three.js 打成一个自带全部依赖的 HTML。
// 产物 double-click 就能跑（走 file:// 协议也能用），不需要起服务器、不需要联网。
//
//   node build.mjs            生成 dist/minecraft.html
//   node build.mjs --watch    监听源码改动自动重建
//
// 为什么要产物：源码是 ES 模块，浏览器在 file:// 下会用同源策略拦掉所有 import。
// 想保留多文件架构（便于维护和测试），又想让别人双击就能玩，就只能两头都顾：
// 源码仍然是多模块，这���只负责把它们揉成一个文件。

import { build, context } from 'esbuild';
import { readFile, writeFile, mkdir } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { join, dirname } from 'node:path';

const ROOT = fileURLToPath(new URL('.', import.meta.url));
const watch = process.argv.includes('--watch');

// three 从 node_modules 解析并打进包里，所以产物不依赖任何 CDN。
// 如果本地没装 three，会回退到 CDN（见下面的 external 逻辑）。
let hasThree = true;
try {
  await readFile(join(ROOT, 'node_modules/three/package.json'));
} catch {
  hasThree = false;
  console.warn('[build] 没找到 node_modules/three，将改用 CDN（产物需要联网）');
}

const options = {
  entryPoints: [join(ROOT, 'src/main.js')],
  bundle: true,
  format: 'iife',
  target: ['chrome100', 'firefox100', 'safari15', 'edge100'],
  minify: !watch,
  legalComments: 'none',
  outfile: join(ROOT, 'dist/bundle.js'),
  logLevel: 'info',
  // 没装 three 时让 import 'three' 保持原样，交给 importmap 处理
  external: hasThree ? [] : ['three'],
  define: { 'process.env.NODE_ENV': '"production"' },
};

async function assemble() {
  const js = await readFile(join(ROOT, 'dist/bundle.js'), 'utf8');
  let html = await readFile(join(ROOT, 'index.html'), 'utf8');

  // 1. 用打包产物替换 importmap + module script
  // 1. 用打包产物替换 module script
  //    importmap 只在「没装 three、three 保持 external」时必须保留 ——
  //    无条件删掉它会让这个回退分支直接坏掉。
  if (hasThree) {
    html = html.replace(
      /<script type="importmap">[\s\S]*?<\/script>/,
      '<!-- three.js 已内联进 bundle，无需 importmap -->',
    );
  }
  html = html.replace(
    /<script type="module" src="\.\/src\/main\.js"><\/script>/,
    () => `<script>\n${js}\n</script>`,
  );

  // 2. 产物是单文件，不该再提示「需要起服务器」
  html = html
    .replace(/    提示：ES 模块不能用 file:\/\/ 打开[^\n]*\n/, '')
    .replace(
      /'[^']*ES 模块不能用 file:\/\/ 打开[^']*'/,
      '"\\n\\n这个文件直接双击就能玩。如果画面是黑的，按 F12 打开控制台看报错。"',
    );

  // 3. 标注这是打包产物
  html = html.replace('<title>', '<!-- 本文件由 build.mjs 从 src/ 自动生成，请勿手改 -->\n<title>');

  await mkdir(join(ROOT, 'dist'), { recursive: true });
  await writeFile(join(ROOT, 'dist/minecraft.html'), html, 'utf8');
  const kb = (Buffer.byteLength(html) / 1024).toFixed(0);
  console.log(`[build] dist/minecraft.html  ${kb} KB`);
}

if (watch) {
  const ctx = await context(options);
  await ctx.watch();
  await assemble();
  console.log('[build] 监听中，源码改动会自动重建（Ctrl+C 停止）');
} else {
  await build(options);
  await assemble();
}
void dirname;
