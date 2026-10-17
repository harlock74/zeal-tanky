#ifndef ZTANK_AI_H
#define ZTANK_AI_H
#include <stdint.h>
void ai_reset(void);
void ai_prepare(void);
void ai_tick(void);
void ai_invalidate(void);
uint8_t ai_controls(uint8_t tank);
uint8_t ai_clear_line(int16_t x,int16_t y,int16_t tx,int16_t ty);
#endif
