// Particles.h —— 粒子特效
#pragma once

#include "Common.h"

#include <vector>

namespace tc {

class World;

enum PartKind { PK_DUST = 0, PK_BLOOD, PK_SPARK, PK_MAGIC, PK_LEAF, PK_SMOKE, PK_FLAME };

struct Particle {
    Vector2 pos, vel;
    float life, maxLife, size, gravity, drag;
    Color color;
    PartKind kind;
    bool active;
};

class Particles {
public:
    void update(float dt, World& w);
    void draw();
    void clear() { m_p.clear(); }

    void spawn(PartKind kind, float x, float y, int count,
               Color color, float speed, float spread, float size, float life);
    void burst(Vector2 at, int count, Color c, float spd);

private:
    std::vector<Particle> m_p;
};

}  // namespace tc
