// 天空：渐变穹顶 + 太阳/月亮 + 星星 + 云层，以及昼夜循环。
// 昼夜只影响一个全局的 uDayLight 系数（乘在天光上），方块光不受影响。

import * as THREE from 'three';

const SKY_VERT = /* glsl */`
varying vec3 vDir;
void main() {
  vDir = position;
  vec4 p = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  gl_Position = p.xyww;    // 永远贴在远裁剪面上
}
`;

const SKY_FRAG = /* glsl */`
precision highp float;
uniform vec3 uTop;
uniform vec3 uHorizon;
uniform vec3 uBottom;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform float uSunGlow;
varying vec3 vDir;

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  vec3 col;
  if (h > 0.0) {
    col = mix(uHorizon, uTop, pow(clamp(h, 0.0, 1.0), 0.55));
  } else {
    col = mix(uHorizon, uBottom, pow(clamp(-h, 0.0, 1.0), 0.4));
  }
  // 太阳附近的辉光
  float s = max(dot(d, normalize(uSunDir)), 0.0);
  col += uSunColor * pow(s, 8.0) * uSunGlow;
  gl_FragColor = vec4(col, 1.0);
}
`;

/** 一天的关键帧：t 是 0..1 的时间（0=日出，0.25=正午，0.5=日落，0.75=午夜） */
const KEYS = [
  { t: 0.00, day: 0.35, top: [0.10, 0.16, 0.34], hor: [0.86, 0.44, 0.22], bot: [0.10, 0.10, 0.16], fog: [0.72, 0.46, 0.34], sun: [1.0, 0.62, 0.30] },
  { t: 0.10, day: 0.92, top: [0.25, 0.50, 0.86], hor: [0.66, 0.80, 0.95], bot: [0.42, 0.56, 0.72], fog: [0.70, 0.83, 0.96], sun: [1.0, 0.94, 0.80] },
  { t: 0.25, day: 1.00, top: [0.22, 0.47, 0.88], hor: [0.66, 0.84, 0.99], bot: [0.46, 0.62, 0.80], fog: [0.70, 0.85, 0.99], sun: [1.0, 0.98, 0.90] },
  { t: 0.40, day: 0.92, top: [0.25, 0.50, 0.86], hor: [0.66, 0.80, 0.95], bot: [0.42, 0.56, 0.72], fog: [0.70, 0.83, 0.96], sun: [1.0, 0.94, 0.80] },
  { t: 0.50, day: 0.30, top: [0.14, 0.13, 0.30], hor: [0.88, 0.40, 0.20], bot: [0.10, 0.09, 0.15], fog: [0.70, 0.42, 0.32], sun: [1.0, 0.52, 0.24] },
  { t: 0.58, day: 0.10, top: [0.03, 0.04, 0.11], hor: [0.10, 0.11, 0.22], bot: [0.03, 0.03, 0.07], fog: [0.09, 0.10, 0.19], sun: [0.35, 0.40, 0.62] },
  { t: 0.75, day: 0.07, top: [0.02, 0.03, 0.09], hor: [0.07, 0.08, 0.18], bot: [0.02, 0.02, 0.05], fog: [0.07, 0.08, 0.16], sun: [0.30, 0.36, 0.58] },
  { t: 0.92, day: 0.12, top: [0.04, 0.05, 0.13], hor: [0.14, 0.13, 0.26], bot: [0.04, 0.04, 0.08], fog: [0.12, 0.12, 0.22], sun: [0.40, 0.42, 0.60] },
  { t: 1.00, day: 0.35, top: [0.10, 0.16, 0.34], hor: [0.86, 0.44, 0.22], bot: [0.10, 0.10, 0.16], fog: [0.72, 0.46, 0.34], sun: [1.0, 0.62, 0.30] },
];

function lerpKey(t) {
  let i = 0;
  while (i < KEYS.length - 2 && KEYS[i + 1].t < t) i++;
  const a = KEYS[i], b = KEYS[i + 1];
  const f = Math.max(0, Math.min(1, (t - a.t) / (b.t - a.t)));
  const mix3 = (u, v) => [u[0] + (v[0] - u[0]) * f, u[1] + (v[1] - u[1]) * f, u[2] + (v[2] - u[2]) * f];
  return {
    day: a.day + (b.day - a.day) * f,
    top: mix3(a.top, b.top), hor: mix3(a.hor, b.hor), bot: mix3(a.bot, b.bot),
    fog: mix3(a.fog, b.fog), sun: mix3(a.sun, b.sun),
  };
}

function makeStarTexture() {
  const c = document.createElement('canvas');
  c.width = c.height = 64;
  const g = c.getContext('2d');
  g.fillStyle = '#000';
  g.fillRect(0, 0, 64, 64);
  for (let i = 0; i < 220; i++) {
    const x = Math.random() * 64, y = Math.random() * 64;
    const r = Math.random() * 1.1 + 0.25;
    const a = 0.35 + Math.random() * 0.65;
    g.fillStyle = `rgba(255,255,255,${a})`;
    g.beginPath(); g.arc(x, y, r, 0, 7); g.fill();
  }
  return c;
}

function makeCloudTexture() {
  const c = document.createElement('canvas');
  c.width = c.height = 128;
  const g = c.getContext('2d');
  g.clearRect(0, 0, 128, 128);
  // 用几个椭圆团块拼出原版那种方块感云
  for (let i = 0; i < 26; i++) {
    const x = Math.floor(Math.random() * 16) * 8;
    const y = Math.floor(Math.random() * 16) * 8;
    const w = 8 * (1 + Math.floor(Math.random() * 2));
    const h = 8 * (1 + Math.floor(Math.random() * 2));
    g.fillStyle = `rgba(255,255,255,${0.55 + Math.random() * 0.4})`;
    g.fillRect(x, y, w, h);
  }
  return c;
}

export class Sky {
  constructor(scene, materials, opts = {}) {
    this.scene = scene;
    this.materials = materials;
    this.dayLength = opts.dayLength || 600;   // 一整天多少秒
    this.time = opts.startTime ?? 0.28;       // 0..1
    this.paused = false;

    // 穹顶
    this.dome = new THREE.Mesh(
      new THREE.SphereGeometry(1, 24, 16),
      new THREE.ShaderMaterial({
        uniforms: {
          uTop: { value: new THREE.Color(0x3a7ce0) },
          uHorizon: { value: new THREE.Color(0xa8d4ff) },
          uBottom: { value: new THREE.Color(0x6a7a8c) },
          uSunDir: { value: new THREE.Vector3(0, 1, 0) },
          uSunColor: { value: new THREE.Color(0xfff0d0) },
          uSunGlow: { value: 0.35 },
        },
        vertexShader: SKY_VERT,
        fragmentShader: SKY_FRAG,
        side: THREE.BackSide,
        depthWrite: false,
        depthTest: false,
      }),
    );
    this.dome.renderOrder = -1000;
    this.dome.frustumCulled = false;
    // 穹顶是 ShaderMaterial，同样必须显式声明 uniform 改动，否则昼夜不会变
    this.dome.material.uniformsNeedUpdate = true;
    scene.add(this.dome);

    // 星星
    const starTex = new THREE.CanvasTexture(makeStarTexture());
    starTex.wrapS = starTex.wrapT = THREE.RepeatWrapping;
    starTex.repeat.set(6, 3);
    starTex.magFilter = THREE.NearestFilter;
    this.starMat = new THREE.MeshBasicMaterial({
      map: starTex, transparent: true, depthWrite: false, depthTest: false,
      side: THREE.BackSide, opacity: 0, fog: false,
    });
    this.stars = new THREE.Mesh(new THREE.SphereGeometry(1.2, 24, 16), this.starMat);
    this.stars.renderOrder = -999;
    this.stars.frustumCulled = false;
    scene.add(this.stars);

    // 太阳 / 月亮
    const mk = (color) => {
      const tex = new THREE.CanvasTexture(makeGlowTexture(color));
      const m = new THREE.Mesh(
        new THREE.PlaneGeometry(56, 56),
        new THREE.MeshBasicMaterial({
          map: tex, transparent: true, depthWrite: false, depthTest: false,
          blending: THREE.AdditiveBlending, fog: false,
        }),
      );
      m.renderOrder = -998;
      m.frustumCulled = false;
      scene.add(m);
      return m;
    };
    this.sun = mk('#fff4c8');
    this.moon = mk('#c8d8ff');

    // 云层。用 4 分量的顶点色做径向淡出：云是一块大平面，不淡出的话
    // 在远处会露出一条笔直的方形边界，把地平线切成一段一段的。
    const cloudTex = new THREE.CanvasTexture(makeCloudTexture());
    cloudTex.wrapS = cloudTex.wrapT = THREE.RepeatWrapping;
    cloudTex.magFilter = THREE.NearestFilter;
    cloudTex.repeat.set(5, 5);
    this.cloudTex = cloudTex;

    const CLOUD_SIZE = 900;
    const cloudGeo = new THREE.PlaneGeometry(CLOUD_SIZE, CLOUD_SIZE);
    {
      const pos = cloudGeo.attributes.position;
      const col = new Float32Array(pos.count * 4);
      const half = CLOUD_SIZE / 2;
      for (let i = 0; i < pos.count; i++) {
        const d = Math.hypot(pos.getX(i), pos.getY(i)) / half;
        col[i * 4] = 1; col[i * 4 + 1] = 1; col[i * 4 + 2] = 1;
        col[i * 4 + 3] = Math.min(1, Math.max(0, 1 - d) * 1.35);
      }
      cloudGeo.setAttribute('color', new THREE.BufferAttribute(col, 4));
    }
    this.clouds = new THREE.Mesh(
      cloudGeo,
      new THREE.MeshBasicMaterial({
        map: cloudTex, transparent: true, opacity: 0.5,
        vertexColors: true, depthWrite: false, side: THREE.DoubleSide, fog: false,
      }),
    );
    this.clouds.rotation.x = -Math.PI / 2;
    this.clouds.position.y = 118;
    this.clouds.renderOrder = -900;
    scene.add(this.clouds);
  }

  /** @param camPos 相机位置（云和天穹跟着走） */
  update(dt, camPos) {
    if (!this.paused) this.time = (this.time + dt / this.dayLength) % 1;
    const k = lerpKey(this.time);

    // 太阳沿一个倾斜圆周走
    const ang = (this.time - 0.25) * Math.PI * 2;
    const sx = Math.cos(ang), sy = Math.sin(ang);
    const sunDir = new THREE.Vector3(sx * 0.35, sy, sx * 0.94).normalize();

    const u = this.dome.material.uniforms;
    u.uTop.value.setRGB(k.top[0], k.top[1], k.top[2]);
    u.uHorizon.value.setRGB(k.hor[0], k.hor[1], k.hor[2]);
    u.uBottom.value.setRGB(k.bot[0], k.bot[1], k.bot[2]);
    u.uSunDir.value.copy(sunDir);
    u.uSunColor.value.setRGB(k.sun[0], k.sun[1], k.sun[2]);
    u.uSunGlow.value = 0.25 + k.day * 0.5;

    this.dome.position.copy(camPos);
    this.stars.position.copy(camPos);
    this.starMat.opacity = Math.max(0, 0.12 - k.day) * 8;

    this.sun.position.copy(camPos).addScaledVector(sunDir, 260);
    this.sun.lookAt(camPos);
    this.sun.material.opacity = 0.35 + k.day * 0.65;
    this.moon.position.copy(camPos).addScaledVector(sunDir, -260);
    this.moon.lookAt(camPos);
    this.moon.material.opacity = 0.30 + (1 - k.day) * 0.5;

    this.clouds.position.set(camPos.x, 118, camPos.z);
    this.cloudTex.offset.x += dt * 0.0035;
    this.clouds.material.opacity = 0.18 + k.day * 0.36;

    // 全局光照 + 雾
    this.materials.opaque.uniforms.uDayLight.value = Math.max(0.035, k.day);
    const fog = this.materials.opaque.uniforms.uFogColor.value;
    fog.setRGB(k.fog[0], k.fog[1], k.fog[2]);
    // 环境光下限不能太低：树冠内部、屋檐下这类「天光被遮住但不是封闭」的地方
    // 纯靠 pow(sky,1.45) 会黑成一片剪影，看不出是什么方块。
    this.materials.opaque.uniforms.uAmbient.value = 0.11 + (1 - k.day) * 0.02;
    // 天穹的 uniform 也要每帧同步（见 materials.js 里的坑）
    this.dome.material.uniformsNeedUpdate = true;

    this.dayFactor = k.day;
    this.sunDir = sunDir;
    this.fogColor = k.fog;
  }

  /** 头顶天光强度，用于昼夜方块光：天光为 0 时天空是黑的 */
  get timeOfDay() { return this.time; }
}

function makeGlowTexture(color) {
  const c = document.createElement('canvas');
  c.width = c.height = 64;
  const g = c.getContext('2d');
  const grd = g.createRadialGradient(32, 32, 0, 32, 32, 32);
  grd.addColorStop(0, color);
  grd.addColorStop(0.35, color);
  grd.addColorStop(1, 'rgba(0,0,0,0)');
  g.fillStyle = grd;
  g.fillRect(0, 0, 64, 64);
  return c;
}
