#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_sfx.py —— 程序化音效生成器

用纯 Python 合成游戏音效, 输出 16bit / 22.05kHz / 单声道 WAV 到 assets/sfx/。
不依赖任何第三方库, 也不需要外部素材。

想换成自己的音效: 保持文件名不变, 直接覆盖 assets/sfx/ 下的 wav 即可。
想关掉全部音效: 删除 assets/sfx/ 目录, 游戏会自动静音。

    python3 tools/gen_sfx.py
"""

import math
import os
import random
import struct

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "sfx")
RATE = 22050


# ================================================================ 合成基元
def _adsr(n, a, d, s, r):
    """返回 0..1 的包络数组"""
    env = [0.0] * n
    an = max(1, int(a * RATE))
    dn = max(1, int(d * RATE))
    rn = max(1, int(r * RATE))
    sn = max(0, n - an - dn - rn)
    idx = 0
    for i in range(min(an, n)):                      # 起音
        env[idx] = i / an
        idx += 1
    for i in range(dn):                              # 衰减
        if idx >= n:
            break
        env[idx] = 1.0 + (s - 1.0) * (i / dn)
        idx += 1
    for _ in range(sn):                              # 延音
        if idx >= n:
            break
        env[idx] = s
        idx += 1
    for i in range(rn):                              # 释音
        if idx >= n:
            break
        env[idx] = s * (1.0 - i / rn)
        idx += 1
    for i in range(idx, n):
        env[i] = 0.0
    return env


def _osc(kind, phase, rng):
    if kind == "sine":
        return math.sin(phase)
    if kind == "square":
        return 1.0 if math.sin(phase) >= 0.0 else -1.0
    if kind == "saw":
        t = (phase / (2 * math.pi)) % 1.0
        return 2.0 * t - 1.0
    if kind == "tri":
        t = (phase / (2 * math.pi)) % 1.0
        return 4.0 * abs(t - 0.5) - 1.0
    if kind == "noise":
        return rng.uniform(-1.0, 1.0)
    return 0.0


def tone(dur, f0, f1=None, wave="sine", amp=0.5, a=0.004, d=0.05, s=0.6, r=0.04,
         mix=None, seed=1, curve=1.0):
    """
    合成一段单音。f0 -> f1 为音高滑音, mix 为要混合的第二个音 (freq, wave, gain)。
    curve > 1 时音高按幂曲线滑动(听起来更自然)。
    """
    rng = random.Random(seed)
    n = int(dur * RATE)
    f1 = f0 if f1 is None else f1
    env = _adsr(n, a, d, s, r)
    out = [0.0] * n
    ph = 0.0
    for i in range(n):
        t = i / n
        f = f0 + (f1 - f0) * (t ** curve)
        ph += 2.0 * math.pi * f / RATE
        v = _osc(wave, ph, rng)
        if mix:
            mf, mw, mg = mix
            v = v * (1.0 - mg) + _osc(mw, ph * (mf / max(f, 1.0)), rng) * mg
        out[i] = v * env[i] * amp
    return out


def noise(dur, amp=0.5, a=0.001, d=0.05, s=0.3, r=0.05, seed=2):
    """白噪声 + 包络"""
    rng = random.Random(seed)
    n = int(dur * RATE)
    env = _adsr(n, a, d, s, r)
    return [rng.uniform(-1.0, 1.0) * env[i] * amp for i in range(n)]


def lowpass(sig, cutoff):
    """一阶低通"""
    a = 1.0 - math.exp(-2.0 * math.pi * cutoff / RATE)
    y = 0.0
    out = []
    for x in sig:
        y += a * (x - y)
        out.append(y)
    return out


def highpass(sig, cutoff):
    lp = lowpass(sig, cutoff)
    return [x - y for x, y in zip(sig, lp)]


def bandpass(sig, lo, hi):
    return highpass(lowpass(sig, hi), lo)


def env_hit(sig, power=2.0):
    """快速衰减(撞击感)"""
    n = len(sig)
    return [x * ((1.0 - i / n) ** power) for i, x in enumerate(sig)]


def mixdown(*tracks):
    n = max(len(t) for t in tracks)
    out = [0.0] * n
    for t in tracks:
        for i, v in enumerate(t):
            out[i] += v
    peak = max(1e-6, max(abs(v) for v in out))
    norm = 0.92 / peak
    return [v * norm for v in out]


def at(sig, delay):
    """延迟叠加用: 返回前面补 0 的列表"""
    return [0.0] * int(delay * RATE) + list(sig)


def write_wav(name, samples):
    os.makedirs(OUT, exist_ok=True)
    data = bytearray()
    for v in samples:
        x = int(max(-1.0, min(1.0, v)) * 32000)
        data += struct.pack("<h", x)
    path = os.path.join(OUT, name + ".wav")
    with open(path, "wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + len(data)))
        f.write(b"WAVEfmt ")
        f.write(struct.pack("<IHHIIHH", 16, 1, 1, RATE, RATE * 2, 2, 16))
        f.write(b"data")
        f.write(struct.pack("<I", len(data)))
        f.write(bytes(data))
    return path


# ================================================================ 音效定义
def sfx_dig_dirt():
    """挖泥土: 低沉的沙沙声 + 一点闷响"""
    return mixdown(
        lowpass(noise(0.085, 0.9, 0.001, 0.03, 0.15, 0.05, seed=11), 1100),
        env_hit(tone(0.07, 150, 90, "sine", 0.45, 0.001, 0.03, 0.1, 0.03), 3.0),
    )


def sfx_dig_wood():
    """挖木头: 干脆的木质敲击"""
    return mixdown(
        bandpass(noise(0.06, 0.6, 0.001, 0.02, 0.1, 0.03, seed=12), 700, 3000),
        env_hit(tone(0.09, 260, 170, "tri", 0.7, 0.001, 0.04, 0.05, 0.04), 2.5),
    )


def sfx_dig_stone():
    """挖石头: 尖脆的金属感"""
    return mixdown(
        highpass(noise(0.055, 0.7, 0.001, 0.02, 0.08, 0.03, seed=13), 1600),
        env_hit(tone(0.06, 620, 480, "square", 0.3, 0.001, 0.025, 0.05, 0.02), 3.0),
    )


def sfx_break():
    """方块碎裂"""
    return mixdown(
        env_hit(lowpass(noise(0.20, 1.0, 0.001, 0.09, 0.2, 0.10, seed=14), 2000), 1.6),
        env_hit(tone(0.16, 320, 90, "saw", 0.35, 0.002, 0.06, 0.1, 0.08), 2.0),
    )


def sfx_place():
    """放置方块: 轻快的哒"""
    return mixdown(
        lowpass(noise(0.05, 0.5, 0.001, 0.02, 0.1, 0.02, seed=15), 2000),
        env_hit(tone(0.06, 520, 400, "sine", 0.4, 0.001, 0.03, 0.05, 0.02), 3.0),
    )


def sfx_jump():
    return tone(0.10, 300, 560, "sine", 0.5, 0.004, 0.05, 0.5, 0.04, curve=0.6)


def sfx_land():
    return mixdown(
        env_hit(tone(0.13, 180, 70, "sine", 0.8, 0.001, 0.05, 0.1, 0.07), 2.4),
        lowpass(noise(0.09, 0.4, 0.001, 0.03, 0.1, 0.05, seed=16), 700),
    )


def sfx_hurt():
    return mixdown(
        tone(0.24, 400, 140, "square", 0.45, 0.003, 0.10, 0.4, 0.10, curve=0.7),
        lowpass(noise(0.14, 0.5, 0.001, 0.05, 0.15, 0.07, seed=17), 1400),
    )


def sfx_pickup():
    return at(tone(0.06, 780, 900, "sine", 0.5, 0.002, 0.03, 0.6, 0.03), 0.0) + \
           at(tone(0.07, 1180, 1320, "sine", 0.42, 0.002, 0.03, 0.5, 0.03), 0.055)


def sfx_craft():
    parts = []
    for k, f in enumerate((523.25, 659.25, 783.99)):
        parts.append(at(tone(0.09, f, f, "tri", 0.45, 0.003, 0.05, 0.5, 0.03), 0.055 * k))
    return mixdown(*parts)


def sfx_open():
    return tone(0.09, 420, 720, "sine", 0.42, 0.004, 0.05, 0.5, 0.03, curve=0.7)


def sfx_close():
    return tone(0.09, 720, 420, "sine", 0.42, 0.004, 0.05, 0.5, 0.03, curve=0.7)


def sfx_hit_enemy():
    return mixdown(
        lowpass(noise(0.07, 0.75, 0.001, 0.03, 0.1, 0.03, seed=18), 1800),
        env_hit(tone(0.08, 300, 170, "square", 0.4, 0.001, 0.035, 0.1, 0.03), 2.6),
    )


def sfx_enemy_die():
    return mixdown(
        env_hit(tone(0.28, 330, 80, "saw", 0.42, 0.004, 0.12, 0.3, 0.10), 1.4),
        lowpass(noise(0.22, 0.5, 0.002, 0.09, 0.2, 0.10, seed=19), 1500),
    )


def sfx_coin():
    return mixdown(
        at(tone(0.05, 987.77, 987.77, "square", 0.30, 0.002, 0.02, 0.6, 0.02), 0.0),
        at(tone(0.09, 1318.51, 1318.51, "square", 0.26, 0.002, 0.03, 0.5, 0.04), 0.045),
    )


def sfx_boss():
    """Boss 登场: 低频轰鸣"""
    return mixdown(
        tone(0.95, 62, 44, "saw", 0.8, 0.10, 0.30, 0.7, 0.45, curve=0.8),
        lowpass(noise(0.9, 0.45, 0.06, 0.30, 0.4, 0.40, seed=20), 420),
        env_hit(tone(0.5, 180, 90, "square", 0.22, 0.02, 0.2, 0.3, 0.25), 1.2),
    )


def sfx_menu():
    return tone(0.045, 900, 900, "square", 0.28, 0.002, 0.015, 0.5, 0.02)


def sfx_splash():
    return mixdown(
        env_hit(bandpass(noise(0.30, 1.0, 0.004, 0.10, 0.25, 0.18, seed=21), 400, 3200), 1.5),
        env_hit(tone(0.18, 700, 300, "sine", 0.3, 0.004, 0.08, 0.2, 0.08), 2.0),
    )


def sfx_step():
    return lowpass(noise(0.045, 0.32, 0.001, 0.015, 0.05, 0.025, seed=22), 900)


def sfx_swing():
    """挥剑破空"""
    return bandpass(noise(0.12, 0.8, 0.004, 0.05, 0.15, 0.05, seed=23), 500, 2600)


def sfx_levelup():
    parts = []
    for k, f in enumerate((523.25, 659.25, 783.99, 1046.50)):
        parts.append(at(tone(0.13, f, f, "tri", 0.42, 0.004, 0.06, 0.5, 0.05), 0.075 * k))
    return mixdown(*parts)


SOUNDS = [
    ("dig_dirt", sfx_dig_dirt),
    ("dig_wood", sfx_dig_wood),
    ("dig_stone", sfx_dig_stone),
    ("break", sfx_break),
    ("place", sfx_place),
    ("jump", sfx_jump),
    ("land", sfx_land),
    ("hurt", sfx_hurt),
    ("pickup", sfx_pickup),
    ("craft", sfx_craft),
    ("open", sfx_open),
    ("close", sfx_close),
    ("hit_enemy", sfx_hit_enemy),
    ("enemy_die", sfx_enemy_die),
    ("coin", sfx_coin),
    ("boss", sfx_boss),
    ("menu", sfx_menu),
    ("splash", sfx_splash),
    ("step", sfx_step),
    ("swing", sfx_swing),
    ("levelup", sfx_levelup),
]


def build():
    n = 0
    for name, fn in SOUNDS:
        write_wav(name, fn())
        n += 1
    print("音效 %d 个 => %s" % (n, os.path.normpath(OUT)))


if __name__ == "__main__":
    build()
