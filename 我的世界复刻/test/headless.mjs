// 无头测试：不启动浏览器，直接在 Node 里跑纯逻辑模块。
// 覆盖噪声、方块表、地形生成、光照传播、网格化、射线拾取、玩家碰撞、存档往返。
// 跑法：node test/headless.mjs

import { Noise, hash2i, hash3i, makeRng } from '../src/core/noise.js';
import { B, BLOCKS, BLOCK_COUNT, faceVisible, IS_OPAQUE, LIGHT_EMIT, LIGHT_FILTER, RENDER_KIND } from '../src/core/blocks.js';
import { CHUNK_X, CHUNK_Z, WORLD_H, CHUNK_AREA, SEA_LEVEL, IDX, COL } from '../src/core/constants.js';
import { ChunkGenerator } from '../src/world/generator.js';
import { World } from '../src/world/world.js';
import { LightEngine } from '../src/world/light.js';
import { Mesher } from '../src/render/mesher.js';
import { raycast, isTargetable } from '../src/physics/raycast.js';
import { Player } from '../src/physics/player.js';
import { BIOMES, BIO, biomeName } from '../src/world/biomes.js';
import TILE, { TILE_ORDER, ATLAS_SIZE } from '../src/render/tiles.js';

let pass = 0, fail = 0;
const failures = [];

/** 除萤石以外的发光方块（岩浆在洞底到处都是，测方块光时要排除） */
const LIGHT_EMIT_TEST = (id) => LIGHT_EMIT[id] > 0 && id !== B.glowstone;
const RENDER_KIND_TEST = (id) => RENDER_KIND[id];
const LIGHT_FILTER_TEST = LIGHT_FILTER;

function ok(cond, label, detail) {
  if (cond) { pass++; return true; }
  fail++;
  failures.push(label + (detail ? '  →  ' + detail : ''));
  return false;
}
function eq(a, b, label) { return ok(a === b, label, `期望 ${b}，实际 ${a}`); }
function section(t) { console.log('\n\x1b[1m── ' + t + ' ──\x1b[0m'); }
const SEED = 20240817;

// ────────────────────────────────────────────── 噪声
section('噪声');
{
  const n1 = new Noise(SEED), n2 = new Noise(SEED);
  let min2 = 9, max2 = -9, min3 = 9, max3 = -9;
  for (let i = 0; i < 4000; i++) {
    const x = i * 0.37 - 500, y = i * 0.11 - 200;
    const a = n1.noise2(x, y), b = n2.noise2(x, y);
    if (a !== b) { ok(false, 'noise2 同种子可复现'); break; }
    min2 = Math.min(min2, a); max2 = Math.max(max2, a);
    const c = n1.noise3(x, y, x * 0.5);
    min3 = Math.min(min3, c); max3 = Math.max(max3, c);
  }
  ok(min2 >= -1.02 && max2 <= 1.02, 'noise2 值域 [-1,1]', `${min2.toFixed(3)}..${max2.toFixed(3)}`);
  ok(min3 >= -1.02 && max3 <= 1.02, 'noise3 值域 [-1,1]', `${min3.toFixed(3)}..${max3.toFixed(3)}`);
  const d1 = new Noise(1), d2 = new Noise(2);
  let diff = false;
  for (let i = 0; i < 200; i++) if (d1.noise2(i * 0.7, i * 0.3) !== d2.noise2(i * 0.7, i * 0.3)) diff = true;
  ok(diff, '不同种子产生不同地形');

  let fb = 0; const nf = new Noise(7);
  for (let i = 0; i < 800; i++) fb = Math.max(fb, Math.abs(nf.fbm2(i * 0.3, i * 0.11, 4)));
  ok(fb <= 1.001, 'fbm2 值域 [-1,1]', fb.toFixed(3));

  const rng = makeRng(42);
  let rmin = 1, rmax = 0;
  for (let i = 0; i < 5000; i++) { const r = rng(); rmin = Math.min(rmin, r); rmax = Math.max(rmax, r); }
  ok(rmin >= 0 && rmax < 1, 'rng 值域 [0,1)');
  const h1 = hash2i(123, -456, 99), h2 = hash2i(123, -456, 99), h3 = hash2i(123, -456, 100);
  eq(h1, h2, 'hash2i 确定性');
  ok(h1 !== h3, 'hash2i 区分种子');
  ok(hash3i(1, 2, 3, 4) !== hash3i(1, 2, 3, 5), 'hash3i 区分种子');
}

// ────────────────────────────────────────────── 方块表
section('方块表');
{
  let bad = [];
  for (let i = 0; i < BLOCK_COUNT; i++) {
    const d = BLOCKS[i];
    if (!d) { bad.push(`id ${i} 未定义`); continue; }
    if (!d.name || d.name === '?') bad.push(`id ${i} 缺名称`);
    if (!d.tiles || d.tiles.length !== 3) bad.push(`id ${i} 贴图槽位不完整`);
    else for (const t of d.tiles) if (!(t >= 0 && t < TILE_ORDER.length)) bad.push(`id ${i} 槽位越界 ${t}`);
    if (typeof d.hardness !== 'number') bad.push(`id ${i} 缺硬度`);
  }
  ok(bad.length === 0, '方块定义自检', bad.join('; '));
  ok(TILE_ORDER.length <= 256, '贴图数不超 16×16 图集容量', `${TILE_ORDER.length} 张`);
  ok(B.air === 0 && BLOCKS[B.air].solid === false, '空气不参与碰撞');
  ok(BLOCKS[B.bedrock].hardness < 0, '基岩不可破坏');
  eq(BLOCKS[B.stone].drop, B.cobblestone, '石头掉落圆石');
  eq(BLOCKS[B.grass_block].drop, B.dirt, '草方块掉落泥土');
  eq(BLOCKS[B.water].drop, B.air, '水不掉落');
  ok(faceVisible(B.stone, B.air), '石头对着空气可见');
  ok(!faceVisible(B.stone, B.dirt), '石头对着泥土不可见');
  ok(!faceVisible(B.water, B.water), '水与水之间不可见');
  ok(faceVisible(B.water, B.air), '水对着空气可见');
  ok(faceVisible(B.glass, B.stone) === false, '玻璃面对石头被遮挡');
  ok(faceVisible(B.stone, B.glass), '石头面对玻璃可见');
  ok(IS_OPAQUE[B.oak_leaves] === 0 && IS_OPAQUE[B.ice] === 0, '树叶/冰不算不透明');
}

// ────────────────────────────────────────────── 地形生成
section('地形生成');
const gen = new ChunkGenerator(SEED);
{
  // 单列高度自检
  let hmin = 999, hmax = -999;
  for (let i = 0; i < 3000; i++) {
    const c = gen.column(i * 7 % 900 - 450, Math.floor(i / 3) * 7 % 900 - 450);
    hmin = Math.min(hmin, c.height); hmax = Math.max(hmax, c.height);
    if (!(c.height >= 3 && c.height <= WORLD_H - 12)) { ok(false, '列高越界', c.height); break; }
    if (!(c.biome >= 0 && c.biome < BIOMES.length)) { ok(false, '群系 id 越界'); break; }
  }
  ok(hmin >= 3 && hmax <= WORLD_H - 12, '列高范围合法', `${hmin}..${hmax}`);
  ok(hmax - hmin > 30, '地形有足够起伏', `落差 ${hmax - hmin}`);

  // 单区块结构
  const d = gen.generate(0, 0);
  eq(d.blocks.length, CHUNK_AREA * WORLD_H, '区块体积素数正确');
  eq(d.biomes.length, CHUNK_AREA, '群系数组长度正确');

  let bedrockOk = true, idOk = true, belowSea = 0, ores = 0, caveAir = 0, trees = 0;
  const counts = new Map();
  for (let i = 0; i < d.blocks.length; i++) {
    const id = d.blocks[i];
    if (id >= BLOCK_COUNT) idOk = false;
    counts.set(id, (counts.get(id) || 0) + 1);
    const y = i >> 8;
    if (y <= 0 && id !== B.bedrock) bedrockOk = false;
    if (id >= B.coal_ore && id <= B.diamond_ore) ores++;
    if (id === B.oak_log || id === B.birch_log || id === B.spruce_log) trees++;
  }
  // 洞：地表以下出现空气的比例
  for (let z = 0; z < CHUNK_Z; z++) for (let x = 0; x < CHUNK_X; x++) {
    const h = d.heights[COL(x, z)];
    for (let y = 6; y < h - 4; y++) if (d.blocks[IDX(x, y, z)] === B.air) caveAir++;
  }
  ok(bedrockOk, 'y=0 必为基岩');
  ok(idOk, '不存在越界方块 id');
  ok(ores > 0, '区块内生成了矿石', `${ores} 个`);
  ok(caveAir > 200, '地表以下有洞穴空腔', `${caveAir} 格空气`);

  // (0,0) 具体是什么群系随种子而变，所以横向看多个区块，要求整体有草有树
  let anyGrass = false, anyLog = false, anySand = false;
  for (let cx = -2; cx <= 2 && !(anyGrass && anyLog && anySand); cx++) {
    for (let cz = -2; cz <= 2; cz++) {
      const dd = gen.generate(cx, cz);
      for (const id of dd.blocks) {
        if (id === B.grass_block) anyGrass = true;
        else if (id === B.oak_log || id === B.birch_log || id === B.spruce_log) anyLog = true;
        else if (id === B.sand) anySand = true;
      }
    }
  }
  ok(anyGrass, '有多处草方块');
  ok(anyLog, '有多处树木');
  ok(anySand, '有多处沙地');

  // 跨区块一致性：同一世界列，在本区块和邻区块里算出的地形必须一致。
  // 树木会长高 heights，所以只有「高度和群系同时对不上」才算不一致。
  const d2 = gen.generate(1, 0);
  let mismatch = 0;
  for (let z = 0; z < CHUNK_Z; z++) {
    for (let x = 0; x < CHUNK_X; x++) {
      const c = gen.column(16 + x, z);          // 区块(1,0) 的局部 (0..15) = 世界 x 16..31
      if (d2.heights[COL(x, z)] !== c.height && d2.biomes[COL(x, z)] !== c.biome) mismatch++;
    }
  }
  eq(mismatch, 0, '跨区块地形一致');

  // 群系多样性。注意采样范围必须显著大于一个群系的尺度：气候噪声的波长约 900 格，
  // 只看 200×200 格的话结果基本等同于掷骰子，会误判成「没有森林」。
  const seen = new Set();
  const seenLand = new Map();
  for (let i = 0; i < 120000; i++) {
    const x = ((i * 0.754877) % 4000) | 0 - 2000;
    const z = ((i * 0.56984) % 4000) | 0 - 2000;
    const c = gen.column(x, z);
    seen.add(c.biome);
    if (c.height > SEA_LEVEL) seenLand.set(c.biome, (seenLand.get(c.biome) || 0) + 1);
  }
  ok(seen.size >= 8, '群系多样性', `${seen.size} 种: ${[...seen].map(biomeName).join('/')}`);
  ok(seen.has(BIO.ocean) || seen.has(BIO.deep_ocean), '存在海洋类群系');
  let landTotal = 0; for (const v of seenLand.values()) landTotal += v;
  const treeShare = ['forest', 'birch_forest', 'taiga', 'jungle']
    .reduce((s, n) => s + (seenLand.get(BIO[n]) || 0), 0);
  ok(treeShare / landTotal > 0.15, '陆地上有 15% 以上是成片森林',
    `${(treeShare / landTotal * 100).toFixed(1)}%`);
  for (const n of ['plains', 'desert', 'savanna', 'snowy_plains', 'taiga', 'jungle', 'birch_forest']) {
    const share = (seenLand.get(BIO[n]) || 0) / landTotal;
    ok(share > 0.005, `陆地群系「${biomeName(BIO[n])}」占比 > 0.5%`, `${(share * 100).toFixed(2)}%`);
  }
}

// ────────────────────────────────────────────── 世界 + 光照
section('世界与光照');
const world = new World(SEED);
const light = new LightEngine(world);
{
  for (let cx = -1; cx <= 1; cx++) for (let cz = -1; cz <= 1; cz++) {
    const c = world.ensureChunk(cx, cz);
    light.initChunk(c);
  }
  let t0 = Date.now();
  light.propagate(5_000_000);
  const dt = Date.now() - t0;
  ok(light.pending === 0, '光照队列已排空', `剩 ${light.pending}`);

  const c = world.getChunk(0, 0);
  // 开阔天空 = 15
  let skyOk = true, worst = '';
  for (let i = 0; i < CHUNK_AREA; i++) {
    const x = i & 15, z = i >> 4;
    const h = c.heights[COL(i & 15, i >> 4)];
    if (world.getSky(x, WORLD_H - 1, z) !== 15) { skyOk = false; worst = `(${x},${WORLD_H - 1},${z})`; break; }
    if (h < WORLD_H - 2 && world.getSky(x, h + 1, z) !== 15) { skyOk = false; worst = `(${x},${h + 1},${z})`; break; }
  }
  ok(skyOk, '地表上方天光为 15', worst);

  // 洞穴内部应当是黑的
  let dark = 0, checked = 0;
  for (let z = 0; z < CHUNK_Z; z += 2) for (let x = 0; x < CHUNK_X; x += 2) {
    const h = c.heights[COL(x, z)];
    for (let y = 8; y < h - 8; y++) {
      if (world.getBlock(x, y, z) !== B.air) continue;
      checked++;
      if (world.getSky(x, y, z) === 0) dark++;
    }
  }
  if (checked > 20) {
    // 洞口附近的洞穴会漏进天光，比例不能要求 100%
    ok(dark / checked > 0.7, '洞穴内部大部分是黑的', `${dark}/${checked}`);
  } else {
    ok(true, '洞穴内部天光检查（该区块洞穴太少，跳过）');
  }

  // 方块光。必须挑一个远离岩浆的洞穴格，否则测到的是岩浆的光而不是萤石的。
  let litX = -1, litY = -1, litZ = -1;
  outer:
  for (let z = 1; z < CHUNK_Z - 1; z++) for (let x = 1; x < CHUNK_X - 1; x++) {
    const h = c.heights[COL(x, z)];
    for (let y = 6; y < h - 6; y++) {
      if (c.blocks[IDX(x, y, z)] !== B.air) continue;
      // 3×3×3 邻域内不能有岩浆
      let clean = true;
      for (let dy = -1; dy <= 1 && clean; dy++) for (let dz = -1; dz <= 1 && clean; dz++)
        for (let dx = -1; dx <= 1 && clean; dx++) {
          const n = world.getBlock(x + dx, y + dy, z + dz);
          if (n === B.lava || LIGHT_EMIT_TEST(n)) clean = false;
        }
      if (clean && world.getBlockLight(x, y, z) === 0) { litX = x; litY = y; litZ = z; break outer; }
    }
  }
  if (litX >= 0) {
    world.setBlock(litX, litY, litZ, B.glowstone);
    light.onBlockChanged(litX, litY, litZ, B.air, B.glowstone);
    light.propagate(5_000_000);
    eq(world.getBlockLight(litX, litY, litZ), 15, '萤石自身方块光 15');
    // 找一个能透光的邻格：洞穴里贴着萤石的一侧可能是石头，光根本传不过去
    let litNb = null;
    for (const [dx, dy, dz] of [[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1], [0, 0, -1]]) {
      if (LIGHT_FILTER_TEST[world.getBlock(litX + dx, litY + dy, litZ + dz)] < 15) {
        litNb = [dx, dy, dz]; break;
      }
    }
    if (ok(!!litNb, '萤石旁有可透光的邻格')) {
      eq(world.getBlockLight(litX + litNb[0], litY + litNb[1], litZ + litNb[2]), 14,
        '萤石向邻格传播一级');
    }
    // 挖掉它。注意：洞里可能还有远处的岩浆在发光，所以这里只断言不再是 15。
    // 「移除后精确归零」交给下面那个封闭空间的确定性用例。
    world.setBlock(litX, litY, litZ, B.air);
    light.onBlockChanged(litX, litY, litZ, B.glowstone, B.air);
    light.propagate(5_000_000);
    ok(world.getBlockLight(litX, litY, litZ) < 15, '移除萤石后本格不再是满光',
      `${world.getBlockLight(litX, litY, litZ)}`);
  } else {
    ok(false, '找不到可放萤石的洞穴格');
  }

  // 封闭空间里的确定性用例：3×3×3 全空气的石头盒子，附近无任何其它光源
  {
    const bx = 8, by = 30, bz = 8;
    for (let x = bx - 2; x <= bx + 2; x++)
      for (let y = by - 2; y <= by + 2; y++)
        for (let z = bz - 2; z <= bz + 2; z++)
          world.setBlock(x, y, z, Math.abs(x - bx) === 2 || Math.abs(y - by) === 2 || Math.abs(z - bz) === 2 ? B.stone : B.air, false);
    world.setBlock(bx, by, bz, B.air, false);
    light.propagate(5_000_000);
    eq(world.getBlockLight(bx, by, bz), 0, '封闭石头盒内初始无方块光');
    world.setBlock(bx, by, bz, B.glowstone);
    light.onBlockChanged(bx, by, bz, B.air, B.glowstone);
    light.propagate(5_000_000);
    eq(world.getBlockLight(bx, by, bz), 15, '封闭盒内萤石光 15');
    eq(world.getBlockLight(bx + 1, by, bz), 14, '封闭盒内向邻格衰减一级');
    eq(world.getBlockLight(bx + 2, by, bz), 0, '封闭盒内光不穿墙');
    world.setBlock(bx, by, bz, B.air);
    light.onBlockChanged(bx, by, bz, B.glowstone, B.air);
    light.propagate(5_000_000);
    eq(world.getBlockLight(bx, by, bz), 0, '封闭盒内移除萤石后归零');
    eq(world.getBlockLight(bx + 1, by, bz), 0, '封闭盒内邻格一并归零');
  }

  // 遮挡天光：在一个全亮的地面格上方盖一块石头
  let skyX = 0, skyZ = 0, skyY = 0;
  for (let z = 0; z < CHUNK_Z && !skyY; z++) for (let x = 0; x < CHUNK_X; x++) {
    const h = c.heights[COL(x, z)];
    if (h < SEA_LEVEL + 4) continue;
    if (world.getSky(x, h + 1, z) === 15) { skyX = x; skyZ = z; skyY = h + 1; break; }
  }
  if (skyY) {
    world.setBlock(skyX, skyY, skyZ, B.stone);
    light.onBlockChanged(skyX, skyY, skyZ, B.air, B.stone);
    light.propagate(5_000_000);
    eq(world.getSky(skyX, skyY, skyZ), 0, '被石头占据的格子天光为 0');
    ok(world.getSky(skyX, skyY - 1, skyZ) < 15, '石头下方天光被遮暗');
    // 拆掉后应恢复
    world.setBlock(skyX, skyY, skyZ, B.air);
    light.onBlockChanged(skyX, skyY, skyZ, B.stone, B.air);
    light.propagate(5_000_000);
    ok(world.getSky(skyX, skyY, skyZ) > 10, '拆掉石头后天光恢复', `${world.getSky(skyX, skyY, skyZ)}`);
  } else {
    ok(false, '找不到可用于遮光测试的地面格');
  }
  console.log(`   （9 区块光照计算耗时 ${dt}ms）`);
}

// ────────────────────────────────────────────── 网格化
section('网格化');
{
  const mesher = new Mesher();
  const c = world.getChunk(0, 0);
  const t0 = Date.now();
  const r = mesher.mesh(world, c);
  const dt = Date.now() - t0;

  let tris = 0, verts = 0;
  const all = [r.opaque, r.cutout, r.liquid];
  let nan = 0, oob = 0, badIdx = 0;
  for (const b of all) {
    const n = b.pos.length / 3;
    verts += n; tris += b.idx.length / 3;
    for (let i = 0; i < b.idx.length; i++) if (b.idx[i] >= n) badIdx++;
    for (let i = 0; i < b.pos.length; i++) if (!Number.isFinite(b.pos[i])) nan++;
    for (let i = 0; i < b.pos.length; i += 3) {
      const x = b.pos[i], y = b.pos[i + 1], z = b.pos[i + 2];
      // 允许正好落在区块边界上
      if (x < -1e-4 || x > CHUNK_X + 1e-4 || z < -1e-4 || z > CHUNK_Z + 1e-4 ||
          y < -1e-4 || y > WORLD_H + 1e-4) oob++;
    }
  }
  ok(nan === 0, '顶点坐标无 NaN', `${nan} 个`);
  ok(badIdx === 0, '索引不越界', `${badIdx} 个`);
  ok(oob === 0, '顶点位于区块范围内', `${oob} 个越界`);
  ok(tris > 0, '生成了三角形', `${tris} 三角形 / ${verts} 顶点`);

  // UV 的平铺值应该出现大于 1 的情况（说明贪心合并真的生效了）
  let merged = 0;
  for (const b of all) for (const u of b.uv) if (u > 1.001) merged++;
  ok(merged > 0, '存在被合并的四边形（UV>1）', `${merged} 个分量`);

  // 灯光取值范围
  let lightBad = 0, shadeBad = 0;
  for (const b of all) {
    for (const s of b.sky) if (s < 0 || s > 1) lightBad++;
    for (const s of b.blk) if (s < 0 || s > 1) lightBad++;
    for (const s of b.shade) if (s < 0 || s > 1.05) shadeBad++;
  }
  ok(lightBad === 0, '光照分量在 [0,1]', `${lightBad} 个越界`);
  ok(shadeBad === 0, '面着色在合理范围', `${shadeBad} 个越界`);

  // 逐面渲染的话三角形数应该是多少？给个数量级参考
  let solidCells = 0;
  for (let i = 0; i < c.blocks.length; i++) {
    const id = c.blocks[i];
    if (id !== B.air && IS_OPAQUE[id]) solidCells++;
  }
  console.log(`   （网格化耗时 ${dt}ms；实心格 ${solidCells}，合并后 ${tris} 三角形）`);
  ok(tris < solidCells * 12, '贪心合并确实减少了三角形', `${tris} < ${solidCells * 12}`);

  // 几何可正常构建成 typed array
  for (const b of all) {
    if (b.empty) continue;
    const g = b.toGeometry();
    ok(g.position.length === g.uv.length * 1.5, 'position 与 uv 顶点数一致');
    ok(g.index.length % 3 === 0, '索引数是 3 的倍数');
  }

  // ---- 覆盖面完整性：在合成世界上，��个「本该可见」的面必须被恰好一个四边形覆盖 ----
  // 这条断言是网格化器最关键的不变量。之前 -X / +Z 两个面因为 u 向量指向坐标轴负方向，
  // 合并出来的四边形整体错位 (w-1) 格，肉眼在复杂地形上很难发现，这里能直接抓住。
  const sw = new World(SEED);
  const sl = new LightEngine(sw);
  for (let cx = -1; cx <= 1; cx++) for (let cz = -1; cz <= 1; cz++) sl.initChunk(sw.ensureChunk(cx, cz));
  sl.propagate(1e7);
  const sc = sw.getChunk(0, 0);
  const DIRS = [[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1], [0, 0, -1]];
  // 造一个既有大块平地（逼出合并）又有高低差和孔洞（逼出分裂）的场景
  for (let x = 0; x < 16; x++) for (let z = 0; z < 16; z++) {
    const base = 20 + ((x < 8 && z < 8) ? 0 : (x < 8 ? 1 : (z < 8 ? 2 : 3)));
    for (let y = 0; y <= base; y++) sw.setBlock(x, y, z, y === base ? B.grass_block : B.stone, false);
  }
  for (let x = 3; x < 7; x++) for (let z = 9; z < 14; z++) for (let y = 0; y < 5; y++) sw.setBlock(x, 20 + y, z, B.air, false);
  for (let x = 10; x < 13; x++) for (let z = 2; z < 6; z++) for (let y = 0; y < 4; y++) sw.setBlock(x, 23 + y, z, B.stone, false);
  sl.propagate(1e7);
  const sr = new Mesher().mesh(sw, sc);

  // 期望：每个面的覆盖次数
  const expect = new Map();
  for (let x = 0; x < 16; x++) for (let z = 0; z < 16; z++) for (let y = 0; y < WORLD_H; y++) {
    const id = sc.blocks[IDX(x, y, z)];
    if (id === B.air) continue;
    if (RENDER_KIND_TEST(id) === 1) continue;      // 十字植物不参与覆盖面统计
    for (let d = 0; d < 6; d++) {
      const n = DIRS[d];
      const nx = x + n[0], ny = y + n[1], nz = z + n[2];
      if (!sw.chunkAt(nx, nz)) continue;
      if (!faceVisible(id, sw.getBlock(nx, ny, nz))) continue;
      expect.set(`${x},${y},${z},${d}`, 0);
    }
  }
  let covered = 0, crossQuads = 0;
  for (const b of [sr.opaque, sr.cutout, sr.liquid]) {
    for (let i = 0; i < b.idx.length; i += 6) {
      const base = b.idx[i];
      const cx = [b.pos[base * 3], b.pos[base * 3 + 1], b.pos[base * 3 + 2]];
      const ax = [b.pos[(base + 1) * 3] - cx[0], b.pos[(base + 1) * 3 + 1] - cx[1], b.pos[(base + 1) * 3 + 2] - cx[2]];
      const bx = [b.pos[(base + 3) * 3] - cx[0], b.pos[(base + 3) * 3 + 1] - cx[1], b.pos[(base + 3) * 3 + 2] - cx[2]];
      // 法线 = u × v。合并后的四边形法线长度是 w×h，所以不比较长度，
      // 只要求「恰好一个分量为 ±长度、其余两个为 0」，即轴对齐。
      let nrm = [ax[1] * bx[2] - ax[2] * bx[1], ax[2] * bx[0] - ax[0] * bx[2], ax[0] * bx[1] - ax[1] * bx[0]];
      const nl = Math.abs(nrm[0]) + Math.abs(nrm[1]) + Math.abs(nrm[2]);
      const axisAligned = nl > 1e-9 &&
        [0, 1, 2].filter(i => Math.abs(Math.abs(nrm[i]) - nl) < 1e-6).length === 1;
      if (!axisAligned) { crossQuads++; continue; }   // 十字植物是斜面，不参与覆盖面统计
      const a = nrm[0] !== 0 ? 0 : (nrm[1] !== 0 ? 1 : 2);
      const sgn = nrm[a] > 0 ? 1 : -1;
      const d = DIRS.findIndex(v => v[a] * sgn === 1 && v[a] !== 0);
      const plane = cx[a];
      // 另外两轴上的覆盖范围
      const o1 = (a + 1) % 3, o2 = (a + 2) % 3;
      const lo1 = Math.min(cx[o1], cx[o1] + ax[o1], cx[o1] + bx[o1]);
      const hi1 = Math.max(cx[o1], cx[o1] + ax[o1], cx[o1] + bx[o1]);
      const lo2 = Math.min(cx[o2], cx[o2] + ax[o2], cx[o2] + bx[o2]);
      const hi2 = Math.max(cx[o2], cx[o2] + ax[o2], cx[o2] + bx[o2]);
      for (let i1 = Math.round(lo1); i1 < Math.round(hi1); i1++) {
        for (let i2 = Math.round(lo2); i2 < Math.round(hi2); i2++) {
          const p = [0, 0, 0];
          p[a] = sgn > 0 ? plane - 1 : plane;
          p[o1] = i1; p[o2] = i2;
          const k = `${p[0]},${p[1]},${p[2]},${d}`;
          if (expect.has(k)) expect.set(k, expect.get(k) + 1);
          else ok(false, '四边形覆盖了本该不可见的面', k);
          covered++;
        }
      }
    }
  }
  const missing = [...expect].filter(([, v]) => v === 0);
  const dup = [...expect].filter(([, v]) => v > 1);
  ok(missing.length === 0, '没有漏掉的面', missing.slice(0, 3).map(m => m[0]).join(' '));
  ok(dup.length === 0, '没有重复覆盖的面', dup.slice(0, 3).map(m => m[0] + '×' + m[1]).join(' '));
  console.log(`   （覆盖面：期望 ${expect.size} 个面，实际覆盖 ${covered} 次，另有 ${crossQuads} 个十字植物斜面）`);
}

// ────────────────────────────────────────────── 射线拾取
section('射线拾取');
{
  const c = world.getChunk(0, 0);
  let found = null;
  for (let z = 0; z < CHUNK_Z && !found; z++) for (let x = 0; x < CHUNK_X; x++) {
    const h = c.heights[COL(x, z)];
    if (h > SEA_LEVEL) { found = { x, z, h }; break; }
  }
  if (ok(!!found, '找到可用于射线测试的地面')) {
    const o = { x: found.x + 0.5, y: found.h + 8, z: found.z + 0.5 };
    const dir = { x: 0, y: -1, z: 0 };
    const hit = raycast(world, o, dir, 20, isTargetable);
    ok(!!hit, '向下射线命中方块');
    if (hit) {
      eq(hit.y, found.h, '命中的是地表那一格');
      eq(hit.ny, 1, '命中面的法线朝上');
      // 从 h+8 出发，走到 y=h+1 的格子边界正好是 7 格
      ok(Math.abs(hit.dist - 7) < 0.01, '命中距离正确', hit.dist.toFixed(3));
    }
    // 斜向射线
    const d2 = { x: 0.6, y: -0.6, z: 0.52 };
    const l = Math.hypot(d2.x, d2.y, d2.z);
    d2.x /= l; d2.y /= l; d2.z /= l;
    const h2 = raycast(world, o, d2, 20, isTargetable);
    ok(!!h2, '斜向射线也能命中');
    // 超远距离不应命中
    ok(raycast(world, o, { x: 0, y: 1, z: 0 }, 3, isTargetable) === null, '射程外不命中');
  }
}

// ────────────────────────────────────────────── 玩家物理
section('玩家物理');
{
  const c = world.getChunk(0, 0);
  let spot = null;
  for (let z = 1; z < CHUNK_Z - 1 && !spot; z++) for (let x = 1; x < CHUNK_X - 1; x++) {
    const h = c.heights[COL(x, z)];
    if (h > SEA_LEVEL + 2 && world.getBlock(x, h + 1, z) === B.air && world.getBlock(x, h + 2, z) === B.air) {
      spot = { x: x + 0.5, z: z + 0.5, h }; break;
    }
  }
  if (ok(!!spot, '找到可站立的地面')) {
    const p = new Player(world, spot.x, spot.h + 6, spot.z);
    const NO = { forward: false, back: false, left: false, right: false, jump: false, sneak: false, sprint: false, up: false, down: false };
    for (let i = 0; i < 400; i++) p.update(1 / 60, NO);
    ok(p.onGround, '自由落体后着地');
    ok(Math.abs(p.y - (spot.h + 1)) < 0.15, '停在地面之上', `y=${p.y.toFixed(3)} 期望≈${spot.h + 1}`);

    // 走路：撞墙时不能穿过去
    const p2 = new Player(world, spot.x, spot.h + 1, spot.z);
    p2.yaw = 0;
    for (let i = 0; i < 240; i++) p2.update(1 / 60, { ...NO, forward: true });
    ok(Math.hypot(p2.x - spot.x, p2.z - spot.z) > 0.5, '能走动');
    ok(!p2.intersectsSolid(p2.x, p2.y, p2.z), '走动后身体不卡在方块里');

    // 跳起来
    const p3 = new Player(world, spot.x, spot.h + 1, spot.z);
    let peak = p3.y;
    for (let i = 0; i < 200; i++) {
      p3.update(1 / 60, { ...NO, jump: i < 40 });
      peak = Math.max(peak, p3.y);
    }
    ok(peak > spot.h + 1.8, '跳跃能达到一格以上', `峰值 +${(peak - spot.h - 1).toFixed(2)}`);

    // 飞行不落地
    const p4 = new Player(world, spot.x, spot.h + 3, spot.z);
    p4.flying = true;
    for (let i = 0; i < 120; i++) p4.update(1 / 60, NO);
    ok(p4.y > spot.h + 2, '飞行模式不会掉落', `y=${p4.y.toFixed(2)}`);

    // 台阶：造一格高的墙，应该能自动跨上去
    const wx = Math.floor(spot.x), wz = Math.floor(spot.z);
    for (let dy = 1; dy <= 1; dy++) world.setBlock(wx + 1, spot.h + dy, wz, B.stone, false);
    const p5 = new Player(world, spot.x, spot.h + 1, spot.z);
    p5.yaw = -Math.PI / 2;   // 朝 +X
    for (let i = 0; i < 180; i++) p5.update(1 / 60, { ...NO, forward: true });
    ok(p5.x > spot.x + 0.6, '自动上了 1 格台阶', `x=${p5.x.toFixed(2)}`);
    world.setBlock(wx + 1, spot.h + 1, wz, B.air, false);
  }
}

// ────────────────────────────────────────────── 存档
section('存档往返');
{
  const w2 = new World(SEED);
  w2.ensureChunk(0, 0);
  w2.setBlock(3, 60, 5, B.stone);
  w2.setBlock(3, 61, 5, B.glass);
  w2.setBlock(3, 60, 5, B.air);
  const json = w2.serializeEdits();
  const w3 = new World(SEED);
  const loaded = w3.loadEdits(json);
  ok(loaded, '编辑数据可反序列化');
  w3.ensureChunk(0, 0);
  eq(w3.getBlock(3, 60, 5), B.air, '破坏记录已还原');
  eq(w3.getBlock(3, 61, 5), B.glass, '放置记录已还原');
  ok(!w3.loadEdits('{坏掉的 json'), '坏数据不会抛异常');
}

// ────────────────────────────────────────────── 汇总
console.log('\n' + '─'.repeat(52));
if (fail === 0) {
  console.log(`\x1b[32m全部通过：${pass} 项断言\x1b[0m`);
} else {
  console.log(`\x1b[31m${fail} 项失败\x1b[0m / 通过 ${pass} 项\n`);
  for (const f of failures) console.log('  \x1b[31m✗\x1b[0m ' + f);
}
console.log('─'.repeat(52));
process.exit(fail === 0 ? 0 : 1);
