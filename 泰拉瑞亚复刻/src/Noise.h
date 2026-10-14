// Noise.h —— 确定性哈希与噪声(无外部依赖)
#pragma once

#include <cstdint>
#include <cmath>

namespace tc {

inline uint32_t hash32(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
inline uint32_t hash2(int a, int b, uint32_t seed)
{
    return hash32((uint32_t)a * 0x27d4eb2dU ^ (uint32_t)b * 0x165667b1U ^ seed);
}
inline uint32_t hash3(int a, int b, int c, uint32_t seed)
{
    return hash32(hash2(a, b, seed) ^ ((uint32_t)c * 0x9e3779b1U));
}
// [0,1)
inline float rand01(uint32_t h) { return (float)(h >> 8) * (1.0f / 16777216.0f); }
inline float randRange(uint32_t h, float a, float b) { return a + (b - a) * rand01(h); }
// [lo, hi] 闭区间整数
inline int randInt(uint32_t h, int lo, int hi) { return lo + (int)(rand01(h) * (float)(hi - lo + 1)) % (hi - lo + 1); }

inline float lerpN(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

// 一维值噪声, 返回 [-1,1]
inline float noise1(float x, uint32_t seed)
{
    int i = (int)floorf(x);
    float f = x - (float)i;
    float a = rand01(hash2(i, 0, seed)) * 2.0f - 1.0f;
    float b = rand01(hash2(i + 1, 0, seed)) * 2.0f - 1.0f;
    return lerpN(a, b, smoothstep(f));
}

// 分形叠加, 返回大致 [-1,1]
inline float fbm1(float x, uint32_t seed, int octaves, float lac = 2.0f, float gain = 0.5f)
{
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        sum += noise1(x * freq, seed + (uint32_t)i * 7919u) * amp;
        norm += amp;
        amp *= gain;
        freq *= lac;
    }
    return norm > 0 ? sum / norm : 0.0f;
}

// 二维值噪声, 返回 [-1,1]
inline float noise2(float x, float y, uint32_t seed)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float xf = x - (float)xi, yf = y - (float)yi;
    float a = rand01(hash3(xi, yi, 0, seed)) * 2.0f - 1.0f;
    float b = rand01(hash3(xi + 1, yi, 0, seed)) * 2.0f - 1.0f;
    float c = rand01(hash3(xi, yi + 1, 0, seed)) * 2.0f - 1.0f;
    float d = rand01(hash3(xi + 1, yi + 1, 0, seed)) * 2.0f - 1.0f;
    return lerpN(lerpN(a, b, smoothstep(xf)), lerpN(c, d, smoothstep(xf)), smoothstep(yf));
}

inline float fbm2(float x, float y, uint32_t seed, int octaves, float lac = 2.0f, float gain = 0.5f)
{
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        sum += noise2(x * freq, y * freq, seed + (uint32_t)i * 6271u) * amp;
        norm += amp;
        amp *= gain;
        freq *= lac;
    }
    return norm > 0 ? sum / norm : 0.0f;
}

}  // namespace tc
