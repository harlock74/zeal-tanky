#ifndef ZTANK_INPUT_H
#define ZTANK_INPUT_H
#include <stdint.h>
#include <zos_errors.h>
typedef struct { uint8_t controls[1], start, quit, pause, mute, left, right, up, down, confirm, accept_held; } input_frame;
zos_err_t tank_input_init(void);
void tank_input_age(uint8_t frames);
void tank_input_poll(input_frame* frame);
#endif
