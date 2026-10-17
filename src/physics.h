#ifndef ZTANK_PHYSICS_H
#define ZTANK_PHYSICS_H
#include <stdint.h>
uint8_t physics_overlap(int16_t x,int16_t y,int16_t ox,int16_t oy,uint8_t distance);
uint8_t physics_move_tank(uint8_t tank,int16_t dx,int16_t dy);
#endif
