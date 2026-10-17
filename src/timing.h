#ifndef ZTANK_TIMING_H
#define ZTANK_TIMING_H
#include <stdint.h>
/* Sample bounded work checkpoints and both VBlank edges. The current native
 * emulator does not supply a running OS time counter or raster position. */
void timing_sample(void);
void timing_reset(void);
uint16_t timing_now(void);
uint8_t timing_due(uint16_t* previous);
uint8_t timing_due_limit(uint16_t* previous,uint8_t limit);
uint8_t timing_present(uint16_t behind,uint8_t may_skip,uint8_t* skipped);
#ifndef __SDCC
extern uint8_t timing_host_blank;
#endif
#endif
