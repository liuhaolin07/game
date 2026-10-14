// HUD：准星、物品栏、手持物名称、调试信息、提示气泡、伤害反馈。
// 全部用 DOM + CSS 做，风格照着原版来（像素字体、灰色描边、格子物品栏）。

import { BLOCKS } from '../core/blocks.js';
import { itemName, isTool, toolOf } from '../core/items.js';
import { drawToolIcon } from '../render/atlas.js';
import { biomeName } from '../world/biomes.js';
import { HOTBAR_SIZE } from './inventory.js';

export class Hud {
  constructor(root, inventory, atlasCanvas) {
    this.root = root;
    this.inv = inventory;
    this.atlasCanvas = atlasCanvas;
    this.iconCache = new Map();
    this.build();
    inventory.onChange(() => this.renderHotbar());
    this.renderHotbar();
  }

  build() {
    const el = (tag, cls, parent, html) => {
      const e = document.createElement(tag);
      if (cls) e.className = cls;
      if (html !== undefined) e.innerHTML = html;
      (parent || this.root).appendChild(e);
      return e;
    };
    this.el = {};

    this.el.cross = el('div', 'crosshair', this.root);
    this.el.cross.innerHTML = '<i></i><i></i>';

    this.el.hotbar = el('div', 'hotbar', this.root);
    this.el.slots = [];
    for (let i = 0; i < HOTBAR_SIZE; i++) {
      const s = el('div', 'slot', this.el.hotbar);
      const num = el('span', 'num', s, String(i + 1));
      const ico = el('canvas', 'icon', s);
      ico.width = ico.height = 32;
      const cnt = el('span', 'count', s, '');
      s.addEventListener('mousedown', (e) => { e.preventDefault(); this.inv.select(i); });
      this.el.slots.push({ s, ico, cnt, num });
    }

    this.el.itemName = el('div', 'item-name', this.root, '');
    this.el.toast = el('div', 'toast', this.root, '');
    this.el.debug = el('div', 'debug', this.root, '');
    this.el.debug.style.display = 'none';
    this.el.vignette = el('div', 'vignette', this.root, '');
    this.el.water = el('div', 'water-overlay', this.root, '');
    this.el.water.style.display = 'none';
    this.el.progress = el('div', 'mine-bar', this.root, '');
    this.el.progressFill = el('i', '', this.el.progress, '');
    this.el.progress.style.display = 'none';
  }

  iconFor(id) {
    if (this.iconCache.has(id)) return this.iconCache.get(id);
    let c;
    if (isTool(id)) {
      const t = toolOf(id);
      const src = drawToolIcon(t.kind, t.tier);
      c = document.createElement('canvas');
      c.width = c.height = 32;
      const g = c.getContext('2d');
      g.imageSmoothingEnabled = false;
      g.drawImage(src, 0, 0, 32, 32);
    } else {
      c = document.createElement('canvas');
      c.width = c.height = 32;
      const g = c.getContext('2d');
      g.imageSmoothingEnabled = false;
      const def = BLOCKS[id];
      if (def) {
        const tile = def.tiles[2];
        g.drawImage(this.atlasCanvas, (tile % 16) * 16, Math.floor(tile / 16) * 16, 16, 16, 0, 0, 32, 32);
      }
    }
    this.iconCache.set(id, c);
    return c;
  }

  renderHotbar() {
    for (let i = 0; i < HOTBAR_SIZE; i++) {
      const { s, ico, cnt } = this.el.slots[i];
      const item = this.inv.hotbar[i];
      s.classList.toggle('sel', i === this.inv.selected);
      cnt.textContent = item && item.n > 1 ? String(item.n) : '';
      const want = item ? item.id : null;
      if (ico.dataset.id !== String(want)) {
        ico.dataset.id = String(want);
        const g = ico.getContext('2d');
        g.clearRect(0, 0, 32, 32);
        if (item) g.drawImage(this.iconFor(item.id), 0, 0);
      }
    }
    this.showItemName();
  }

  showItemName() {
    const id = this.inv.heldItem ? this.inv.heldItem.id : null;
    this.el.itemName.textContent = id === null ? '空手' : itemName(id);
    this.el.itemName.classList.remove('fade');
    void this.el.itemName.offsetWidth;      // 强制回流以重启动画
    this.el.itemName.classList.add('fade');
  }

  say(msg, ms = 1800) {
    this.el.toast.textContent = msg;
    this.el.toast.classList.remove('fade');
    void this.el.toast.offsetWidth;
    this.el.toast.classList.add('fade');
    clearTimeout(this._toastT);
    this._toastT = setTimeout(() => { this.el.toast.textContent = ''; }, ms);
  }

  setMining(progress) {
    if (progress <= 0) { this.el.progress.style.display = 'none'; return; }
    this.el.progress.style.display = 'block';
    this.el.progressFill.style.width = (progress * 100).toFixed(1) + '%';
  }

  setUnderwater(on) {
    this.el.water.style.display = on ? 'block' : 'none';
  }

  toggleDebug() {
    this.el.debug.style.display = this.el.debug.style.display === 'none' ? 'block' : 'none';
    return this.el.debug.style.display !== 'none';
  }
  get debugVisible() { return this.el.debug.style.display !== 'none'; }

  setDebug(lines) {
    if (!this.debugVisible) return;
    this.el.debug.textContent = lines;
  }
}

export { biomeName };
