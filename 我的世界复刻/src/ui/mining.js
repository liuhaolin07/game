// 挖掘与放置：逐格进度、裂纹叠加、碎屑、掉落。
//
// 手感按原版来：破坏不是「点一下就碎」，而是持续按住直到进度条走满；
// 进度速度取决于方块硬度和手上的工具对不对口；破坏过程中每隔一段时间响一次
// 材质音；碎了才结算掉落和音效。

import * as THREE from 'three';
import { B, BLOCKS, IS_SOLID } from '../core/blocks.js';
import { REACH, WORLD_H } from '../core/constants.js';
import { breakTime, isTool } from '../core/items.js';
import { raycast, isTargetable } from '../physics/raycast.js';
import * as sfx from './sounds.js';

const STAGES = 10;          // 裂纹贴图级数
const DIG_TICK = 0.22;      // 每隔多久响一次挖掘音（秒）

export class Mining {
  constructor(world, scene, player, materials, breakCanvas, inventory) {
    this.world = world;
    this.player = player;
    this.materials = materials;
    this.inventory = inventory;

    this.target = null;       // 当前准星命中的方块
    this.progress = 0;        // 0..1
    this.digging = null;      // {x,y,z}
    this.digTimer = 0;
    this.breakTime = Infinity;
    this.placeCooldown = 0;
    this.onDrop = null;       // 回调：交给掉落物系统

    // 选框
    this.outline = new THREE.LineSegments(
      new THREE.EdgesGeometry(new THREE.BoxGeometry(1.003, 1.003, 1.003)),
      new THREE.LineBasicMaterial({ color: 0x000000, transparent: true, opacity: 0.5 }),
    );
    this.outline.visible = false;
    scene.add(this.outline);

    // 裂纹叠加盒
    const tex = new THREE.CanvasTexture(breakCanvas);
    tex.magFilter = THREE.NearestFilter;
    tex.minFilter = THREE.NearestFilter;
    tex.generateMipmaps = false;
    tex.repeat.set(1 / STAGES, 1);
    this.breakTex = tex;
    this.breakMat = materials.breakOverlay.clone();
    this.breakMat.map = tex;
    this.breakBox = new THREE.Mesh(new THREE.BoxGeometry(1.006, 1.006, 1.006), this.breakMat);
    this.breakBox.visible = false;
    scene.add(this.breakBox);
  }

  /** 从眼睛沿视线取准星目标 */
  pick() {
    const p = this.player;
    // 相机朝向 = R_y(yaw) · R_x(pitch) · (0,0,-1)，展开后是
    //   (-sin(yaw)·cos(pitch),  sin(pitch),  -cos(yaw)·cos(pitch))
    // y 分量是 +sin(pitch)：实测 camera.getWorldDirection() 在 pitch=+0.5 时
    // 返回 y=+0.48，即 three 里 rotation.x 为正表示「抬头」。写成负号的话，
    // 低头时准星会跑到天上选方块。
    const cp = Math.cos(p.pitch), sp = Math.sin(p.pitch);
    const cy = Math.cos(p.yaw), sy = Math.sin(p.yaw);
    const dir = { x: -sy * cp, y: sp, z: -cy * cp };
    return raycast(this.world, { x: p.x, y: p.eyeY, z: p.z }, dir, REACH, isTargetable);
  }

  /**
   * 主工具 id：优先手持；空手时自动从背包里挑一把工具。
   * 顺手做的是「有工具就用」而不是「挑对口的那把」——判断对口要等到破坏那一刻才知道，
   * 那时已经来不及切换了。
   */
  effectiveTool() {
    const held = this.inventory.heldItem;
    if (isTool(held)) return held;
    for (const slot of this.inventory.slots) {
      if (slot && isTool(slot.id)) return slot.id;
    }
    return 0;
  }

  update(dt, mouseDown, wantPlace) {
    const p = this.player;
    const t = this.pick();
    this.target = t;

    if (t) {
      this.outline.visible = true;
      this.outline.position.set(t.x + 0.5, t.y + 0.5, t.z + 0.5);
    } else {
      this.outline.visible = false;
    }

    if (this.placeCooldown > 0) this.placeCooldown -= dt;

    // ---------- 挖掘 ----------
    if (mouseDown && t) {
      if (!this.digging || this.digging.x !== t.x || this.digging.y !== t.y || this.digging.z !== t.z) {
        this.digging = { x: t.x, y: t.y, z: t.z };
        this.breakTime = breakTime(t.id, this.effectiveTool());
        this.progress = 0;
        this.digTimer = 0;
      }
      this.digTimer += dt;
      if (this.digTimer >= DIG_TICK) {
        this.digTimer = 0;
        sfx.playDig(BLOCKS[t.id].sound);
      }
      if (this.breakTime <= 0) {
        this.progress = 1;          // 硬度 0（花草、火把）：一碰就碎
      } else {
        this.progress += dt / this.breakTime;
      }
      if (this.progress >= 1) {
        this.breakBlock(t.x, t.y, t.z, t.id);
        this.reset();
      } else {
        this.showBreak(t, this.progress);
      }
    } else {
      this.reset();
    }

    // ---------- 放置 ----------
    if (wantPlace && this.placeCooldown <= 0) {
      this.place();
    }
  }

  reset() {
    this.digging = null;
    this.progress = 0;
    this.digTimer = 0;
    this.breakBox.visible = false;
  }

  showBreak(t, progress) {
    const stage = Math.min(STAGES - 1, Math.floor(progress * STAGES));
    this.breakTex.offset.x = stage / STAGES;
    this.breakBox.position.set(t.x + 0.5, t.y + 0.5, t.z + 0.5);
    this.breakBox.visible = true;
  }

  breakBlock(x, y, z, id) {
    if (BLOCKS[id].hardness < 0) return;
    if (!this.world.setBlock(x, y, z, B.air)) return;
    sfx.playBreak(BLOCKS[id].sound);
    if (this.onDrop) this.onDrop(x, y, z, id);
    this.inventory.notifyChange?.(x, y, z);
  }

  /** 当前手持物能不能放在这里（不能放进玩家身体里） */
  canPlaceAt(x, y, z, id) {
    const cur = this.world.getBlock(x, y, z);
    if (cur !== B.air && cur !== B.water && !this.isReplaceable(cur)) return false;
    if (!IS_SOLID[id]) return true;
    const p = this.player;
    const HW = 0.3, PH = 1.8;
    if (x < p.x + HW && x + 1 > p.x - HW &&
        z < p.z + HW && z + 1 > p.z - HW &&
        y < p.y + PH && y + 1 > p.y) return false;
    return true;
  }

  isReplaceable(id) {
    return id === B.tall_grass || id === B.dandelion || id === B.poppy ||
      id === B.dead_bush || id === B.sugar_cane || id === B.water;
  }

  place() {
    const t = this.target;
    if (!t) return;
    const inv = this.inventory;
    const id = inv.heldBlock();
    if (id === null) return;
    const x = t.x + t.nx, y = t.y + t.ny, z = t.z + t.nz;
    if (y < 1 || y >= WORLD_H) return;
    if (!this.canPlaceAt(x, y, z, id)) return;
    if (!this.world.setBlock(x, y, z, id)) return;
    sfx.playPlace(BLOCKS[id].sound);
    inv.consumeHeld();
    this.placeCooldown = 0.18;
    inv.notifyChange?.(x, y, z);
  }
}
