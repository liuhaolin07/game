// 掉落物：破坏方块后掉出来的小方块，飘一会儿再被玩家吸走。
//
// 渲染用 InstancedMesh + 一个 per-instance 的 aTile 属性。之所以不能直接用普通材质：
// 实例共用一份几何体和一份材质，UV 也共用，整个 16×16 图集会被糊到每一个小方块上。
// 自己写一小段着色器把 UV 映射到「本格的贴图」最省事。

import * as THREE from 'three';
import { B } from '../core/blocks.js';
import { BLOCKS } from '../core/blocks.js';
import { rollDrop } from '../core/items.js';
import { SEA_LEVEL } from '../core/constants.js';
import * as sfx from './sounds.js';

const MAX_DROPS = 64;
const LIFETIME = 300;
const PICKUP_RANGE = 1.6;

const VERT = /* glsl */`
attribute float aTile;
varying vec2 vUv;
varying float vTile;
varying float vLocalY;
varying float vFogDepth;
void main() {
  vUv = uv;
  vTile = aTile;
  // 只把「局部法线的 Y 分量」传下去。掉落物是 InstancedMesh，实例自带的旋转
  // 不会反映到 normalMatrix 里，变换法线反而会算错；顶面/侧面只要区分这两者。
  vLocalY = normal.y;
  vec4 mv = modelViewMatrix * instanceMatrix * vec4(position, 1.0);
  vFogDepth = -mv.z;
  gl_Position = projectionMatrix * mv;
}
`;

const FRAG = /* glsl */`
precision mediump float;
uniform sampler2D uAtlas;
uniform float uLight;
varying vec2 vUv;
varying float vTile;
varying float vLocalY;
varying float vFogDepth;
void main() {
  vec2 t = vec2(mod(vTile, 16.0), floor(vTile / 16.0));
  vec4 c = texture2D(uAtlas, (t + vUv) / 16.0);
  if (c.a < 0.4) discard;
  // 六个面给一点点方向明暗，避免小方块看起来是张贴纸
  float d = 0.82 + 0.18 * abs(vLocalY);
  gl_FragColor = vec4(c.rgb * uLight * d, 1.0);
}
`;

export class Drops {
  constructor(scene, world, atlasTexture, materials) {
    this.world = world;
    this.items = [];

    const geo = new THREE.BoxGeometry(0.28, 0.28, 0.28);
    this.tileAttr = new THREE.InstancedBufferAttribute(new Float32Array(MAX_DROPS), 1);
    this.tileAttr.setUsage(THREE.DynamicDrawUsage);
    geo.setAttribute('aTile', this.tileAttr);

    // 直接复用主图集那张纹理（已经是 DataTexture，见 atlas.js 的说明），
    // 不再单独上传一份画布。aTile 属性负责把实例映射到各自的那一格。
    const atlas = atlasTexture;

    this.mat = new THREE.ShaderMaterial({
      uniforms: { uAtlas: { value: atlas }, uLight: { value: 1 } },
      vertexShader: VERT,
      fragmentShader: FRAG,
    });
    this.mesh = new THREE.InstancedMesh(geo, this.mat, MAX_DROPS);
    this.mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
    this.mesh.count = 0;
    this.mesh.frustumCulled = false;
    scene.add(this.mesh);

    this._m = new THREE.Matrix4();
    this._q = new THREE.Quaternion();
    this._e = new THREE.Euler();
    this._v = new THREE.Vector3();
    this._s = new THREE.Vector3(1, 1, 1);
    this.materials = materials;   // 只为保留引用，uniform 改动见 sync()
  }

  spawn(x, y, z, blockId, toolId) {
    if (this.items.length >= MAX_DROPS) this.items.shift();
    const id = rollDrop(blockId, toolId);
    if (id === null) return null;
    this.items.push({
      id,
      x: x + 0.5 + (Math.random() - 0.5) * 0.3,
      y: y + 0.35,
      z: z + 0.5 + (Math.random() - 0.5) * 0.3,
      vx: (Math.random() - 0.5) * 1.6,
      vy: 2.2 + Math.random() * 1.2,
      vz: (Math.random() - 0.5) * 1.6,
      age: 0,
      spin: Math.random() * Math.PI * 2,
    });
    return id;
  }

  update(dt, player, inventory, dayLight) {
    const w = this.world;
    let picked = 0;

    for (let i = this.items.length - 1; i >= 0; i--) {
      const it = this.items[i];
      it.age += dt;
      if (it.age > LIFETIME) { this.items.splice(i, 1); continue; }
      if (w.getBlock(Math.floor(it.x), Math.floor(it.y), Math.floor(it.z)) === B.lava) {
        this.items.splice(i, 1); continue;
      }

      // 简单的弹跳 + 水里漂浮
      it.vy -= 20 * dt;
      if (w.getBlock(Math.floor(it.x), Math.floor(it.y), Math.floor(it.z)) === B.water) {
        it.vy = Math.min(it.vy + 34 * dt, 1.6);
        it.vx *= 0.94; it.vz *= 0.94;
      }

      const nx = it.x + it.vx * dt;
      if (w.isSolidAt(Math.floor(nx), Math.floor(it.y), Math.floor(it.z))) it.vx = 0;
      else it.x = nx;
      const nz = it.z + it.vz * dt;
      if (w.isSolidAt(Math.floor(it.x), Math.floor(it.y), Math.floor(nz))) it.vz = 0;
      else it.z = nz;
      const ny = it.y + it.vy * dt;
      if (w.isSolidAt(Math.floor(it.x), Math.floor(ny), Math.floor(it.z))) {
        if (it.vy < 0) { it.y = Math.floor(ny) + 1.01; it.vy = 0; }
      } else it.y = ny;
      if (it.y < SEA_LEVEL - 24) { this.items.splice(i, 1); continue; }

      it.spin += dt * 1.6;

      // 靠近玩家就被吸走
      const dx = player.x - it.x;
      const dy = (player.y + 0.9) - it.y;
      const dz = player.z - it.z;
      const d2 = dx * dx + dy * dy + dz * dz;
      if (d2 < PICKUP_RANGE * PICKUP_RANGE && d2 > 1e-6) {
        const d = Math.sqrt(d2);
        const pull = 9 * dt;
        it.x += (dx / d) * pull; it.y += (dy / d) * pull; it.z += (dz / d) * pull;
        if (inventory.add(it.id, 1) > 0) { picked++; this.items.splice(i, 1); }
      }
    }

    if (picked > 0) sfx.playPickup();
    this.sync(dayLight);
  }

  /** 把逻辑状态写进 InstancedMesh */
  sync(dayLight = 1) {
    const n = Math.min(this.items.length, MAX_DROPS);
    this.mat.uniforms.uLight.value = 0.25 + 0.75 * dayLight;
    this.mat.uniformsNeedUpdate = true;   // ShaderMaterial 必须显式声明 uniform 改动
    for (let i = 0; i < n; i++) {
      const it = this.items[i];
      this._v.set(it.x, it.y + 0.14 + Math.sin(it.age * 2.4) * 0.05, it.z);
      this._e.set(0.42, it.spin, 0);
      this._q.setFromEuler(this._e);
      this._m.compose(this._v, this._q, this._s);
      this.mesh.setMatrixAt(i, this._m);
      const def = BLOCKS[it.id];
      this.tileAttr.array[i] = def ? def.tiles[2] : 0;
    }
    this.mesh.count = n;
    if (n > 0) {
      this.mesh.instanceMatrix.needsUpdate = true;
      this.tileAttr.needsUpdate = true;
    }
  }
}
