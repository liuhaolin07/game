// 贴图名 → 图集槽位的纯数据映射。
// 这里刻意不放任何 Canvas/DOM 代码，好让方块表能被 Node 单测直接引入。

// 顺序即槽位顺序（16×16 网格 = 最多 256 张）。新增贴图请追加到末尾。
export const TILE_ORDER = [
  // 泥土系
  'dirt', 'grass_top', 'grass_side', 'coarse_dirt',
  // 石
  'stone', 'cobblestone', 'mossy_cobblestone', 'stone_bricks', 'bedrock', 'obsidian',
  // 沙 / 雪 / 冰 / 黏土 / 砂砾
  'sand', 'sandstone_top', 'sandstone_side', 'snow', 'ice', 'clay', 'gravel',
  // 木
  'oak_log_side', 'oak_log_top', 'oak_planks',
  'birch_log_side', 'birch_log_top',
  'spruce_log_side', 'spruce_log_top',
  'oak_leaves', 'birch_leaves', 'spruce_leaves',
  'birch_planks', 'spruce_planks',
  // 其它透光
  'glass', 'water', 'lava', 'glowstone',
  // 矿
  'ore_coal', 'ore_iron', 'ore_gold', 'ore_diamond', 'ore_redstone', 'ore_lapis',
  // 植物（十字交叉渲染）
  'tall_grass', 'dandelion', 'poppy', 'dead_bush', 'sugar_cane', 'torch',
  // 其它
  'cactus_side', 'cactus_top', 'pumpkin_side', 'pumpkin_top', 'pumpkin_face',
  'wool_white', 'wool_orange', 'wool_yellow', 'wool_red',
];

const TILE = {};
TILE_ORDER.forEach((name, i) => {
  if (i > 255) throw new Error('贴图数量超过 16x16 图集容量 (256)');
  TILE[name] = i;
});

export const ATLAS_SIZE = 256;   // 16×16 像素 tile → 256×256 图集
export const TILE_PX = 16;
export const TILES_PER_ROW = ATLAS_SIZE / TILE_PX;

export default TILE;
