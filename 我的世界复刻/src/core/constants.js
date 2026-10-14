// 全局调参常量。世界尺寸、区块、渲染距离、玩法速度都集中在这里。

export const CHUNK_X = 16;          // 区块 X 方向方块数
export const CHUNK_Z = 16;          // 区块 Z 方向方块数
export const WORLD_H = 128;         // 世界总高度
export const CHUNK_AREA = CHUNK_X * CHUNK_Z;
export const CHUNK_VOL = CHUNK_AREA * WORLD_H;

// 体素内存索引：低 4 位 x，中 4 位 z，其余位 y。y 连续，便于按列做天光扫描。
export const IDX = (x, y, z) => x | (z << 4) | (y << 8);

// 列索引（区块内的 x/z 平面数组，如 biomes / heights）。
// 全项目统一用这一个约定：x 变化最快。写错成 z 变化最快会得到一张转置的表，
// 症状是「查询某坐标的高度」悄悄返回了另一个坐标的高度。
export const COL = (lx, lz) => lx + lz * CHUNK_X;

export const SEA_LEVEL = 46;        // 海平面高度
export const BEACH_LEVEL = SEA_LEVEL + 2;
export const LAVA_LEVEL = 12;       // 低于此高度用岩浆填洞底
export const CAVE_TOP = 62;         // 洞穴不再往上挖
export const BEDROCK_TOP = 4;       // 基岩层抖动厚度

export const DEFAULT_SEED = 20240817;

export const RENDER_DISTANCE = 7;   // 视距（区块数）
// 流式加载每帧的时间预算（毫秒）。按时间而不是按个数限流：
// 单区块地形生成约 10ms、光照约 6ms、网格化中位约 10ms，机器慢的时候按个数限流会掉帧。
export const STREAM_BUDGET_MS = 12;
export const STREAM_BUDGET_MS_IDLE = 30;   // 停在菜单时用大预算，把世界预加载好

// 玩家
export const PLAYER_HALF = 0.3;     // AABB 半宽（也作半深）
export const PLAYER_HEIGHT = 1.8;
export const PLAYER_EYE = 1.62;
export const GRAVITY = 28;
export const JUMP_SPEED = 8.6;
export const WALK_SPEED = 4.4;
export const SPRINT_SPEED = 6.2;
export const FLY_SPEED = 12;
export const SWIM_SPEED = 3.4;
export const TERMINAL_VELOCITY = 60;

export const REACH = 5.0;           // 交互距离（方块）
