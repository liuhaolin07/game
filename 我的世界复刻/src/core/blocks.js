// 方块注册表：整个游戏唯一的方块真相来源。
// 网格化、光照、挖掘、掉落、粒子全部读这张表。
// 纯数据，无 DOM 依赖。

import TILE from '../render/tiles.js';

export const B = {
  air: 0,
  stone: 1,
  dirt: 2,
  grass_block: 3,
  cobblestone: 4,
  oak_planks: 5,
  sand: 6,
  sandstone: 7,
  gravel: 8,
  oak_log: 9,
  oak_leaves: 10,
  bedrock: 11,
  water: 12,
  glass: 13,
  coal_ore: 14,
  iron_ore: 15,
  gold_ore: 16,
  diamond_ore: 17,
  redstone_ore: 18,
  lapis_ore: 19,
  snow_block: 20,
  ice: 21,
  cactus: 22,
  tall_grass: 23,
  dandelion: 24,
  poppy: 25,
  dead_bush: 26,
  sugar_cane: 27,
  glowstone: 28,
  obsidian: 29,
  clay: 30,
  birch_log: 31,
  birch_leaves: 32,
  spruce_log: 33,
  spruce_leaves: 34,
  pumpkin: 35,
  mossy_cobblestone: 36,
  stone_bricks: 37,
  lava: 38,
  coarse_dirt: 39,
  birch_planks: 40,
  spruce_planks: 41,
  torch: 42,
};

export const BLOCK_COUNT = 43;

const D = (o) => Object.assign({
  name: '?', tiles: null, solid: true, opaque: true, filter: 15, light: 0,
  render: 'cube', hardness: 1, tool: null, tier: 0, drop: null, gravity: false,
  color: [128, 128, 128], sound: 'stone',
}, o);

// drop 语义：null = 掉落自身；B.air = 不掉落；其它 = 掉落指定方块

export const BLOCKS = [];

function def(id, o) {
  BLOCKS[id] = D(o);
  BLOCKS[id].id = id;
  // 三面贴图：没写全就补齐
  const t = o.tiles;
  if (t) {
    if (t.length === 1) { BLOCKS[id].tiles = [t[0], t[0], t[0]]; }
    else if (t.length === 2) { BLOCKS[id].tiles = [t[0], t[0], t[1]]; }
    else { BLOCKS[id].tiles = t; }
  }
}

def(B.air, { name: '空气', tiles: [TILE.stone], solid: false, opaque: false, filter: 0, hardness: -1, sound: 'none' });

def(B.stone, {
  name: '石头', tiles: [TILE.stone], hardness: 1.5, tool: 'pickaxe',
  drop: B.cobblestone, color: [125, 125, 125], sound: 'stone',
});
def(B.dirt, { name: '泥土', tiles: [TILE.dirt], hardness: 0.5, tool: 'shovel', color: [134, 96, 67] });
def(B.grass_block, {
  name: '草方块', tiles: [TILE.grass_top, TILE.dirt, TILE.grass_side],
  hardness: 0.6, tool: 'shovel', drop: B.dirt, color: [95, 159, 53], sound: 'grass',
});
def(B.cobblestone, {
  name: '圆石', tiles: [TILE.cobblestone], hardness: 2, tool: 'pickaxe',
  color: [122, 122, 122], sound: 'stone',
});
def(B.oak_planks, {
  name: '橡木木板', tiles: [TILE.oak_planks], hardness: 2, tool: 'axe',
  color: [162, 130, 78], sound: 'wood',
});
def(B.sand, { name: '沙子', tiles: [TILE.sand], hardness: 0.5, tool: 'shovel', color: [219, 207, 163], sound: 'sand' });
def(B.sandstone, {
  name: '砂岩', tiles: [TILE.sandstone_top, TILE.sandstone_side], hardness: 0.8,
  tool: 'pickaxe', color: [216, 203, 155], sound: 'stone',
});
def(B.gravel, { name: '砂砾', tiles: [TILE.gravel], hardness: 0.6, tool: 'shovel', color: [131, 127, 126], sound: 'gravel' });
def(B.oak_log, {
  name: '橡木原木', tiles: [TILE.oak_log_top, TILE.oak_log_top, TILE.oak_log_side],
  hardness: 2, tool: 'axe', color: [104, 83, 49], sound: 'wood',
});
def(B.oak_leaves, {
  name: '橡树树叶', tiles: [TILE.oak_leaves], opaque: false, filter: 1, hardness: 0.2,
  tool: 'axe', color: [60, 130, 44], sound: 'grass',
});
def(B.bedrock, {
  name: '基岩', tiles: [TILE.bedrock], hardness: -1, tool: 'pickaxe', drop: B.air,
  color: [85, 85, 85], sound: 'stone',
});
def(B.water, {
  name: '水', tiles: [TILE.water], solid: false, opaque: false, filter: 1,
  render: 'liquid', hardness: -1, drop: B.air, sound: 'splash', color: [58, 110, 200],
});
def(B.lava, {
  name: '岩浆', tiles: [TILE.lava], solid: false, opaque: false, filter: 1,
  light: 15, render: 'liquid', hardness: -1, drop: B.air, sound: 'lava', color: [220, 110, 20],
});
def(B.glass, {
  name: '玻璃', tiles: [TILE.glass], opaque: false, filter: 0, hardness: 0.3,
  drop: B.glass, color: [200, 230, 240], sound: 'glass',
});
def(B.coal_ore, {
  name: '煤矿石', tiles: [TILE.ore_coal], hardness: 3, tool: 'pickaxe',
  color: [45, 45, 45], sound: 'stone',
});
def(B.iron_ore, {
  name: '铁矿石', tiles: [TILE.ore_iron], hardness: 3, tool: 'pickaxe', tier: 1,
  color: [200, 160, 130], sound: 'stone',
});
def(B.gold_ore, {
  name: '金矿石', tiles: [TILE.ore_gold], hardness: 3, tool: 'pickaxe', tier: 2,
  color: [240, 200, 80], sound: 'stone',
});
def(B.diamond_ore, {
  name: '钻石矿石', tiles: [TILE.ore_diamond], hardness: 3, tool: 'pickaxe', tier: 2,
  color: [110, 235, 225], sound: 'stone',
});
def(B.redstone_ore, {
  name: '红石矿石', tiles: [TILE.ore_redstone], hardness: 3, tool: 'pickaxe', tier: 2,
  light: 4, color: [200, 40, 40], sound: 'stone',
});
def(B.lapis_ore, {
  name: '青金石矿石', tiles: [TILE.ore_lapis], hardness: 3, tool: 'pickaxe', tier: 1,
  color: [50, 90, 190], sound: 'stone',
});
def(B.snow_block, { name: '雪块', tiles: [TILE.snow], hardness: 0.2, tool: 'shovel', color: [245, 250, 255], sound: 'snow' });
def(B.ice, {
  name: '冰', tiles: [TILE.ice], opaque: false, filter: 2, hardness: 0.5,
  tool: 'pickaxe', color: [150, 200, 240], sound: 'glass',
});
def(B.cactus, {
  name: '仙人掌', tiles: [TILE.cactus_top, TILE.cactus_top, TILE.cactus_side],
  hardness: 0.4, sound: 'wool', color: [85, 130, 60],
});
def(B.tall_grass, {
  name: '高草', tiles: [TILE.tall_grass], solid: false, opaque: false, filter: 0,
  render: 'cross', hardness: 0, sound: 'plant', color: [110, 170, 60],
});
def(B.dandelion, {
  name: '蒲公英', tiles: [TILE.dandelion], solid: false, opaque: false, filter: 0,
  render: 'cross', hardness: 0, sound: 'plant', color: [230, 210, 60],
});
def(B.poppy, {
  name: '虞美人', tiles: [TILE.poppy], solid: false, opaque: false, filter: 0,
  render: 'cross', hardness: 0, sound: 'plant', color: [200, 50, 50],
});
def(B.dead_bush, {
  name: '枯萎的灌木', tiles: [TILE.dead_bush], solid: false, opaque: false, filter: 0,
  render: 'cross', hardness: 0, sound: 'plant', color: [140, 110, 60],
});
def(B.sugar_cane, {
  name: '甘蔗', tiles: [TILE.sugar_cane], solid: false, opaque: false, filter: 0,
  render: 'cross', hardness: 0, sound: 'plant', color: [150, 200, 110],
});
def(B.glowstone, {
  name: '萤石', tiles: [TILE.glowstone], light: 15, hardness: 0.3,
  color: [245, 205, 130], sound: 'glass',
});
def(B.obsidian, {
  name: '黑曜石', tiles: [TILE.obsidian], hardness: 25, tool: 'pickaxe', tier: 3,
  color: [20, 18, 30], sound: 'stone',
});
def(B.clay, { name: '黏土', tiles: [TILE.clay], hardness: 0.6, tool: 'shovel', color: [160, 166, 179], sound: 'dirt' });
def(B.birch_log, {
  name: '白桦原木', tiles: [TILE.birch_log_top, TILE.birch_log_top, TILE.birch_log_side],
  hardness: 2, tool: 'axe', color: [215, 215, 205], sound: 'wood',
});
def(B.birch_leaves, {
  name: '白桦树叶', tiles: [TILE.birch_leaves], opaque: false, filter: 1, hardness: 0.2,
  tool: 'axe', color: [110, 160, 70], sound: 'grass',
});
def(B.spruce_log, {
  name: '云杉原木', tiles: [TILE.spruce_log_top, TILE.spruce_log_top, TILE.spruce_log_side],
  hardness: 2, tool: 'axe', color: [86, 62, 38], sound: 'wood',
});
def(B.spruce_leaves, {
  name: '云杉树叶', tiles: [TILE.spruce_leaves], opaque: false, filter: 1, hardness: 0.2,
  tool: 'axe', color: [58, 92, 52], sound: 'grass',
});
def(B.pumpkin, {
  name: '南瓜', tiles: [TILE.pumpkin_top, TILE.pumpkin_top, TILE.pumpkin_side],
  hardness: 1, tool: 'axe', color: [200, 130, 30], sound: 'wood',
});
def(B.mossy_cobblestone, {
  name: '苔石', tiles: [TILE.mossy_cobblestone], hardness: 2, tool: 'pickaxe',
  color: [110, 125, 100], sound: 'stone',
});
def(B.stone_bricks, {
  name: '石砖', tiles: [TILE.stone_bricks], hardness: 1.5, tool: 'pickaxe',
  color: [122, 122, 122], sound: 'stone',
});
def(B.coarse_dirt, { name: '砂土', tiles: [TILE.coarse_dirt], hardness: 0.5, tool: 'shovel', color: [120, 90, 60] });
def(B.birch_planks, {
  name: '白桦木板', tiles: [TILE.birch_planks], hardness: 2, tool: 'axe',
  color: [200, 190, 160], sound: 'wood',
});
def(B.spruce_planks, {
  name: '云杉木板', tiles: [TILE.spruce_planks], hardness: 2, tool: 'axe',
  color: [125, 95, 60], sound: 'wood',
});
def(B.torch, {
  name: '火把', tiles: [TILE.torch], solid: false, opaque: false, filter: 0,
  render: 'cross', light: 14, hardness: 0, sound: 'wood', color: [255, 214, 120],
  cross: [0.11, 0.62],          // 十字模型的半宽与高度，火把要比花草细短
});

// 未定义 id 的占位，避免越界读到 undefined
for (let i = 0; i < BLOCK_COUNT; i++) {
  if (!BLOCKS[i]) def(i, { name: '未知方块', tiles: [TILE.stone], hardness: -1 });
}

// ---- 快查表（热路径上避免反复取属性） ----
export const IS_OPAQUE = new Uint8Array(BLOCK_COUNT);
export const IS_SOLID = new Uint8Array(BLOCK_COUNT);
export const LIGHT_EMIT = new Uint8Array(BLOCK_COUNT);
export const LIGHT_FILTER = new Uint8Array(BLOCK_COUNT);
export const RENDER_KIND = new Uint8Array(BLOCK_COUNT);   // 0 cube, 1 cross, 2 liquid
for (let i = 0; i < BLOCK_COUNT; i++) {
  const b = BLOCKS[i];
  IS_OPAQUE[i] = b.opaque ? 1 : 0;
  IS_SOLID[i] = b.solid ? 1 : 0;
  LIGHT_EMIT[i] = b.light;
  LIGHT_FILTER[i] = b.filter;
  RENDER_KIND[i] = b.render === 'cross' ? 1 : b.render === 'liquid' ? 2 : 0;
}

/** 相邻面是否需要绘制。cur=本格方块, nb=邻格方块 */
export function faceVisible(cur, nb) {
  if (nb === B.air) return true;
  if (IS_OPAQUE[nb]) return false;              // 被实心块挡住
  if (nb === cur) return false;                 // 同种透明块之间不画
  if (cur === B.water) return nb === B.glass;  // 水只对玻璃可见
  return true;
}

export { TILE };
