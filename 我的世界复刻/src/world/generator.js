// 地形生成：高度场 → 生物群系 → 地层填充 → 洞穴开挖 → 矿脉 → 地表植被。
// 纯计算模块，不碰 three.js / DOM，可以在 Node 下整块跑单测。

import { Noise, hash2i, hash3i, makeRng, lerp, smoothstep, clamp } from '../core/noise.js';
import { B, RENDER_KIND } from '../core/blocks.js';
import {
  CHUNK_X, CHUNK_Z, WORLD_H, CHUNK_AREA, CHUNK_VOL, IDX, COL,
  SEA_LEVEL, LAVA_LEVEL, CAVE_TOP, BEDROCK_TOP,
} from '../core/constants.js';
import { pickBiome, BIOMES, BIO } from './biomes.js';

const FEATURE_PAD = 4;   // 植被向区块外溢的余量（树冠最大半径 3）

// 每区块矿脉配置：数量随区块数线性缩放
const ORE_TABLE = [
  { block: B.coal_ore, veins: 18, minY: 6, maxY: 104, minSize: 5, maxSize: 14 },
  { block: B.iron_ore, veins: 12, minY: 5, maxY: 64, minSize: 4, maxSize: 9 },
  { block: B.gold_ore, veins: 3, minY: 4, maxY: 32, minSize: 3, maxSize: 7 },
  { block: B.redstone_ore, veins: 6, minY: 4, maxY: 24, minSize: 4, maxSize: 9 },
  { block: B.lapis_ore, veins: 2, minY: 4, maxY: 30, minSize: 3, maxSize: 6 },
  { block: B.diamond_ore, veins: 1.4, minY: 3, maxY: 16, minSize: 2, maxSize: 6 },
];

export class ChunkGenerator {
  constructor(seed) {
    this.seed = seed | 0;
    this.nCont = new Noise(seed + 11);
    this.nEros = new Noise(seed + 23);
    this.nTemp = new Noise(seed + 37);
    this.nHumid = new Noise(seed + 53);
    this.nRidge = new Noise(seed + 71);
    this.nDetail = new Noise(seed + 89);
    this.nRiver = new Noise(seed + 101);
    this.nCaveCheese = new Noise(seed + 113);
    this.nCaveWormA = new Noise(seed + 131);
    this.nCaveWormB = new Noise(seed + 149);
  }

  /**
   * 单列的地形信息。纯函数：同样 (wx,wz) 永远得到同样结果，跨区块一致。
   *
   * 关于气候噪声的倍频与增益：fbm 每多叠一个倍频，输出范围就向 0 收缩一截。
   * 用 4 个倍频时温度只能落在 -0.1~0.3，阈值根本分不开群系。这里统一用 2 个倍频
   * （大范围下 p10/p90 约 ±0.43，分布居中），再各自乘一个增益把量程拉回 [-1,1]。
   */
  column(wx, wz) {
    const cont = clamp(this.nCont.fbm2(wx * 0.00085, wz * 0.00085, 2) * 2.4, -1.2, 1.2);
    const erosion = clamp(this.nEros.fbm2(wx * 0.0013 + 31.7, wz * 0.0013 + 31.7, 2) * 2.6, -1.2, 1.2);
    const temp = clamp(this.nTemp.fbm2(wx * 0.0011 + 71.3, wz * 0.0011 + 71.3, 2) * 3.0, -1.2, 1.2);
    const humid = clamp(this.nHumid.fbm2(wx * 0.0012 + 113.9, wz * 0.0012 + 113.9, 2) * 2.4, -1.2, 1.2);
    const ridge = this.nRidge.ridged2(wx * 0.0052, wz * 0.0052, 4);
    const detail = this.nDetail.fbm2(wx * 0.013, wz * 0.013, 3);

    const landMask = smoothstep(-0.15, 0.25, cont);

    // 基准面抬高 2 格并把海洋压浅：cont 分布大致以 0 为中心，直接用
    // 海平面当零点会有将近一半的世界泡在水里。
    let h = SEA_LEVEL + 2 + cont * 15;
    // 侵蚀度越低（地貌越"年轻"）山越高
    const peakiness = Math.min(1.8, Math.max(0, 0.30 - erosion) / 0.65);
    h += Math.pow(ridge, 1.7) * 46 * peakiness * landMask;
    h += detail * 3.6 * landMask;
    if (cont < 0) h -= Math.pow(-cont, 1.5) * 8;   // 海底继续下沉
    h = Math.round(h);

    // 河道：只有在低海拔陆地才下切，避免把山顶劈开
    let isRiver = false;
    if (cont > 0 && h <= SEA_LEVEL + 22) {
      const rv = Math.abs(this.nRiver.noise2(wx * 0.0016 + 500.5, wz * 0.0016 + 500.5));
      if (rv < 0.055) {
        const t = smoothstep(0.055, 0.004, rv);
        h = Math.round(lerp(h, SEA_LEVEL - 1, t));
        isRiver = t > 0.42;
      }
    }
    h = clamp(h, 3, WORLD_H - 12);

    const biome = pickBiome(temp, humid, cont, h, erosion, isRiver);
    return { height: h, biome, temp, humid, cont, erosion, isRiver };
  }

  /** 该列是否应该开洞：双层 3D 噪声。y 需传入用于近地表收敛。 */
  isCave(wx, y, wz, surface) {
    if (y < 2 || y > CAVE_TOP) return false;
    // 空腔型：大块洞穴，靠近地表时收敛以免把地面炸穿
    let t = 0.575 + Math.max(0, y - (surface - 5)) * 0.035;
    const cheese = this.nCaveCheese.fbm3(wx * 0.019, y * 0.033, wz * 0.019, 3);
    if (cheese > t) return true;
    // 蠕虫型：两条噪声同时接近 0 → 蜿蜒长隧道
    if (y < surface - 7) {
      const a = this.nCaveWormA.noise3(wx * 0.0125, y * 0.021, wz * 0.0125);
      const b = this.nCaveWormB.noise3(wx * 0.0125 + 91.3, y * 0.021 + 57.1, wz * 0.0125 + 13.7);
      if (Math.abs(a) < 0.052 && Math.abs(b) < 0.052) return true;
    }
    return false;
  }

  /**
   * 生成一个区块。
   * @returns {{blocks:Uint8Array, biomes:Uint8Array, heights:Uint8Array, maxY:number}}
   */
  generate(cx, cz) {
    const blocks = new Uint8Array(CHUNK_VOL);
    const biomes = new Uint8Array(CHUNK_AREA);
    const ox = cx * CHUNK_X, oz = cz * CHUNK_Z;

    // ---------- 1. 高度 + 群系 + 地层 ----------
    const colH = new Int16Array(CHUNK_AREA);
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        const wx = ox + lx, wz = oz + lz;
        const c = this.column(wx, wz);
        const bio = BIOMES[c.biome];
        const h = c.height;
        colH[COL(lx, lz)] = h;
        biomes[COL(lx, lz)] = c.biome;
        const base = COL(lx, lz);

        const bedrockTop = 1 + Math.floor(hash2i(wx, wz, this.seed + 7) * (BEDROCK_TOP - 1));
        for (let y = 0; y <= h; y++) {
          let id;
          if (y <= bedrockTop) id = B.bedrock;
          else if (y === h) id = bio.top;
          else if (y > h - bio.fillDepth) id = bio.filler;
          else id = B.stone;
          blocks[IDX(lx, y, lz)] = id;
        }
        // 海洋 / 河流补沙底，避免草块沉在海底
        if (h < SEA_LEVEL) {
          for (let y = Math.max(1, h - 3); y <= h; y++) {
            const cur = blocks[IDX(lx, y, lz)];
            if (cur !== B.bedrock) blocks[IDX(lx, y, lz)] = bio.top === B.gravel ? B.gravel : B.sand;
          }
        }
        for (let y = h + 1; y <= SEA_LEVEL; y++) blocks[IDX(lx, y, lz)] = B.water;
        // 雪原铺雪
        if (c.biome === BIO.snowy_plains && h > SEA_LEVEL && blocks[IDX(lx, h, lz)] === B.grass_block) {
          blocks[IDX(lx, h, lz)] = B.snow_block;
        }
      }
    }

    // ---------- 2. 洞穴开挖 + 岩浆 ----------
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        const wx = ox + lx, wz = oz + lz;
        const h = colH[COL(lx, lz)];
        for (let y = 2; y <= Math.min(CAVE_TOP, h); y++) {
          const i = IDX(lx, y, lz);
          const cur = blocks[i];
          if (cur === B.bedrock || cur === B.water) continue;
          if (!this.isCave(wx, y, wz, h)) continue;
          blocks[i] = y <= LAVA_LEVEL ? B.lava : B.air;
        }
      }
    }

    // ---------- 3. 矿脉（随机游走团块） ----------
    const rng = makeRng((cx * 341873128 + cz * 132897987 + this.seed) | 0);
    for (const ore of ORE_TABLE) {
      const n = Math.floor(ore.veins) + (rng() < (ore.veins % 1) ? 1 : 0);
      for (let v = 0; v < n; v++) {
        const yMin = Math.max(1, ore.minY);
        let y = yMin + Math.floor(rng() * (ore.maxY - yMin + 1));
        let x = Math.floor(rng() * CHUNK_X);
        let z = Math.floor(rng() * CHUNK_Z);
        const size = ore.minSize + Math.floor(rng() * (ore.maxSize - ore.minSize + 1));
        for (let s = 0; s < size; s++) {
          if (blocks[IDX(x, y, z)] === B.stone) blocks[IDX(x, y, z)] = ore.block;
          const d = rng();
          if (d < 0.3) x = (x + 1) % CHUNK_X;
          else if (d < 0.55) x = (x - 1 + CHUNK_X) % CHUNK_X;
          else if (d < 0.78) z = (z + 1) % CHUNK_Z;
          else if (d < 0.9) z = (z - 1 + CHUNK_Z) % CHUNK_Z;
          else y = clamp(y + (rng() < 0.5 ? -1 : 1) * (1 + Math.floor(rng() * 2)), 1, WORLD_H - 1);
        }
      }
    }
    // 砂砾团块，让石头不那么单调
    for (let v = 0; v < 6; v++) {
      let x = Math.floor(rng() * CHUNK_X), z = Math.floor(rng() * CHUNK_Z);
      const sy = 20 + Math.floor(rng() * (SEA_LEVEL - 24));
      const size = 10 + Math.floor(rng() * 24);
      for (let s = 0; s < size; s++) {
        let y = sy + Math.floor(rng() * 4) - 2;
        if (y > 0 && y < WORLD_H && blocks[IDX(x, y, z)] === B.stone) blocks[IDX(x, y, z)] = B.gravel;
        const d = rng();
        if (d < 0.34) x = (x + 1) % CHUNK_X;
        else if (d < 0.67) z = (z + 1) % CHUNK_Z;
        else y += rng() < 0.5 ? -1 : 1;
      }
    }

    // ---------- 4. 地表植被（跨区块一致：按世界列哈希 + padding 扫描） ----------
    this.decorate(cx, cz, blocks, biomes);

    // ---------- 5. 重算高度图 / maxY ----------
    // 只统计「实心」方块：花草是十字模型，没有碰撞面，把它算进地面高度会让
    // 出生点判定和网格化的高度上限都偏高一格。
    const heights = new Int16Array(CHUNK_AREA);
    let maxY = 1;
    for (let lx = 0; lx < CHUNK_X; lx++) {
      for (let lz = 0; lz < CHUNK_Z; lz++) {
        const b = COL(lx, lz);
        let top = 0;
        for (let y = WORLD_H - 1; y >= 0; y--) {
          const id = blocks[IDX(lx, y, lz)];
          if (id === B.air || id === B.water || id === B.lava) continue;
          if (RENDER_KIND[id] === 1) continue;      // 花草类不算地面
          top = y;
          break;
        }
        heights[b] = top;
        if (top > maxY) maxY = top;
      }
    }
    return { blocks, biomes, heights, maxY };
  }

  /** 只写入本区块内的方块 */
  makeSetter(blocks, cx, cz) {
    const ox = cx * CHUNK_X, oz = cz * CHUNK_Z;
    return function put(wx, y, wz, id, overwrite) {
      const lx = wx - ox, lz = wz - oz;
      if (lx < 0 || lx >= CHUNK_X || lz < 0 || lz >= CHUNK_Z) return false;
      if (y < 1 || y >= WORLD_H) return false;
      const i = IDX(lx, y, lz);
      const cur = blocks[i];
      if (!overwrite && cur !== B.air && cur !== B.water && cur !== B.tall_grass &&
          cur !== B.dandelion && cur !== B.poppy && cur !== B.dead_bush) return false;
      blocks[i] = id;
      return true;
    };
  }

  decorate(cx, cz, blocks, biomes) {
    const put = this.makeSetter(blocks, cx, cz);
    const ox = cx * CHUNK_X, oz = cz * CHUNK_Z;
    const span = CHUNK_X + FEATURE_PAD * 2;

    for (let px = -FEATURE_PAD; px < span; px++) {
      for (let pz = -FEATURE_PAD; pz < span; pz++) {
        const wx = ox + px, wz = oz + pz;
        const col = this.column(wx, wz);
        const h = col.height;
        if (h <= SEA_LEVEL || h >= WORLD_H - 14) continue;
        const bio = BIOMES[col.biome];
        const r = hash2i(wx, wz, this.seed + 900);

        // --- 树 ---
        if (bio.tree && r < bio.treeDensity) {
          const tr = makeRng((Math.imul(wx, 73856093) ^ Math.imul(wz, 19349663) ^ this.seed) | 0);
          this.placeTree(bio.tree.kind, put, wx, h + 1, wz, tr);
          continue;
        }
        // --- 仙人掌 ---
        if (bio.cactus && r < bio.cactus + 0.0001 && hash2i(wx, wz, this.seed + 901) < bio.cactus) {
          const ht = 2 + Math.floor(hash2i(wx, wz, this.seed + 902) * 3);
          for (let i = 0; i < ht; i++) put(wx, h + 1 + i, wz, B.cactus, true);
          continue;
        }
        const inside = px >= 0 && px < CHUNK_X && pz >= 0 && pz < CHUNK_Z;
        // 地表装饰只能落在草方块上
        if (!inside || blocks[IDX(px, h, pz)] !== B.grass_block) continue;

        const r2 = hash2i(wx, wz, this.seed + 903);
        if (r2 < bio.flower) {
          const f = hash2i(wx, wz, this.seed + 904) < 0.5 ? B.dandelion : B.poppy;
          put(wx, h + 1, wz, f, true);
        } else if (r2 < bio.flower + bio.plant) {
          put(wx, h + 1, wz, B.tall_grass, true);
        } else if (r2 > 0.9992) {
          put(wx, h + 1, wz, B.pumpkin, true);
        }

        // 甘蔗：贴着水边长
        if (r2 > 0.9995) {
          for (const [dx, dz] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
            const nx2 = px + dx, nz2 = pz + dz;
            if (nx2 < 0 || nx2 >= CHUNK_X || nz2 < 0 || nz2 >= CHUNK_Z) continue;
            if (blocks[IDX(nx2, h, nz2)] === B.water || blocks[IDX(nx2, h - 1, nz2)] === B.water) {
              const sh = 2 + Math.floor(hash2i(wx, wz, this.seed + 905) * 2);
              for (let i = 0; i < sh; i++) put(wx, h + 1 + i, wz, B.sugar_cane, true);
              break;
            }
          }
        }
      }
    }
  }

  /**
   * 树形。形状只依赖 (wx,wy,wz) 与传入 rng，邻居区块生成的同一棵树完全一致。
   * put() 会自动丢弃区块外的写入。
   */
  placeTree(kind, put, wx, wy, wz, rng) {
    const R = () => rng();
    if (kind === 'oak') {
      const h = 4 + Math.floor(R() * 3);
      for (let i = 0; i < h; i++) put(wx, wy + i, wz, B.oak_log, true);
      const top = wy + h;
      this.canopy(put, wx, top - 2, wz, 2, B.oak_leaves, 0.62);
      this.canopy(put, wx, top - 1, wz, 2, B.oak_leaves, 0.18);
      this.canopy(put, wx, top, wz, 1, B.oak_leaves, 0);
    } else if (kind === 'birch') {
      const h = 5 + Math.floor(R() * 3);
      for (let i = 0; i < h; i++) put(wx, wy + i, wz, B.birch_log, true);
      const top = wy + h;
      this.canopy(put, wx, top - 2, wz, 2, B.birch_leaves, 0.4);
      this.canopy(put, wx, top - 1, wz, 1, B.birch_leaves, 0);
    } else if (kind === 'spruce') {
      const h = 7 + Math.floor(R() * 5);
      for (let i = 0; i < h; i++) put(wx, wy + i, wz, B.spruce_log, true);
      // 逐层收窄的圆锥
      let r = 0;
      for (let i = h - 1; i >= 2; i--) {
        const layer = h - 1 - i;
        r = layer < 2 ? 1 : (layer % 3 === 0 ? 2 : 1);
        if (layer > 6) r = 1;
        for (let dx = -r; dx <= r; dx++) {
          for (let dz = -r; dz <= r; dz++) {
            if (dx * dx + dz * dz > r * r + 1) continue;
            put(wx + dx, wy + i, wz + dz, B.spruce_leaves, false);
          }
        }
        if (layer % 4 === 0) put(wx, wy + i + 1, wz, B.spruce_leaves, false);
      }
      put(wx, wy + h, wz, B.spruce_leaves, false);
    } else if (kind === 'savanna') {
      const h = 4 + Math.floor(R() * 3);
      for (let i = 0; i < h; i++) put(wx, wy + i, wz, B.oak_log, true);
      const top = wy + h;
      for (let dx = -2; dx <= 2; dx++) {
        for (let dz = -2; dz <= 2; dz++) {
          if (Math.abs(dx) === 2 && Math.abs(dz) === 2) continue;
          put(wx + dx, top - 1, wz + dz, B.oak_leaves, false);
          put(wx + dx, top, wz + dz, B.oak_leaves, false);
        }
      }
    } else if (kind === 'jungle') {
      const h = 9 + Math.floor(R() * 6);
      const wide = R() < 0.5;
      for (let i = 0; i < h; i++) {
        put(wx, wy + i, wz, B.oak_log, true);
        if (wide && i > 1) { put(wx + 1, wy + i, wz, B.oak_log, true); put(wx, wy + i, wz + 1, B.oak_log, true); }
      }
      const top = wy + h;
      for (let dy = -3; dy <= 0; dy++) {
        const r = dy === 0 ? 1 : dy === -1 ? 2 : 3;
        this.canopy(put, wx, top + dy, wz, r, B.oak_leaves, dy < 0 ? 0.25 : 0);
      }
    } else if (kind === 'swamp') {
      const h = 5 + Math.floor(R() * 3);
      for (let i = 0; i < h; i++) put(wx, wy + i, wz, B.oak_log, true);
      const top = wy + h;
      this.canopy(put, wx, top - 1, wz, 3, B.oak_leaves, 0.3);
      this.canopy(put, wx, top, wz, 2, B.oak_leaves, 0);
    }
  }

  /** 球-ish 树冠，skipChance 用于随机打洞 */
  canopy(put, wx, wy, wz, r, leaf, skipChance) {
    for (let dx = -r; dx <= r; dx++) {
      for (let dz = -r; dz <= r; dz++) {
        const d2 = dx * dx + dz * dz;
        if (d2 > r * r + 1) continue;
        // 边角更容易空
        if (skipChance > 0 && d2 >= r * r - 1 && hash3i(wx + dx, wy, wz + dz, this.seed + 5) < skipChance) continue;
        put(wx + dx, wy, wz + dz, leaf, false);
      }
    }
  }
}
