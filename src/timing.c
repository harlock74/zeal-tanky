#include "timing.h"
#ifdef __SDCC
#include <zvb_hardware.h>
#define BLANK() (zvb_ctrl_status&2)
#else
uint8_t timing_host_blank;
#define BLANK() timing_host_blank
#endif
static uint16_t frames;
static uint8_t was_blank;
void timing_reset(void){frames=0;was_blank=BLANK();}
void timing_sample(void)
{
    uint8_t blank=BLANK();
    if(blank && !was_blank)frames++;
    was_blank=blank;
}
uint16_t timing_now(void){timing_sample();return frames;}
uint8_t timing_due_limit(uint16_t* previous,uint8_t limit)
{
    uint16_t now=timing_now(),delta=now-*previous;
    uint8_t due=delta>limit ? limit : (uint8_t)delta;
    /* Keep ordinary backlog across short batches instead of losing a tick
     * whenever a busy frame crosses four refreshes. Drop only long hitches;
     * pause/loading transitions reset the schedule explicitly. */
    if(delta>8)*previous=now;else *previous+=due;
    return due;
}

uint8_t timing_due(uint16_t* previous){return timing_due_limit(previous,3);}
/* Never skip more than one consecutive presentations, even under overload. */
uint8_t timing_present(uint16_t behind,uint8_t may_skip,uint8_t* skipped)
{
    if(may_skip && behind>=2 && *skipped<1){(*skipped)++;return 0;}
    *skipped=0;return 1;
}
