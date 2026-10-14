// 玩家物理：AABB 对体素网格的扫掠碰撞，含行走/疾跑/跳跃/游泳/飞行与自动上台阶。
//
// 碰撞策略是「逐轴、小步长」：每个轴单独推进，单步不超过 0.2 格，这样即使帧率很低、
// 速度很快也不会穿墙。撞上时把位置吸附到方块边界。

import {
  WORLD_H, PLAYER_HALF, PLAYER_HEIGHT, PLAYER_EYE,
  GRAVITY, JUMP_SPEED, WALK_SPEED, SPRINT_SPEED, FLY_SPEED, SWIM_SPEED,
  TERMINAL_VELOCITY,
} from '../core/constants.js';
import { B } from '../core/blocks.js';

const STEP = 0.2;          // 单次推进的最大位移
// 自动上台阶。这里必须给足一整格：地形起伏最常见的就是 1 格台阶，
// 高度设成 0.6 的话抬脚后身体仍然卡在方块侧面，爬不上去。
const STEP_HEIGHT = 1.05;
const EPS = 1e-3;

export class Player {
  constructor(world, x = 0, y = 80, z = 0) {
    this.world = world;
    this.x = x; this.y = y; this.z = z;   // 脚底中心
    this.vx = 0; this.vy = 0; this.vz = 0;
    this.yaw = 0; this.pitch = 0;
    this.onGround = false;
    this.flying = false;
    this.inWater = false;
    this.sprinting = false;
    this.sneaking = false;
    this.bobPhase = 0;
  }

  get eyeY() { return this.y + (this.sneaking ? PLAYER_EYE - 0.18 : PLAYER_EYE); }

  /** AABB 与世界是否相交 */
  intersectsSolid(x, y, z) {
    const w = this.world;
    const minX = Math.floor(x - PLAYER_HALF), maxX = Math.floor(x + PLAYER_HALF);
    const minY = Math.floor(y), maxY = Math.floor(y + PLAYER_HEIGHT - EPS);
    const minZ = Math.floor(z - PLAYER_HALF), maxZ = Math.floor(z + PLAYER_HALF);
    for (let by = minY; by <= maxY; by++) {
      for (let bz = minZ; bz <= maxZ; bz++) {
        for (let bx = minX; bx <= maxX; bx++) {
          if (w.isSolidAt(bx, by, bz)) return true;
        }
      }
    }
    return false;
  }

  blockAtFeet() {
    return this.world.getBlock(Math.floor(this.x), Math.floor(this.y + 0.1), Math.floor(this.z));
  }
  blockAtEye() {
    return this.world.getBlock(Math.floor(this.x), Math.floor(this.eyeY), Math.floor(this.z));
  }

  updateWaterState() {
    const b = this.world.getBlock(Math.floor(this.x), Math.floor(this.y + 0.4), Math.floor(this.z));
    this.inWater = b === B.water;
  }

  /** 沿单轴推进并处理碰撞。axis: 0=x 1=y 2=z */
  moveAxis(axis, amount) {
    if (amount === 0) return false;
    const steps = Math.max(1, Math.ceil(Math.abs(amount) / STEP));
    const inc = amount / steps;
    const p = axis === 0 ? 'x' : axis === 1 ? 'y' : 'z';
    for (let i = 0; i < steps; i++) {
      const before = this[p];
      this[p] = before + inc;
      if (!this.intersectsSolid(this.x, this.y, this.z)) continue;
      // 吸附到方块边界
      if (axis === 1) {
        if (inc < 0) { this.y = Math.floor(this.y) + 1 + EPS; this.onGround = true; }
        else this.y = Math.ceil(this.y + PLAYER_HEIGHT) - PLAYER_HEIGHT - EPS;
        this.vy = 0;
      } else {
        if (inc > 0) this[p] = Math.floor(this[p] + PLAYER_HALF) - PLAYER_HALF - EPS;
        else this[p] = Math.floor(this[p] - PLAYER_HALF) + 1 + PLAYER_HALF + EPS;
        this[axis === 0 ? 'vx' : 'vz'] = 0;
      }
      return true;
    }
    return false;
  }

  /**
   * @param input {forward, strafe, jump, sneak, sprint, up, down}
   */
  update(dt, input) {
    this.updateWaterState();

    // ---- 期望水平速度 ----
    let speed = this.flying ? FLY_SPEED
      : this.inWater ? SWIM_SPEED
        : (this.sprinting ? SPRINT_SPEED : WALK_SPEED);
    if (this.sneaking && !this.flying) speed = 1.9;

    const sin = Math.sin(this.yaw), cos = Math.cos(this.yaw);
    let fx = 0, fz = 0;
    if (input.forward) { fx -= sin; fz -= cos; }
    if (input.back) { fx += sin; fz += cos; }
    if (input.left) { fx -= cos; fz += sin; }
    if (input.right) { fx += cos; fz -= sin; }
    const len = Math.hypot(fx, fz);
    if (len > 0) { fx /= len; fz /= len; }

    // 地面/水中给一点惯性，空中保留大部分动量
    const accel = this.onGround || this.inWater ? 14 : this.flying ? 14 : 5.5;
    this.vx += (fx * speed - this.vx) * Math.min(1, accel * dt);
    this.vz += (fz * speed - this.vz) * Math.min(1, accel * dt);

    // ---- 竖直 ----
    if (this.flying) {
      let vy = 0;
      if (input.up) vy += FLY_SPEED;
      if (input.down) vy -= FLY_SPEED;
      this.vy += (vy - this.vy) * Math.min(1, 14 * dt);
    } else if (this.inWater) {
      this.vy += (input.jump ? 16 : -5) * dt;
      this.vy = Math.max(-3.2, Math.min(3.6, this.vy));
      this.vy *= (1 - Math.min(1, 2.2 * dt));
    } else {
      this.vy -= GRAVITY * dt;
      if (this.vy < -TERMINAL_VELOCITY) this.vy = -TERMINAL_VELOCITY;
      if (input.jump && this.onGround) {
        this.vy = JUMP_SPEED;
        this.onGround = false;
      }
    }

    // ---- 推进：先竖直，再水平（X/Z 分开）----
    this.onGround = false;
    const dy = this.vy * dt;
    const dx = this.vx * dt, dz = this.vz * dt;

    if (dy !== 0 && this.moveAxis(1, dy) && dy < 0) this.onGround = true;

    const x0 = this.x, y0 = this.y, z0 = this.z;
    let stepped = false;
    // 潜行时不自动上台阶，这样玩家可以贴着墙根走
    if ((dx !== 0 || dz !== 0) && this.onGround && !this.flying && !this.sneaking) {
      // 抬高 → 走 → 检查。抬高后要先走再看结果：在抬高之前就做碰撞判定，
      // 玩家此刻还压在挡路的方块上，判定必然失败，台阶就永远跨不过去。
      this.y = y0 + STEP_HEIGHT;
      this.moveAxis(0, dx);
      this.moveAxis(2, dz);
      if (this.intersectsSolid(this.x, this.y, this.z)) {
        this.x = x0; this.y = y0; this.z = z0;
      } else {
        stepped = true;
        this.moveAxis(1, -(STEP_HEIGHT + 0.05));   // 走过去了，落回地面
      }
    }
    if (!stepped) {
      this.moveAxis(0, dx);
      this.moveAxis(2, dz);
    }

    // 兜底：万一浮点误差让身体卡进方块，直接往上顶
    if (this.intersectsSolid(this.x, this.y, this.z)) {
      for (let i = 0; i < 12 && this.intersectsSolid(this.x, this.y, this.z); i++) this.y += 0.25;
      this.vy = 0;
    }

    if (this.y < -8) { this.y = WORLD_H + 6; this.vy = 0; }
    if (this.y > WORLD_H + 40) this.vy = Math.min(this.vy, 0);

    // 走路时的镜头轻微上下摆动
    const horiz = Math.hypot(this.vx, this.vz);
    if (this.onGround && horiz > 0.5) this.bobPhase += dt * horiz * 1.9;
    else this.bobPhase += dt * 0.6;
  }

  toggleFly() {
    this.flying = !this.flying;
    if (this.flying) this.vy = 0;
  }

  /** 把玩家放到某个地表之上（出生点用） */
  placeOnSurface(x, z) {
    const y = this.world.getSurfaceY(Math.floor(x), Math.floor(z));
    this.x = x; this.z = z; this.y = y + 1.02;
    this.vx = this.vy = this.vz = 0;
  }
}

export { PLAYER_HALF, PLAYER_HEIGHT, PLAYER_EYE };
