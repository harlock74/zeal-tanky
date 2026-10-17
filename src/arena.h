#ifndef ZTANK_ARENA_H
#define ZTANK_ARENA_H
#include "config.h"
extern uint8_t arena_cells[ARENA_W*ARENA_H];
extern uint8_t arena_dirty;
extern const char* const arena_names[ARENA_COUNT];
void arena_load(uint8_t index);
int16_t arena_spawn_x(uint8_t actor);int16_t arena_spawn_y(uint8_t actor);
void arena_damage(int16_t x,int16_t y,uint8_t heading);
uint8_t arena_at(int16_t x,int16_t y);
uint8_t arena_can_place(int16_t x,int16_t y);
#endif
