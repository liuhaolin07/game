// 背包与物品栏。
// 保持轻量：一个 9 格快捷栏（可滚动/按键切换）+ 一个更大的存储区，
// 方块和工具共用一套物品 id（工具 id 从 TOOL_BASE 起，避开方块 id 空间）。

import { B } from '../core/blocks.js';
import { TOOL_BASE, itemName, isTool, toolOf } from '../core/items.js';

export const HOTBAR_SIZE = 9;
export const BACKPACK_SIZE = 27;

export class Inventory {
  constructor() {
    this.hotbar = new Array(HOTBAR_SIZE).fill(null);
    this.backpack = new Array(BACKPACK_SIZE).fill(null);
    this.selected = 0;
    this.listeners = [];
  }

  get slots() { return this.hotbar; }
  get heldItem() { return this.hotbar[this.selected]; }
  get heldCount() { return this.hotbar[this.selected] ? this.hotbar[this.selected].n : 0; }

  /** 手持物对应的方块 id；是工具或空手则返回 null */
  heldBlock() {
    const s = this.heldItem;
    if (!s) return null;
    if (isTool(s.id)) return null;
    return s.id;
  }

  select(i) {
    this.selected = ((i % HOTBAR_SIZE) + HOTBAR_SIZE) % HOTBAR_SIZE;
    this.emit();
  }
  scroll(dir) { this.select(this.selected + dir); }

  /** 加入物品：先并入已有堆，再找空格。返回实际加入的数量。 */
  add(id, n = 1) {
    if (id === null || id === undefined) return 0;
    let remaining = n;
    const take = (room) => { const t = Math.min(room, remaining); remaining -= t; return t; };

    for (const arr of [this.hotbar, this.backpack]) {
      for (const s of arr) {
        if (s && s.id === id && s.n < 64) {
          s.n += take(64 - s.n);
          if (remaining === 0) { this.emit(); return n; }
        }
      }
    }
    for (const arr of [this.hotbar, this.backpack]) {
      for (let i = 0; i < arr.length; i++) {
        if (!arr[i]) {
          arr[i] = { id, n: take(64) };
          if (remaining === 0) { this.emit(); return n; }
        }
      }
    }
    this.emit();
    return n - remaining;   // 背包满了，返回少收了多少
  }

  count(id) {
    let c = 0;
    for (const arr of [this.hotbar, this.backpack])
      for (const s of arr) if (s && s.id === id) c += s.n;
    return c;
  }

  consumeHeld() {
    const s = this.hotbar[this.selected];
    if (!s) return;
    s.n--;
    if (s.n <= 0) this.hotbar[this.selected] = null;
    this.emit();
  }

  has(id) { return this.count(id) > 0; }

  onChange(fn) { this.listeners.push(fn); }
  emit() { for (const fn of this.listeners) fn(this); }

  serialize() {
    return JSON.stringify({ hotbar: this.hotbar, backpack: this.backpack, selected: this.selected });
  }
  load(json) {
    try {
      const o = JSON.parse(json);
      if (Array.isArray(o.hotbar)) this.hotbar = pad(o.hotbar, HOTBAR_SIZE);
      if (Array.isArray(o.backpack)) this.backpack = pad(o.backpack, BACKPACK_SIZE);
      this.selected = o.selected | 0;
    } catch { /* 存档坏了就当没有 */ }
    this.emit();
  }
}

function pad(arr, n) {
  const a = arr.slice(0, n);
  while (a.length < n) a.push(null);
  return a.map(s => (s && typeof s.id === 'number' && s.n > 0) ? { id: s.id, n: s.n | 0 } : null);
}

/** 新档的初始物品：木镐 + 一些基础方块 */
export function starterInventory() {
  const inv = new Inventory();
  inv.add(B.oak_planks, 32);
  inv.add(B.cobblestone, 32);
  inv.add(B.oak_log, 16);
  inv.add(B.torch, 8);
  inv.hotbar[0] = { id: TOOL_BASE + 0, n: 1 };     // 木镐
  inv.hotbar[1] = { id: B.oak_planks, n: 32 };
  inv.hotbar[2] = { id: B.cobblestone, n: 32 };
  inv.hotbar[3] = { id: B.oak_log, n: 16 };
  inv.hotbar[4] = { id: B.torch, n: 8 };
  inv.emit();
  return inv;
}

export { itemName, isTool, toolOf };
