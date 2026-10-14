// 程序化音效：用 WebAudio 现场合成，不加载任何音频文件。
// 每种材质一个「噪声 + 滤波 + 包络」的配方，模仿原版那种短促的敲击感。

let ctx = null;
let noiseBuf = null;
let master = null;
let enabled = true;

function ensure() {
  if (ctx) return ctx;
  const AC = window.AudioContext || window.webkitAudioContext;
  if (!AC) return null;
  ctx = new AC();
  master = ctx.createGain();
  master.gain.value = 0.55;
  master.connect(ctx.destination);

  // 2 秒白噪声，循环复用
  const len = ctx.sampleRate * 2;
  noiseBuf = ctx.createBuffer(1, len, ctx.sampleRate);
  const d = noiseBuf.getChannelData(0);
  for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;
  return ctx;
}

export function unlockAudio() {
  const c = ensure();
  if (c && c.state === 'suspended') c.resume();
}

// 每种材质的配方：中心频率、Q、时长、音量、是否带音高下滑
const RECIPES = {
  grass: { f: 900, q: 1.1, dur: 0.10, vol: 0.16, sweep: 0.55 },
  dirt: { f: 420, q: 0.9, dur: 0.12, vol: 0.20, sweep: 0.5 },
  stone: { f: 1500, q: 2.2, dur: 0.08, vol: 0.20, sweep: 0.6 },
  wood: { f: 780, q: 3.0, dur: 0.10, vol: 0.20, sweep: 0.7 },
  sand: { f: 2400, q: 0.7, dur: 0.14, vol: 0.14, sweep: 0.35 },
  gravel: { f: 1100, q: 0.9, dur: 0.12, vol: 0.18, sweep: 0.5 },
  glass: { f: 4200, q: 6.0, dur: 0.12, vol: 0.16, sweep: 1.0 },
  wool: { f: 320, q: 0.6, dur: 0.10, vol: 0.12, sweep: 0.3 },
  plant: { f: 1800, q: 1.0, dur: 0.08, vol: 0.10, sweep: 0.5 },
  snow: { f: 2000, q: 0.5, dur: 0.12, vol: 0.11, sweep: 0.3 },
  splash: { f: 1400, q: 0.6, dur: 0.30, vol: 0.16, sweep: 0.2 },
  lava: { f: 260, q: 0.8, dur: 0.40, vol: 0.10, sweep: 0.2 },
  none: null,
};

function burst(sound, pitch = 1, gainMul = 1) {
  if (!enabled) return;
  const c = ensure();
  if (!c) return;
  if (c.state === 'suspended') c.resume();
  const r = RECIPES[sound];
  if (!r) return;

  const now = c.currentTime;
  const src = c.createBufferSource();
  src.buffer = noiseBuf;
  src.loop = true;
  // 每次从噪声里随机取一段，重复听不会觉得是同一个音
  const offset = Math.random() * 1.5;

  const filt = c.createBiquadFilter();
  filt.type = 'bandpass';
  filt.frequency.setValueAtTime(r.f * pitch, now);
  filt.frequency.exponentialRampToValueAtTime(Math.max(60, r.f * pitch * r.sweep), now + r.dur);
  filt.Q.value = r.q;

  const gain = c.createGain();
  const vol = r.vol * gainMul;
  gain.gain.setValueAtTime(0, now);
  gain.gain.linearRampToValueAtTime(vol, now + 0.004);
  gain.gain.exponentialRampToValueAtTime(0.0008, now + r.dur);

  src.connect(filt).connect(gain).connect(master);
  src.start(now, offset);
  src.stop(now + r.dur + 0.02);
}

/** 挖掘中的「咔、咔」声 */
export function playDig(sound) { burst(sound, 0.92 + Math.random() * 0.2, 0.45); }
/** 方块破坏 */
export function playBreak(sound) { burst(sound, 1.0, 1.0); burst(sound, 1.45, 0.5); }
/** 放置 */
export function playPlace(sound) { burst(sound, 1.12, 0.8); }
/** 走路脚步 */
export function playStep(sound) { burst(sound, 0.75 + Math.random() * 0.2, 0.28); }

/** 物品拾取：一声短促上扬的提示音 */
export function playPickup() {
  if (!enabled) return;
  const c = ensure();
  if (!c) return;
  if (c.state === 'suspended') c.resume();
  const now = c.currentTime;
  const o = c.createOscillator();
  const g = c.createGain();
  o.type = 'triangle';
  o.frequency.setValueAtTime(660, now);
  o.frequency.exponentialRampToValueAtTime(990, now + 0.07);
  g.gain.setValueAtTime(0.09, now);
  g.gain.exponentialRampToValueAtTime(0.001, now + 0.10);
  o.connect(g).connect(master);
  o.start(now); o.stop(now + 0.11);
}
