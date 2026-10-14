// 材质：自写着色器。
//
// 顶点属性里的 uv 是「平铺 UV」（贪心合并出来的四边形会大于 1），片元里用 fract
// 重新映射回 16×16 图集里的那一格。这样既能保留合并带来的三角形数量优势，又只需要
// 一张普通 2D 图集，不必上 sampler2DArray。
//
// 代价是 mipmap 不能用（fract 会让屏幕空间导数在接缝处跳变），所以统一 NearestFilter
// 关掉 mipmap，靠雾把远处的锯齿盖掉——这也是原版的观感。

import * as THREE from 'three';
import { ATLAS_SIZE } from './tiles.js';

const VERT = /* glsl */`
attribute float tile;
attribute float shade;
attribute float skylight;
attribute float blocklight;
attribute vec3 tint;

uniform float uDayLight;

varying vec2 vUv;
varying float vTile;
varying float vShade;
varying vec2 vLight;
varying vec3 vTint;
varying float vFogDepth;

void main() {
  vUv = uv;
  vTile = tile;
  vShade = shade;
  vLight = vec2(skylight * uDayLight, blocklight);
  vTint = tint;
  vec4 mv = modelViewMatrix * vec4(position, 1.0);
  vFogDepth = -mv.z;
  gl_Position = projectionMatrix * mv;
}
`;

const FRAG = /* glsl */`
precision highp float;

uniform sampler2D uAtlas;
uniform vec3 uFogColor;
uniform float uFogNear;
uniform float uFogFar;
uniform float uAmbient;
uniform float uTime;
uniform float uWave;      // 0=静态，1=液体波动
uniform float uAlphaScale;

varying vec2 vUv;
varying float vTile;
varying float vShade;
varying vec2 vLight;
varying vec3 vTint;
varying float vFogDepth;

// 半个像素的收缩量：fract 在整数处可能因为浮点误差落到 0.9999…，
// 于是采样到隔壁贴图的左边缘，整片墙面会出现细缝。
const float INSET = ${(1 / ATLAS_SIZE).toFixed(8)};

void main() {
  vec2 f = vUv;
  // 波纹幅度要小：没有 mipmap，UV 一直在平铺重映射，幅度一大
  // 大片水面在掠射角下会闪出很重的摩尔纹。
  if (uWave > 0.5) f += vec2(sin(uTime * 0.7) * 0.02, uTime * 0.045);
  f = fract(f);
  f = f * (1.0 - 2.0 * INSET) + INSET;
  vec2 tileOrigin = vec2(mod(vTile, 16.0), floor(vTile / 16.0));
  vec2 auv = (tileOrigin + f) / 16.0;

  vec4 tex = texture2D(uAtlas, auv);
  if (tex.a < 0.4) discard;

  // 天光和方块光取较大者。方块光不随昼夜变化，天光由 uDayLight 统一缩放。
  float lum = max(vLight.x, vLight.y);
  lum = uAmbient + (1.0 - uAmbient) * pow(lum, 1.25);

  vec3 col = tex.rgb * vTint * vShade * lum;

  // 半透明像素（水面）再按贴图 alpha 调一次
  float a = tex.a * uAlphaScale;
  if (a < 0.98) col *= 1.08;   // 半透明部分略微提亮，模拟水的散射

  float fog = smoothstep(uFogNear, uFogFar, vFogDepth);
  col = mix(col, uFogColor, fog);

  gl_FragColor = vec4(col, a);
}
`;

export function createMaterials(atlasTexture) {
  const shared = {
    uAtlas: { value: atlasTexture },
    uDayLight: { value: 1 },
    uFogColor: { value: new THREE.Color(0x9ecfff) },
    uFogNear: { value: 40 },
    uFogFar: { value: 120 },
    uAmbient: { value: 0.11 },
    uTime: { value: 0 },
  };
  // 三个材质共享同一批 uniform 对象的引用，改一个三个一起生效
  const uniforms = (extra) => Object.assign({}, shared, extra);

  const opaque = new THREE.ShaderMaterial({
    uniforms: uniforms(),
    vertexShader: VERT,
    fragmentShader: FRAG,
  });
  opaque.uniforms.uWave = { value: 0 };
  opaque.uniforms.uAlphaScale = { value: 1 };

  const cutout = new THREE.ShaderMaterial({
    uniforms: uniforms(),
    vertexShader: VERT,
    fragmentShader: FRAG,
    side: THREE.DoubleSide,     // 十字植物需要双面
  });
  cutout.uniforms.uWave = { value: 0 };
  cutout.uniforms.uAlphaScale = { value: 1 };

  const liquid = new THREE.ShaderMaterial({
    uniforms: uniforms(),
    vertexShader: VERT,
    fragmentShader: FRAG,
    transparent: true,
    depthWrite: true,           // 水面本身不互相重叠，保留深度写入可以正确遮挡水下地形
  });
  liquid.uniforms.uWave = { value: 1 };
  liquid.uniforms.uAlphaScale = { value: 1 };

  const breakOverlay = new THREE.MeshBasicMaterial({
    transparent: true,
    depthWrite: false,
    polygonOffset: true,
    polygonOffsetFactor: -4,
    polygonOffsetUnits: -4,
    opacity: 0.85,
  });

  const all = [opaque, cutout, liquid];
  // 【必须】ShaderMaterial 只在 uniformsNeedUpdate === true 时才把 material.uniforms
  // 交给 GPU。该字段默认是 false，也就是说：不清这个标志，uAtlas 会一直是空纹理，
  // uDayLight / 雾参数 / uTime 也永远停在默认值。症状是「画面一片空白」——
  // 采样到全黑、alpha 为 0，于是每个片元都被 discard 掉，看起来像什么都没渲染。
  for (const m of all) m.uniformsNeedUpdate = true;

  return {
    opaque, cutout, liquid, breakOverlay,
    shared,           // 暴露给天空/雾统一调参
    all,
  };
}

/** 标记 uniform 已改动，下一帧 three 会上传 */
export function markUniformsDirty(target) {
  if (!target) return;
  if (Array.isArray(target)) { for (const m of target) m.uniformsNeedUpdate = true; return; }
  if (target.all) for (const m of target.all) m.uniformsNeedUpdate = true;
  if (target.uniforms) target.uniformsNeedUpdate = true;
}

export function updateMaterialTime(materials, t) {
  materials.opaque.uniforms.uTime.value = t;
  materials.cutout.uniforms.uTime.value = t;
  materials.liquid.uniforms.uTime.value = t;
  markUniformsDirty(materials);
}
