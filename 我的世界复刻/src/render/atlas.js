// 程序化贴图图集：全部 16×16 贴图在运行时用 Canvas 画出来，不依赖任何外部素材。
// 槽位编号由 tiles.js 统一决定，这里只负责「画」。
//
// 同时提供：
//   buildAtlasTexture()   把画布变成 DataTexture（见那里的注释：为什么不用 CanvasTexture）
//   buildBreakTexture()   破坏进度的 10 级裂纹叠加图
//   drawToolIcon()        工具图标（镐/斧/锹），同样程序化

import * as THREE from 'three';
import TILE, { ATLAS_SIZE, TILE_PX } from './tiles.js';
import { makeRng, hash2i } from '../core/noise.js';

const rng = makeRng(0x5EED);

function newCanvas(w, h) {
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  return c;
}

/**
 * 画一张贴图。fn(x, y, set) 中 set(r,g,b,a) 写单个像素。
 */
function paint(g, name, fn) {
  const ox = (TILE[name] % 16) * TILE_PX;
  const oy = Math.floor(TILE[name] / 16) * TILE_PX;
  const img = g.createImageData(TILE_PX, TILE_PX);
  const d = img.data;
  const px = (x, y, r, gg, b, a = 255) => {
    if (x < 0 || y < 0 || x >= TILE_PX || y >= TILE_PX) return;
    const i = (y * TILE_PX + x) * 4;
    d[i] = r; d[i + 1] = gg; d[i + 2] = b; d[i + 3] = a;
  };
  fn(px);
  g.putImageData(img, ox, oy);
}

const clamp255 = (v) => (v < 0 ? 0 : v > 255 ? 255 : v | 0);
const shade = (r, g, b, v) => `rgb(${clamp255(r + v)},${clamp255(g + v)},${clamp255(b + v)})`;

/** 纯色 + 亮度抖动 */
function noisy(r, g, b, amt) {
  return (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      px(x, y, r, g, b);
    }
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const v = (rng() * 2 - 1) * amt;
      const i = (y * 16 + x) * 4;
      px(x, y, clamp255(r + v), clamp255(g + v), clamp255(b + v));
    }
  };
}

/** 叠加一层明暗斑块 */
function blotch(r, g, b, seed, density = 0.12, amt = 18) {
  return (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const n = hash2i(x >> 2, y >> 2, seed);
      const v = (n < density ? -amt : n > 1 - density ? amt * 0.6 : 0) + (rng() * 2 - 1) * 5;
      px(x, y, clamp255(r + v), clamp255(g + v), clamp255(b + v));
    }
  };
}

/** 在底色上画矿石/苔藓/结晶的团块 */
function blobs(px, base, spots, seed) {
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, base[0], base[1], base[2]);
  for (const [sx, sy, n, col] of spots) {
    for (let k = 0; k < n; k++) {
      const cx = Math.floor(rng() * 16), cy = Math.floor(rng() * 16);
      const rad = 1 + Math.floor(rng() * 2);
      for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
        const d = Math.hypot(x - cx, y - cy);
        if (d > rad) continue;
        const v = (rng() * 2 - 1) * 22 - (d < rad * 0.5 ? 16 : 0);
        const edge = d > rad - 0.7;
        px(x, y, clamp255(col[0] + v), clamp255(col[1] + v), clamp255(col[2] + v), edge ? 190 : 255);
      }
    }
    void seed;
  }
}

// 圆环（年轮）
function rings(px, r, g, b, cx = 7.5, cy = 7.5) {
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const d = Math.max(Math.abs(x - cx), Math.abs(y - cy));
    const ring = Math.floor(d) % 2 === 0 ? 10 : -12;
    const v = ring + (rng() * 2 - 1) * 7;
    px(x, y, clamp255(r + v), clamp255(g + v), clamp255(b + v));
  }
}

export function buildAtlas() {
  const canvas = newCanvas(ATLAS_SIZE, ATLAS_SIZE);
  // 不要 willReadFrequently：这里只用 putImageData 写入，从不 getImageData 回来读。
  // 加上它会让 Chrome 把画布放到 CPU 内存，在某些环境（含无头浏览器的软件光栅化）
  // 上 texImage2D 从这样的画布取到的是全黑——表现是世界整个渲染不出来。
  const g = canvas.getContext('2d');
  g.imageSmoothingEnabled = false;
  g.clearRect(0, 0, ATLAS_SIZE, ATLAS_SIZE);

  // ---- 泥土系 ----
  paint(g, 'dirt', blotch(134, 96, 67, 11, 0.16, 16));
  paint(g, 'coarse_dirt', (px) => { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 120, 90, 60); for (let k = 0; k < 26; k++) { const cx = rng() * 16, cy = rng() * 16, r = 1 + rng(); for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const d = Math.hypot(x - cx, y - cy); if (d > r) continue; const v = (rng() * 2 - 1) * 26; px(x, y, clamp255(120 + v), clamp255(90 + v), clamp255(60 + v)); } } });
  paint(g, 'grass_top', noisy(110, 168, 68, 18));
  paint(g, 'grass_side', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const gr = y < 3 || (y === 3 && hash2i(x, 0, 3) < 0.5) || (y === 4 && hash2i(x, 0, 9) < 0.25);
      if (gr) { const v = (rng() * 2 - 1) * 18; px(x, y, clamp255(110 + v), clamp255(168 + v), clamp255(68 + v)); }
      else { const v = (rng() * 2 - 1) * 14; px(x, y, clamp255(134 + v), clamp255(96 + v), clamp255(67 + v)); }
    }
  });

  // ---- 石 ----
  paint(g, 'stone', (px) => { blotch(128, 128, 128, 21, 0.18, 20)(px); });
  paint(g, 'cobblestone', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const cell = hash2i(x >> 2, y >> 2, 33);
      const mortar = (x % 5 === 0) || (y % 5 === 0);
      let v = (cell - 0.5) * 34 + (mortar ? -30 : 0) + (rng() * 2 - 1) * 9;
      px(x, y, clamp255(122 + v), clamp255(122 + v), clamp255(122 + v));
    }
  });
  paint(g, 'mossy_cobblestone', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const cell = hash2i(x >> 2, y >> 2, 33);
      const mortar = (x % 5 === 0) || (y % 5 === 0);
      let v = (cell - 0.5) * 30 + (mortar ? -28 : 0) + (rng() * 2 - 1) * 9;
      let r = 112 + v, gg = 122 + v, b = 104 + v;
      if (hash2i(x >> 1, y >> 1, 77) < 0.34) { r = 84 + v; gg = 122 + v; b = 58 + v; }   // 苔
      px(x, y, clamp255(r), clamp255(gg), clamp255(b));
    }
  });
  paint(g, 'stone_bricks', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const row = Math.floor(y / 8);
      const off = row % 2 ? 4 : 0;
      const bx = (x + off) % 8, by = y % 8;
      const mortar = bx === 0 || by === 0;
      const v = (mortar ? -30 : (hash2i(bx, by + row * 8, 44) - 0.5) * 22) + (rng() * 2 - 1) * 7;
      px(x, y, clamp255(124 + v), clamp255(124 + v), clamp255(124 + v));
    }
  });
  paint(g, 'bedrock', (px) => { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const v = (hash2i(x >> 1, y >> 1, 55) - 0.5) * 60 + (rng() * 2 - 1) * 12; px(x, y, clamp255(72 + v), clamp255(72 + v), clamp255(72 + v)); } });
  paint(g, 'obsidian', (px) => { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const n = hash2i(x, y, 66); const v = n < 0.08 ? 46 : (n > 0.95 ? 26 : 0) + (rng() * 2 - 1) * 8; px(x, y, clamp255(20 + v), clamp255(16 + v), clamp255(32 + v)); } });

  // ---- 沙 / 雪 / 冰 / 黏土 / 砂砾 ----
  paint(g, 'sand', noisy(219, 207, 163, 13));
  paint(g, 'sandstone_top', (px) => { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const d = Math.max(Math.abs(x - 7.5), Math.abs(y - 7.5)); const v = (Math.floor(d) % 2 === 0 ? 7 : -7) + (rng() * 2 - 1) * 8; px(x, y, clamp255(216 + v), clamp255(203 + v), clamp255(155 + v)); } });
  paint(g, 'sandstone_side', (px) => { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const band = Math.floor(y / 4); const v = (band % 2 ? -12 : 6) + (rng() * 2 - 1) * 8; px(x, y, clamp255(216 + v), clamp255(203 + v), clamp255(155 + v)); } });
  paint(g, 'snow', noisy(243, 248, 255, 10));
  paint(g, 'ice', (px) => { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const crack = hash2i(x + y, y - x, 88) > 0.93; const v = (rng() * 2 - 1) * 12 + (crack ? 34 : 0); px(x, y, clamp255(150 + v), clamp255(200 + v), clamp255(245 + v), crack ? 215 : 190); } });
  paint(g, 'clay', noisy(160, 166, 179, 12));
  paint(g, 'gravel', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 118, 114, 112);
    for (let k = 0; k < 22; k++) {
      const cx = rng() * 16, cy = rng() * 16, r = 1 + rng() * 1.6;
      const t = 90 + rng() * 80;
      for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
        const d = Math.hypot(x - cx, y - cy);
        if (d > r) continue;
        const v = (rng() * 2 - 1) * 16;
        px(x, y, clamp255(t + v), clamp255(t - 4 + v), clamp255(t - 8 + v));
      }
    }
  });

  // ---- 木 ----
  const bark = (r0, g0, b0, streak) => (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const s = (x % 4 === 0 ? -streak : 0) + (hash2i(x, y >> 1, 99) - 0.5) * 12 + (rng() * 2 - 1) * 7;
      px(x, y, clamp255(r0 + s), clamp255(g0 + s), clamp255(b0 + s));
    }
  };
  paint(g, 'oak_log_side', bark(104, 83, 49, 16));
  paint(g, 'oak_log_top', (px) => rings(px, 160, 128, 78));
  paint(g, 'birch_log_side', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const v = (rng() * 2 - 1) * 8;
      px(x, y, clamp255(216 + v), clamp255(216 + v), clamp255(206 + v));
    }
    for (let k = 0; k < 5; k++) {
      const y0 = Math.floor(rng() * 15), x0 = Math.floor(rng() * 13), w = 1 + Math.floor(rng() * 3);
      for (let i = 0; i < w; i++) px((x0 + i) % 16, y0, 58, 56, 54);
    }
  });
  paint(g, 'birch_log_top', (px) => rings(px, 206, 200, 176));
  paint(g, 'spruce_log_side', bark(84, 62, 38, 18));
  paint(g, 'spruce_log_top', (px) => rings(px, 138, 104, 66));

  const planks = (r0, g0, b0, seam) => (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      let v = (rng() * 2 - 1) * 10;
      if (y % 4 === 3) v -= seam;
      if ((x === 7 && y < 4) || (x === 12 && y >= 8 && y < 12)) v -= seam * 0.8;
      px(x, y, clamp255(r0 + v), clamp255(g0 + v), clamp255(b0 + v));
    }
  };
  paint(g, 'oak_planks', planks(162, 130, 78, 30));
  paint(g, 'birch_planks', planks(200, 190, 160, 30));
  paint(g, 'spruce_planks', planks(125, 95, 60, 30));

  const leaves = (r0, g0, b0, holeChance) => (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const n = hash2i(x, y, 123);
      if (n < holeChance) { px(x, y, 0, 0, 0, 0); continue; }
      const v = (rng() * 2 - 1) * 22 + (hash2i(x >> 1, y >> 1, 45) < 0.22 ? -26 : 0);
      px(x, y, clamp255(r0 + v), clamp255(g0 + v), clamp255(b0 + v));
    }
  };
  paint(g, 'oak_leaves', leaves(58, 122, 40, 0.10));
  paint(g, 'birch_leaves', leaves(104, 152, 62, 0.10));
  paint(g, 'spruce_leaves', leaves(48, 86, 48, 0.10));

  // ---- 透光材质 ----
  paint(g, 'glass', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const border = x === 0 || y === 0 || x === 15 || y === 15;
      const inner = (x === 1 || y === 1 || x === 14 || y === 14);
      if (border) px(x, y, 214, 238, 244, 235);
      else if (inner) px(x, y, 200, 230, 240, 90);
      else if ((x + y) % 11 === 0 && x < 8) px(x, y, 236, 250, 255, 150);
      else px(x, y, 0, 0, 0, 0);
    }
  });
  paint(g, 'water', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const w = Math.sin((x + y * 0.6) * 0.7) * 10 + (hash2i(0, y, 7) < 0.25 ? 14 : 0);
      const v = (rng() * 2 - 1) * 6 + w;
      px(x, y, clamp255(52 + v * 0.4), clamp255(110 + v), clamp255(198 + v * 0.7), 175);
    }
  });
  paint(g, 'lava', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const n = hash2i(x >> 1, y >> 1, 61);
      const v = (rng() * 2 - 1) * 20;
      let r = 200, gg = 90, b = 20;
      if (n > 0.7) { r = 245; gg = 190; b = 60; }
      else if (n < 0.18) { r = 120; gg = 30; b = 10; }
      px(x, y, clamp255(r + v), clamp255(gg + v), clamp255(b + v));
    }
  });
  paint(g, 'glowstone', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const n = hash2i(x, y, 31);
      let v = (rng() * 2 - 1) * 18;
      let r = 226, gg = 190, b = 110;
      if (n > 0.7) { r = 255; gg = 240; b = 190; v += 20; }
      else if (n < 0.2) { r = 176; gg = 140; b = 70; v -= 20; }
      px(x, y, clamp255(r + v), clamp255(gg + v), clamp255(b + v));
    }
  });

  // ---- 矿 ----
  const STONE = [128, 128, 128];
  const ore = (col, n) => (px) => blobs(px, STONE, [[0, 0, n, col]], 0);
  paint(g, 'ore_coal', ore([32, 32, 32], 5));
  paint(g, 'ore_iron', ore([206, 168, 140], 5));
  paint(g, 'ore_gold', ore([238, 202, 74], 4));
  paint(g, 'ore_diamond', ore([104, 232, 222], 4));
  paint(g, 'ore_redstone', ore([196, 32, 32], 5));
  paint(g, 'ore_lapis', ore([44, 84, 186], 4));

  // ---- 十字植物 ----
  paint(g, 'tall_grass', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 0, 0, 0, 0);
    for (let k = 0; k < 7; k++) {
      let x = 2 + Math.floor(rng() * 12);
      const h = 7 + Math.floor(rng() * 8);
      for (let i = 0; i < h; i++) {
        const y = 15 - i;
        if (rng() < 0.3) x += rng() < 0.5 ? -1 : 1;
        if (x < 0 || x > 15) break;
        const t = i / h;
        px(x, y, clamp255(70 + t * 40), clamp255(140 + t * 55), clamp255(40 + t * 30), 255);
      }
    }
  });
  const flower = (petal, center) => (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 0, 0, 0, 0);
    for (let i = 0; i < 8; i++) { const y = 15 - i; px(7 + (i > 4 ? 1 : 0), y, 60, 130, 40, 255); }
    px(7, 11, 78, 150, 48, 255); px(9, 12, 78, 150, 48, 255); px(5, 13, 78, 150, 48, 255);
    for (const [dx, dy] of [[-1, -1], [0, -2], [1, -1], [-2, 0], [2, 0], [-1, 1], [0, 2], [1, 1]])
      px(7 + dx, 6 + dy, petal[0], petal[1], petal[2], 255);
    px(7, 6, center[0], center[1], center[2], 255);
  };
  paint(g, 'dandelion', flower([238, 222, 70], [250, 246, 170]));
  paint(g, 'poppy', flower([198, 42, 42], [40, 30, 30]));
  paint(g, 'dead_bush', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 0, 0, 0, 0);
    for (let k = 0; k < 5; k++) {
      let x = 5 + Math.floor(rng() * 7);
      const h = 5 + Math.floor(rng() * 6);
      for (let i = 0; i < h; i++) {
        const y = 15 - i;
        if (rng() < 0.4) x += rng() < 0.5 ? -1 : 1;
        if (x < 0 || x > 15) break;
        px(x, y, 150, 118, 62, 255);
      }
    }
  });
  paint(g, 'sugar_cane', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 0, 0, 0, 0);
    for (let y = 0; y < 16; y++) {
      for (let x = 4; x < 12; x++) {
        const edge = (x === 4 || x === 11);
        const v = (rng() * 2 - 1) * 10 + (edge ? -26 : 0) + (y % 5 === 0 ? 16 : 0);
        px(x, y, clamp255(140 + v), clamp255(196 + v), clamp255(96 + v), 255);
      }
    }
  });
  paint(g, 'torch', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) px(x, y, 0, 0, 0, 0);
    for (let y = 5; y < 16; y++) for (let x = 7; x < 9; x++) px(x, y, 132, 96, 52, 255);
    px(6, 12, 132, 96, 52, 255); px(9, 13, 132, 96, 52, 255);
    for (const [x, y, c] of [[7, 3, [255, 236, 170]], [8, 4, [255, 214, 90]], [7, 5, [240, 150, 40]]])
      px(x, y, c[0], c[1], c[2], 255);
  });

  // ---- 仙人掌 / 南瓜 / 羊毛 ----
  paint(g, 'cactus_side', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const edge = (x === 0 || x === 15);
      const v = (rng() * 2 - 1) * 10 + (edge ? -30 : 0) + (y % 4 === 0 ? 10 : 0);
      px(x, y, clamp255(74 + v), clamp255(126 + v), clamp255(52 + v));
    }
    for (let k = 0; k < 8; k++) px(3 + Math.floor(rng() * 10), 2 + Math.floor(rng() * 12), 224, 226, 210);
  });
  paint(g, 'cactus_top', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const d = Math.max(Math.abs(x - 7.5), Math.abs(y - 7.5));
      const v = (rng() * 2 - 1) * 10 + (d > 6 ? -22 : 4);
      px(x, y, clamp255(84 + v), clamp255(138 + v), clamp255(58 + v));
    }
  });
  paint(g, 'pumpkin_side', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const rib = (x % 4 === 0) ? -26 : 0;
      const v = rib + (rng() * 2 - 1) * 10;
      px(x, y, clamp255(202 + v), clamp255(128 + v), clamp255(28 + v));
    }
  });
  paint(g, 'pumpkin_top', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const d = Math.hypot(x - 7.5, y - 7.5);
      const v = (rng() * 2 - 1) * 10 + (d < 2 ? -18 : (d > 6 ? -14 : 4));
      px(x, y, clamp255(198 + v), clamp255(126 + v), clamp255(30 + v));
    }
  });
  paint(g, 'pumpkin_face', (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const rib = (x % 4 === 0) ? -26 : 0;
      const v = rib + (rng() * 2 - 1) * 10;
      px(x, y, clamp255(202 + v), clamp255(128 + v), clamp255(28 + v));
    }
    const dark = [48, 30, 10];
    for (let x = 2; x < 6; x++) for (let y = 4; y < 7; y++) px(x, y, ...dark);   // 左眼
    for (let x = 10; x < 14; x++) for (let y = 4; y < 7; y++) px(x, y, ...dark);  // 右眼
    for (let x = 4; x < 12; x++) for (let y = 9; y < 13; y++) px(x, y, ...dark);  // 嘴
    for (let x = 4; x < 6; x++) for (let y = 12; y < 15; y++) px(x, y, ...dark);  // 嘴角
  });
  const wool = (r0, g0, b0) => (px) => {
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const v = (rng() * 2 - 1) * 9 + (hash2i(x >> 1, y >> 1, 66) < 0.2 ? -8 : 0);
      px(x, y, clamp255(r0 + v), clamp255(g0 + v), clamp255(b0 + v));
    }
  };
  paint(g, 'wool_white', wool(232, 236, 240));
  paint(g, 'wool_orange', wool(216, 122, 44));
  paint(g, 'wool_yellow', wool(228, 202, 62));
  paint(g, 'wool_red', wool(186, 52, 48));

  return canvas;
}

/**
 * 把图集画布变成可上传 GPU 的纹理。
 *
 * 这里用 DataTexture（直接喂像素数组）而不是 CanvasTexture。两者在正常桌面浏览器上
 * 等价，但走 canvas → texImage2D 这条路在某些环境（无头浏览器 + 软件光栅化、部分
 * 移动端 WebView）会拿到全黑纹理，表现是画面整个空白、且很难定位——因为几何、光照、
 * UV 全都正常，只有贴图是黑的。DataTexture 不经过画布，稳得多。
 */
export function buildAtlasTexture(canvas) {
  const ctx = canvas.getContext('2d');
  let image;
  try {
    const img = ctx.getImageData(0, 0, canvas.width, canvas.height);
    // 必须是 Uint8Array。getImageData 给的是 Uint8ClampedArray，而 WebGL 的
    // texImage2D 在 type=UNSIGNED_BYTE 时只接受 Uint8Array——传错类型会让上传
    // 静默失败，纹理保持全零，采样出来是 (0,0,0,0)。于是每个片元 alpha=0 被
    // discard 掉，整个世界一个像素都画不出来，而控制台一条错误都没有。
    image = { data: new Uint8Array(img.data.buffer, img.data.byteOffset, img.data.byteLength),
      width: img.width, height: img.height };
  } catch {
    image = canvas;   // 万一读不出来，退回画布
  }
  const tex = new THREE.DataTexture(
    image.data !== undefined ? image.data : image,
    image.width, image.height,
    THREE.RGBAFormat, THREE.UnsignedByteType,
  );
  tex.magFilter = THREE.NearestFilter;
  tex.minFilter = THREE.NearestFilter;
  tex.generateMipmaps = false;
  // 刻意不设 colorSpace = SRGBColorSpace：three 会给 sRGB 标记的纹理挑一个
  // SRGB8_ALPHA8 内部格式，采样时硬件自动解码成线性值；而自写着色器不像内置材质
  // 那样在输出前做线性→sRGB 的转换。两者一叠加，画面会明显发白。
  // 保持 NoColorSpace，采样拿到的就是原始 sRGB 字节，直接输出即所见即所得。
  tex.needsUpdate = true;
  return tex;
}

/** 破坏进度：10 级裂纹叠加图（160×16，每 16px 一级） */
export function buildBreakTexture() {
  const stages = 10;
  const c = newCanvas(stages * TILE_PX, TILE_PX);
  const g = c.getContext('2d');
  g.clearRect(0, 0, c.width, c.height);
  for (let s = 0; s < stages; s++) {
    const ox = s * TILE_PX;
    g.strokeStyle = 'rgba(0,0,0,0.72)';
    g.lineWidth = 1;
    // 分形：从中心向外的裂纹，数量随级数增长
    let seed = 1000 + s * 37;
    const rnd = () => { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return (seed >> 8) / 8388608; };
    const branches = 2 + s;
    for (let b = 0; b < branches; b++) {
      let x = 8 + (rnd() - 0.5) * 4, y = 8 + (rnd() - 0.5) * 4;
      let a = rnd() * Math.PI * 2;
      const segs = 3 + Math.floor(rnd() * 4);
      g.beginPath();
      g.moveTo(ox + x, y);
      for (let i = 0; i < segs; i++) {
        a += (rnd() - 0.5) * 1.5;
        const len = 1.5 + rnd() * 2.5;
        x += Math.cos(a) * len; y += Math.sin(a) * len;
        g.lineTo(ox + x, y);
      }
      g.stroke();
    }
    if (s >= 4) {
      g.strokeStyle = 'rgba(0,0,0,0.5)';
      for (let b = 0; b < branches - 2; b++) {
        let x = 8 + (rnd() - 0.5) * 6, y = 8 + (rnd() - 0.5) * 6;
        let a = rnd() * Math.PI * 2;
        g.beginPath(); g.moveTo(ox + x, y);
        for (let i = 0; i < 3; i++) {
          a += (rnd() - 0.5) * 2;
          x += Math.cos(a) * 2.2; y += Math.sin(a) * 2.2;
          g.lineTo(ox + x, y);
        }
        g.stroke();
      }
    }
  }
  return c;
}

/**
 * 工具图标。kind: pickaxe | axe | shovel
 * 材质配色按等级给，和原版一个思路。
 */
export function drawToolIcon(kind, tier) {
  const PALETTE = [
    { wood: [150, 110, 66], head: [162, 130, 78], dark: [96, 68, 40] },   // 0 木
    { wood: [110, 82, 52], head: [128, 128, 128], dark: [72, 72, 72] },    // 1 石
    { wood: [110, 82, 52], head: [216, 216, 216], dark: [140, 140, 140] },  // 2 铁
    { wood: [110, 82, 52], head: [110, 226, 216], dark: [56, 168, 160] },   // 3 钻石
    { wood: [110, 82, 52], head: [206, 150, 210], dark: [130, 84, 136] },   // 4 下界合金
  ];
  const P = PALETTE[Math.min(tier, PALETTE.length - 1)];
  const c = newCanvas(16, 16);
  const g = c.getContext('2d');
  g.clearRect(0, 0, 16, 16);
  const put = (x, y, col, a = 1) => {
    g.fillStyle = `rgba(${col[0]},${col[1]},${col[2]},${a})`;
    g.fillRect(x, y, 1, 1);
  };
  // 木柄：从右下角斜向左上
  for (let i = 0; i < 9; i++) put(11 - i, 14 - i, P.wood);
  for (let i = 0; i < 9; i++) put(12 - i, 14 - i, P.dark, 0.55);
  if (kind === 'pickaxe') {
    for (let x = 2; x < 12; x++) put(x, 3 + Math.round(Math.abs(x - 6) * 0.16), P.head);
    for (let x = 2; x < 12; x++) put(x, 4 + Math.round(Math.abs(x - 6) * 0.16), P.dark);
    for (let x = 3; x < 6; x++) put(x, 2 + Math.round(Math.abs(x - 6) * 0.16), P.head);
    for (let x = 8; x < 11; x++) put(x, 2 + Math.round(Math.abs(x - 6) * 0.16), P.head);
  } else if (kind === 'axe') {
    for (let y = 2; y < 9; y++) {
      const w = y < 4 ? 2 : (y < 7 ? 4 : 2);
      for (let x = 0; x < w; x++) put(4 + x, y, x === 0 ? P.dark : P.head);
    }
    for (let y = 3; y < 8; y++) put(3, y, P.head);
  } else {
    for (let y = 1; y < 6; y++) { put(4, y, P.head); put(5, y, y < 5 ? P.dark : P.head); }
    for (let x = 4; x < 7; x++) put(x, 6, P.head);
  }
  return c;
}

export { TILE };
