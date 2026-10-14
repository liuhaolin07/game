// 入口：把世界、光照、渲染、输入、UI 串起来，跑主循环。

import * as THREE from 'three';
import { DEFAULT_SEED, WORLD_H, STREAM_BUDGET_MS, STREAM_BUDGET_MS_IDLE, SEA_LEVEL } from './core/constants.js';
import { B, BLOCKS } from './core/blocks.js';
import { itemName } from './core/items.js';
import { World } from './world/world.js';
import { LightEngine } from './world/light.js';
import { biomeName } from './world/biomes.js';
import { buildAtlas, buildAtlasTexture, buildBreakTexture } from './render/atlas.js';
import { createMaterials, updateMaterialTime, markUniformsDirty } from './render/materials.js';
import { ChunkRenderer } from './render/chunkRenderer.js';
import { Sky } from './render/sky.js';
import { Particles } from './render/particles.js';
import { Player } from './physics/player.js';
import { Mining } from './ui/mining.js';
import { starterInventory, HOTBAR_SIZE } from './ui/inventory.js';
import { Drops } from './ui/drops.js';
import { Hud } from './ui/hud.js';
import * as sfx from './ui/sounds.js';

const SAVE_KEY = 'mcjs_save_v2';

class Game {
  constructor() {
    this.paused = true;
    this.mouseDown = false;
    this.placeQueued = false;
    this.lastSpaceTap = 0;
    this.time = 0;
    this.fpsFrames = 0;
    this.fpsTime = performance.now();
    this.fps = 0;
    this.locked = false;
    this.keys = {};
    this.wireInput();
  }

  // ─────────────────────────── 启动
  start() {
    const app = document.getElementById('app');
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(75, innerWidth / innerHeight, 0.1, 900);
    this.camera.rotation.order = 'YXZ';
    this.renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: 'high-performance' });
    this.renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
    this.renderer.setSize(innerWidth, innerHeight);
    app.appendChild(this.renderer.domElement);

    // 贴图与材质
    this.atlasCanvas = buildAtlas();
    const atlasTex = buildAtlasTexture(this.atlasCanvas);
    this.materials = createMaterials(atlasTex);

    // 世界
    this.seed = this.readSeed();
    this.world = new World(this.seed);
    this.light = new LightEngine(this.world);
    this.chunks = new ChunkRenderer(this.world, this.light, this.materials, this.scene);
    this.sky = new Sky(this.scene, this.materials, { startTime: 0.30 });
    this.particles = new Particles(this.scene);

    // 玩家
    this.player = new Player(this.world, 0, 90, 0);
    this.loadPlayer();
    this.ensureSpawnLoaded();

    // UI 全部挂在 #gameui 下：解锁鼠标时整个容器隐藏，
    // 准星和物品栏就不会在开始菜单上露出来。
    this.inv = starterInventory();
    this.hud = new Hud(document.getElementById('gameui'), this.inv, this.atlasCanvas);
    this.drops = new Drops(this.scene, this.world, atlasTex, this.materials);
    this.mining = new Mining(this.world, this.scene, this.player, this.materials,
      buildBreakTexture(), this.inv);
    this.mining.onDrop = (x, y, z, id) => {
      this.particles.burst(x + 0.5, y + 0.5, z + 0.5, BLOCKS[id].color, 16);
      this.drops.spawn(x, y, z, id, this.mining.effectiveTool());
    };

    // 玩家改方块后：重算光照 + 标记网格重建
    this.world.listeners.onEdit.push((x, y, z, oldId, newId) => {
      this.light.onBlockChanged(x, y, z, oldId, newId);
      this.chunks.markDirtyAround(x, y, z);
    });

    this.loadSave();

    addEventListener('resize', () => this.onResize());
    document.addEventListener('pointerlockchange', () => this.onLockChange());
    document.getElementById('playBtn').addEventListener('click', () => {
      sfx.unlockAudio();
      this.renderer.domElement.requestPointerLock();
    });

    this.lastT = performance.now();
    requestAnimationFrame(this.loop.bind(this));
    document.getElementById('loading').style.display = 'none';
  }

  // ─────────────────────────── 存档
  readSeed() {
    const p = new URLSearchParams(location.search);
    if (p.has('seed')) return (parseInt(p.get('seed'), 10) || DEFAULT_SEED) | 0;
    return DEFAULT_SEED;
  }

  loadPlayer() {
    let sp = null;
    try { sp = JSON.parse(localStorage.getItem(SAVE_KEY + '_pos') || 'null'); } catch { sp = null; }
    if (sp && Number.isFinite(sp.x)) {
      this.player.x = sp.x; this.player.y = sp.y; this.player.z = sp.z;
      this.player.yaw = sp.yaw || 0; this.player.pitch = sp.pitch || 0;
      this.player.flying = !!sp.flying;
      return;
    }
    this.findSpawn();
  }

  /** 找一个海平面以上、地势不高的落脚点 */
  findSpawn() {
    for (let r = 4; r < 4000; r++) {
      const a = r * 2.39996;               // 黄金角螺旋，避免来回扫同一条线
      const x = Math.round(Math.cos(a) * r * 0.8);
      const z = Math.round(Math.sin(a) * r * 0.8);
      const col = this.world.gen.column(x, z);
      if (col.height > SEA_LEVEL + 2 && col.height < SEA_LEVEL + 20) {
        this.player.x = x + 0.5;
        this.player.z = z + 0.5;
        this.player.y = col.height + 1.2;
        return;
      }
    }
    this.player.x = 0.5; this.player.z = 0.5; this.player.y = 100;
  }

  /** 出生前先把周围几层区块强制生成，否则玩家会站在空气里掉下去 */
  ensureSpawnLoaded() {
    const cx = this.player.x >> 4, cz = this.player.z >> 4;
    for (let dx = -2; dx <= 2; dx++) {
      for (let dz = -2; dz <= 2; dz++) {
        const c = this.world.ensureChunk(cx + dx, cz + dz);
        if (c.state < 2) { this.light.initChunk(c); c.state = 2; }
      }
    }
    this.light.propagate(5_000_000);
    for (let i = 0; i < 16 && this.player.intersectsSolid(this.player.x, this.player.y, this.player.z); i++) {
      this.player.y += 1;
    }
  }

  loadSave() {
    try {
      const s = localStorage.getItem(SAVE_KEY);
      if (!s) return;
      const o = JSON.parse(s);
      if (o.edits) this.world.loadEdits(o.edits);
      if (o.inv) this.inv.load(o.inv);
      if (o.sky !== undefined) this.sky.time = o.sky;
    } catch { /* 存档坏了就当新档 */ }
  }

  save() {
    if (this.saveTimer) return;
    this.saveTimer = setTimeout(() => {
      this.saveTimer = null;
      try {
        localStorage.setItem(SAVE_KEY, JSON.stringify({
          edits: this.world.serializeEdits(),
          inv: this.inv.serialize(),
          sky: this.sky.time,
        }));
        localStorage.setItem(SAVE_KEY + '_pos', JSON.stringify({
          x: this.player.x, y: this.player.y, z: this.player.z,
          yaw: this.player.yaw, pitch: this.player.pitch, flying: this.player.flying,
        }));
      } catch { /* 隐私模式下写不进去，忽略 */ }
    }, 800);
  }

  // ─────────────────────────── 输入
  wireInput() {
    addEventListener('keydown', (e) => {
      if (e.code === 'F3') { e.preventDefault(); this.hud?.toggleDebug(); return; }
      if (e.code === 'KeyF' && this.locked) {
        this.player.toggleFly();
        this.hud?.say(this.player.flying ? '飞行模式' : '步行模式');
      }
      if (e.code === 'Space') {
        const now = performance.now();
        if (this.locked && now - this.lastSpaceTap < 280) {
          this.player.toggleFly();
          this.hud?.say(this.player.flying ? '飞行模式' : '步行模式');
        }
        this.lastSpaceTap = now;
        if (this.locked) e.preventDefault();
      }
      if (e.code.startsWith('Digit')) {
        const n = +e.code.slice(5);
        if (n >= 1 && n <= 9) this.inv?.select(n - 1);
      }
      if (e.code === 'KeyE' && this.locked) this.cycleHeld();
      if (this.locked && (e.code === 'Minus' || e.code === 'NumpadSubtract')) {
        this.hud?.say('视距 ' + this.chunks.setRenderDistance(this.chunks.renderDistance - 1) + ' 区块');
      }
      if (this.locked && (e.code === 'Equal' || e.code === 'NumpadAdd')) {
        this.hud?.say('视距 ' + this.chunks.setRenderDistance(this.chunks.renderDistance + 1) + ' 区块');
      }
      this.keys[e.code] = true;
    });
    addEventListener('keyup', (e) => { this.keys[e.code] = false; });
    addEventListener('mousemove', (e) => {
      if (!this.locked) return;
      this.player.yaw -= e.movementX * 0.0023;
      this.player.pitch -= e.movementY * 0.0023;
      const lim = Math.PI / 2 - 0.001;
      this.player.pitch = Math.max(-lim, Math.min(lim, this.player.pitch));
    });
    addEventListener('mousedown', (e) => {
      if (!this.locked) return;
      if (e.button === 0) this.mouseDown = true;
      if (e.button === 2) this.placeQueued = true;
      if (e.button === 1) { e.preventDefault(); this.pickBlock(); }
    });
    addEventListener('mouseup', (e) => { if (e.button === 0) this.mouseDown = false; });
    addEventListener('contextmenu', (e) => e.preventDefault());
    addEventListener('wheel', (e) => {
      if (!this.locked) return;
      this.inv.scroll(e.deltaY > 0 ? 1 : -1);
    }, { passive: true });
    addEventListener('beforeunload', () => this.save());
  }

  /** 中键吸取：把准星指着的方块收进背包 */
  pickBlock() {
    const t = this.mining.target;
    if (!t) return;
    if (this.inv.heldItem && this.inv.heldItem.id === t.id) return;
    if (this.inv.add(t.id, 1) > 0) this.hud.say('吸取：' + itemName(t.id));
    else this.hud.say('背包满了');
  }

  /** E 键：跳到下一个非空快捷栏格 */
  cycleHeld() {
    for (let i = 1; i <= HOTBAR_SIZE; i++) {
      const s = (this.inv.selected + i) % HOTBAR_SIZE;
      if (this.inv.hotbar[s]) { this.inv.select(s); return; }
    }
  }

  onLockChange() {
    this.locked = document.pointerLockElement === this.renderer.domElement;
    this.paused = !this.locked;
    document.getElementById('menu').style.display = this.locked ? 'none' : 'flex';
    document.getElementById('gameui').style.display = this.locked ? 'block' : 'none';
    if (!this.locked) {
      document.getElementById('title').innerHTML = 'MINECRAFT<span>.JS</span>';
      document.getElementById('playBtn').textContent = '继续游戏';
      this.save();
    }
  }

  onResize() {
    this.camera.aspect = innerWidth / innerHeight;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(innerWidth, innerHeight);
  }

  // ─────────────────────────── 主循环
  loop() {
    requestAnimationFrame(this.loop.bind(this));
    const now = performance.now();
    const dt = Math.min((now - this.lastT) / 1000, 0.05);
    this.lastT = now;
    this.time += dt;

    // 一帧里任何异常都不该让整个游戏停摆：requestAnimationFrame 的回调一旦抛错，
    // 浏览器不会再调它，表现就是画面直接冻住。这里兜住并只提示一次。
    try {
      if (!this.paused) this.step(dt);
      this.render(dt);
    } catch (e) {
      if (!this._crashed) {
        this._crashed = true;
        console.error('[Minecraft.js] 主循环异常，已停止后续更新：', e);
        this.hud?.say('主循环出错，详情见控制台 (F12)');
      }
    }

    this.fpsFrames++;
    if (now - this.fpsTime > 500) {
      this.fps = Math.round(this.fpsFrames * 1000 / (now - this.fpsTime));
      this.fpsFrames = 0;
      this.fpsTime = now;
    }
  }

  step(dt) {
    const k = this.keys, p = this.player;
    p.sneaking = !!k.ShiftLeft && !p.flying;
    p.sprinting = !!k.ControlLeft && !p.sneaking;
    p.update(dt, {
      forward: !!k.KeyW, back: !!k.KeyS, left: !!k.KeyA, right: !!k.KeyD,
      jump: !!k.Space, sneak: p.sneaking, sprint: p.sprinting,
      up: !!k.Space, down: !!k.ShiftLeft || !!k.ShiftRight,
    });

    // 走路脚步声
    this.stepTimer = (this.stepTimer || 0) + dt;
    if (p.onGround && Math.hypot(p.vx, p.vz) > 1.2 && this.stepTimer > 0.42) {
      this.stepTimer = 0;
      const below = this.world.getBlock(Math.floor(p.x), Math.floor(p.y - 0.2), Math.floor(p.z));
      if (below) sfx.playStep(BLOCKS[below].sound);
    }

    this.mining.update(dt, this.mouseDown, this.placeQueued);
    this.placeQueued = false;
    this.hud.setMining(this.mining.progress);

    this.drops.update(dt, p, this.inv, this.sky.dayFactor ?? 1);

    // 区块流式加载
    this.chunks.update(p.x >> 4, p.z >> 4, STREAM_BUDGET_MS);
    this.light.propagate(40000);

    if (p.y < -12) { p.y = WORLD_H - 4; p.vy = 0; this.hud.say('你掉出了世界'); }
    this.save();
  }

  render(dt) {
    const p = this.player;
    updateMaterialTime(this.materials, this.time);
    this.sky.update(dt, this.camera.position);
    this.particles.update(dt, this.world, this.sky.dayFactor ?? 1);

    // 停在开始菜单时也要把世界建起来，否则点「进入世界」的第一帧面对的是一片虚空。
    // 这里给更大的预算，反正没人操作，帧率不重要。
    if (this.paused) {
      this.chunks.update(p.x >> 4, p.z >> 4, STREAM_BUDGET_MS_IDLE);
      this.light.propagate(120000);
    }

    // 走路时镜头轻微起伏
    const bob = p.onGround && !p.flying
      ? Math.sin(p.bobPhase * 2) * 0.035 * Math.min(1, Math.hypot(p.vx, p.vz) / 4) : 0;
    this.camera.position.set(p.x, p.eyeY + bob, p.z);
    this.camera.rotation.set(p.pitch, p.yaw, 0);

    // 雾的距离跟着视距走
    const far = this.chunks.renderDistance * 16;
    for (const m of this.materials.all) {
      m.uniforms.uFogNear.value = far * 0.55;
      m.uniforms.uFogFar.value = far * 0.98;
    }
    // 【必须】ShaderMaterial 只有在 uniformsNeedUpdate 为 true 时才把 uniforms 交给 GPU。
    // 昼夜、雾、水波时间都是每帧改的，不标这一下，画面会一直是「uAtlas 没上传 +
    // 光照停在默认值」的样子——全黑、alpha 0、每个片元被 discard，看着像什么都没渲染。
    markUniformsDirty(this.materials);

    // 潜水时镜头整体染色
    const eye = this.world.getBlock(Math.floor(p.x), Math.floor(p.eyeY), Math.floor(p.z));
    this.hud.setUnderwater(eye === B.water);

    this.renderer.render(this.scene, this.camera);
    if (this.hud.debugVisible) this.updateDebug();
  }

  updateDebug() {
    const p = this.player;
    const t = this.mining.target;
    const cx = p.x >> 4, cz = p.z >> 4;
    const dirs = ['北 −Z', '东 +X', '南 +Z', '西 −X'];
    const dir = dirs[Math.round(((p.yaw % (Math.PI * 2)) + Math.PI * 2) % (Math.PI * 2) / (Math.PI / 2)) % 4];
    const lx = Math.floor(p.x), ly = Math.floor(p.y + 1), lz = Math.floor(p.z);
    this.hud.setDebug([
      `Minecraft.js · ${this.fps} FPS`,
      `XYZ  ${p.x.toFixed(2)} / ${p.y.toFixed(2)} / ${p.z.toFixed(2)}`,
      `区块 ${cx}, ${cz}  群系 ${biomeName(this.world.getBiome(lx, lz))}`,
      `光照 天光 ${this.world.getSky(lx, ly, lz)} · 方块光 ${this.world.getBlockLight(lx, ly, lz)}`,
      `模式 ${p.flying ? '飞行' : p.inWater ? '游泳' : p.sneaking ? '潜行' : '步行'}${p.onGround ? ' · 着地' : ''}`,
      `朝向 ${dir}`,
      `种子 ${this.seed}  时间 ${(this.sky.time * 24).toFixed(1)} 时`,
      `区块 已建 ${this.chunks.stats.meshed} · 待建 ${this.chunks.stats.pendingMesh} · 光照队列 ${this.light.pending}`,
      t ? `瞄准 ${BLOCKS[t.id].name} @ ${t.x},${t.y},${t.z}` : '瞄准 —',
    ].join('\n'));
  }
}

const game = new Game();
window.__game = game;
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', () => game.start());
} else {
  game.start();
}
