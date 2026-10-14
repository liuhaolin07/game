// Particles.cpp
#include "Particles.h"

#include "Assets.h"
#include "Noise.h"
#include "World.h"

#include <algorithm>

namespace tc {

void Particles::spawn(PartKind kind, float x, float y, int count, Color color,
                      float speed, float spread, float size, float life)
{
    for (int i = 0; i < count; i++) {
        Particle p{};
        p.active = true;
        p.kind = kind;
        const float a = randRange(hash3((int)x, (int)y, i, (uint32_t)(GetFrameTime() * 1000.0f) * 7919u + (uint32_t)m_p.size()),
                                  -spread, spread);
        const float sp = speed * randRange(hash3(i, (int)x, (int)y, 77u), 0.35f, 1.0f);
        p.vel = { cosf(a) * sp, sinf(a) * sp };
        p.pos = { x + p.vel.x * 0.012f, y + p.vel.y * 0.012f };
        p.life = p.maxLife = life * randRange(hash3(i, 7, (int)x, 31u), 0.65f, 1.15f);
        p.size = size * randRange(hash3(i, 11, (int)y, 53u), 0.7f, 1.25f);
        p.color = color;
        switch (kind) {
        case PK_SMOKE:
            p.gravity = -6.0f; p.drag = 1.6f; break;
        case PK_MAGIC:
            p.gravity = -2.0f; p.drag = 2.4f; break;
        case PK_SPARK:
            p.gravity = 22.0f; p.drag = 0.4f; break;
        case PK_FLAME:
            p.gravity = -14.0f; p.drag = 1.8f; break;
        case PK_LEAF:
            p.gravity = 6.0f; p.drag = 1.2f; break;
        default:
            p.gravity = 34.0f; p.drag = 0.9f; break;
        }
        m_p.push_back(p);
    }
    if (m_p.size() > 900) m_p.erase(m_p.begin(), m_p.begin() + 300);
}

void Particles::burst(Vector2 at, int count, Color c, float spd)
{
    spawn(PK_DUST, at.x, at.y, count, c, spd, 3.14f, 2.2f, 0.55f);
}

void Particles::update(float dt, World& w)
{
    for (Particle& p : m_p) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0) { p.active = false; continue; }
        p.vel.y += p.gravity * dt;
        p.vel.x -= p.vel.x * p.drag * dt;
        p.vel.y -= p.vel.y * p.drag * dt * 0.5f;

        float nx = p.pos.x + p.vel.x * dt;
        float ny = p.pos.y + p.vel.y * dt;

        // 简易碰撞
        if (p.kind != PK_MAGIC && p.kind != PK_FLAME && p.kind != PK_SMOKE) {
            if (w.solidAtPx(nx, p.pos.y)) { nx = p.pos.x; p.vel.x *= -0.35f; }
            if (w.solidAtPx(nx, ny)) { ny = p.pos.y; p.vel.y *= -0.3f; p.vel.x *= 0.7f; }
        }
        p.pos.x = nx;
        p.pos.y = ny;
    }
    m_p.erase(std::remove_if(m_p.begin(), m_p.end(), [](const Particle& p) { return !p.active; }), m_p.end());
}

void Particles::draw()
{
    for (const Particle& p : m_p) {
        const float t = clampf(p.life / p.maxLife, 0.0f, 1.0f);
        Color c = p.color;
        c.a = (unsigned char)(255.0f * t);
        float s = p.size;
        switch (p.kind) {
        case PK_SMOKE: s = p.size * (2.0f - t); break;
        case PK_FLAME: s = p.size * (0.4f + t * 0.9f); break;
        default: break;
        }
        DrawRectangleRec({ p.pos.x - s * .5f, p.pos.y - s * .5f, s, s }, c);
    }
}

}  // namespace tc
