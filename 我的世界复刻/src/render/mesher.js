// 区块网格化。
//
// 顶点属性刻意做成「平铺 UV + 槽位号」的形式：贪心合并出来的四边形 UV 可以大于 1，
// 由片元着色器用 fract 重新映射回 16×16 图集里的那一格。这样既保住贪心合并带来的
// 三角形数量优势，又只需要一张普通 2D 图集，不必上 sampler2DArray。
//
// 合并粒度是「面完全一致」：贴图、4 个角的 AO、4 个角的灯光、所在群系染色全都要
// 相同才合并。合并之后每个四边形内部仍保留 AO 渐变，只有跨方块才有光照差——
// 观感和逐面渲染几乎一致，三角形数通常能降一个数量级。
//
// 纯计算，不 import three.js，可在 Node 下直接跑。

import {
  CHUNK_X, CHUNK_Z, WORLD_H, CHUNK_AREA, SEA_LEVEL, IDX,
} from '../core/constants.js';
import { B, BLOCKS, IS_OPAQUE, RENDER_KIND, faceVisible } from '../core/blocks.js';
import { BIOMES, isTintable } from '../world/biomes.js';

// 每个面满足 u × v = n（保证从外侧看是逆时针缠绕）
// p0 是「u 那一端 / v 那一端」的角点，偏移是单位方块内的 0/1 坐标
// 侧面一律让 v 指向 -Y，这样贴图的纵轴在世界坐标里就是竖直的
//
// su / sv 这两个字段很关键：u、v 未必沿着它们所属坐标轴的正方向走（-X 面的 u 是
// -Z，+Z 面的 u 是 -X）。贪心合并是沿坐标轴正方向推进的，当 u 指向负方向时，四边形
// 的几何起点必须挪到这一串的最外侧那一格，否则合并出来的面会整体偏移 (w-1) 格。
// su/sv 用来告诉 AO 和顶点序号「哪个角对应正方向」。
const FACES = [
  { n: [1, 0, 0], p0: [1, 1, 0], u: [0, 0, 1], v: [0, -1, 0], a: 0, ua: 2, va: 1, shade: 0.62, tileSlot: 2, su: -1, sv: 1 },
  { n: [-1, 0, 0], p0: [0, 1, 1], u: [0, 0, -1], v: [0, -1, 0], a: 0, ua: 2, va: 1, shade: 0.62, tileSlot: 2, su: 1, sv: 1 },
  { n: [0, 1, 0], p0: [0, 1, 1], u: [1, 0, 0], v: [0, 0, -1], a: 1, ua: 0, va: 2, shade: 1.0, tileSlot: 0, su: -1, sv: 1 },
  { n: [0, -1, 0], p0: [0, 0, 0], u: [1, 0, 0], v: [0, 0, 1], a: 1, ua: 0, va: 2, shade: 0.5, tileSlot: 1, su: -1, sv: -1 },
  { n: [0, 0, 1], p0: [1, 1, 1], u: [-1, 0, 0], v: [0, -1, 0], a: 2, ua: 0, va: 1, shade: 0.8, tileSlot: 2, su: 1, sv: 1 },
  { n: [0, 0, -1], p0: [0, 1, 0], u: [1, 0, 0], v: [0, -1, 0], a: 2, ua: 0, va: 1, shade: 0.8, tileSlot: 2, su: -1, sv: 1 },
];

const AXIS_SIZE = [CHUNK_X, WORLD_H, CHUNK_Z];
const MAX_PLANE = CHUNK_X * WORLD_H;

// 合并键的位布局（keyA，共 28 位）
//   0..7   贴图槽位
//   8..11  群系 id
//   12..19 4 个角的 AO（每角 2 位）
//   20     是否需要群系染色（草/叶）。只有它参与合并即可——染什么颜色由群系 id
//          唯一决定；不给草叶之外的东西染色，否则石块会被草原染成绿的。
// keyB：低 16 位 = 4 个角的天光，高 16 位 = 4 个角的方块光

export class MeshBuffer {
  constructor() {
    this.pos = []; this.uv = []; this.tile = [];
    this.shade = []; this.sky = []; this.blk = []; this.tint = [];
    this.idx = [];
  }
  get empty() { return this.idx.length === 0; }
  clear() {
    this.pos.length = 0; this.uv.length = 0; this.tile.length = 0;
    this.shade.length = 0; this.sky.length = 0; this.blk.length = 0;
    this.tint.length = 0; this.idx.length = 0;
  }
  toGeometry() {
    const n = this.pos.length / 3;
    return {
      position: new Float32Array(this.pos),
      uv: new Float32Array(this.uv),
      tile: new Float32Array(this.tile),
      shade: new Float32Array(this.shade),
      skylight: new Float32Array(this.sky),
      blocklight: new Float32Array(this.blk),
      tint: new Float32Array(this.tint),
      index: n > 65535 ? new Uint32Array(this.idx) : new Uint16Array(this.idx),
    };
  }
}

export class Mesher {
  constructor() {
    const n = MAX_PLANE + 64;
    this.used = new Uint8Array(n);
    this.keyA = new Int32Array(n);
    this.keyB = new Int32Array(n);
  }

  /** @returns {{opaque:MeshBuffer, cutout:MeshBuffer, liquid:MeshBuffer}} */
  mesh(world, chunk) {
    const opaque = new MeshBuffer();
    const cutout = new MeshBuffer();
    const liquid = new MeshBuffer();
    const ox = chunk.cx * CHUNK_X, oz = chunk.cz * CHUNK_Z;
    const yLimit = Math.min(WORLD_H - 1, Math.max(chunk.maxY, SEA_LEVEL) + 1);
    const blocks = chunk.blocks;

    for (let d = 0; d < 6; d++) {
      const F = FACES[d];
      const sizeA = AXIS_SIZE[F.a];
      const SU = AXIS_SIZE[F.ua];
      const SV = AXIS_SIZE[F.va];
      const planeN = SU * SV;
      const fx = F.ua, fy = F.va;

      for (let la = 0; la < sizeA; la++) {
        if (F.a === 1 && la > yLimit) break;
        this.used.fill(0, 0, planeN);

        // ---------- 1. 填掩码 ----------
        for (let iu = 0; iu < SU; iu++) {
          for (let iv = 0; iv < SV; iv++) {
            const p = iu * SV + iv;
            const coord = [0, 0, 0];
            coord[F.a] = la; coord[fx] = iu; coord[fy] = iv;
            const cx = coord[0], cy = coord[1], cz = coord[2];
            const id = blocks[IDX(cx, cy, cz)];
            if (id === B.air) continue;
            const kind = RENDER_KIND[id];
            if (kind === 1) continue;              // 十字植物另行处理

            const wx = ox + cx, wy = cy, wz = oz + cz;
            const nx = wx + F.n[0], ny = wy + F.n[1], nz = wz + F.n[2];
            // 邻接区块尚未加载：先不画这一面，等它加载完会标记本区块重建
            if (!world.chunkAt(nx, nz)) continue;
            if (!faceVisible(id, world.getBlock(nx, ny, nz))) continue;

            // ---- AO：4 个角 ----
            let aoCode = 0;
            for (let ci = 0; ci < 4; ci++) {
              const su = ((ci === 0 || ci === 3) ? -1 : 1) * F.su;
              const sv = ((ci < 2) ? -1 : 1) * F.sv;
              const o1 = [0, 0, 0]; o1[fx] = su;
              const o2 = [0, 0, 0]; o2[fy] = sv;
              const s1 = IS_OPAQUE[world.getBlock(nx + o1[0], ny + o1[1], nz + o1[2])] ? 1 : 0;
              const s2 = IS_OPAQUE[world.getBlock(nx + o2[0], ny + o2[1], nz + o2[2])] ? 1 : 0;
              const cc = IS_OPAQUE[world.getBlock(nx + o1[0] + o2[0], ny + o1[1] + o2[1], nz + o1[2] + o2[2])] ? 1 : 0;
              aoCode |= ((s1 && s2) ? 0 : 3 - (s1 + s2 + cc)) << (ci * 2);
            }

            // ---- 光照 ----
            const frontSky = world.getSky(nx, ny, nz);
            const frontBlk = world.getBlockLight(nx, ny, nz);
            let sky, blk;
            if (frontSky === 15 && frontBlk === 0) {
              sky = 0xffff; blk = 0;              // 满日照，没必要逐角采样
            } else {
              sky = 0; blk = 0;
              for (let ci = 0; ci < 4; ci++) {
                const su = ((ci === 0 || ci === 3) ? -1 : 1) * F.su;
                const sv = ((ci < 2) ? -1 : 1) * F.sv;
                const o1 = [0, 0, 0]; o1[fx] = su;
                const o2 = [0, 0, 0]; o2[fy] = sv;
                const px1 = nx + o1[0], py1 = ny + o1[1], pz1 = nz + o1[2];
                const px2 = nx + o2[0], py2 = ny + o2[1], pz2 = nz + o2[2];
                const px3 = px1 + o2[0], py3 = py1 + o2[1], pz3 = pz1 + o2[2];
                let ss = 0, sb = 0, cnt = 0;
                if (!IS_OPAQUE[world.getBlock(nx, ny, nz)]) { ss += frontSky; sb += frontBlk; cnt++; }
                if (!IS_OPAQUE[world.getBlock(px1, py1, pz1)]) { ss += world.getSky(px1, py1, pz1); sb += world.getBlockLight(px1, py1, pz1); cnt++; }
                if (!IS_OPAQUE[world.getBlock(px2, py2, pz2)]) { ss += world.getSky(px2, py2, pz2); sb += world.getBlockLight(px2, py2, pz2); cnt++; }
                if (!IS_OPAQUE[world.getBlock(px3, py3, pz3)]) { ss += world.getSky(px3, py3, pz3); sb += world.getBlockLight(px3, py3, pz3); cnt++; }
                if (cnt === 0) cnt = 1;             // 全被挡死，当作光源本身
                sky |= (((ss / cnt) | 0) & 15) << (ci * 4);
                blk |= (((sb / cnt) | 0) & 15) << (ci * 4);
              }
            }

            const bio = world.getBiome(wx, wz);
            this.used[p] = 1;
            this.keyA[p] = (BLOCKS[id].tiles[F.tileSlot] & 0xff) |
              ((bio & 15) << 8) | (aoCode << 12) |
              (isTintable(id) ? (1 << 20) : 0);
            this.keyB[p] = sky | (blk << 16);
          }
        }

        // ---------- 2. 贪心合并 ----------
        for (let iu = 0; iu < SU; iu++) {
          for (let iv = 0; iv < SV; iv++) {
            const p = iu * SV + iv;
            if (!this.used[p]) continue;
            const ka = this.keyA[p], kb = this.keyB[p];

            let w = 1;
            while (iu + w < SU) {
              const q = (iu + w) * SV + iv;
              if (!this.used[q] || this.keyA[q] !== ka || this.keyB[q] !== kb) break;
              w++;
            }
            let h = 1;
            grow: while (iv + h < SV) {
              for (let k = 0; k < w; k++) {
                const q = (iu + k) * SV + (iv + h);
                if (!this.used[q] || this.keyA[q] !== ka || this.keyB[q] !== kb) break grow;
              }
              h++;
            }
            for (let k = 0; k < w; k++) {
              for (let m = 0; m < h; m++) this.used[(iu + k) * SV + (iv + m)] = 0;
            }

            // 几何起点：u / v 指向负方向的那一轴，要从这串的最外侧一格起算
            const coord = [0, 0, 0];
            coord[F.a] = la;
            coord[fx] = F.su > 0 ? iu + w - 1 : iu;
            coord[fy] = F.sv > 0 ? iv + h - 1 : iv;
            const id = blocks[IDX(coord[0], coord[1], coord[2])];
            const buf = RENDER_KIND[id] === 2 ? liquid : (BLOCKS[id].opaque ? opaque : cutout);
            this.emitQuad(buf, F, ox + coord[0], coord[1], oz + coord[2], w, h, ka, kb);
          }
        }
      }
    }

    // ---------- 3. 十字植物 ----------
    this.emitCrosses(world, chunk, cutout, ox, oz, yLimit);

    return { opaque, cutout, liquid };
  }

  emitQuad(buf, F, wx, wy, wz, w, h, ka, kb) {
    const tile = ka & 0xff;
    const bio = (ka >> 8) & 15;
    const aoCode = (ka >> 12) & 0xff;
    const sky = kb & 0xffff, blk = (kb >> 16) & 0xffff;

    const p0x = wx + F.p0[0], p0y = wy + F.p0[1], p0z = wz + F.p0[2];
    const ux = F.u[0], uy = F.u[1], uz = F.u[2];
    const vx = F.v[0], vy = F.v[1], vz = F.v[2];
    const base = buf.pos.length / 3;

    const corners = [
      [p0x, p0y, p0z, 0, 0],
      [p0x + ux * w, p0y + uy * w, p0z + uz * w, w, 0],
      [p0x + ux * w + vx * h, p0y + uy * w + vy * h, p0z + uz * w + vz * h, w, h],
      [p0x + vx * h, p0y + vy * h, p0z + vz * h, 0, h],
    ];
    const td = BIOMES[bio] || BIOMES[3];
    // 只有草/叶类才吃群系染色，其余方块保持原色
    const tintOn = (ka >> 20) & 1;
    const tr = tintOn ? td.tint[0] / 255 : 1;
    const tg = tintOn ? td.tint[1] / 255 : 1;
    const tb = tintOn ? td.tint[2] / 255 : 1;

    for (let c = 0; c < 4; c++) {
      const cr = corners[c];
      buf.pos.push(cr[0], cr[1], cr[2]);
      buf.uv.push(cr[3], cr[4]);
      buf.tile.push(tile);
      const ao = (aoCode >> (c * 2)) & 3;
      buf.shade.push((0.42 + ao * 0.194) * F.shade);
      buf.sky.push(((sky >> (c * 4)) & 15) / 15);
      buf.blk.push(((blk >> (c * 4)) & 15) / 15);
      buf.tint.push(tr, tg, tb);
    }
    // 三角形缠绕。顶点顺序 0→1→2→3 从外侧看是逆时针，所以合法的两种剖分是：
    //   对角线 0-2： (0,1,2) + (0,2,3)
    //   对角线 1-3： (0,1,3) + (1,2,3)
    // 第二个三角形写成 (0,3,2) 或 (2,1,3) 会把绕向翻过来，被背面剔除吃掉半张面——
    // 表现是地面上出现一条条纹，而不是完全看不见，很容易被误判成别的问题。
    const a0 = aoCode & 3, a1 = (aoCode >> 2) & 3, a2 = (aoCode >> 4) & 3, a3 = (aoCode >> 6) & 3;
    if (a0 + a2 > a1 + a3) buf.idx.push(base, base + 1, base + 3, base + 1, base + 2, base + 3);
    else buf.idx.push(base, base + 1, base + 2, base, base + 2, base + 3);
  }

  /** 花草这类十字交叉方块：两个对角面，双面材质渲染，不参与合并 */
  emitCrosses(world, chunk, buf, ox, oz, yLimit) {
    for (let y = 1; y <= yLimit; y++) {
      for (let z = 0; z < CHUNK_Z; z++) {
        for (let x = 0; x < CHUNK_X; x++) {
          const id = chunk.blocks[IDX(x, y, z)];
          if (RENDER_KIND[id] !== 1) continue;
          const wx = ox + x, wz = oz + z;
          const sky = world.getSky(wx, y, wz) / 15;
          const blk = world.getBlockLight(wx, y, wz) / 15;
          const tint = BIOMES[world.getBiome(wx, wz)].tint;
          const tile = BLOCKS[id].tiles[2];
          const cr = BLOCKS[id].cross;                 // [半宽, 高度]，火把比花草细短
          const halfW = cr ? cr[0] : 0.4;
          const top = y + (cr ? cr[1] : 0.92);
          // 轻微随机偏移，免得一片花草排成整齐方阵。偏移量必须小到四边形的角点
          // 仍然落在本格内，否则网格会伸出区块边界，和邻接区块的植物打架。
          const jx = ((Math.imul(wx, 0x9E3779B1) >>> 24) / 255 - 0.5) * (1 - halfW * 2) * 0.5;
          const jz = ((Math.imul(wz, 0x85EBCA77) >>> 24) / 255 - 0.5) * (1 - halfW * 2) * 0.5;
          const cx = wx + 0.5 + jx, cz = wz + 0.5 + jz;
          this.crossQuad(buf, cx, y, cz, top, halfW, tile, sky, blk, tint, false);
          this.crossQuad(buf, cx, y, cz, top, halfW, tile, sky, blk, tint, true);
        }
      }
    }
  }

  crossQuad(buf, cx, y, cz, top, d, tile, sky, blk, tint, swap) {
    const x0 = cx - d, x1 = cx + d, z0 = cz - d, z1 = cz + d;
    const base = buf.pos.length / 3;
    const pts = swap
      ? [[x0, y, z1], [x1, y, z0], [x1, top, z0], [x0, top, z1]]
      : [[x0, y, z0], [x1, y, z1], [x1, top, z1], [x0, top, z0]];
    const uvs = [[0, 0], [1, 0], [1, 1], [0, 1]];
    for (let c = 0; c < 4; c++) {
      buf.pos.push(pts[c][0], pts[c][1], pts[c][2]);
      buf.uv.push(uvs[c][0], uvs[c][1]);
      buf.tile.push(tile);
      buf.shade.push(1);
      buf.sky.push(sky);
      buf.blk.push(blk);
      buf.tint.push(tint[0] / 255, tint[1] / 255, tint[2] / 255);
    }
    buf.idx.push(base, base + 1, base + 2, base, base + 2, base + 3);
  }
}
