#include "audio.h"
#include <zvb_sound.h>
static uint8_t frames,effect,engine,enabled=1,priority;
static void route(void)
{
    uint8_t voices=enabled ? (frames ? VOICE0 : 0)|(engine ? VOICE1 : 0) : 0;
    zvb_sound_set_channels(voices,voices);
    zvb_sound_set_volume(voices ? VOL_50 : VOL_0);
}
void audio_quiet(void)
{
    frames=engine=priority=0;
    zvb_sound_set_volume(VOL_0);zvb_sound_set_channels(0,0);
    zvb_sound_set_voices_vol(0x0f,VOL_0);zvb_sound_set_hold(0xff,1);
}
void audio_init(void){zvb_sound_initialize(1);enabled=1;audio_quiet();}
uint8_t audio_is_enabled(void){return enabled;}
void audio_toggle(void){enabled=!enabled;audio_quiet();}
void audio_engine(uint8_t moving,uint8_t use_engine)
{
    uint8_t next=enabled && moving && use_engine;
    if(next!=engine) {
        engine=next;
        if(engine) {zvb_sound_set_voices(VOICE1,95,WAV_TRIANGLE);zvb_sound_set_voices_vol(VOICE1,VOL_25);zvb_sound_set_hold(VOICE1,0);}
        else {zvb_sound_set_voices_vol(VOICE1,VOL_0);zvb_sound_set_hold(VOICE1,1);}
        route();
    }
}
void audio_play(uint8_t e)
{
    static const uint8_t duration[5]={6,5,24,18,24},order[5]={1,0,3,2,2};
    /* ZVB phase dividers, not Hz: approximately 404, 121, 175, 296, 444 Hz. */
    static const uint16_t divider[5]={600,180,260,440,660};
    if(!enabled || e>SFX_RESULT || (frames && order[e]<priority)) return;
    effect=e;priority=order[e];frames=duration[e];
    zvb_sound_set_channels(engine ? VOICE1 : 0,engine ? VOICE1 : 0);
    zvb_sound_set_voices_vol(VOICE0,VOL_0);
    zvb_sound_set_voices(VOICE0,divider[e],e==SFX_EXPLOSION || e==SFX_IMPACT ? WAV_NOISE : WAV_TRIANGLE);
    zvb_sound_set_voices_vol(VOICE0,e==SFX_EXPLOSION ? VOL_50 : VOL_25);
    zvb_sound_set_hold(VOICE0,0);route();
}
void audio_tick(void)
{
    if(frames) {
        frames--;
        if(!frames) {zvb_sound_set_voices_vol(VOICE0,VOL_0);zvb_sound_set_hold(VOICE0,1);priority=0;}
        else if(effect==SFX_START && frames%6==0) zvb_sound_set_voices(VOICE0,440+(18-frames)*30,WAV_TRIANGLE);
        else if(effect==SFX_EXPLOSION && frames==12) zvb_sound_set_voices_vol(VOICE0,VOL_25);
    }
    /* Idle output is disconnected and master-muted on every frame. */
    route();
}
