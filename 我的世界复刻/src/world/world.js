// 世界容器：区块的增删查改、光照读写、玩家修改的持久化。
// 所有坐标一律使用世界坐标（可能为负），内部换算到区块局部索引。

import {
  CHUNK_X, CHUNK_Z, WORLD_H, CHUNK_AREA, CHUNK_VOL, IDX, COL,
} from '../core/constants.js';
import { B } from '../core/blocks.js';
import { ChunkGenerator } from './generator.js';

export const chunkKey = (cx, cz) => cx + ',' + cz;

export const CHUNK_EMPTY = 0;
export const CHUNK_TERRAIN = 1;   // 地形已生成
export const CHUNK_LIT = 2;        // 初始光照已计算

export class Chunk {
  constructor(cx, cz) {
    this.cx = cx;
    this.cz = cz;
    this.blocks = new Uint8Array(CHUNK_VOL);
    this.light = new Uint8Array(CHUNK_VOL);   // 高 4 位天光，低 4 位方块光
    this.biomes = new Uint8Array(CHUNK_AREA);
    this.heights = new Int16Array(CHUNK_AREA);
    this.maxY = 1;
    this.state = CHUNK_EMPTY;
    this.meshDirty = true;      // 需要重建网格
    this.meshes = null;         // three.js Mesh[] ，由渲染层挂载
  }
  setSky(i, v) { this.light[i] = (this.light[i] & 0x0f) | (v << 4); }
  setBlockLight(i, v) { this.light[i] = (this.light[i] & 0xf0) | v; }
  getSky(i) { return this.light[i] >> 4; }
  getBlockLight(i) { return this.light[i] & 0x0f; }
}

export class World {
  constructor(seed) {
    this.seed = seed;
    this.gen = new ChunkGenerator(seed);
    this.chunks = new Map();
    this.edits = new Map();     // chunkKey -> Map<index, blockId>
    this.listeners = { onEdit: [] };
    this._cache = null;         // chunkAt 的单槽缓存
  }

  getChunk(cx, cz) {
    return this.chunks.get(chunkKey(cx, cz));
  }

  hasChunk(cx, cz) {
    return this.chunks.has(chunkKey(cx, cz));
  }

  /** 取地形数据并套用玩家修改。未生成时返回 null。 */
  ensureChunk(cx, cz) {
    const k = chunkKey(cx, cz);
    let c = this.chunks.get(k);
    if (c) return c;
    const data = this.gen.generate(cx, cz);
    c = new Chunk(cx, cz);
    c.blocks.set(data.blocks);
    c.biomes.set(data.biomes);
    c.heights.set(data.heights);
    c.maxY = data.maxY;
    c.state = CHUNK_TERRAIN;
    const ed = this.edits.get(k);
    if (ed) for (const [i, id] of ed) c.blocks[i] = id;
    this.chunks.set(k, c);
    return c;
  }

  unloadChunk(cx, cz) {
    const k = chunkKey(cx, cz);
    const c = this.chunks.get(k);
    if (c) this.chunks.delete(k);
    return c;
  }

  // ---------- 方块读写 ----------

  getBlock(x, y, z) {
    if (y < 0 || y >= WORLD_H) return B.air;
    const c = this.chunkAt(x, z);
    if (!c) return B.air;
    return c.blocks[IDX(x & 15, y, z & 15)];
  }

  /**
   * 取某世界坐标所在区块，未加载返回 null。
   * 热路径说明：光照 BFS 与网格化会逐体素调用 getBlock/getSky。每次都拼字符串做
   * Map 查找的开销会淹没一切，所以挂一个单槽缓存——这些调用几乎总在同一个区块内
   * 连续发生，命中率极高，且不改变任何语义。
   */
  chunkAt(x, z) {
    const cx = x >> 4, cz = z >> 4;
    const c = this._cache;
    if (c && c.cx === cx && c.cz === cz) return c;
    const n = this.chunks.get(cx + ',' + cz) || null;
    this._cache = n;
    return n;
  }

  /** 未知区块按实心处理，防止玩家在区块边界掉出世界 */
  isSolidAt(x, y, z) {
    if (y < 0) return true;
    if (y >= WORLD_H) return false;
    const c = this.chunkAt(x, z);
    if (!c) return true;
    const id = c.blocks[IDX(x & 15, y, z & 15)];
    return id !== B.air && id !== B.water && id !== B.lava &&
      !(id === B.tall_grass || id === B.dandelion || id === B.poppy ||
        id === B.dead_bush || id === B.sugar_cane);
  }

  /** 光照完全透明（空气/水/植物/玻璃…） */
  isTransparentAt(x, y, z) {
    if (y < 0) return false;
    if (y >= WORLD_H) return true;
    const c = this.chunkAt(x, z);
    if (!c) return true;
    const id = c.blocks[IDX(x & 15, y, z & 15)];
    return id === B.air || id === B.water || id === B.lava;
  }

  setBlock(x, y, z, id, record = true) {
    if (y < 1 || y >= WORLD_H) return false;
    const cx = x >> 4, cz = z >> 4;
    const c = this.chunkAt(x, z);
    if (!c) return false;
    const i = IDX(x & 15, y, z & 15);
    const old = c.blocks[i];
    if (old === id) return false;
    c.blocks[i] = id;
    if (record) {
      const k = cx + ',' + cz;
      let m = this.edits.get(k);
      if (!m) { m = new Map(); this.edits.set(k, m); }
      m.set(i, id);
    }
    c.meshDirty = true;
    for (const fn of this.listeners.onEdit) fn(x, y, z, old, id);
    return true;
  }

  markMeshDirty(x, y, z) {
    const c = this.chunks.get(chunkKey(x >> 4, z >> 4));
    if (c) c.meshDirty = true;
  }

  getBiome(x, z) {
    const c = this.chunkAt(x, z);
    if (!c) return 3;
    return c.biomes[COL(x & 15, z & 15)];
  }

  /** 该列最上面的非水方块高度 */
  getSurfaceY(x, z) {
    const c = this.chunkAt(x, z);
    if (!c) return 0;
    return c.heights[COL(x & 15, z & 15)];
  }

  // ---------- 光照读写 ----------

  getSky(x, y, z) {
    if (y < 0) return 0;
    if (y >= WORLD_H) return 15;
    const c = this.chunkAt(x, z);
    if (!c) return 15;
    return c.getSky(IDX(x & 15, y, z & 15));
  }

  getBlockLight(x, y, z) {
    if (y < 0 || y >= WORLD_H) return 0;
    const c = this.chunkAt(x, z);
    if (!c) return 0;
    return c.getBlockLight(IDX(x & 15, y, z & 15));
  }

  setSky(x, y, z, v) {
    if (y < 0 || y >= WORLD_H) return;
    const c = this.chunkAt(x, z);
    if (!c) return;
    const i = IDX(x & 15, y, z & 15);
    if (c.getSky(i) === v) return;
    c.setSky(i, v);
    c.meshDirty = true;
  }

  setBlockLight(x, y, z, v) {
    if (y < 0 || y >= WORLD_H) return;
    const c = this.chunkAt(x, z);
    if (!c) return;
    const i = IDX(x & 15, y, z & 15);
    if (c.getBlockLight(i) === v) return;
    c.setBlockLight(i, v);
    c.meshDirty = true;
  }

  // ---------- 存档 ----------

  serializeEdits() {
    const out = {};
    for (const [k, m] of this.edits) {
      const o = {};
      for (const [i, id] of m) o[i] = id;
      out[k] = o;
    }
    return JSON.stringify(out);
  }

  loadEdits(json) {
    try {
      const obj = JSON.parse(json);
      for (const k in obj) {
        const m = new Map();
        for (const i in obj[k]) m.set(+i, obj[k][i]);
        this.edits.set(k, m);
      }
      return true;
    } catch { return false; }
  }
}
