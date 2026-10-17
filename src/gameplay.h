#ifndef ZTANK_GAMEPLAY_H
#define ZTANK_GAMEPLAY_H
#include "config.h"
typedef struct {
    int16_t x,y;
    uint8_t heading,active,invulnerable,moving,plan_wait,fire_wait,stuck;
} tank_state;
typedef struct { int16_t x,y;uint8_t heading,active,age,owner; } shell_state;
typedef struct { int16_t x,y;uint8_t frames,kind; } effect_state;
typedef struct {
    tank_state tanks[TANK_COUNT];
    shell_state shells[SHELL_COUNT];
    effect_state effects[6];
    uint32_t score;
    uint16_t stage,elapsed;
    uint8_t arena,difficulty,lives,spawned,destroyed,spawn_wait,respawn_wait;
    uint8_t hq_alive,bonus_awarded,over,previous_controls,animation_tick;
} match_state;
extern match_state match;
void game_start(uint8_t difficulty);
void game_next_stage(void);
void game_tick(uint8_t controls);
void game_end_tick(void);
uint16_t game_hit_score(uint8_t shell_heading,uint8_t victim_heading,uint16_t distance);
uint8_t game_spawnable(int16_t x,int16_t y,uint8_t ignore);
#endif
