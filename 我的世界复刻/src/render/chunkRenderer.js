// 区块渲染管理：按视距加载/卸载区块，把网格化结果变成 three.js Mesh。
//
// 每帧的处理分成三档，共享一个时间预算：
//   1. 生成地形（最贵，放 Web Worker 之外也要省）
//   2. 光照传播
//   3. 网格化
// 全部走「每帧最多 N 个」的配额，避免玩家移动时集中卡顿。

import * as THREE from 'three';
import { CHUNK_X, CHUNK_Z, RENDER_DISTANCE } from '../core/constants.js';
import { chunkKey } from '../world/world.js';
import { Mesher } from './mesher.js';

export class ChunkRenderer {
  constructor(world, light, materials, scene) {
    this.world = world;
    this.light = light;
    this.materials = materials;
    this.scene = scene;
    this.mesher = new Mesher();
    this.group = new THREE.Group();
    scene.add(this.group);
    this.renderDistance = RENDER_DISTANCE;
    this.stats = { meshed: 0, generated: 0, pendingMesh: 0 };
  }

  setRenderDistance(r) {
    const v = Math.max(2, Math.min(16, r | 0));
    if (v === this.renderDistance) return v;
    this.renderDistance = v;
    return v;
  }

  /** 把 Buffer 变成 Mesh 并挂到场景上；空 Buffer 返回 null */
  makeMesh(buf, material) {
    if (buf.empty) return null;
    // 必须用 toGeometry()。three 的 BufferAttribute 只接受 TypedArray，
    // 直接把 MeshBuffer 里的普通 JS 数组塞进去会在运行时报
    // "array should be a Typed Array"，而且是一个方块都渲染不出来的那种失败。
    const geo = new THREE.BufferGeometry();
    const d = buf.toGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(d.position, 3));
    geo.setAttribute('uv', new THREE.BufferAttribute(d.uv, 2));
    geo.setAttribute('tile', new THREE.BufferAttribute(d.tile, 1));
    geo.setAttribute('shade', new THREE.BufferAttribute(d.shade, 1));
    geo.setAttribute('skylight', new THREE.BufferAttribute(d.skylight, 1));
    geo.setAttribute('blocklight', new THREE.BufferAttribute(d.blocklight, 1));
    geo.setAttribute('tint', new THREE.BufferAttribute(d.tint, 3));
    geo.setIndex(new THREE.BufferAttribute(d.index, 1));
    geo.computeBoundingSphere();
    const m = new THREE.Mesh(geo, material);
    m.matrixAutoUpdate = false;
    m.updateMatrix();
    return m;
  }

  disposeChunk(chunk) {
    if (!chunk.meshes) return;
    for (const m of chunk.meshes) {
      this.group.remove(m);
      m.geometry.dispose();
    }
    chunk.meshes = null;
  }

  buildMesh(chunk) {
    this.disposeChunk(chunk);
    const r = this.mesher.mesh(this.world, chunk);
    const meshes = [
      this.makeMesh(r.opaque, this.materials.opaque),
      this.makeMesh(r.cutout, this.materials.cutout),
      this.makeMesh(r.liquid, this.materials.liquid),
    ].filter(Boolean);
    for (const m of meshes) this.group.add(m);
    chunk.meshes = meshes;
    chunk.meshDirty = false;
    this.stats.meshed++;
  }

  /**
   * 围绕玩家做一次增量更新。
   *
   * 预算按「毫秒」而不是「个数」：单个区块的地形生成约 10ms、光照初始化约 6ms、
   * 网格化中位数约 10ms（最慢能到 67ms）。按个数限流在慢机器上会直接掉帧，
   * 按时间限流则不管机器快慢，每帧的额外开销都有上限。
   *
   * @param cx,cz 玩家所在区块
   * @param msBudget 本帧允许花在流式加载上的毫秒数
   */
  update(cx, cz, msBudget = 12) {
    const now = () => (typeof performance !== 'undefined' ? performance.now() : Date.now());
    const deadline = now() + msBudget;
    const R = this.renderDistance;
    const R2 = (R + 0.5) * (R + 0.5);
    const world = this.world;

    // --- 1. 卸载视野外的区块 ---
    for (const [k, chunk] of world.chunks) {
      const dx = chunk.cx - cx, dz = chunk.cz - cz;
      if (dx * dx + dz * dz > R2) {
        this.disposeChunk(chunk);
        world.chunks.delete(k);
      }
    }

    // --- 2. 按由近及远生成缺失区块 ---
    const wanted = [];
    for (let dx = -R; dx <= R; dx++) {
      for (let dz = -R; dz <= R; dz++) {
        const d2 = dx * dx + dz * dz;
        if (d2 > R2) continue;
        wanted.push([cx + dx, cz + dz, d2]);
      }
    }
    wanted.sort((a, b) => a[2] - b[2]);

    for (const [wx, wz] of wanted) {
      if (now() >= deadline) break;
      if (world.hasChunk(wx, wz)) continue;
      const chunk = world.ensureChunk(wx, wz);
      this.light.initChunk(chunk);
      // 新区块一出现，四个邻居那侧的边界面之前被跳过了（当时邻块还没加载），
      // 必须让它们重建一次。这里必须做成「事件驱动」：如果每帧去扫一圈并标脏，
      // 整个世界会被无限重建。
      for (const [nx, nz] of [[wx - 1, wz], [wx + 1, wz], [wx, wz - 1], [wx, wz + 1]]) {
        const n = world.chunks.get(chunkKey(nx, nz));
        if (n && n.meshes) n.meshDirty = true;
      }
      this.stats.generated++;
    }

    // --- 3. 光照传播（共享预算） ---
    this.light.propagate(40000);

    // --- 4. 网格化：先补没网格的，再重建被改脏的 ---
    const list = [];
    for (const [wx, wz, d2] of wanted) {
      const chunk = world.chunks.get(chunkKey(wx, wz));
      if (!chunk) continue;
      if (!chunk.meshes) { list.push([chunk, d2]); continue; }
      if (chunk.meshDirty) list.push([chunk, -1]);   // 改脏的排最前
    }
    list.sort((a, b) => a[1] - b[1]);

    let meshed = 0;
    for (const [chunk] of list) {
      if (now() >= deadline) break;
      this.buildMesh(chunk);
      meshed++;
    }

    this.stats.pendingMesh = list.length;
    return meshed;
  }

  /** 玩家改动方块后，标记受影响的区块（含跨边界的邻居） */
  markDirtyAround(x, y, z) {
    const c = this.world.chunkAt(x, z);
    if (c) c.meshDirty = true;
    const lx = x & 15, lz = z & 15;
    const marks = [];
    if (lx === 0) marks.push([x - 1, z]);
    if (lx === CHUNK_X - 1) marks.push([x + 1, z]);
    if (lz === 0) marks.push([x, z - 1]);
    if (lz === CHUNK_Z - 1) marks.push([x, z + 1]);
    for (const [mx, mz] of marks) {
      const n = this.world.chunks.get(chunkKey(mx >> 4, mz >> 4));
      if (n) n.meshDirty = true;
    }
  }

  dispose() {
    for (const chunk of this.world.chunks.values()) this.disposeChunk(chunk);
    this.scene.remove(this.group);
  }
}
