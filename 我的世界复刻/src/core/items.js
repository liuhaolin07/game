// 物品与工具。
//
// 方块本身既是「方块」也是「物品」（id 直接复用），工具则额外挂在方块 id 空间之外。
// 挖掘速度按原版那套模型算：
//   用对工具：t = hardness × 1.5 ÷ 工具速度
//   用错工具：t = hardness × 5
//   徒手    ：t = hardness × 5
// 另外要满足工具等级门槛才掉落后继材料（石头徒手挖也只有圆石，铁矿徒手挖不出东西）。

import { B, BLOCKS } from '../core/blocks.js';

export const TOOL_TIERS = [
  { name: '木', speed: 2, tier: 0 },
  { name: '石', speed: 4, tier: 1 },
  { name: '铁', speed: 6, tier: 2 },
  { name: '钻石', speed: 8, tier: 3 },
  { name: '下界合金', speed: 9, tier: 4 },
];

/** 工具 id 从这里开始，避开方块 id 空间 */
export const TOOL_BASE = 1000;
export const TOOLS = [
  { id: TOOL_BASE + 0, name: '木镐', kind: 'pickaxe', tier: 0 },
  { id: TOOL_BASE + 1, name: '石镐', kind: 'pickaxe', tier: 1 },
  { id: TOOL_BASE + 2, name: '铁镐', kind: 'pickaxe', tier: 2 },
  { id: TOOL_BASE + 3, name: '钻石镐', kind: 'pickaxe', tier: 3 },
  { id: TOOL_BASE + 4, name: '木斧', kind: 'axe', tier: 0 },
  { id: TOOL_BASE + 5, name: '石斧', kind: 'axe', tier: 1 },
  { id: TOOL_BASE + 6, name: '铁斧', kind: 'axe', tier: 2 },
  { id: TOOL_BASE + 7, name: '钻石斧', kind: 'axe', tier: 3 },
  { id: TOOL_BASE + 8, name: '木锹', kind: 'shovel', tier: 0 },
  { id: TOOL_BASE + 9, name: '石锹', kind: 'shovel', tier: 1 },
  { id: TOOL_BASE + 10, name: '铁锹', kind: 'shovel', tier: 2 },
  { id: TOOL_BASE + 11, name: '钻石锹', kind: 'shovel', tier: 3 },
];
export const TOOL_BY_ID = new Map(TOOLS.map(t => [t.id, t]));

export const isTool = (id) => id >= TOOL_BASE;
export function toolOf(id) { return TOOL_BY_ID.get(id) || null; }

export function itemName(id) {
  if (id === B.air) return '空手';
  const t = toolOf(id);
  if (t) return t.name;
  return BLOCKS[id] ? BLOCKS[id].name : '未知';
}

/** 工具对某个方块的挖掘加成系数。返回 1 表示没加成。 */
export function toolMultiplier(blockId, toolId) {
  const def = BLOCKS[blockId];
  if (!def || !def.tool) return 1;
  const t = toolOf(toolId);
  if (!t) return 1;                       // 徒手
  if (t.kind !== def.tool) return 1;      // 用错工具
  return TOOL_TIERS[t.tier].speed;
}

/** 挖掉这个方块要多少秒 */
export function breakTime(blockId, toolId) {
  const def = BLOCKS[blockId];
  if (!def || def.hardness < 0) return Infinity;   // 水、岩浆、基岩
  const mul = toolMultiplier(blockId, toolId);
  // 徒手和用错工具都是 hardness × 5（原版规则）
  if (mul === 1) return def.hardness * 5;
  return (def.hardness * 1.5) / mul;
}

/** 破坏后掉落什么；null 表示什么都不掉 */
export function dropOf(blockId, toolId) {
  const def = BLOCKS[blockId];
  if (!def) return null;
  if (def.hardness < 0) return null;
  // 工具等级不够，掉不出后继材料（例如徒手挖矿石）
  if (def.tier > 0) {
    const t = toolOf(toolId);
    const tier = t ? TOOL_TIERS[t.tier].tier : -1;
    if (tier < def.tier) return null;
  }
  return def.drop === null ? blockId : def.drop;
}

/** 随机掉落 1 个：矿石只有 25% 概率掉（其它石头类必掉） */
export function rollDrop(blockId, toolId) {
  const d = dropOf(blockId, toolId);
  if (d === null) return null;
  const def = BLOCKS[blockId];
  const isOre = blockId >= B.coal_ore && blockId <= B.lapis_ore;
  if (isOre && Math.random() > 0.32) return null;
  if (def.gravity) return null;   // 沙/砂砾这轮先不处理掉落方块实体
  return d;
}
