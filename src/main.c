#include <zos_sys.h>
#include <zos_video.h>
#include <zos_vfs.h>
#include <zgdk.h>
#include <zgdk/tilemap/scroll.h>
#include "input.h"
#include "gameplay.h"
#include "render.h"
#include "assets.h"
#include "audio.h"
#include "splash.h"
#include "arena.h"
#include "timing.h"
static uint8_t phase=APP_SPLASH,selected,difficulty=1,engine,fire_block;
static uint8_t ending_frames,result_frames,result_armed;
static void menu(void)
{
    static const char* const levels[3]={"EASY  ","NORMAL","HARD  "};
    render_hide_sprites();render_clear_ui();audio_quiet();
    render_center("ZEAL TANK",6);
    render_center("DEFEND YOUR HEADQUARTERS",10);
    render_text("DIFFICULTY",8,14);render_text(levels[difficulty],23,14);
    render_text("ENGINE SOUND",8,17);render_text(engine ? "ON " : "OFF",23,17);
    render_text("-",6,14+selected*3);
    render_center("TWENTY ENEMIES PER STAGE",21);
    render_center("UP DOWN SELECT  LEFT RIGHT CHANGE",25);
    render_center("ENTER OR B TO START",27);
    render_center("X AUDIO  R SHIFT QUIT",29);
}
static void begin_match(const input_frame* input,uint8_t next)
{
    gfx_enable_screen(0);audio_quiet();
    if(next) game_next_stage();else game_start(difficulty);
    fire_block=!!(input->controls[0]&CMD_FIRE);
    render_hide_sprites();render_arena();render_frame();
    gfx_enable_screen(1);audio_play(SFX_START);phase=APP_PLAY;
}
static void result(void)
{
    result_frames=45;result_armed=0;
    render_hide_sprites();render_clear_ui();audio_engine(0,0);
    render_center(match.over==GAME_CLEAR ? "STAGE CLEAR" : "GAME OVER",6);
    render_center(arena_names[match.arena],9);
    if(match.over==GAME_LOST) render_center(match.hq_alive ? "NO TANKS REMAINING" : "HEADQUARTERS DESTROYED",12);
    render_text("SCORE",11,15);render_score(match.score,22,15);
    render_text("STAGE",11,17);render_number(match.stage,22,17);
    render_text("LIVES",11,19);render_number(match.lives,22,19);
    render_center(match.over==GAME_CLEAR ? "PRESS START FOR NEXT STAGE" : "PRESS START TO TRY AGAIN",23);
    render_center("LEFT OR RIGHT TO SETUP",26);
    render_center("X AUDIO  R SHIFT QUIT",29);phase=APP_RESULT;
}
int main(void)
{
    input_frame input;
    uint8_t splash_frames=0,splash_armed=0,skipped=0;
    uint16_t last_tick=65535;
    phase=APP_SPLASH;
    if(tank_input_init()!=ERR_SUCCESS) return 1;
    gfx_enable_screen(0);
    if(gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_4BIT,&video)!=GFX_SUCCESS || assets_load(&video)!=GFX_SUCCESS) {
        ioctl(DEV_STDOUT,CMD_RESET_SCREEN,0);return 1;
    }
    tilemap_scroll(0,0,0);tilemap_scroll(1,0,0);
    render_hide_sprites();audio_init();splash_render();gfx_enable_screen(1);
    timing_reset();
    while(1) {
        uint8_t before=phase,was_skipped=skipped,due=timing_due_limit(&last_tick,skipped ? 1 : 3),i;
        tank_input_age(due);
        tank_input_poll(&input);
        if(input.quit && phase!=APP_PLAY && phase!=APP_ENDING)break;
        if(input.mute) audio_toggle();
        if(phase==APP_SPLASH) {
            /* Discard launch-time events, then require a neutral button state
             * followed by a fresh press. A held launcher Enter cannot skip. */
            if(splash_frames<30)splash_frames++;
            else if(!input.accept_held && !input.start && !input.confirm)splash_armed=1;
            if(splash_armed && (input.start || input.confirm)) {phase=APP_MENU;menu();audio_play(SFX_START);}
        } else if(phase==APP_MENU) {
            if(input.up || input.down) {selected=!selected;menu();}
            if(input.left || input.right) {
                if(!selected) difficulty=(difficulty+(input.left ? 2 : 1))%3;else engine=!engine;
                menu();
            }
            if(input.confirm) begin_match(&input,0);
        } else if(phase==APP_PLAY) {
            if(input.pause || input.start || input.quit) {
                phase=APP_PAUSE;audio_quiet();render_center("PAUSED",11);render_center("P START RESUME  R SHIFT QUIT",13);
            } else {
                if(fire_block) {if(!(input.controls[0]&CMD_FIRE)) fire_block=0;input.controls[0]&=(uint8_t)~CMD_FIRE;}
                for(i=0;i<due && phase==APP_PLAY;i++) {
                    game_tick(input.controls[0]);
                    if(match.over){phase=APP_ENDING;ending_frames=60;audio_engine(0,0);}
                }
                if(phase==APP_PLAY)audio_engine(match.tanks[0].moving,engine);
            }
        } else if(phase==APP_PAUSE) {
            if(input.pause || input.start) {
                fire_block=!!(input.controls[0]&CMD_FIRE);render_resume();phase=APP_PLAY;
            }
        } else if(phase==APP_ENDING) {
            for(i=0;i<due && ending_frames;i++){game_end_tick();ending_frames--;}
            if(!ending_frames)result();
        } else if(phase==APP_RESULT) {
            if(result_frames)result_frames=due>=result_frames ? 0 : result_frames-due;
            else if(!input.accept_held && !input.confirm)result_armed=1;
            if(input.left || input.right) {phase=APP_MENU;menu();}
            else if(result_armed && input.confirm) begin_match(&input,match.over==GAME_CLEAR);
        }
        if(before!=phase){timing_reset();last_tick=0;due=1;skipped=0;}
        for(i=0;i<due;i++)audio_tick();
        /* Catch simulation up before spending time preparing another picture.
         * Input still polls on every batch, so pause/quit stay responsive. */
        if(!timing_present((uint16_t)(timing_now()-last_tick),phase==APP_PLAY && before==phase,&skipped))continue;
        /* Sustained overload cannot be caught up indefinitely. After a forced
         * presentation, discard old simulation debt instead of starving video. */
        /* Prepare outside VBlank. Only the 400-byte sprite transfer belongs
         * in the short blanking interval, not the entire renderer. */
        if(phase==APP_SPLASH)for(i=0;i<due;i++)splash_tick();
        else if(phase==APP_PLAY || phase==APP_ENDING)render_prepare_frame();
        gfx_wait_vblank(&video);timing_sample();
        if(phase==APP_SPLASH || phase==APP_PLAY || phase==APP_ENDING)render_present();
        gfx_wait_end_vblank(&video);timing_sample();
        if(was_skipped && before==phase)last_tick=(uint16_t)(timing_now()-1);
    }
    audio_quiet();render_hide_sprites();tilemap_scroll(0,0,0);tilemap_scroll(1,0,0);
    ioctl(DEV_STDOUT,CMD_RESET_SCREEN,0);return 0;
}
