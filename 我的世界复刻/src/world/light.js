// 光照引擎：天光（skylight）+ 方块光（blocklight）两条独立通道，各自 BFS 泛洪。
//
// 设计要点：
//  · 区块生成后做一次「天光柱状填充」——从顶往下逐格减光，这是零成本的且完全正确的
//    竖直方向解；随后的 BFS 只负责水平方向（以及跨区块边界）。
//  · 编辑方块时走经典的「加光 / 减光」双算法：减光时把比本格暗的邻居一起清零，
//    比本格亮的邻居重新入队，靠它们把光补回来。
//  · 队列用整型打包坐标，避免 BFS 里产生对象垃圾。

import { WORLD_H, CHUNK_X, CHUNK_Z, IDX } from '../core/constants.js';
import { LIGHT_EMIT, LIGHT_FILTER } from '../core/blocks.js';

// 坐标打包：x(12位) y(7位) z(12位)，共 31 位。可用范围 ±2048 格。
const OFF = 2048;
const pack = (x, y, z) => (((x + OFF) << 19) | (y << 12) | (z + OFF));
const ux = (v) => (v >> 19) - OFF;
const uy = (v) => (v >> 12) & 127;
const uz = (v) => (v & 4095) - OFF;

const DIRS = [
  1, 0, 0, -1, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 1, 0, 0, -1,
];

export class LightEngine {
  constructor(world) {
    this.world = world;
    this.skyQ = [];
    this.blockQ = [];
    this.stats = { propagated: 0, lastOps: 0 };
  }

  get pending() { return this.skyQ.length + this.blockQ.length; }

  /**
   * 新区块的地形生成完毕后调用。
   * 只处理本区块内部的格子；跨区块的连通靠「双方都会把自己边界格入队」来收敛。
   */
  initChunk(chunk) {
    const { blocks, light } = chunk;
    const ox = chunk.cx * CHUNK_X, oz = chunk.cz * CHUNK_Z;
    const w = this.world;

    // --- 1. 天光柱状填充 ---
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        const base = IDX(lx, 0, lz);
        let L = 15;
        for (let y = WORLD_H - 1; y >= 0; y--) {
          const i = base + (y << 8);
          const f = LIGHT_FILTER[blocks[i]];
          if (f >= 15) L = 0;
          else if (f > 0) L = Math.max(0, L - f);
          light[i] = (L << 4) | (light[i] & 0x0f);
        }
      }
    }

    // --- 2. 水平扩散种子 ---
    // 高度上限取本区块最高实体再往上几格：再往上的天空四面都是 15，无需传播。
    const yTop = Math.min(WORLD_H - 1, chunk.maxY + 3);
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        const wx = ox + lx, wz = oz + lz;
        const base = IDX(lx, 0, lz);
        for (let y = 1; y <= yTop; y++) {
          const L = light[base + (y << 8)] >> 4;
          if (L <= 1) continue;
          if (w.getSky(wx + 1, y, wz) < L - 1 || w.getSky(wx - 1, y, wz) < L - 1 ||
              w.getSky(wx, y, wz + 1) < L - 1 || w.getSky(wx, y, wz - 1) < L - 1) {
            this.skyQ.push(pack(wx, y, wz));
          }
        }
      }
    }

    // --- 3. 边界格强制入队，让光能流进邻接区块 ---
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        if (lx !== 0 && lx !== 15 && lz !== 0 && lz !== 15) continue;
        const wx = ox + lx, wz = oz + lz;
        const base = IDX(lx, 0, lz);
        for (let y = 1; y <= yTop; y++) {
          if ((light[base + (y << 8)] >> 4) > 1) this.skyQ.push(pack(wx, y, wz));
        }
      }
    }

    // --- 4. 方块光源 ---
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        const wx = ox + lx, wz = oz + lz;
        const base = IDX(lx, 0, lz);
        for (let y = 1; y < WORLD_H; y++) {
          const i = base + (y << 8);
          const e = LIGHT_EMIT[blocks[i]];
          if (e > 0) {
            light[i] = (light[i] & 0xf0) | e;
            this.blockQ.push(pack(wx, y, wz));
          }
        }
      }
    }
  }

  // ---------- 传播 ----------

  propagate(budget = 120000) {
    const w = this.world;
    let ops = 0;

    while (ops < budget && this.blockQ.length) {
      const p = this.blockQ.pop();
      const x = ux(p), y = uy(p), z = uz(p);
      const L = w.getBlockLight(x, y, z);
      if (L <= 1) continue;
      ops++;
      for (let d = 0; d < 6; d++) {
        const nx = x + DIRS[d * 3], ny = y + DIRS[d * 3 + 1], nz = z + DIRS[d * 3 + 2];
        if (ny < 0 || ny >= WORLD_H) continue;
        if (!w.chunkAt(nx, nz)) continue;
        if (LIGHT_FILTER[w.getBlock(nx, ny, nz)] >= 15) continue;
        const target = L - 1;
        if (target <= 0) continue;
        if (w.getBlockLight(nx, ny, nz) < target) {
          w.setBlockLight(nx, ny, nz, target);
          this.blockQ.push(pack(nx, ny, nz));
          ops++;
        }
      }
    }

    while (ops < budget && this.skyQ.length) {
      const p = this.skyQ.pop();
      const x = ux(p), y = uy(p), z = uz(p);
      const L = w.getSky(x, y, z);
      if (L <= 1) continue;
      ops++;
      for (let d = 0; d < 6; d++) {
        const nx = x + DIRS[d * 3], ny = y + DIRS[d * 3 + 1], nz = z + DIRS[d * 3 + 2];
        if (ny < 0 || ny >= WORLD_H) continue;
        if (!w.chunkAt(nx, nz)) continue;
        const f = LIGHT_FILTER[w.getBlock(nx, ny, nz)];
        if (f >= 15) continue;
        const target = L - 1 - f;      // 水/树叶横穿时再多吸一格
        if (target <= 0) continue;
        if (w.getSky(nx, ny, nz) < target) {
          w.setSky(nx, ny, nz, target);
          this.skyQ.push(pack(nx, ny, nz));
          ops++;
        }
      }
    }

    this.stats.propagated += ops;
    this.stats.lastOps = ops;
    return ops;
  }

  // ---------- 减光 ----------

  /**
   * 减光的级联清除。
   * 关键在于顺序：必须「先从栈里取出、读到它此刻的亮度、然后才置零」。如果像很多
   * 实现那样先 set(0) 再入栈，主循环第一轮读到的就是 0，`if (L === 0) continue`
   * 会把起点直接跳过，整条清除链就此断掉——表现为「拆掉光源后光还亮着」。
   *
   * @param chan 0=天光 1=方块光
   */
  remove(chan, x, y, z) {
    const w = this.world;
    const get = chan === 0
      ? (a, b, c) => w.getSky(a, b, c)
      : (a, b, c) => w.getBlockLight(a, b, c);
    const set = chan === 0
      ? (a, b, c, v) => w.setSky(a, b, c, v)
      : (a, b, c, v) => w.setBlockLight(a, b, c, v);
    const requeue = chan === 0
      ? (p) => this.skyQ.push(p)
      : (p) => this.blockQ.push(p);

    if (get(x, y, z) === 0) return;
    const stack = [pack(x, y, z)];
    while (stack.length) {
      const p = stack.pop();
      const cx = ux(p), cy = uy(p), cz = uz(p);
      const L = get(cx, cy, cz);
      if (L === 0) continue;
      set(cx, cy, cz, 0);
      for (let d = 0; d < 6; d++) {
        const nx = cx + DIRS[d * 3], ny = cy + DIRS[d * 3 + 1], nz = cz + DIRS[d * 3 + 2];
        if (ny < 0 || ny >= WORLD_H) continue;
        if (!w.chunkAt(nx, nz)) continue;
        const nl = get(nx, ny, nz);
        if (nl !== 0 && nl < L) {
          set(nx, ny, nz, 0);
          stack.push(pack(nx, ny, nz));     // 和我一样暗，继续一起灭
        } else if (nl >= L) {
          requeue(pack(nx, ny, nz));        // 比我亮，它会把光补回来
        }
      }
    }
  }

  removeSky(x, y, z) { this.remove(0, x, y, z); }
  removeBlockLight(x, y, z) { this.remove(1, x, y, z); }

  // ---------- 加光（从邻居借光） ----------

  addSky(x, y, z) {
    const w = this.world;
    const f = LIGHT_FILTER[w.getBlock(x, y, z)];
    let best = 0;
    for (let d = 0; d < 6; d++) {
      const nx = x + DIRS[d * 3], ny = y + DIRS[d * 3 + 1], nz = z + DIRS[d * 3 + 2];
      if (ny < 0 || ny >= WORLD_H) continue;
      if (!w.chunkAt(nx, nz)) continue;
      const nl = w.getSky(nx, ny, nz);
      if (nl <= 1) continue;
      const t = nl - 1 - f;
      if (t > best) best = t;
    }
    w.setSky(x, y, z, best);
    if (best > 1) this.skyQ.push(pack(x, y, z));
  }

  addBlockLight(x, y, z) {
    const w = this.world;
    let best = 0;
    for (let d = 0; d < 6; d++) {
      const nx = x + DIRS[d * 3], ny = y + DIRS[d * 3 + 1], nz = z + DIRS[d * 3 + 2];
      if (ny < 0 || ny >= WORLD_H) continue;
      if (!w.chunkAt(nx, nz)) continue;
      const t = w.getBlockLight(nx, ny, nz) - 1;
      if (t > best) best = t;
    }
    w.setBlockLight(x, y, z, best);
    if (best > 1) this.blockQ.push(pack(x, y, z));
  }

  /**
   * 玩家放置/破坏方块后调用。
   * 注意 oldId：拆掉一个光源时必须走「级联清除」，不能直接走「向邻居借光」——
   * 后者会立刻从被这块光源照亮的邻居那里借回 13、14 级光，表现为拆了灯还亮着。
   */
  onBlockChanged(x, y, z, oldId, newId) {
    const w = this.world;
    const opaque = LIGHT_FILTER[newId] >= 15;
    const wasOpaque = LIGHT_FILTER[oldId] >= 15;

    if (opaque) this.removeSky(x, y, z);
    else this.addSky(x, y, z);

    const emit = LIGHT_EMIT[newId];
    if (emit > 0) {
      w.setBlockLight(x, y, z, emit);
      this.blockQ.push(pack(x, y, z));
    } else if (opaque || LIGHT_EMIT[oldId] > 0) {
      this.removeBlockLight(x, y, z);
    } else {
      this.addBlockLight(x, y, z);
    }
    void wasOpaque;

    // 六邻格复核：本格变暗后它们可能需要重新往里灌光
    for (let d = 0; d < 6; d++) {
      const nx = x + DIRS[d * 3], ny = y + DIRS[d * 3 + 1], nz = z + DIRS[d * 3 + 2];
      if (ny < 0 || ny >= WORLD_H) continue;
      if (!w.chunkAt(nx, nz)) continue;
      if (w.getSky(nx, ny, nz) > 1) this.skyQ.push(pack(nx, ny, nz));
      if (w.getBlockLight(nx, ny, nz) > 1) this.blockQ.push(pack(nx, ny, nz));
    }
  }
}
