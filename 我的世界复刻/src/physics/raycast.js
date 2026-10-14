// 体素射线投射（Amanatides & Woo 网格步进）。
// 用于准星选取方块：从眼睛沿视线逐格前进，记录进入的哪一面。

import { WORLD_H } from '../core/constants.js';
import { B } from '../core/blocks.js';

/**
 * @param world  提供 getBlock(x,y,z)
 * @param o      起点 {x,y,z}
 * @param dir    单位方向 {x,y,z}
 * @param maxDist  最大距离
 * @param accept  (id)=>bool  是否算命中（空气、水默认不算）
 * @returns {x,y,z,nx,ny,nz,id,dist}|null
 */
export function raycast(world, o, dir, maxDist, accept) {
  let x = Math.floor(o.x), y = Math.floor(o.y), z = Math.floor(o.z);
  const stepX = dir.x > 0 ? 1 : -1;
  const stepY = dir.y > 0 ? 1 : -1;
  const stepZ = dir.z > 0 ? 1 : -1;

  const tDeltaX = dir.x !== 0 ? Math.abs(1 / dir.x) : Infinity;
  const tDeltaY = dir.y !== 0 ? Math.abs(1 / dir.y) : Infinity;
  const tDeltaZ = dir.z !== 0 ? Math.abs(1 / dir.z) : Infinity;

  let tMaxX = dir.x !== 0
    ? ((dir.x > 0 ? x + 1 - o.x : o.x - x) * tDeltaX) : Infinity;
  let tMaxY = dir.y !== 0
    ? ((dir.y > 0 ? y + 1 - o.y : o.y - y) * tDeltaY) : Infinity;
  let tMaxZ = dir.z !== 0
    ? ((dir.z > 0 ? z + 1 - o.z : o.z - z) * tDeltaZ) : Infinity;

  let nx = 0, ny = 0, nz = 0;

  // 起点所在格也要判一次（玩家可能正贴着方块）
  let id = world.getBlock(x, y, z);
  if (id !== B.air && accept(id)) {
    return { x, y, z, nx: 0, ny: 1, nz: 0, id, dist: 0 };
  }

  let t = 0;
  // 上限按最粗一格估算，够用且防止方向退化时死循环
  const limit = Math.ceil(maxDist * 3) + 3;
  for (let i = 0; i < limit; i++) {
    if (tMaxX < tMaxY) {
      if (tMaxX < tMaxZ) { if (tMaxX > maxDist) break; x += stepX; t = tMaxX; tMaxX += tDeltaX; nx = -stepX; ny = 0; nz = 0; }
      else { if (tMaxZ > maxDist) break; z += stepZ; t = tMaxZ; tMaxZ += tDeltaZ; nx = 0; ny = 0; nz = -stepZ; }
    } else {
      if (tMaxY < tMaxZ) { if (tMaxY > maxDist) break; y += stepY; t = tMaxY; tMaxY += tDeltaY; nx = 0; ny = -stepY; nz = 0; }
      else { if (tMaxZ > maxDist) break; z += stepZ; t = tMaxZ; tMaxZ += tDeltaZ; nx = 0; ny = 0; nz = -stepZ; }
    }
    if (y < 0 || y >= WORLD_H) {
      if (y < 0) break;
      continue;
    }
    id = world.getBlock(x, y, z);
    if (id !== B.air && accept(id)) return { x, y, z, nx, ny, nz, id, dist: t };
  }
  return null;
}

/** 默认的「可被准星选中」判定：空气和水不算 */
export const isTargetable = (id) => id !== B.air && id !== B.water && id !== B.lava;
