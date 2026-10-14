// 生物群系：四张气候图（大陆度 / 侵蚀度 / 温度 / 湿度）决定地表方块、植被与配色。
// 纯数据 + 纯函数，可无头测试。

import { B } from '../core/blocks.js';
import { SEA_LEVEL } from '../core/constants.js';

export const BIO = {
  ocean: 0,
  deep_ocean: 1,
  beach: 2,
  plains: 3,
  forest: 4,
  birch_forest: 5,
  taiga: 6,
  desert: 7,
  savanna: 8,
  jungle: 9,
  swamp: 10,
  snowy_plains: 11,
  mountains: 12,
  stony_peaks: 13,
  river: 14,
};

export const BIOME_COUNT = 15;

/**
 * top/filler  地表与次表层方块
 * fillDepth   次表层厚度
 * tint        草/叶的群系染色（RGB 0-255），0 表示不着色
 * tree        树木种类与每列生成概率
 */
export const BIOMES = [
  { name: '海洋', top: B.sand, filler: B.sand, fillDepth: 3, tint: [0, 0, 0], tree: null, treeDensity: 0, plant: 0, flower: 0 },
  { name: '深海', top: B.gravel, filler: B.gravel, fillDepth: 3, tint: [0, 0, 0], tree: null, treeDensity: 0, plant: 0, flower: 0 },
  { name: '沙滩', top: B.sand, filler: B.sand, fillDepth: 4, tint: [0, 0, 0], tree: null, treeDensity: 0, plant: 0, flower: 0.002 },
  { name: '平原', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [145, 189, 89], tree: { log: B.oak_log, leaf: B.oak_leaves, kind: 'oak' }, treeDensity: 0.006, plant: 0.14, flower: 0.012 },
  { name: '森林', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [110, 170, 70], tree: { log: B.oak_log, leaf: B.oak_leaves, kind: 'oak' }, treeDensity: 0.075, plant: 0.16, flower: 0.008 },
  { name: '白桦森林', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [130, 180, 90], tree: { log: B.birch_log, leaf: B.birch_leaves, kind: 'birch' }, treeDensity: 0.085, plant: 0.14, flower: 0.01 },
  { name: '针叶林', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [90, 150, 90], tree: { log: B.spruce_log, leaf: B.spruce_leaves, kind: 'spruce' }, treeDensity: 0.09, plant: 0.1, flower: 0.003 },
  { name: '沙漠', top: B.sand, filler: B.sand, fillDepth: 5, tint: [191, 191, 85], tree: null, treeDensity: 0, plant: 0, flower: 0, cactus: 0.012 },
  { name: '热带草原', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [191, 183, 85], tree: { log: B.oak_log, leaf: B.oak_leaves, kind: 'savanna' }, treeDensity: 0.012, plant: 0.18, flower: 0.004 },
  { name: '丛林', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [89, 176, 59], tree: { log: B.oak_log, leaf: B.oak_leaves, kind: 'jungle' }, treeDensity: 0.13, plant: 0.3, flower: 0.006 },
  { name: '沼泽', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [106, 134, 54], tree: { log: B.oak_log, leaf: B.oak_leaves, kind: 'swamp' }, treeDensity: 0.02, plant: 0.22, flower: 0.002 },
  { name: '雪原', top: B.grass_block, filler: B.dirt, fillDepth: 4, tint: [128, 178, 128], tree: { log: B.spruce_log, leaf: B.spruce_leaves, kind: 'spruce' }, treeDensity: 0.006, plant: 0.02, flower: 0 },
  { name: '山地', top: B.grass_block, filler: B.dirt, fillDepth: 3, tint: [138, 176, 96], tree: null, treeDensity: 0, plant: 0.05, flower: 0.002 },
  { name: '裸岩山峰', top: B.stone, filler: B.stone, fillDepth: 3, tint: [0, 0, 0], tree: null, treeDensity: 0, plant: 0, flower: 0 },
  { name: '河流', top: B.sand, filler: B.sand, fillDepth: 3, tint: [0, 0, 0], tree: null, treeDensity: 0, plant: 0, flower: 0 },
];

// 会被群系染色影响的方块
const TINTABLE = new Set([B.grass_block, B.oak_leaves, B.birch_leaves, B.tall_grass]);
export function isTintable(id) { return TINTABLE.has(id); }

/**
 * 由四张气候图挑一个生物群系。
 *
 * 阈值不是随手写的：气候值经过增益后落在约 [-1.2, 1.2]，但因为 fbm 的钟形分布，
 * 真正均匀的是分位数而不是数值本身——p25/p75 约在 ∓0.55，p10/p90 约在 ∓1.0。
 * 所以分界取在 -0.30 / 0.30 / 0.50 这种「中位数附近带宽」的位置，
 * 而不是 0 附近极窄的区间，否则森林、白桦这类气候会几乎抽不到。
 *
 * @param temp   -1.2..1.2  温度
 * @param humid  -1.2..1.2  湿度
 * @param cont   -1.2..1.2  大陆度
 * @param height 地形高度
 * @param erosion -1.2..1.2 侵蚀度（越低山越多）
 * @param isRiver 是否落在河道上
 */
export function pickBiome(temp, humid, cont, height, erosion, isRiver) {
  if (isRiver && height <= SEA_LEVEL) return BIO.river;
  if (height < SEA_LEVEL - 7) return BIO.deep_ocean;
  if (height < SEA_LEVEL) return BIO.ocean;
  if (height <= SEA_LEVEL + 1) return BIO.beach;

  // 高海拔：冷的地方是雪，暖的地方是裸岩
  if (height > SEA_LEVEL + 38) {
    return temp < -0.25 ? BIO.snowy_plains : BIO.stony_peaks;
  }
  if (height > SEA_LEVEL + 22) {
    if (temp < -0.35) return BIO.snowy_plains;
    if (erosion < -0.10) return BIO.stony_peaks;
    return BIO.mountains;
  }

  // 湿的低洼地当沼泽
  if (humid > 0.45 && height <= SEA_LEVEL + 4) return BIO.swamp;

  if (temp < -0.30) return humid > 0.10 ? BIO.taiga : BIO.snowy_plains;
  if (temp > 0.50) {
    if (humid < -0.10) return BIO.desert;
    return humid > 0.35 ? BIO.jungle : BIO.savanna;
  }
  if (humid > 0.30) return temp < 0.15 ? BIO.taiga : BIO.birch_forest;
  if (humid > -0.05) return BIO.forest;
  if (temp > 0.15) return BIO.savanna;
  return BIO.plains;
}

export function biomeName(id) {
  return (BIOMES[id] || BIOMES[BIO.plains]).name;
}
