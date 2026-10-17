#ifndef ZTANK_PROFILE_H
#define ZTANK_PROFILE_H
#ifdef TANK_PROFILE
#include <stdint.h>
void profile_start(uint8_t id);
void profile_stop(uint8_t id);
#else
#define profile_start(id) ((void)0)
#define profile_stop(id) ((void)0)
#endif
#endif
