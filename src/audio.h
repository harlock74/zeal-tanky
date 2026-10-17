#ifndef ZTANK_AUDIO_H
#define ZTANK_AUDIO_H
#include <stdint.h>
enum {SFX_FIRE,SFX_IMPACT,SFX_EXPLOSION,SFX_START,SFX_RESULT};
void audio_init(void);void audio_tick(void);void audio_play(uint8_t effect);
void audio_quiet(void);void audio_toggle(void);
void audio_engine(uint8_t moving,uint8_t enabled);
uint8_t audio_is_enabled(void);
#endif
