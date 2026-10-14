# -*- coding: utf-8 -*-
"""
泰拉瑞亚复刻 —— 像素美术素材生成器

运行:  python tools/gen_assets.py
输出:  assets/tiles.png    方块图集 (16x16/格)
        assets/items.png    物品图标图集 (16x16/格)
        assets/entities.png 角色/怪物精灵图集 (尺寸不固定)
        assets/gui.png      界面元素图集
        assets/atlas.txt    清单: <文件名> <名字> <x> <y> <w> <h>

所有素材由代码程序化生成, 无需外部资源, 可重复生成。
"""
import math
import os
import random

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "assets"))

T = 16          # 方块/图标边长
GRID = 16       # 每行格数
PREVIEW = 4     # 预览放大倍数(写入 preview.png)


# --------------------------------------------------------------------------
# 绘图小工具
# --------------------------------------------------------------------------
def img(w, h):
    return Image.new("RGBA", (w, h), (0, 0, 0, 0))


def put(im, x, y, c):
    if 0 <= x < im.width and 0 <= y < im.height and c[3] > 0:
        im.putpixel((int(x), int(y)), c)


def rect(im, x, y, w, h, c):
    for j in range(int(h)):
        for i in range(int(w)):
            put(im, x + i, y + j, c)


def ell(im, cx, cy, rx, ry, c):
    for j in range(int(cy - ry), int(cy + ry) + 1):
        for i in range(int(cx - rx), int(cx + rx) + 1):
            d = ((i - cx) / rx) ** 2 + ((j - cy) / ry) ** 2
            if d <= 1.0:
                put(im, i, j, c)


def outline(im, c):
    """给不透明区域描一圈外轮廓"""
    src = im.load()
    w, h = im.size
    edges = []
    for y in range(h):
        for x in range(w):
            if src[x, y][3] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and src[nx, ny][3] > 200:
                    edges.append((x, y))
                    break
    for p in edges:
        put(im, p[0], p[1], c)


def shade(c, f):
    """亮度调整, f<1 变暗 f>1 变亮"""
    return (min(255, int(c[0] * f)), min(255, int(c[1] * f)),
            min(255, int(c[2] * f)), c[3])


def mix(c1, c2, t):
    return (int(c1[0] + (c2[0] - c1[0]) * t), int(c1[1] + (c2[1] - c1[1]) * t),
            int(c1[2] + (c2[2] - c1[2]) * t), int(c1[3] + (c2[3] - c1[3]) * t))


def speckle(im, x, y, w, h, colors, n, rng, clumpy=False):
    """在区域里撒噪点, clumpy=True 时用 2x2 块, 让纹理更接近手绘"""
    for _ in range(n):
        px_ = rng.randrange(x, x + w)
        py_ = rng.randrange(y, y + h)
        c = rng.choice(colors)
        put(im, px_, py_, c)
        if clumpy:
            put(im, px_ + 1, py_, c)
            put(im, px_, py_ + 1, c)


# --------------------------------------------------------------------------
# 图集打包
# --------------------------------------------------------------------------
class Atlas:
    def __init__(self, cell, cols=GRID):
        self.cell = cell
        self.cols = cols
        self.frames = []          # (name, image)

    def add(self, name, image):
        self.frames.append((name, image))
        return image

    def save(self, filename):
        cell = self.cell
        rows = (len(self.frames) + self.cols - 1) // self.cols
        sheet = Image.new("RGBA", (self.cols * cell, rows * cell), (0, 0, 0, 0))
        lines = []
        for i, (name, im) in enumerate(self.frames):
            cx = (i % self.cols) * cell
            cy = (i // self.cols) * cell
            sheet.alpha_composite(im, (cx, cy))
            lines.append("%s %s %d %d %d %d" % (filename, name, cx, cy, im.width, im.height))
        sheet.save(os.path.join(OUT, filename))
        return lines


class Packer:
    """自由尺寸精灵图集: 逐行装箱"""

    def __init__(self, width=512):
        self.width = width
        self.frames = []

    def add(self, name, image):
        self.frames.append((name, image))

    def save(self, filename):
        x = y = row_h = 0
        placed = []
        for name, im in self.frames:
            if x + im.width + 1 > self.width:
                x = 0
                y += row_h + 1
                row_h = 0
            placed.append((name, im, x, y))
            x += im.width + 1
            row_h = max(row_h, im.height)
        h = y + row_h
        sheet = Image.new("RGBA", (self.width, h), (0, 0, 0, 0))
        lines = []
        for name, im, px_, py_ in placed:
            sheet.alpha_composite(im, (px_, py_))
            lines.append("%s %s %d %d %d %d" % (filename, name, px_, py_, im.width, im.height))
        sheet.save(os.path.join(OUT, filename))
        return lines


# --------------------------------------------------------------------------
# 方块画法
# --------------------------------------------------------------------------
def t_rock(base, dark, light, rng, veins=True):
    im = img(T, T)
    rect(im, 0, 0, T, T, base)
    speckle(im, 0, 0, T, T, [dark, light, shade(dark, .9)], 26, rng, clumpy=True)
    if veins:
        # 裂纹
        for _ in range(rng.randrange(1, 3)):
            x0 = rng.randrange(T)
            y0 = rng.randrange(T)
            for k in range(rng.randrange(3, 8)):
                put(im, x0, y0, shade(dark, .8))
                x0 += rng.choice((-1, 0, 1))
                y0 += rng.choice((0, 1))
    return im


def t_ore(base, dark, light, ore, ore2, rng, count=6):
    im = t_rock(base, dark, light, rng)
    for _ in range(count):
        cx, cy = rng.randrange(2, T - 3), rng.randrange(2, T - 3)
        n = rng.randrange(3, 7)
        for k in range(n):
            x = cx + rng.randrange(-1, 2)
            y = cy + rng.randrange(-1, 2)
            put(im, x, y, ore)
        put(im, cx, cy - 1, ore2)
    return im


def t_dirt(dark, mid, light):
    def f(rng):
        im = img(T, T)
        rect(im, 0, 0, T, T, mid)
        speckle(im, 0, 0, T, T, [dark, light], 30, rng, clumpy=True)
        return im
    return f


def t_grass(soil, grass_d, grass_m, grass_l, rng):
    im = img(T, T)
    rect(im, 0, 0, T, T, soil[1])
    speckle(im, 0, 0, T, T, [soil[0], soil[2]], 30, rng, clumpy=True)
    h = 4
    for x in range(T):
        hh = h + rng.choice((0, 0, 0, 1, 1, 2))
        for y in range(hh):
            c = grass_l if y == 0 else (grass_m if y < hh - 1 else grass_d)
            put(im, x, y, c)
    for _ in range(6):                      # 草叶点缀
        x, y = rng.randrange(T), rng.randrange(h, T)
        if im.getpixel((x, y))[3] > 0:
            put(im, x, y, grass_d)
    return im


def t_wood():
    im = img(T, T)
    base = (139, 100, 63, 255)
    rect(im, 0, 0, T, T, base)
    for x in range(T):
        v = 0.88 + 0.24 * ((x * 7) % 5) / 4.0
        col = shade(base, v)
        for y in range(T):
            put(im, x, y, col)
    r = random.Random(3)
    for _ in range(3):                       # 树皮纹
        x = r.randrange(1, T - 1)
        col = shade(base, .72)
        for y in range(0, T, 2):
            put(im, x, y, col)
    rect(im, 0, 0, T, 1, shade(base, 1.18))
    rect(im, 0, T - 1, T, 1, shade(base, .66))
    return im


def t_leaves(a, b, c, rng):
    im = img(T, T)
    rect(im, 0, 0, T, T, b)
    for _ in range(46):
        put(im, rng.randrange(T), rng.randrange(T), rng.choice((a, b, c)))
    # 边缘打洞, 避免方块感
    for x in range(T):
        if rng.random() < .35:
            put(im, x, 0, (0, 0, 0, 0))
        if rng.random() < .35:
            put(im, x, T - 1, (0, 0, 0, 0))
        if rng.random() < .25:
            put(im, x, rng.randrange(T), (0, 0, 0, 0))
    for y in range(T):
        if rng.random() < .35:
            put(im, 0, y, (0, 0, 0, 0))
        if rng.random() < .35:
            put(im, T - 1, y, (0, 0, 0, 0))
    return im


def t_sand():
    rng = random.Random(7)
    im = img(T, T)
    base = (219, 199, 133, 255)
    rect(im, 0, 0, T, T, base)
    speckle(im, 0, 0, T, T, [shade(base, .9), shade(base, 1.06)], 30, rng)
    return im


def t_snow():
    rng = random.Random(11)
    im = img(T, T)
    base = (233, 240, 248, 255)
    rect(im, 0, 0, T, T, base)
    speckle(im, 0, 0, T, T, [(205, 219, 236, 255), (255, 255, 255, 255)], 22, rng)
    return im


def t_ice():
    rng = random.Random(13)
    im = img(T, T)
    base = (141, 205, 232, 220)
    rect(im, 0, 0, T, T, base)
    speckle(im, 0, 0, T, T, [(170, 220, 240, 220), (110, 180, 215, 220)], 20, rng)
    for _ in range(3):
        x0, y0 = rng.randrange(T), rng.randrange(T)
        for k in range(6):
            put(im, x0, y0, (200, 235, 250, 235))
            x0 += rng.choice((-1, 0, 1))
            y0 += 1
    return im


def t_glass():
    im = img(T, T)
    rect(im, 0, 0, T, T, (168, 200, 214, 70))
    rect(im, 0, 0, T, 1, (206, 232, 242, 170))
    rect(im, 0, T - 1, T, 1, (120, 155, 175, 170))
    rect(im, 0, 0, 1, T, (190, 222, 235, 140))
    rect(im, T - 1, 0, 1, T, (120, 155, 175, 140))
    for i in range(4, 10):                    # 反光斜线
        put(im, i, T - 1 - i // 2, (235, 250, 255, 190))
    return im


def t_bricks(brick, mortar, rng):
    im = img(T, T)
    rect(im, 0, 0, T, T, mortar)
    h = 4
    for row in range(T // h):
        off = 0 if row % 2 == 0 else 4
        for col in range(-1, 3):
            x = col * 8 + off
            y = row * h
            rect(im, x, y, 7, h - 1, brick)
            rect(im, x, y, 7, 1, shade(brick, 1.14))
            rect(im, x, y + h - 2, 7, 1, shade(brick, .82))
    return im


def t_torch():
    im = img(T, T)
    rect(im, 7, 6, 2, 8, (128, 92, 56, 255))     # 木柄
    rect(im, 7, 6, 1, 8, (168, 124, 76, 255))
    ell(im, 8, 4, 2.6, 3.4, (255, 176, 48, 230))  # 火焰
    ell(im, 8, 4, 1.5, 2.2, (255, 232, 130, 255))
    put(im, 8, 2, (255, 255, 220, 255))
    return im


def t_campfire():
    rng = random.Random(21)
    im = img(T, T)
    for y in range(11, 15):
        for x in range(2, 14):
            put(im, x, y, (96, 68, 44, 255))
    for i, x in enumerate((3, 6, 9, 12)):
        rect(im, x, 9 - (i % 2) * 2, 2, 6, (146, 104, 62, 255))
    ell(im, 8, 7, 4.6, 4.0, (240, 110, 40, 220))
    ell(im, 8, 7, 3.0, 2.8, (255, 190, 70, 240))
    ell(im, 8, 6, 1.4, 1.6, (255, 246, 190, 255))
    return im


def t_workbench():
    im = img(T, T)
    wood = (154, 112, 70, 255)
    rect(im, 1, 5, 14, 4, wood)
    rect(im, 1, 5, 14, 1, shade(wood, 1.18))
    rect(im, 1, 8, 14, 1, shade(wood, .7))
    rect(im, 2, 9, 2, 5, shade(wood, .82))
    rect(im, 12, 9, 2, 5, shade(wood, .82))
    rect(im, 5, 2, 6, 3, (108, 74, 44, 255))     # 工具
    rect(im, 6, 1, 2, 2, (196, 196, 210, 255))
    rect(im, 9, 1, 2, 2, (196, 196, 210, 255))
    rect(im, 3, 4, 10, 1, (208, 208, 220, 255))
    return im


def t_furnace(lit):
    im = img(T, T)
    stone = (122, 122, 128, 255)
    rect(im, 2, 3, 12, 12, stone)
    rect(im, 2, 3, 12, 1, shade(stone, 1.2))
    rect(im, 1, 2, 14, 1, shade(stone, 1.35))
    rect(im, 4, 8, 8, 7, (38, 34, 32, 255))
    if lit:
        rect(im, 5, 11, 6, 4, (214, 96, 28, 255))
        rect(im, 6, 12, 4, 3, (255, 178, 60, 255))
        rect(im, 7, 13, 2, 2, (255, 240, 170, 255))
    else:
        rect(im, 4, 8, 8, 1, shade(stone, .7))
    return im


def t_anvil():
    im = img(T, T)
    m = (86, 88, 98, 255)
    rect(im, 2, 6, 12, 3, m)
    rect(im, 2, 6, 12, 1, shade(m, 1.3))
    rect(im, 5, 9, 6, 4, shade(m, .85))
    rect(im, 3, 13, 10, 2, shade(m, .7))
    rect(im, 12, 4, 3, 2, shade(m, 1.1))
    return im


def t_chest():
    im = img(T, T)
    wood = (162, 110, 58, 255)
    rect(im, 1, 6, 14, 9, wood)
    rect(im, 1, 6, 14, 1, shade(wood, 1.2))
    rect(im, 1, 4, 14, 3, shade(wood, .86))
    rect(im, 1, 4, 14, 1, shade(wood, 1.05))
    rect(im, 1, 9, 14, 1, (74, 60, 44, 255))
    rect(im, 7, 8, 2, 4, (222, 190, 96, 255))
    rect(im, 7, 8, 2, 1, (250, 232, 160, 255))
    return im


def t_door(closed):
    im = img(T, T)
    wood = (156, 106, 60, 255)
    if closed:
        rect(im, 1, 2, 14, 14, wood)
        rect(im, 1, 2, 14, 1, shade(wood, 1.2))
        rect(im, 1, 2, 1, 14, shade(wood, 1.1))
        rect(im, 14, 2, 1, 14, shade(wood, .7))
        rect(im, 7, 3, 1, 13, shade(wood, .72))
        rect(im, 10, 8, 2, 2, (226, 196, 104, 255))
    else:
        rect(im, 1, 2, 3, 14, wood)
        rect(im, 12, 2, 3, 14, wood)
        rect(im, 0, 1, 16, 1, (92, 66, 40, 255))
    return im


def t_platform():
    im = img(T, T)
    wood = (150, 104, 60, 255)
    rect(im, 0, 0, T, 4, wood)
    rect(im, 0, 0, T, 1, shade(wood, 1.2))
    rect(im, 0, 3, T, 1, shade(wood, .68))
    return im


def t_fence():
    im = img(T, T)
    wood = (150, 104, 60, 255)
    rect(im, 2, 0, 2, T, wood)
    rect(im, 12, 0, 2, T, wood)
    rect(im, 0, 5, T, 2, shade(wood, 1.05))
    rect(im, 0, 10, T, 2, shade(wood, .95))
    return im


def t_vine():
    rng = random.Random(31)
    im = img(T, T)
    g = (46, 118, 44, 255)
    for y in range(T):
        x = 3 + int(2.4 * math.sin(y * .8))
        rect(im, x, y, 2, 1, g)
        put(im, x - 1, y, shade(g, .78))
        put(im, x + 2, y, shade(g, 1.18))
    for _ in range(3):
        x = 1 + rng.randrange(0, T - 3)
        y = rng.randrange(2, T - 3)
        rect(im, x, y, 3, 2, (72, 150, 58, 255))
    return im


def t_water():
    im = img(T, T)
    rect(im, 0, 0, T, T, (58, 110, 205, 165))
    rng = random.Random(41)
    for y in range(T):
        for x in range(T):
            if rng.random() < .12:
                put(im, x, y, (92, 148, 226, 170))
            elif rng.random() < .06:
                put(im, x, y, (36, 82, 172, 170))
    return im


def t_lava():
    im = img(T, T)
    rect(im, 0, 0, T, T, (214, 88, 22, 255))
    rng = random.Random(43)
    for y in range(T):
        for x in range(T):
            r = rng.random()
            if r < .14:
                put(im, x, y, (250, 176, 52, 255))
            elif r < .24:
                put(im, x, y, (150, 40, 14, 255))
    return im


def t_glowstone():
    rng = random.Random(47)
    im = t_rock((178, 152, 96, 255), (150, 126, 78, 255), (214, 196, 138, 255), rng)
    for _ in range(7):
        cx, cy = rng.randrange(2, T - 3), rng.randrange(2, T - 3)
        ell(im, cx, cy, 1.8, 1.8, (255, 226, 130, 255))
        put(im, cx, cy, (255, 250, 214, 255))
    return im


def t_gem(c, c2):
    rng = random.Random(hash(c) & 0xFFFF)
    im = t_rock((122, 122, 128, 255), (100, 100, 108, 255), (150, 150, 158, 255), rng, veins=False)
    for _ in range(4):
        cx, cy = rng.randrange(3, T - 4), rng.randrange(3, T - 4)
        pts = [(cx, cy - 2), (cx + 2, cy), (cx, cy + 2), (cx - 2, cy)]
        for i in range(4):
            a, b = pts[i], pts[(i + 1) % 4]
            steps = 8
            for k in range(steps + 1):
                put(im, a[0] + (b[0] - a[0]) * k / steps,
                    a[1] + (b[1] - a[1]) * k / steps, c)
        ell(im, cx, cy, 1.2, 1.2, c2)
    return im


# --------------------------------------------------------------------------
# 物品画法 (16x16 图标)
# --------------------------------------------------------------------------
def i_block(tile_fn, rng_seed=0):
    """物品栏里显示的方块图标: 立体小方块"""
    im = img(T, T)
    base = tile_fn(random.Random(rng_seed or 5))
    top = base.crop((0, 0, T, 5))
    front = base.crop((0, 5, T, T))
    front = front.resize((12, 9))
    top = top.resize((12, 5))
    im.alpha_composite(top, (2, 1))
    im.alpha_composite(front, (2, 5))
    outline(im, (24, 18, 14, 220))
    return im


def i_ore(ore, ore2):
    im = i_block(lambda r: t_ore((122, 122, 128, 255), (98, 98, 106, 255),
                                  (150, 150, 158, 255), ore, ore2, r, 7), 9)
    return im


def i_pick(tier_color, dark):
    im = img(T, T)
    rect(im, 2, 11, 3, 5, (140, 100, 60, 255))
    rect(im, 2, 11, 1, 5, (176, 132, 84, 255))
    rect(im, 4, 9, 9, 2, tier_color)
    rect(im, 4, 9, 9, 1, shade(tier_color, 1.3))
    rect(im, 4, 11, 9, 1, shade(tier_color, .68))
    rect(im, 11, 8, 2, 4, tier_color)
    rect(im, 12, 8, 1, 4, shade(tier_color, .7))
    outline(im, (26, 20, 16, 230))
    return im


def i_axe(tier_color):
    im = img(T, T)
    rect(im, 6, 4, 3, 11, (140, 100, 60, 255))
    rect(im, 6, 4, 1, 11, (176, 132, 84, 255))
    rect(im, 3, 3, 5, 6, tier_color)
    rect(im, 3, 3, 5, 1, shade(tier_color, 1.3))
    rect(im, 3, 8, 5, 1, shade(tier_color, .7))
    rect(im, 7, 2, 3, 2, shade(tier_color, .85))
    outline(im, (26, 20, 16, 230))
    return im


def i_sword(blade, guard):
    im = img(T, T)
    for k in range(11):
        c = blade if k % 3 else shade(blade, 1.25)
        rect(im, 4 + k, 13 - k, 1, 1, c)
    rect(im, 3, 12, 8, 1, shade(blade, .7))
    rect(im, 3, 12, 8, 1, guard)
    rect(im, 4, 13, 2, 2, (120, 84, 48, 255))
    rect(im, 4, 13, 1, 2, (152, 108, 62, 255))
    put(im, 13, 2, (255, 255, 255, 220))
    outline(im, (26, 20, 16, 230))
    return im


def i_bar(c):
    im = img(T, T)
    rect(im, 2, 7, 12, 5, c)
    rect(im, 2, 7, 12, 1, shade(c, 1.28))
    rect(im, 2, 11, 12, 1, shade(c, .66))
    rect(im, 3, 6, 3, 1, shade(c, 1.12))
    rect(im, 9, 6, 3, 1, shade(c, 1.12))
    outline(im, (26, 20, 16, 230))
    return im


def i_orb(c, c2):
    im = img(T, T)
    ell(im, 8, 8, 5, 5, shade(c, .55))
    ell(im, 8, 8, 4.2, 4.2, c)
    ell(im, 6.6, 6.6, 1.8, 1.8, c2)
    put(im, 6, 5, (255, 255, 255, 200))
    outline(im, (18, 14, 30, 230))
    return im


def i_gem(c, c2):
    im = img(T, T)
    pts = [(8, 2), (13, 7), (8, 14), (3, 7)]
    for i in range(4):
        a, b = pts[i], pts[(i + 1) % 4]
        for k in range(13):
            put(im, a[0] + (b[0] - a[0]) * k / 12, a[1] + (b[1] - a[1]) * k / 12, c)
    for i in range(4):
        a, b = pts[i], pts[(i + 1) % 4]
        for k in range(13):
            put(im, a[0] + (b[0] - a[0]) * k / 24, a[1] + (b[1] - a[1]) * k / 24, c2)
    put(im, 7, 5, (255, 255, 255, 230))
    outline(im, (20, 16, 30, 230))
    return im


def i_potion(c, cork=(178, 132, 84, 255)):
    im = img(T, T)
    rect(im, 6, 1, 4, 2, (188, 200, 214, 255))
    rect(im, 5, 3, 6, 3, (206, 220, 232, 255))
    rect(im, 4, 6, 8, 8, (200, 214, 228, 90))
    rect(im, 5, 7, 6, 6, c)
    rect(im, 5, 7, 2, 6, shade(c, 1.35))
    outline(im, (26, 20, 16, 230))
    return im


def i_generic(color, shape="round"):
    im = img(T, T)
    if shape == "round":
        ell(im, 8, 8, 5.4, 5.4, color)
        ell(im, 6.4, 6.4, 2, 2, shade(color, 1.35))
    elif shape == "blob":
        for y in range(3, 14):
            w = int(6 - abs(8 - y) * .3)
            rect(im, 8 - w, y, w * 2, 1, color)
    else:
        rect(im, 3, 5, 10, 8, color)
    outline(im, (24, 18, 16, 220))
    return im


def i_crown():
    im = img(T, T)
    g = (226, 190, 84, 255)
    rect(im, 2, 9, 12, 5, g)
    rect(im, 2, 13, 12, 1, shade(g, .68))
    for x in (2, 6, 10, 13):
        rect(im, x, 4, 2, 5, g)
        put(im, x, 3, (255, 90, 90, 255))
        put(im, x + 1, 3, (255, 220, 120, 255))
    rect(im, 7, 11, 2, 2, (110, 200, 240, 255))
    outline(im, (40, 30, 10, 230))
    return im


def i_eye():
    im = img(T, T)
    ell(im, 8, 8, 6, 6, (120, 30, 40, 255))
    ell(im, 8, 8, 5, 5, (190, 60, 66, 255))
    ell(im, 8, 8, 2.6, 2.6, (232, 226, 200, 255))
    ell(im, 8, 8, 1.4, 2.2, (40, 20, 20, 255))
    put(im, 6, 6, (255, 255, 255, 230))
    outline(im, (40, 14, 18, 230))
    return im


def i_bow():
    im = img(T, T)
    for k in range(9):
        y = 3 + k
        rect(im, 11 - int((k ** 1.4) / 1.1), y, 1, 1, (150, 104, 58, 255))
        rect(im, 4 + int((k ** 1.4) / 1.1), y, 1, 1, (150, 104, 58, 255))
    for y in range(3, 12):
        put(im, 8, y, (226, 226, 226, 220))
    outline(im, (26, 20, 16, 230))
    return im


def i_arrow():
    im = img(T, T)
    for k in range(11):
        put(im, 3 + k, 12 - k, (150, 104, 58, 255))
    rect(im, 3, 12, 1, 1, (60, 60, 66, 255))
    put(im, 12, 2, (226, 226, 226, 255))
    put(im, 11, 2, (226, 226, 226, 255))
    put(im, 12, 3, (226, 226, 226, 255))
    rect(im, 3, 12, 3, 1, (230, 230, 230, 255))
    rect(im, 3, 13, 3, 1, (230, 230, 230, 255))
    return im


def i_torch_icon():
    im = img(T, T)
    rect(im, 7, 6, 3, 9, (140, 100, 60, 255))
    rect(im, 7, 6, 1, 9, (180, 134, 86, 255))
    ell(im, 8.5, 4, 3, 3.6, (255, 168, 44, 235))
    ell(im, 8.5, 4, 1.6, 2.2, (255, 234, 138, 255))
    outline(im, (26, 20, 16, 220))
    return im


# --------------------------------------------------------------------------
# 角色 / 怪物
# --------------------------------------------------------------------------
SKIN = (232, 190, 152, 255)
SKIN_D = (188, 148, 116, 255)
HAIR = (92, 62, 40, 255)
SHIRT = (72, 118, 196, 255)
SHIRT_D = (48, 82, 146, 255)
PANTS = (74, 74, 92, 255)
SHOE = (56, 42, 34, 255)


def humanoid(w, h, arm=0, leg=0, tilt_head=0, pal=None, hat=None, beard=None, extra=None):
    """用矩形拼出的小人, arm/leg 为肢体相位(像素偏移)"""
    p = pal or {}
    skin = p.get("skin", SKIN)
    skin_d = p.get("skin_d", SKIN_D)
    shirt = p.get("shirt", SHIRT)
    shirt_d = p.get("shirt_d", SHIRT_D)
    pants = p.get("pants", PANTS)
    shoe = p.get("shoe", SHOE)
    im = img(w, h)
    cx = w // 2
    hy = 6                                   # 头顶
    # 头
    rect(im, cx - 4, hy, 9, 8, skin)
    rect(im, cx - 4, hy, 9, 1, shade(skin, 1.1))
    rect(im, cx - 4, hy + 7, 9, 1, skin_d)
    rect(im, cx - 5, hy + 1, 1, 6, skin_d)
    rect(im, cx + 5, hy + 1, 1, 6, skin_d)
    # 头发
    hair = p.get("hair", HAIR)
    rect(im, cx - 5, hy - 1, 11, 3, hair)
    rect(im, cx - 5, hy + 1, 2, 4, hair)
    rect(im, cx + 4, hy + 1, 2, 3, hair)
    # 眼睛
    eye = p.get("eye", (28, 28, 40, 255))
    put(im, cx - 2, hy + 4, eye)
    put(im, cx + 2, hy + 4, eye)
    if beard:
        rect(im, cx - 4, hy + 6, 9, 4, beard)
        rect(im, cx - 2, hy + 6, 5, 1, (0, 0, 0, 0))
    if hat:
        rect(im, cx - 7, hy - 2, 15, 2, hat)
        rect(im, cx - 5, hy - 6, 11, 4, shade(hat, .9))
    # 身体
    by = hy + 9
    rect(im, cx - 5, by, 11, 11, shirt)
    rect(im, cx - 5, by, 11, 1, shade(shirt, 1.12))
    rect(im, cx - 5, by, 2, 11, shade(shirt, 1.1))
    rect(im, cx + 4, by, 2, 11, shirt_d)
    rect(im, cx - 5, by + 10, 11, 2, shirt_d)
    # 手臂
    ax = cx - 7 + arm
    rect(im, ax, by + 1, 3, 9, skin)
    rect(im, ax, by + 1, 1, 9, shade(skin, 1.08))
    rect(im, ax + 2, by + 1, 1, 9, skin_d)
    rect(im, ax, by + 9, 3, 2, skin)
    bx = cx + 5 - arm
    rect(im, bx, by + 1, 3, 9, shirt)
    rect(im, bx, by + 1, 1, 9, shade(shirt, 1.1))
    rect(im, bx, by + 9, 3, 2, skin)
    # 腿
    ly = by + 12
    rect(im, cx - 4, ly, 4, 8 - abs(leg) // 2, pants)
    rect(im, cx + 1, ly, 4, 8 - abs(leg) // 2, shade(pants, .86))
    rect(im, cx - 4, ly + 8 - abs(leg) // 2, 4, 2, shoe)
    rect(im, cx + 1, ly + 8 - abs(leg) // 2, 4, 2, shoe)
    if extra:
        extra(im, cx, hy, by, ly)
    outline(im, (26, 18, 16, 235))
    return im


def player_frame(n, total):
    """行走循环: 4 帧"""
    swing = [0, 1, 0, -1][n % 4]
    bob = [0, 0, -1, 0][n % 4]
    im = humanoid(20, 40, arm=swing, leg=swing)
    if bob:
        im2 = img(20, 40)
        im2.alpha_composite(im, (0, bob))
        im = im2
    return im


def zombie_frame(n, total):
    swing = [0, 1, 0, -1][n % 4]
    pal = {"skin": (124, 152, 106, 255), "skin_d": (92, 120, 80, 255),
           "shirt": (72, 92, 128, 255), "shirt_d": (52, 68, 98, 255),
           "pants": (58, 54, 52, 255), "hair": (44, 56, 40, 255),
           "eye": (200, 60, 60, 255)}

    def arm_up(im, cx, hy, by, ly):
        rect(im, cx + 5, by - 3, 3, 8, pal["shirt"])
        rect(im, cx + 5, by + 5, 3, 2, pal["skin"])
    im = humanoid(20, 40, arm=swing, leg=-swing, pal=pal, extra=arm_up)
    return im


def slime(color, dark, light, w=32, h=22, crown=False, seed=0):
    im = img(w, h)
    rng = random.Random(seed or w)
    cx = w / 2
    for y in range(h):
        t = y / h
        rx = cx * math.sin(min(1.0, 0.12 + t * 0.95) * math.pi * 0.5)
        if y >= h - 3:
            rx *= 1.05
        for x in range(w):
            if abs(x - cx) <= rx and t < 0.98:
                # 内部渐变 + 果冻高光
                k = (x - (cx - rx)) / max(1.0, 2 * rx)
                c = mix(light, dark, min(1.0, t * .8 + k * .5))
                if rng.random() < .07:
                    c = shade(c, .88)
                put(im, x, y, c)
    # 底部阴影
    for x in range(int(cx - 5), int(cx + 6)):
        put(im, x, h - 2, shade(dark, .6))
    # 高光
    ell(im, cx - rx * .42, h * .34, rx * .22, h * .16, (255, 255, 255, 120))
    # 眼睛
    ey = int(h * .55)
    for ex in (-4, 4):
        ell(im, cx + ex, ey, 2.0, 2.4, (18, 18, 24, 255))
        put(im, cx + ex - 1, ey - 1, (255, 255, 255, 230))
    # 嘴
    for x in range(int(cx - 3), int(cx + 4)):
        put(im, x, ey + 4, (18, 18, 24, 255))
    if crown:
        g = (226, 190, 84, 255)
        rect(im, int(cx - 7), 1, 14, 3, g)
        for x in (-7, -2, 3, 7):
            rect(im, int(cx + x), int(1 - 4), 2, 4, g)
            put(im, int(cx + x), int(1 - 5), (255, 90, 90, 255))
        rect(im, int(cx - 8), 4, 16, 1, shade(g, .7))
    outline(im, (18, 14, 20, 235))
    return im


def demon_eye(frame):
    im = img(28, 24)
    cx, cy = 14, 12
    ell(im, cx, cy, 11, 9, (118, 34, 44, 255))
    ell(im, cx, cy - 1, 9.6, 7.8, (172, 54, 62, 255))
    ell(im, cx, cy - 2, 7.2, 5.6, (200, 78, 82, 255))
    for _ in range(9):                       # 血丝
        a = frame * 0.5 + _ * 0.9
        x0 = cx + math.cos(a) * 10
        y0 = cy + math.sin(a) * 8
        for k in range(4):
            put(im, x0 + math.cos(a) * k, y0 + math.sin(a) * k, (128, 26, 34, 255))
    # 眼球
    look = [-1, 0, 1][frame % 3]
    ell(im, cx, cy - 1, 5.4, 5.4, (238, 232, 208, 255))
    ell(im, cx + look, cy - 1, 2.6, 3.0, (58, 30, 30, 255))
    ell(im, cx + look - 1, cy - 2, 1.2, 1.2, (255, 255, 255, 255))
    # 上眼睑阴影
    for x in range(cx - 10, cx + 11):
        d = abs(x - cx) / 10.0
        put(im, x, int(cy - 9 + d * 3), (96, 24, 32, 255))
    outline(im, (44, 12, 18, 240))
    return im


def servant(frame):
    im = img(16, 14)
    cx, cy = 8, 7
    ell(im, cx, cy, 5.4, 4.6, (140, 40, 50, 255))
    ell(im, cx, cy, 4.4, 3.8, (184, 64, 70, 255))
    look = [-1, 0, 1][frame % 3]
    ell(im, cx, cy, 2.2, 2.4, (232, 226, 200, 255))
    ell(im, cx + look, cy, 1.1, 1.4, (48, 24, 24, 255))
    for x in range(cx - 5, cx + 6):
        d = abs(x - cx) / 5.0
        put(im, x, int(cy - 4.4 + d), (104, 28, 36, 255))
    outline(im, (44, 12, 18, 240))
    return im


def eye_of_cthulhu(frame):
    """第 3 帧为第二形态(瞳孔收缩/张大)"""
    w = h = 60
    im = img(w, h)
    cx, cy = 30, 30
    muscle = (86, 30, 42, 255)
    flesh = (150, 52, 60, 255)
    flesh_l = (196, 84, 88, 255)
    # 血丝外壳
    for y in range(h):
        for x in range(w):
            d = math.hypot((x - cx) / 27.0, (y - cy) / 27.0)
            if d <= 1.0:
                base = mix(flesh_l, muscle, d * d)
                if (x * 7 + y * 13 + frame * 5) % 11 < 2:
                    base = shade(base, .78)
                put(im, x, y, base)
    # 血丝
    for k in range(16):
        a = k * 0.39 + frame * 0.12
        for j in range(6, 20):
            put(im, cx + math.cos(a) * j * 1.1, cy + math.sin(a) * j * 1.1, (104, 26, 34, 255))
    # 眼球
    er = 15 if frame < 2 else (13 if frame == 2 else 18)
    ell(im, cx, cy, er + 3, er + 3, (238, 232, 206, 255))
    ell(im, cx, cy, er, er, (250, 246, 226, 255))
    # 虹膜
    ir = 7.5
    ell(im, cx, cy, ir, ir, (78, 108, 84, 255))
    for k in range(6):
        a = k * 1.05 + frame * 0.2
        put(im, cx + math.cos(a) * ir * .8, cy + math.sin(a) * ir * .8, (44, 66, 50, 255))
    pupil = 2.2 if frame < 3 else 5.2
    ell(im, cx, cy, pupil, pupil, (12, 12, 16, 255))
    put(im, cx - er * .45, cy - er * .5, (255, 255, 255, 235))
    # 眼睑
    for x in range(cx - 18, cx + 19):
        d = abs(x - cx) / 18.0
        top = int(cy - er - 4 + d * d * 7)
        for y in range(top, top + 2):
            put(im, x, y, (74, 22, 30, 255))
    outline(im, (36, 10, 16, 245))
    return im


def slime_crown_frame(n, total):
    return slime((66, 130, 208, 235), (26, 66, 132, 235), (128, 190, 240, 235),
                 w=64, h=48, crown=True, seed=3)


def npc_guide(frame):
    pal = {"skin": (226, 194, 160, 255), "skin_d": (188, 158, 128, 255),
           "shirt": (188, 158, 74, 255), "shirt_d": (146, 118, 52, 255),
           "pants": (110, 86, 62, 255), "hair": (226, 226, 230, 255),
           "eye": (40, 40, 48, 255)}

    def add(im, cx, hy, by, ly):
        # 眼镜 + 白胡子
        rect(im, cx - 3, hy + 3, 3, 2, (60, 60, 70, 200))
        rect(im, cx + 1, hy + 3, 3, 2, (60, 60, 70, 200))
        rect(im, cx, hy + 3, 1, 1, (60, 60, 70, 200))
    im = humanoid(20, 40, arm=[0, 1, 0, -1][frame % 4], leg=[0, 1, 0, -1][frame % 4],
                 pal=pal, beard=(236, 236, 240, 255), extra=add)
    return im


def npc_merchant(frame):
    pal = {"skin": (232, 188, 146, 255), "skin_d": (188, 148, 112, 255),
           "shirt": (196, 74, 60, 255), "shirt_d": (150, 50, 42, 255),
           "pants": (66, 62, 78, 255), "hair": (60, 44, 32, 255),
           "eye": (36, 36, 44, 255)}

    def add(im, cx, hy, by, ly):
        rect(im, cx - 8, hy - 1, 17, 2, (58, 46, 38, 255))   # 帽檐
        rect(im, cx - 5, hy - 6, 11, 5, (74, 58, 46, 255))    # 帽冠
        rect(im, cx - 5, hy - 2, 11, 1, (196, 186, 120, 255))  # 帽带
        rect(im, cx - 4, by + 11, 9, 5, (236, 226, 200, 255))  # 围裙
    im = humanoid(20, 40, arm=[0, 1, 0, -1][frame % 4], leg=0, pal=pal, extra=add)
    return im


# --------------------------------------------------------------------------
# GUI / 天空
# --------------------------------------------------------------------------
def g_heart(filled=True):
    im = img(18, 16)
    if filled:
        c, cd, cl = (222, 46, 62, 255), (140, 20, 34, 255), (255, 130, 140, 255)
    else:
        c, cd, cl = (56, 22, 26, 200), (36, 14, 18, 200), (76, 34, 40, 200)
    for y in range(16):
        for x in range(18):
            dx = (x - 8.5) / 7.5
            dy = (y - 6.5) / 7.2
            if dx * dx + dy * dy <= 1.0:
                put(im, x, y, c)
            elif y > 6 and (x < 4 or x > 13) and ((x - 8.5) / 8.2) ** 2 + ((y - 6.5) / 6.6) ** 2 <= 1.0:
                put(im, x, y, c)
    outline(im, cd)
    ell(im, 6, 6, 1.6, 1.6, cl)
    put(im, 5, 4, (255, 255, 255, 200))
    return im


def g_star(filled=True):
    im = img(18, 18)
    c, cd, cl = ((110, 190, 240, 255), (44, 96, 176, 255), (200, 232, 255, 255)) if filled \
        else ((38, 58, 92, 200), (26, 38, 64, 200), (60, 84, 120, 200))
    pts = []
    for k in range(10):
        a = -math.pi / 2 + k * math.pi / 5
        r = 8 if k % 2 == 0 else 3.4
        pts.append((9 + math.cos(a) * r, 9 + math.sin(a) * r))
    for i in range(10):
        a, b = pts[i], pts[(i + 1) % 10]
        for k in range(16):
            put(im, a[0] + (b[0] - a[0]) * k / 15, a[1] + (b[1] - a[1]) * k / 15, c)
    outline(im, cd)
    put(im, 7, 6, cl)
    return im


def g_slot():
    im = img(T + 2, T + 2)
    rect(im, 0, 0, T + 2, T + 2, (28, 22, 34, 150))
    rect(im, 1, 1, T, T, (74, 62, 88, 90))
    rect(im, 1, 1, T, 1, (140, 124, 158, 110))
    rect(im, 1, T, T, 1, (24, 18, 28, 160))
    return im


def g_slot_sel():
    im = img(T + 2, T + 2)
    rect(im, 0, 0, T + 2, T + 2, (255, 236, 160, 90))
    rect(im, 0, 0, T + 2, 1, (255, 244, 190, 255))
    rect(im, 0, T + 1, T + 2, 1, (255, 244, 190, 255))
    rect(im, 0, 0, 1, T + 2, (255, 244, 190, 255))
    rect(im, T + 1, 0, 1, T + 2, (255, 244, 190, 255))
    return im


def g_panel(w, h):
    im = img(w, h)
    rect(im, 0, 0, w, h, (30, 26, 44, 226))
    rect(im, 0, 0, w, 1, (128, 116, 158, 255))
    rect(im, 0, 1, w, 1, (86, 76, 112, 255))
    rect(im, 0, h - 1, w, 1, (16, 13, 24, 255))
    rect(im, 0, 0, 1, h, (86, 76, 112, 255))
    rect(im, w - 1, 0, 1, h, (86, 76, 112, 255))
    return im


def g_cloud(kind):
    w, h = 96, 40
    im = img(w, h)
    rng = random.Random(hash(kind) & 0xFFFF)
    base = (236, 240, 250, 210) if kind == "day" else (72, 78, 108, 220)
    dark = (198, 208, 228, 200) if kind == "day" else (48, 54, 80, 210)
    lobes = [(20, 24, 20), (44, 18, 24), (70, 22, 20), (34, 14, 16), (60, 13, 15)]
    for cx, cy, r in lobes:
        ell(im, cx, cy, r, r * .62, base)
    for cx, cy, r in lobes[:3]:
        ell(im, cx, cy - r * .18, r * .7, r * .34, (252, 253, 255, 220))
    for x in range(w):
        put(im, x, h - 8, dark)
    return im


def g_sun():
    im = img(64, 64)
    cx = cy = 32
    for r in range(30, 20, -1):
        ell(im, cx, cy, r, r, mix((255, 246, 200, 255), (255, 214, 90, 255), (30 - r) / 10.0))
    ell(im, cx, cy, 20, 20, (255, 248, 210, 255))
    ell(im, cx, cy, 14, 14, (255, 240, 160, 255))
    return im


def g_moon(full=True):
    im = img(48, 48)
    cx = cy = 24
    col = (244, 246, 236, 255) if full else (96, 100, 118, 255)
    ell(im, cx, cy, 22, 22, col)
    for x, y, r in ((16, 18, 4), (30, 20, 5), (22, 30, 3), (32, 28, 2.5), (14, 28, 2.5)):
        ell(im, x, y, r, r, shade(col, .86) if full else shade(col, .9))
    if not full:                             # 弯月遮罩
        ov = img(48, 48)
        ell(im, cx + 11, cy - 2, 21, 21, (0, 0, 0, 0))
    outline(im, (60, 64, 84, 220))
    return im


def g_starfield():
    im = img(64, 64)
    rng = random.Random(99)
    for _ in range(46):
        x, y = rng.randrange(64), rng.randrange(64)
        b = rng.choice((150, 190, 220, 255))
        put(im, x, y, (255, 255, 255, b))
    return im


def g_vignette():
    im = img(64, 64)
    for y in range(64):
        for x in range(64):
            d = math.hypot((x - 32) / 32.0, (y - 32) / 32.0)
            if d > 0.72:
                a = min(150, int((d - 0.72) * 420))
                put(im, x, y, (0, 0, 0, a))
    return im


def g_particle(size, color):
    im = img(size, size)
    ell(im, size / 2, size / 2, size / 2, size / 2, color)
    return im


def g_cracks(stage):
    """挖掘裂纹 1-4 阶"""
    im = img(T, T)
    rng = random.Random(100 + stage)
    dark = (18, 14, 12, 220)
    mid = (40, 32, 28, 190)
    n = stage * 3
    for _ in range(n):
        x, y = rng.randrange(T), rng.randrange(T)
        c = dark if rng.random() < .6 else mid
        for k in range(rng.randrange(2, 5)):
            put(im, x, y, c)
            x += rng.choice((-1, 0, 1))
            y += rng.choice((-1, 0, 1))
    return im


# --------------------------------------------------------------------------
# 主流程
# --------------------------------------------------------------------------
def build():
    os.makedirs(OUT, exist_ok=True)
    lines = []
    tiles = Atlas(T)
    items = Atlas(T, 20)
    ents = Packer(512)
    gui = Atlas(T, 16)

    # ---------------- 方块 ----------------
    rng = random.Random(1)
    stone_c = ((128, 128, 134, 255), (104, 104, 112, 255), (156, 156, 162, 255))
    def add_tile(name, im):
        tiles.add(name, im)

    add_tile("air", img(T, T))
    add_tile("dirt", t_rock((134, 96, 67, 255), (108, 76, 52, 255), (160, 120, 86, 255), rng))
    add_tile("grass", t_grass(((108, 76, 52, 255), (134, 96, 67, 255), (160, 120, 86, 255)),
                              (44, 96, 40, 255), (86, 158, 58, 255), (134, 200, 92, 255), rng))
    add_tile("stone", t_rock(*stone_c, rng))
    add_tile("wood", t_wood())
    add_tile("wood_panel", t_rock((170, 122, 72, 255), (140, 98, 58, 255), (196, 148, 94, 255), rng))
    add_tile("leaves", t_leaves((38, 92, 34, 255), (58, 128, 44, 255), (92, 166, 66, 255), rng))
    add_tile("leaves_jungle", t_leaves((30, 100, 48, 255), (48, 140, 60, 255), (86, 186, 90, 255), rng))
    add_tile("sand", t_sand())
    add_tile("sandstone", t_rock((190, 162, 110, 255), (166, 140, 94, 255), (214, 190, 140, 255), rng))
    add_tile("snow", t_snow())
    add_tile("ice", t_ice())
    add_tile("mud", t_rock((104, 72, 52, 255), (82, 56, 40, 255), (126, 92, 68, 255), rng))
    add_tile("stone_jungle", t_rock((88, 118, 96, 255), (68, 94, 76, 255), (112, 146, 122, 255), rng))
    add_tile("grass_jungle", t_grass(((82, 56, 40, 255), (104, 72, 52, 255), (126, 92, 68, 255)),
                                     (26, 96, 40, 255), (58, 156, 66, 255), (110, 198, 96, 255), rng))
    add_tile("ebonstone", t_rock((62, 58, 88, 255), (44, 42, 66, 255), (84, 80, 116, 255), rng))
    add_tile("grass_corruption", t_grass(((44, 42, 66, 255), (62, 58, 88, 255), (84, 80, 116, 255)),
                                          (58, 40, 82, 255), (108, 66, 148, 255), (154, 104, 190, 255), rng))
    add_tile("obsidian", t_rock((46, 38, 66, 255), (32, 26, 48, 255), (68, 58, 96, 255), rng))
    add_tile("ash", t_rock((74, 70, 70, 255), (56, 52, 52, 255), (98, 94, 94, 255), rng))
    add_tile("hellstone", t_rock((92, 58, 42, 255), (70, 42, 32, 255), (128, 82, 54, 255), rng))
    add_tile("glowstone", t_glowstone())
    add_tile("glass", t_glass())
    add_tile("bricks", t_bricks((158, 76, 68, 255), (188, 172, 156, 255), rng))
    add_tile("bricks_stone", t_bricks((122, 122, 130, 255), (94, 94, 102, 255), rng))
    add_tile("mud_brick", t_bricks((112, 80, 58, 255), (76, 56, 42, 255), rng))
    add_tile("snow_brick", t_bricks((206, 224, 240, 255), (140, 164, 188, 255), rng))
    add_tile("torch", t_torch())
    add_tile("campfire", t_campfire())
    add_tile("workbench", t_workbench())
    add_tile("furnace", t_furnace(False))
    add_tile("furnace_lit", t_furnace(True))
    add_tile("anvil", t_anvil())
    add_tile("chest", t_chest())
    add_tile("door", t_door(True))
    add_tile("door_open", t_door(False))
    add_tile("platform", t_platform())
    add_tile("fence", t_fence())
    add_tile("vine", t_vine())
    add_tile("water", t_water())
    add_tile("lava", t_lava())
    add_tile("ore_copper", t_ore(*stone_c, (176, 96, 56, 255), (232, 156, 96, 255), rng, 7))
    add_tile("ore_iron", t_ore(*stone_c, (152, 110, 84, 255), (206, 172, 140, 255), rng, 7))
    add_tile("ore_silver", t_ore(*stone_c, (198, 202, 214, 255), (240, 244, 252, 255), rng, 6))
    add_tile("ore_gold", t_ore(*stone_c, (216, 180, 62, 255), (250, 226, 130, 255), rng, 6))
    add_tile("ore_demonite", t_ore(*stone_c, (86, 44, 70, 255), (140, 78, 120, 255), rng, 6))
    add_tile("ore_hellstone", t_ore((92, 58, 42, 255), (70, 42, 32, 255), (128, 82, 54, 255),
                                    (232, 122, 42, 255), (255, 196, 110, 255), rng, 6))
    add_tile("gem_topaz", t_gem((226, 168, 52, 255), (255, 224, 140, 255)))
    add_tile("gem_amethyst", t_gem((154, 88, 210, 255), (206, 156, 246, 255)))
    add_tile("gem_emerald", t_gem((56, 190, 122, 255), (140, 240, 190, 255)))
    add_tile("gem_diamond", t_gem((96, 194, 232, 255), (186, 236, 252, 255)))
    add_tile("clay", t_rock((160, 122, 128, 255), (136, 100, 108, 255), (186, 152, 156, 255), rng))
    add_tile("clay_brick", t_bricks((150, 106, 112, 255), (112, 82, 86, 255), rng))

    # 背景墙
    add_tile("wall_dirt", t_rock((86, 66, 50, 255), (68, 52, 40, 255), (104, 82, 62, 255), rng))
    add_tile("wall_stone", t_rock((86, 86, 92, 255), (68, 68, 74, 255), (106, 106, 112, 255), rng))
    add_tile("wall_wood", t_rock((104, 76, 48, 255), (82, 60, 38, 255), (126, 96, 62, 255), rng))
    add_tile("wall_sand", t_rock((168, 150, 106, 255), (146, 130, 92, 255), (190, 174, 132, 255), rng))
    add_tile("wall_snow", t_rock((176, 194, 212, 255), (152, 172, 194, 255), (204, 218, 232, 255), rng))
    add_tile("wall_mud", t_rock((78, 56, 40, 255), (60, 42, 30, 255), (96, 72, 52, 255), rng))
    add_tile("wall_ebonstone", t_rock((44, 42, 66, 255), (32, 30, 50, 255), (60, 58, 86, 255), rng))
    add_tile("wall_hellstone", t_rock((74, 46, 36, 255), (56, 34, 26, 255), (100, 66, 48, 255), rng))
    add_tile("wall_brick", t_bricks((122, 60, 56, 255), (150, 138, 126, 255), rng))
    add_tile("wall_jungle", t_rock((62, 88, 68, 255), (48, 70, 52, 255), (80, 110, 86, 255), rng))

    # ---------------- 物品 ----------------
    def add_item(name, im):
        items.add(name, im)

    blocky = {
        "dirt": "dirt", "stone": "stone", "sand": "sand", "sandstone": "sandstone",
        "snow": "snow", "ice": "ice", "mud": "mud", "stone_jungle": "stone_jungle",
        "ebonstone": "ebonstone", "obsidian": "obsidian", "ash": "ash", "hellstone": "hellstone",
        "glass": "glass", "bricks": "bricks", "bricks_stone": "bricks_stone",
        "mud_brick": "mud_brick", "snow_brick": "snow_brick", "wood_panel": "wood_panel",
        "clay": "clay", "clay_brick": "clay_brick", "glowstone": "glowstone",
        "workbench": "workbench", "furnace": "furnace", "anvil": "anvil", "chest": "chest",
        "campfire": "campfire", "fence": "fence", "vine": "vine", "platform": "platform",
    }
    for k, v in blocky.items():
        add_item(k, i_block(lambda r, nm=v: _tile_img(nm, r)))
    add_item("wood", _item_wood())
    add_item("torch", i_torch_icon())
    add_item("door", _item_door())
    for t, col in (("ore_copper", (176, 96, 56, 255)), ("ore_iron", (152, 110, 84, 255)),
                   ("ore_silver", (198, 202, 214, 255)), ("ore_gold", (216, 180, 62, 255)),
                   ("ore_demonite", (86, 44, 70, 255)),
                   ("ore_hellstone", (232, 122, 42, 255))):
        add_item(t, i_ore(col, shade(col, 1.35)))
    add_item("gem_topaz", i_gem((226, 168, 52, 255), (255, 230, 150, 255)))
    add_item("gem_amethyst", i_gem((154, 88, 210, 255), (212, 168, 250, 255)))
    add_item("gem_emerald", i_gem((56, 190, 122, 255), (150, 244, 196, 255)))
    add_item("gem_diamond", i_gem((96, 194, 232, 255), (196, 240, 252, 255)))
    for m, c in (("copper", (198, 122, 74, 255)), ("iron", (176, 172, 178, 255)),
                 ("silver", (216, 220, 230, 255)), ("gold", (238, 200, 74, 255))):
        add_item("bar_" + m, i_bar(c))
    tiers = (("copper", (198, 122, 74, 255)), ("iron", (176, 172, 178, 255)),
             ("silver", (216, 220, 230, 255)), ("gold", (238, 200, 74, 255)),
             ("platinum", (196, 214, 236, 255)))
    for name, c in tiers:
        add_item("pickaxe_" + name, i_pick(c, shade(c, .6)))
        add_item("axe_" + name, i_axe(c))
        add_item("sword_" + name, i_sword((198, 206, 216, 255), c))
    add_item("sword_copper", i_sword((226, 166, 110, 255), (150, 90, 54, 255)))
    add_item("bow", i_bow())
    add_item("arrow", i_arrow())
    add_item("magic_dart", i_orb((92, 148, 236, 255), (196, 226, 255, 255)))
    add_item("gel", i_generic((150, 216, 236, 235), "blob"))
    add_item("bone", _item_bone())
    add_item("lens", _item_lens())
    add_item("string", _item_string())
    add_item("potion_health", i_potion((216, 60, 70, 190)))
    add_item("potion_mana", i_potion((72, 120, 230, 190)))
    add_item("crown_slime", i_crown())
    add_item("eye_suspicious", i_eye())
    add_item("amulet_life", _item_amulet((216, 60, 70, 255)))
    add_item("amulet_mana", _item_amulet((72, 130, 236, 255)))
    add_item("boots_speed", _item_boots())
    add_item("bracer_defense", _item_bracer())
    add_item("helm_copper", _item_helm((198, 122, 74, 255)))
    add_item("helm_iron", _item_helm((176, 172, 178, 255)))
    add_item("helm_silver", _item_helm((216, 220, 230, 255)))
    add_item("helm_gold", _item_helm((238, 200, 74, 255)))
    add_item("armor_copper", _item_armor((198, 122, 74, 255)))
    add_item("armor_iron", _item_armor((176, 172, 178, 255)))
    add_item("armor_silver", _item_armor((216, 220, 230, 255)))
    add_item("armor_gold", _item_armor((238, 200, 74, 255)))

    # ---------------- 角色 ----------------
    for n in range(4):
        ents.add("player_walk_%d" % n, player_frame(n, 4))
    ents.add("player_idle", humanoid(20, 40, 0, 0))
    ents.add("player_jump", humanoid(20, 40, -1, -1))
    ents.add("player_fall", humanoid(20, 40, 1, 1))
    for n in range(4):
        ents.add("zombie_walk_%d" % n, zombie_frame(n, 4))
    ents.add("slime_blue", slime((70, 134, 214, 235), (30, 68, 136, 235), (140, 196, 244, 235), seed=1))
    ents.add("slime_green", slime((86, 186, 108, 235), (36, 96, 52, 235), (160, 230, 178, 235), seed=2))
    ents.add("slime_purple", slime((150, 92, 208, 235), (72, 36, 116, 235), (206, 168, 244, 235), seed=4))
    ents.add("slime_red", slime((206, 90, 70, 235), (116, 32, 24, 235), (250, 168, 140, 235), seed=5))
    for n in range(3):
        ents.add("demon_eye_%d" % n, demon_eye(n))
    for n in range(3):
        ents.add("servant_%d" % n, servant(n))
    for n in range(4):
        ents.add("eye_of_cthulhu_%d" % n, eye_of_cthulhu(n))
    ents.add("king_slime", slime_crown_frame(0, 1))
    for n in range(4):
        ents.add("npc_guide_%d" % n, npc_guide(n))
    for n in range(4):
        ents.add("npc_merchant_%d" % n, npc_merchant(n))

    # ---------------- GUI ----------------
    gui.add("heart_full", g_heart(True))
    gui.add("heart_empty", g_heart(False))
    gui.add("star_full", g_star(True))
    gui.add("star_empty", g_star(False))
    gui.add("slot", g_slot())
    gui.add("slot_sel", g_slot_sel())
    gui.add("panel", g_panel(T * 10 + 8, T * 5 + 8))
    gui.add("cloud_day", g_cloud("day"))
    gui.add("cloud_night", g_cloud("night"))
    gui.add("sun", g_sun())
    gui.add("moon_full", g_moon(True))
    gui.add("moon_new", g_moon(False))
    gui.add("stars", g_starfield())
    gui.add("vignette", g_vignette())
    gui.add("spark", g_particle(4, (255, 244, 200, 255)))
    gui.add("dust", g_particle(4, (176, 168, 160, 255)))
    gui.add("blood", g_particle(4, (196, 44, 40, 255)))
    gui.add("magic", g_particle(4, (140, 170, 255, 255)))
    for s in range(1, 5):
        gui.add("cracks_%d" % s, g_cracks(s))

    # ---------------- 打包 ----------------
    # tiles.png 与 items.png 有同名帧(dirt/stone/...), 加前缀区分

    lines += _prefix(tiles.save("tiles.png"), "tile:")
    lines += _prefix(items.save("items.png"), "item:")
    lines += ents.save("entities.png")
    lines += gui.save("gui.png")
    with open(os.path.join(OUT, "atlas.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("方块 %d / 物品 %d / 精灵 %d / 界面 %d  =>  %s" %
          (len(tiles.frames), len(items.frames), len(ents.frames), len(gui.frames), OUT))


def _prefix(lines, pfx):
    out = []
    for ln in lines:
        parts = ln.split()
        if len(parts) == 6:
            parts[1] = pfx + parts[1]
            out.append(" ".join(parts))
        else:
            out.append(ln)
    return out


# 一些物品需要直接引用方块画法
_tile_cache = {}


def _tile_img(name, rng):
    key = name
    if key in _tile_cache:
        return _tile_cache[key]
    import copy
    a = Atlas(T)
    r = random.Random(5)
    imgs = {}
    # 复用 build() 中的同名画法
    globals()["_last_rng"] = r
    imgs["dirt"] = t_rock((134, 96, 67, 255), (108, 76, 52, 255), (160, 120, 86, 255), r)
    imgs["stone"] = t_rock((128, 128, 134, 255), (104, 104, 112, 255), (156, 156, 162, 255), r)
    imgs["sand"] = t_sand()
    imgs["sandstone"] = t_rock((190, 162, 110, 255), (166, 140, 94, 255), (214, 190, 140, 255), r)
    imgs["snow"] = t_snow()
    imgs["ice"] = t_ice()
    imgs["mud"] = t_rock((104, 72, 52, 255), (82, 56, 40, 255), (126, 92, 68, 255), r)
    imgs["stone_jungle"] = t_rock((88, 118, 96, 255), (68, 94, 76, 255), (112, 146, 122, 255), r)
    imgs["ebonstone"] = t_rock((62, 58, 88, 255), (44, 42, 66, 255), (84, 80, 116, 255), r)
    imgs["obsidian"] = t_rock((46, 38, 66, 255), (32, 26, 48, 255), (68, 58, 96, 255), r)
    imgs["ash"] = t_rock((74, 70, 70, 255), (56, 52, 52, 255), (98, 94, 94, 255), r)
    imgs["hellstone"] = t_rock((92, 58, 42, 255), (70, 42, 32, 255), (128, 82, 54, 255), r)
    imgs["glowstone"] = t_glowstone()
    imgs["glass"] = t_glass()
    imgs["bricks"] = t_bricks((158, 76, 68, 255), (188, 172, 156, 255), r)
    imgs["bricks_stone"] = t_bricks((122, 122, 130, 255), (94, 94, 102, 255), r)
    imgs["mud_brick"] = t_bricks((112, 80, 58, 255), (76, 56, 42, 255), r)
    imgs["snow_brick"] = t_bricks((206, 224, 240, 255), (140, 164, 188, 255), r)
    imgs["wood_panel"] = t_rock((170, 122, 72, 255), (140, 98, 58, 255), (196, 148, 94, 255), r)
    imgs["clay"] = t_rock((160, 122, 128, 255), (136, 100, 108, 255), (186, 152, 156, 255), r)
    imgs["clay_brick"] = t_bricks((150, 106, 112, 255), (112, 82, 86, 255), r)
    imgs["workbench"] = t_workbench()
    imgs["furnace"] = t_furnace(False)
    imgs["anvil"] = t_anvil()
    imgs["chest"] = t_chest()
    imgs["campfire"] = t_campfire()
    imgs["fence"] = t_fence()
    imgs["vine"] = t_vine()
    imgs["platform"] = t_platform()
    imgs["wood"] = t_wood()
    imgs["torch"] = t_torch()
    imgs["door"] = t_door(True)
    imgs["grass"] = t_grass(((108, 76, 52, 255), (134, 96, 67, 255), (160, 120, 86, 255)),
                            (44, 96, 40, 255), (86, 158, 58, 255), (134, 200, 92, 255), r)
    imgs["leaves"] = t_leaves((38, 92, 34, 255), (58, 128, 44, 255), (92, 166, 66, 255), r)
    _tile_cache[key] = imgs[name]
    return imgs[name]


def _item_wood():
    im = img(T, T)
    rect(im, 3, 2, 10, 12, (139, 100, 63, 255))
    rect(im, 3, 2, 10, 1, (172, 130, 86, 255))
    rect(im, 3, 13, 10, 1, (98, 68, 42, 255))
    rect(im, 3, 2, 1, 12, (164, 122, 78, 255))
    rect(im, 12, 2, 1, 12, (100, 70, 44, 255))
    ell(im, 8, 8, 2.4, 2.4, (120, 86, 54, 255))
    outline(im, (26, 20, 16, 230))
    return im


def _item_door():
    im = img(T, T)
    rect(im, 3, 1, 10, 14, (156, 106, 60, 255))
    rect(im, 3, 1, 10, 1, (196, 140, 84, 255))
    rect(im, 3, 1, 1, 14, (182, 128, 76, 255))
    rect(im, 12, 1, 1, 14, (110, 74, 42, 255))
    rect(im, 7, 2, 1, 13, (112, 76, 44, 255))
    put(im, 10, 8, (226, 196, 104, 255))
    outline(im, (26, 20, 16, 230))
    return im


def _item_bone():
    im = img(T, T)
    b = (238, 234, 216, 255)
    for k in range(10):
        put(im, 4 + k, 12 - k, b)
    ell(im, 4, 12, 2.4, 2.0, b)
    ell(im, 5, 13, 1.4, 1.2, (190, 184, 166, 255))
    ell(im, 13, 3, 2.4, 2.0, b)
    outline(im, (120, 114, 96, 230))
    return im


def _item_lens():
    im = img(T, T)
    ell(im, 8, 8, 6, 6, (72, 96, 72, 255))
    ell(im, 8, 8, 5, 5, (128, 176, 128, 255))
    ell(im, 8, 8, 3.4, 3.4, (170, 220, 180, 200))
    ell(im, 6.4, 6.2, 1.8, 1.6, (255, 255, 255, 200))
    outline(im, (36, 60, 40, 230))
    return im


def _item_string():
    im = img(T, T)
    c = (222, 214, 190, 255)
    for y in range(3, 14):
        x = 4 + int(2.2 * math.sin(y * .9))
        put(im, x, y, c)
        put(im, x + 1, y, (188, 178, 152, 255))
    outline(im, (110, 104, 84, 220))
    return im


def _item_amulet(c):
    im = img(T, T)
    for k in range(9):
        x = 3 + k
        y = 2 + int(2.2 * math.sin(k * .7))
        put(im, x, y, (198, 176, 120, 255))
        put(im, x, y + 1, (156, 136, 88, 255))
    ell(im, 8, 9, 3.4, 3.4, (150, 130, 84, 255))
    ell(im, 8, 9, 2.4, 2.4, c)
    put(im, 7, 8, (255, 255, 255, 220))
    outline(im, (48, 38, 20, 230))
    return im


def _item_boots():
    im = img(T, T)
    c = (146, 96, 60, 255)
    rect(im, 3, 5, 4, 6, c)
    rect(im, 9, 5, 4, 6, c)
    rect(im, 2, 10, 6, 3, (108, 70, 44, 255))
    rect(im, 8, 10, 6, 3, (108, 70, 44, 255))
    rect(im, 3, 5, 4, 1, (182, 126, 82, 255))
    put(im, 6, 8, (120, 200, 255, 255))
    put(im, 10, 8, (120, 200, 255, 255))
    outline(im, (26, 20, 16, 230))
    return im


def _item_bracer():
    im = img(T, T)
    c = (168, 120, 70, 255)
    rect(im, 3, 6, 10, 5, c)
    rect(im, 3, 6, 10, 1, (204, 154, 96, 255))
    rect(im, 3, 10, 10, 1, (116, 78, 44, 255))
    rect(im, 5, 4, 6, 3, (140, 100, 60, 255))
    put(im, 8, 8, (120, 200, 255, 255))
    outline(im, (26, 20, 16, 230))
    return im


def _item_helm(c):
    im = img(T, T)
    ell(im, 8, 8, 5.6, 5.6, shade(c, .78))
    for y in range(2, 9):
        for x in range(T):
            if (x - 8) ** 2 / 32.0 + (y - 8) ** 2 / 32.0 <= 1.0:
                put(im, x, y, c)
    rect(im, 2, 8, 12, 2, shade(c, .7))
    rect(im, 3, 4, 10, 1, shade(c, 1.3))
    rect(im, 4, 7, 3, 2, (26, 24, 30, 255))
    rect(im, 9, 7, 3, 2, (26, 24, 30, 255))
    outline(im, (26, 20, 16, 230))
    return im


def _item_armor(c):
    im = img(T, T)
    for y in range(2, 12):
        for x in range(3, 13):
            if 3 <= x <= 12 and abs(x - 7.5) < 4.6 - (y - 2) * .05:
                put(im, x, y, c)
    rect(im, 2, 2, 2, 4, c)
    rect(im, 12, 2, 2, 4, c)
    rect(im, 4, 6, 8, 6, shade(c, .86))
    rect(im, 4, 6, 8, 1, shade(c, 1.2))
    rect(im, 7, 7, 2, 4, shade(c, .6))
    outline(im, (26, 20, 16, 230))
    return im


if __name__ == "__main__":
    build()
