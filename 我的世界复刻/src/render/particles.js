// 粒子：破坏方块时迸出的碎屑。
// 用一个共享的 Points + 自写着色器，一次 draw call 画完所有碎屑。

import * as THREE from 'three';

const VERT = /* glsl */`
attribute float size;
attribute vec3 tint;
uniform float uDayLight;
varying vec3 vColor;
varying float vAlpha;
void main() {
  vColor = tint * (0.45 + 0.55 * uDayLight);
  vec4 mv = modelViewMatrix * vec4(position, 1.0);
  // 距离越远越小，但保持一个最小值，避免远处看不见
  gl_PointSize = max(1.0, size * (140.0 / -mv.z));
  vAlpha = clamp(1.0 - (-mv.z - 90.0) / 40.0, 0.0, 1.0);
  gl_Position = projectionMatrix * mv;
}
`;

const FRAG = /* glsl */`
precision mediump float;
varying vec3 vColor;
varying float vAlpha;
void main() {
  vec2 d = gl_PointCoord - 0.5;
  if (dot(d, d) > 0.25) discard;
  gl_FragColor = vec4(vColor, vAlpha);
}
`;

const MAX = 900;

export class Particles {
  constructor(scene) {
    this.n = 0;
    this.pos = new Float32Array(MAX * 3);
    this.vel = new Float32Array(MAX * 3);
    this.life = new Float32Array(MAX);
    this.maxLife = new Float32Array(MAX);
    this.size = new Float32Array(MAX);
    this.tint = new Float32Array(MAX * 3);

    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(this.pos, 3));
    g.setAttribute('size', new THREE.BufferAttribute(this.size, 1));
    g.setAttribute('tint', new THREE.BufferAttribute(this.tint, 3));
    g.setDrawRange(0, 0);
    this.geo = g;

    this.mat = new THREE.ShaderMaterial({
      uniforms: { uDayLight: { value: 1 } },
      vertexShader: VERT,
      fragmentShader: FRAG,
      transparent: true,
      depthWrite: false,
    });
    this.points = new THREE.Points(g, this.mat);
    this.points.frustumCulled = false;
    this.points.renderOrder = 5;
    scene.add(this.points);
  }

  /** 在方块位置炸出一簇碎屑 */
  burst(x, y, z, color, count = 14, spread = 0.28) {
    for (let i = 0; i < count; i++) {
      const s = this.n < MAX ? this.n++ : (this.oldest());
      const i3 = s * 3;
      this.pos[i3] = x + (Math.random() - 0.5) * 0.9;
      this.pos[i3 + 1] = y + (Math.random() - 0.5) * 0.9;
      this.pos[i3 + 2] = z + (Math.random() - 0.5) * 0.9;
      this.vel[i3] = (Math.random() - 0.5) * 3.2;
      this.vel[i3 + 1] = Math.random() * 3.4 + 0.6;
      this.vel[i3 + 2] = (Math.random() - 0.5) * 3.2;
      const l = 0.5 + Math.random() * 0.6;
      this.life[s] = l;
      this.maxLife[s] = l;
      this.size[s] = 2.5 + Math.random() * 3.5;
      const t = 0.75 + Math.random() * 0.5;
      this.tint[i3] = (color[0] / 255) * t;
      this.tint[i3 + 1] = (color[1] / 255) * t;
      this.tint[i3 + 2] = (color[2] / 255) * t;
    }
    void spread;
  }

  oldest() {
    let best = 0, bl = Infinity;
    for (let i = 0; i < this.n; i++) if (this.life[i] < bl) { bl = this.life[i]; best = i; }
    return best;
  }

  update(dt, world, dayLight) {
    this.mat.uniforms.uDayLight.value = dayLight;
    const g = 9;
    let live = 0;
    for (let i = 0; i < this.n; i++) {
      if (this.life[i] <= 0) continue;
      this.life[i] -= dt;
      const i3 = i * 3;
      this.vel[i3 + 1] -= g * dt;
      const nx = this.pos[i3] + this.vel[i3] * dt;
      const ny = this.pos[i3 + 1] + this.vel[i3 + 1] * dt;
      const nz = this.pos[i3 + 2] + this.vel[i3 + 2] * dt;
      // 撞到方块就贴住，避免碎屑穿墙
      if (world.isSolidAt(Math.floor(nx), Math.floor(this.pos[i3 + 1]), Math.floor(this.pos[i3 + 2]))) {
        this.vel[i3] = 0;
      } else this.pos[i3] = nx;
      if (world.isSolidAt(Math.floor(this.pos[i3]), Math.floor(ny), Math.floor(this.pos[i3 + 2]))) {
        this.vel[i3 + 1] = 0;
      } else this.pos[i3 + 1] = ny;
      if (world.isSolidAt(Math.floor(this.pos[i3]), Math.floor(this.pos[i3 + 1]), Math.floor(nz))) {
        this.vel[i3 + 2] = 0;
      } else this.pos[i3 + 2] = nz;

      if (this.life[i] > 0) {
        live = Math.max(live, i + 1);
      }
    }
    // 收紧缓冲区：把还活着的粒子往前挪
    let w = 0;
    for (let i = 0; i < this.n; i++) {
      if (this.life[i] <= 0) continue;
      if (w !== i) {
        const a = i * 3, b = w * 3;
        for (let k = 0; k < 3; k++) {
          this.pos[b + k] = this.pos[a + k];
          this.vel[b + k] = this.vel[a + k];
          this.tint[b + k] = this.tint[a + k];
        }
        this.life[w] = this.life[i];
        this.maxLife[w] = this.maxLife[i];
        this.size[w] = this.size[i];
      }
      w++;
    }
    this.n = w;
    this.geo.setDrawRange(0, this.n);
    if (this.n > 0) {
      this.geo.attributes.position.needsUpdate = true;
      this.geo.attributes.size.needsUpdate = true;
      this.geo.attributes.tint.needsUpdate = true;
    }
    void live;
  }
}
