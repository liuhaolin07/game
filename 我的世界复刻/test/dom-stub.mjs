// 环境替身：DOM、Canvas 2D 上下文、requestAnimationFrame、WebAudio、localStorage。
// 单独抽出来，供 import-check 与 smoke 共用。

let ctx2d;
{
  const handler = {
    get(_, k) {
      if (k === 'createImageData' || k === 'getImageData') {
        return (x, y, w, h) => ({ data: new Uint8ClampedArray(w * h * 4), width: w, height: h });
      }
      if (k === 'measureText') return () => ({ width: 8 });
      if (k === 'canvas') return { width: 256, height: 256 };
      if (k === Symbol.toPrimitive) return () => 0;
      if (k === Symbol.toStringTag) return 'CanvasRenderingContext2D';
      return () => ctx2d;
    },
    set() { return true; },
    apply() { return ctx2d; },
  };
  ctx2d = new Proxy(function () {}, handler);
}

export function makeEl(tag = 'div') {
  const el = {
    tagName: String(tag).toUpperCase(),
    style: { display: '' },
    dataset: {},
    children: [],
    className: '',
    textContent: '',
    innerHTML: '',
    width: 16,
    height: 16,
    classList: { add() {}, remove() {}, toggle() {}, contains: () => false },
    getContext: () => ctx2d,
    appendChild(c) { this.children.push(c); return c; },
    addEventListener() {},
    removeEventListener() {},
    setAttribute() {},
    getAttribute: () => null,
    querySelector: () => null,
    querySelectorAll: () => [],
    focus() {},
    requestPointerLock() {},
  };
  return el;
}

export function installDom({ search = '' } = {}) {
  const store = new Map();
  const raf = [];
  // 虚拟时钟。游戏主循环读的是 performance.now()，所以这里必须提供同一个时钟，
  // 否则测试里推进的「帧」和游戏里的 dt 对不上——200 帧可能只相当于 0.02 秒游戏时间，
  // 物理根本来不及跑，看起来就像玩家悬在半空。
  let clock = 0;

  globalThis.document = {
    readyState: 'complete',
    createElement: (t) => makeEl(t),
    createElementNS: (ns, t) => makeEl(t),
    getElementById: () => makeEl('div'),
    querySelector: () => null,
    querySelectorAll: () => [],
    addEventListener() {},
    body: makeEl('body'),
  };
  globalThis.window = globalThis;
  globalThis.localStorage = {
    getItem: (k) => (store.has(k) ? store.get(k) : null),
    setItem: (k, v) => store.set(k, String(v)),
    removeItem: (k) => store.delete(k),
  };
  globalThis.location = { search, href: 'http://localhost:8080/' };
  globalThis.devicePixelRatio = 1;
  globalThis.innerWidth = 1280;
  globalThis.innerHeight = 720;
  globalThis.performance = { now: () => clock };
  globalThis.requestAnimationFrame = (cb) => { raf.push(cb); return raf.length; };
  globalThis.cancelAnimationFrame = () => {};
  globalThis.addEventListener = () => {};
  globalThis.removeEventListener = () => {};
  globalThis.AudioContext = undefined;
  globalThis.webkitAudioContext = undefined;

  return {
    storage: store,
    get now() { return clock; },
    /** 推进 n 帧；每帧默认 16ms，和真实 60fps 对齐 */
    frames(n, dtMs = 16) {
      for (let i = 0; i < n; i++) {
        clock += dtMs;
        for (const cb of raf.splice(0, raf.length)) cb(clock);
      }
    },
  };
}
