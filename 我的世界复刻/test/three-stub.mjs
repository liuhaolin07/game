// three.js 的最小桩件。只为在 Node 下检查模块能否正常加载与跑通逻辑，
// 不做任何真实渲染。

const any = (...a) => a[0];

// Vector3 / Euler 要在 Obj3D 之前定义——桩件的 position / rotation 就是它们
class Vector3 {
  constructor(x = 0, y = 0, z = 0) { this.x = x; this.y = y; this.z = z; }
  set(x, y, z) { this.x = x; this.y = y; this.z = z; return this; }
  setScalar(s) { this.x = this.y = this.z = s; return this; }
  copy(v) { this.x = v.x; this.y = v.y; this.z = v.z; return this; }
  add(v) { this.x += v.x; this.y += v.y; this.z += v.z; return this; }
  addScaledVector(v, s) { this.x += v.x * s; this.y += v.y * s; this.z += v.z * s; return this; }
  sub(v) { this.x -= v.x; this.y -= v.y; this.z -= v.z; return this; }
  multiplyScalar(s) { this.x *= s; this.y *= s; this.z *= s; return this; }
  length() { return Math.hypot(this.x, this.y, this.z); }
  lengthSq() { return this.x * this.x + this.y * this.y + this.z * this.z; }
  normalize() { const l = this.length() || 1; return this.multiplyScalar(1 / l); }
}

class Euler {
  constructor(x = 0, y = 0, z = 0, order = 'XYZ') { this.x = x; this.y = y; this.z = z; this.order = order; }
  set(x, y, z) { this.x = x; this.y = y; this.z = z; return this; }
  copy(e) { return this.set(e.x, e.y, e.z); }
}

class Quaternion {
  setFromEuler() { return this; }
  copy() { return this; }
}

class Matrix4 {
  compose() { return this; }
  identity() { return this; }
  copy() { return this; }
}

class Obj3D {
  constructor() {
    this.children = [];
    this.position = new Vector3();
    this.rotation = new Euler();
    this.scale = new Vector3(1, 1, 1);
    this.up = new Vector3(0, 1, 0);
    this.quaternion = new Quaternion();
    this.matrix = new Matrix4();
    this.renderOrder = 0;
    this.frustumCulled = true;
    this.matrixAutoUpdate = true;
    this.visible = true;
  }
  add(...o) { this.children.push(...o); return this; }
  remove(...o) { this.children = this.children.filter(c => !o.includes(c)); return this; }
  lookAt() { return this; }
  traverse() {}
  updateMatrix() {}
  setMatrixAt() {}
  getMatrixAt() { return this.matrix; }
}

class Color {
  constructor(c) { this.r = this.g = this.b = 1; if (c !== undefined) this.set(c); }
  set(c) {
    if (typeof c === 'number') { this.r = ((c >> 16) & 255) / 255; this.g = ((c >> 8) & 255) / 255; this.b = (c & 255) / 255; }
    return this;
  }
  setRGB(r, g, b) { this.r = r; this.g = g; this.b = b; return this; }
  copy(c) { return this.setRGB(c.r, c.g, c.b); }
}

// BufferAttribute 必须像真实的 three 一样校验「必须是 TypedArray」。
// 这一条不是形式主义：早期版本把 MeshBuffer 里的普通 JS 数组直接塞进来，
// 真实浏览器里每建一个区块就抛一次 "array should be a Typed Array"，
// 结果一个三角形都渲染不出来。桩件一开始没校验，这道防线就漏过去了。
const TYPED = new Set([
  Int8Array, Uint8Array, Uint8ClampedArray, Int16Array, Uint16Array,
  Int32Array, Uint32Array, Float32Array, Float64Array,
]);

class BufferAttribute {
  constructor(array, itemSize) {
    if (!TYPED.has(array.constructor)) {
      throw new Error('THREE.BufferAttribute: array should be a Typed Array. ' +
        `收到的是 ${Object.prototype.toString.call(array)}`);
    }
    this.array = array; this.itemSize = itemSize; this.count = array.length / itemSize;
    this.needsUpdate = false;
  }
  setUsage() { return this; }
  getX(i) { return this.array[i * this.itemSize]; }
  getY(i) { return this.array[i * this.itemSize + 1]; }
  getZ(i) { return this.array[i * this.itemSize + 2]; }
  setX(i, v) { this.array[i * this.itemSize] = v; return this; }
  setY(i, v) { this.array[i * this.itemSize + 1] = v; return this; }
  setZ(i, v) { this.array[i * this.itemSize + 2] = v; return this; }
}

class InstancedBufferAttribute extends BufferAttribute {}

class BufferGeometry {
  constructor() { this.attributes = {}; this.index = null; }
  setAttribute(n, a) { this.attributes[n] = a; return this; }
  setIndex(a) { this.index = a; return this; }
  computeBoundingSphere() {}
  setDrawRange() {}
  dispose() {}
}

class Material {
  constructor(p = {}) { Object.assign(this, p); this.uniforms = p.uniforms || {}; this.needsUpdate = false; }
  clone() { return new Material(this); }
  dispose() {}
}

class ShaderMaterial extends Material {}
class MeshBasicMaterial extends Material {}
class MeshLambertMaterial extends Material {}
class LineBasicMaterial extends Material {}

class Texture {
  constructor(data, width, height) {
    this.image = width !== undefined ? { data, width, height } : data;
    this.magFilter = 0; this.minFilter = 0; this.wrapS = 0; this.wrapT = 0;
    this.generateMipmaps = true; this.colorSpace = ''; this.repeat = { set() {} };
    this.offset = { x: 0, y: 0 };
  }
  dispose() {}
}

class CanvasTexture extends Texture {
  constructor(canvas) { super(canvas); this.isCanvasTexture = true; }
}

// DataTexture 只接受 TypedArray —— 和 BufferAttribute 一样，这里也保持校验
class DataTexture extends Texture {
  constructor(data, width, height) {
    if (data && !TYPED.has(data.constructor)) {
      throw new Error('THREE.DataTexture: Data must be a Typed Array. 收到的是 ' +
        Object.prototype.toString.call(data));
    }
    super(data, width, height);
    this.isDataTexture = true;
  }
}

class Mesh extends Obj3D {
  constructor(geo, mat) { super(); this.geometry = geo; this.material = mat; this.count = 0; }
}

class InstancedMesh extends Mesh {
  constructor(geo, mat, n) { super(geo, mat); this.instanceMatrix = new BufferAttribute(new Float32Array(n * 16), 16); }
}

class Group extends Obj3D {}
class Scene extends Obj3D { constructor() { super(); this.background = null; this.fog = null; } }

class Camera extends Obj3D {
  constructor() { super(); this.aspect = 1; this.fov = 50; this.updateProjectionMatrix = () => {}; }
}

class PerspectiveCamera extends Camera {}
class OrthographicCamera extends Camera {}

class WebGLRenderer {
  constructor(p = {}) { this.domElement = { getContext: () => ({}) }; Object.assign(this, p); }
  setSize() {}
  setPixelRatio() {}
  render() {}
  dispose() {}
}

// 几何体在桩件里统一用 BufferGeometry 的实现：BoxGeometry / EdgesGeometry 等
// 在真实 three 里也都有 setAttribute / setIndex。
// PlaneGeometry 会真的生成四个角点，因为 sky.js 要读 position 的坐标算径向淡出。
class Geometry extends BufferGeometry {
  constructor(w = 1, h = 1) {
    super();
    this.parameters = [w, h];
    const x = w / 2, y = h / 2;
    this.setAttribute('position', new BufferAttribute(new Float32Array([
      -x, -y, 0, x, -y, 0, x, y, 0, -x, y, 0,
    ]), 3));
    this.setAttribute('uv', new BufferAttribute(new Float32Array([0, 0, 1, 0, 1, 1, 0, 1]), 2));
  }
}

class LineSegments extends Mesh {}

// 常量：值不重要，只要类型对得上
const AdditiveBlending = 2;
const BackSide = 1;
const FrontSide = 0;
const DoubleSide = 2;
const NearestFilter = 1003;
const LinearFilter = 1006;
const RepeatWrapping = 1000;
const ClampToEdgeWrapping = 1001;
const SRGBColorSpace = 'srgb';
const DynamicDrawUsage = 35048;
const StaticDrawUsage = 35044;

// 别名：几个几何体类在桩件里都用同一个空实现
const BoxGeometry = Geometry;
const PlaneGeometry = Geometry;
const SphereGeometry = Geometry;
const EdgesGeometry = Geometry;
const Points = Mesh;
const Vector2 = Vector3;

// 导出命名空间
export {
  AdditiveBlending, BackSide, FrontSide, DoubleSide,
  NearestFilter, LinearFilter, RepeatWrapping, ClampToEdgeWrapping,
  SRGBColorSpace, DynamicDrawUsage, StaticDrawUsage,
  Vector2, Vector3, Color, Euler, Quaternion, Matrix4,
  BufferAttribute, InstancedBufferAttribute, BufferGeometry,
  Material, ShaderMaterial, MeshBasicMaterial, MeshLambertMaterial, LineBasicMaterial,
  Texture, CanvasTexture, DataTexture,
  Mesh, InstancedMesh, LineSegments, Group, Scene,
  Camera, PerspectiveCamera, OrthographicCamera, WebGLRenderer,
  BoxGeometry, PlaneGeometry, SphereGeometry, EdgesGeometry, Points,
};

export default {
  AdditiveBlending, BackSide, FrontSide, DoubleSide,
  NearestFilter, LinearFilter, RepeatWrapping, ClampToEdgeWrapping,
  SRGBColorSpace, DynamicDrawUsage, StaticDrawUsage,
  Vector2, Vector3, Color, Euler, Quaternion, Matrix4,
  BufferAttribute, InstancedBufferAttribute, BufferGeometry,
  ShaderMaterial, MeshBasicMaterial, MeshLambertMaterial, LineBasicMaterial,
  CanvasTexture, DataTexture, Mesh, InstancedMesh, LineSegments, Group, Scene,
  PerspectiveCamera, WebGLRenderer,
  BoxGeometry, PlaneGeometry, SphereGeometry, EdgesGeometry, Points,
};
void any;
