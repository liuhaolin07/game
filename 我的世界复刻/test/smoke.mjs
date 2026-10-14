// 端到端冒烟测试：在 Node 里用替身把整个游戏真的跑起来。
// 覆盖的是「接线对不对」——初始化顺序、每帧流程、挖掘/放置的完整链路。
// 跑法：node --import ./test/register.mjs test/smoke.mjs

import { installDom } from './dom-stub.mjs';
import { B, BLOCKS } from '../src/core/blocks.js';
import { CHUNK_VOL } from '../src/core/constants.js';

const env = installDom({ search: '?seed=20240817' });

let pass = 0, fail = 0;
const failures = [];
const ok = (c, label, detail) => {
  if (c) { pass++; return true; }
  fail++; failures.push(label + (detail ? '  →  ' + detail : ''));
  return false;
};
const section = (t) => console.log('\n\x1b[1m── ' + t + ' ──\x1b[0m');

section('启动');
let game;
try {
  await import('../src/main.js');
  ok(true, 'main.js 加载并执行了 start()');
} catch (e) {
  fail++;
  console.log('\n\x1b[31m启动失败：\x1b[0m ' + (e && e.message));
  console.log((e && e.stack ? e.stack : String(e)).split('\n').slice(0, 12).join('\n'));
  console.log('');
  process.exit(1);
}
game = globalThis.__game;
ok(!!game, '全局暴露了 game 实例');
ok(!!game.world && !!game.chunks && !!game.mining && !!game.hud && !!game.inv,
  '各子系统都已构造');

section('跑 90 帧');
let crashed = null;
try {
  env.frames(90);
} catch (e) {
  crashed = e;
}
ok(!crashed, '主循环连续 90 帧无异常', crashed ? (crashed.stack || '').split('\n').slice(0, 3).join(' | ') : '');

if (crashed) { report(); }

section('世界状态');
{
  ok(game.world.chunks.size > 0, '已加载区块', `${game.world.chunks.size} 个`);
  let meshed = 0;
  for (const c of game.world.chunks.values()) if (c.meshes) meshed++;
  ok(meshed > 0, '已生成网格', `${meshed} 个区块有网格`);
  ok(game.player.y > 1, '玩家不在世界底部', `y=${game.player.y.toFixed(2)}`);
  ok(!Number.isNaN(game.player.x + game.player.y + game.player.z), '玩家坐标有效');
  ok(Number.isFinite(game.camera.position.x), '相机位置有效');
}

section('物理：不会卡住也不会掉出世界');
{
  game.paused = false;
  const p = game.player;
  // 先从高处落到地面
  p.y += 6; p.vy = 0;
  env.frames(150);
  ok(p.onGround || p.flying || p.inWater, '落地或处于特殊状态',
    `onGround=${p.onGround} y=${p.y.toFixed(3)}`);
  ok(p.y > 0, '没有掉出世界', `y=${p.y.toFixed(2)}`);
  ok(!p.intersectsSolid(p.x, p.y, p.z), '身体没有卡进方块');

  // 脚底下必须踩着实心方块（不是花草这种没有碰撞面的）
  const under = game.world.getBlock(Math.floor(p.x), Math.floor(p.y - 0.1), Math.floor(p.z));
  ok(under !== B.air && BLOCKS[under].solid, '脚下踩的是实心方块',
    `${BLOCKS[under]?.name} @ ${Math.floor(p.x)},${Math.floor(p.y - 0.1)},${Math.floor(p.z)}`);

  // 站着不动应该稳稳待着，不该有缓慢漂移
  const yA = p.y;
  env.frames(120);
  ok(Math.abs(p.y - yA) < 0.02, '静止时高度稳定', `Δy=${(p.y - yA).toFixed(4)}`);

  // 走动一段
  const x0 = p.x, z0 = p.z;
  game.keys.KeyW = true;
  env.frames(90);
  game.keys.KeyW = false;
  ok(Math.hypot(p.x - x0, p.z - z0) > 1, '能走动', `位移 ${Math.hypot(p.x - x0, p.z - z0).toFixed(2)} 格`);
  ok(!p.intersectsSolid(p.x, p.y, p.z), '走动后身体仍不卡在方块里');
}

section('挖掘链路');
{
  const p = game.player;
  // 站到一片实地上，低头看准星
  const bx = Math.floor(p.x), bz = Math.floor(p.z);
  const by = Math.floor(p.y) - 1;
  const under = game.world.getBlock(bx, by, bz);
  ok(under !== B.air, '脚下方块是实的', `${BLOCKS[under]?.name} @ ${bx},${by},${bz}`);

  p.pitch = -1.2;  // 低头（实测 three 里 rotation.x 为正=抬头，所以低头取负）
  env.frames(3);
  const t = game.mining.target;
  ok(!!t, '准星命中了方块', t ? `${BLOCKS[t.id].name} @ ${t.x},${t.y},${t.z}` : '没命中');
  if (t) {
    // 进度应当随按住时间增长
    game.mouseDown = true;
    env.frames(10);
    const p1 = game.mining.progress;
    ok(p1 > 0, '按住左键后挖掘进度增长', p1.toFixed(3));
    env.frames(10);
    const p2 = game.mining.progress;
    ok(p2 > p1, '进度持续增长', `${p1.toFixed(3)} → ${p2.toFixed(3)}`);
    ok(p2 <= 1, '进度不超过 1');

    // 一直按到碎掉
    env.frames(400);
    ok(game.world.getBlock(t.x, t.y, t.z) === B.air, '方块最终被破坏',
      `${BLOCKS[game.world.getBlock(t.x, t.y, t.z)]?.name}`);
    game.mouseDown = false;
    ok(game.drops.items.length > 0 || game.inv.count(t.id) > 0, '产生了掉落物或直接进包',
      `掉落 ${game.drops.items.length} 个`);
  }
}

section('放置链路');
{
  const p = game.player;
  // 稍微低头看身前几米的地面：目标离玩家够远，放置格不会落在玩家身体里
  // （落在身体里会被「不能把方块放进自己」正确拒绝，那样就测不到放置逻辑了）
  p.pitch = -0.38;   // 低头看身前几米的地面
  let t2 = null;
  for (const yaw of [0, Math.PI / 2, Math.PI, -Math.PI / 2]) {
    p.yaw = yaw;
    env.frames(3);
    t2 = game.mining.target;
    if (t2) break;
  }
  if (ok(!!t2, '取得放置目标')) {
    game.inv.add(B.cobblestone, 8);
    let okSel = false;
    for (let s = 0; s < game.inv.hotbar.length; s++) {
      if (game.inv.hotbar[s] && game.inv.hotbar[s].id === B.cobblestone) { game.inv.select(s); okSel = true; }
    }
    ok(okSel && game.inv.heldBlock() === B.cobblestone, '切到圆石', String(game.inv.heldBlock()));
    const before = game.inv.heldCount;
    // 钩住 place：放下去的格子由放置逻辑自己报出来。
    // 不能在放置后再去读 game.mining.target —— 新放下的方块立刻就成了新的目标，
    // 它的「下一个格」当然是空的。
    let placedAt = null;
    const origPlace = game.mining.place;
    game.mining.place = function () {
      const t = this.target;
      if (t) placedAt = { x: t.x + t.nx, y: t.y + t.ny, z: t.z + t.nz };
      return origPlace.call(this);
    };
    game.placeQueued = true;
    env.frames(1);
    game.mining.place = origPlace;

    ok(!!placedAt, '放置逻辑被调用');
    if (placedAt) {
      const placedId = game.world.getBlock(placedAt.x, placedAt.y, placedAt.z);
      ok(placedId === B.cobblestone, '方块被放到目标格',
        `${BLOCKS[placedId]?.name} @ ${placedAt.x},${placedAt.y},${placedAt.z}`);
      ok(game.inv.heldCount === before - 1, '放置后扣掉一个', `${before} → ${game.inv.heldCount}`);
      ok(Math.hypot(placedAt.x + 0.5 - p.x, placedAt.z + 0.5 - p.z) > 1.0, '放置点确实不在玩家身上');
    }
  }
}

section('光照随编辑更新');
{
  const p = game.player;
  const bx = Math.floor(p.x) + 2, bz = Math.floor(p.z);
  const by = Math.floor(p.y);
  // 在露天格子上方放一块石头，天光应当被遮暗
  let y = WORLDH_SCAN();
  function WORLDH_SCAN() { return by + 3; }
  game.world.setBlock(bx, y, bz, B.air);
  game.light.onBlockChanged(bx, y, bz, BLOCKS[B.stone].id, B.air);
  game.light.propagate(2_000_000);
  const skyBefore = game.world.getSky(bx, y, bz);
  game.world.setBlock(bx, y, bz, B.stone);
  game.light.onBlockChanged(bx, y, bz, B.air, B.stone);
  game.light.propagate(2_000_000);
  const skyAfter = game.world.getSky(bx, y, bz);
  ok(skyBefore > skyAfter, '放石头后天光变暗', `${skyBefore} → ${skyAfter}`);
  ok(game.world.getSky(bx, y, bz) === 0, '被占据的格子天光为 0');
}

section('存档往返');
{
  const p = game.player;
  const bx = Math.floor(p.x) + 1, bz = Math.floor(p.z) + 1;
  const by = Math.floor(p.y) + 4;
  game.world.setBlock(bx, by, bz, B.glowstone);
  game.save();
  // save() 是防抖的，直接同步写一次来验证序列化本身
  const json = game.world.serializeEdits();
  ok(json.length > 2, '编辑数据可序列化', `${json.length} 字节`);
  const invJson = game.inv.serialize();
  ok(JSON.parse(invJson).hotbar.length === 9, '物品栏序列化正确');
}

section('继续跑 120 帧不崩');
{
  let err = null;
  try { env.frames(120); } catch (e) { err = e; }
  ok(!err, '混合操作后主循环仍稳定', err ? (err.stack || '').split('\n').slice(0, 3).join(' | ') : '');
  ok(game.world.chunks.size > 0, '区块仍在内存中', `${game.world.chunks.size}`);
  let tris = 0;
  for (const c of game.world.chunks.values()) {
    if (!c.meshes) continue;
    for (const m of c.meshes) tris += (m.geometry.index?.count || 0) / 3;
  }
  ok(tris > 0, '场景里确实有三角形', `${Math.round(tris)} 个`);
  ok(CHUNK_VOL === 16 * 16 * 128, '区块体积常量一致');
}

report();

function report() {
  console.log('\n' + '─'.repeat(52));
  if (fail === 0) console.log(`\x1b[32m冒烟测试全部通过：${pass} 项\x1b[0m`);
  else {
    console.log(`\x1b[31m${fail} 项失败\x1b[0m / 通过 ${pass} 项\n`);
    for (const f of failures) console.log('  \x1b[31m✗\x1b[0m ' + f);
  }
  console.log('─'.repeat(52));
  process.exit(fail === 0 ? 0 : 1);
}
