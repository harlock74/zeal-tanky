#include "splash.h"
#include "render.h"
#include "gameplay.h"
#include <string.h>
#include "splash.generated.h"
static uint16_t ticks;
void splash_render(void)
{
    uint8_t i,x,row[40],left=0,tile=8;uint16_t p=0;
    render_hide_sprites();render_clear_ui();render_reset();ticks=0;
    for(i=0;i<30;i++) {
        for(x=0;x<40;x++) {
            if(!left){left=splash_runs[p++];tile=splash_runs[p++];}
            row[x]=tile;left--;
        }
        gfx_tilemap_load(&video,row,40,0,0,i);
    }
    memset(&match,0,sizeof(match));
    for(i=0;i<3;i++) {
        match.tanks[i].x=splash_actors[i][0]*16;
        match.tanks[i].y=splash_actors[i][1]*16;
        match.tanks[i].heading=splash_actors[i][2];match.tanks[i].active=1;
    }
    render_prepare_sprites();render_present();
}
static void demo_fire(uint8_t id)
{
    tank_state* t=&match.tanks[id];shell_state* s=&match.shells[id];
    if(s->active || !splash_actors[id][3])return;
    s->x=t->x+(t->heading==0 ? 256 : t->heading==8 ? -256 : 0);
    s->y=t->y+(t->heading==4 ? 256 : t->heading==12 ? -256 : 0);
    s->heading=t->heading;s->active=1;s->age=0;s->owner=id;
}
void splash_tick(void)
{
    uint8_t i,patrol;uint16_t beat;
    if(++ticks==5760)ticks=0;match.elapsed++;if(++match.animation_tick==12)match.animation_tick=0;beat=ticks%180;
    if(ticks%30==1)gfx_tilemap_load(&video,(void*)((ticks/30)&1 ? splash_flash_off : splash_flash_on),SPLASH_FLASH_LENGTH,0,SPLASH_FLASH_X,SPLASH_FLASH_Y);
    patrol=(uint8_t)(ticks&127);if(patrol>=64)patrol=127-patrol;
    for(i=0;i<3;i++) {
        tank_state* t=&match.tanks[i];int16_t offset=(uint16_t)patrol*splash_actors[i][4]/63;
        t->x=(splash_actors[i][0]+(t->heading==0 ? offset : t->heading==8 ? -offset : 0))*16;
        t->y=(splash_actors[i][1]+(t->heading==4 ? offset : t->heading==12 ? -offset : 0))*16;
        t->moving=!!splash_actors[i][4];
    }
    for(i=0;i<6;i++)if(match.effects[i].frames)match.effects[i].frames--;
    if(beat==10)demo_fire(1);if(beat==70)demo_fire(2);if(beat==110)demo_fire(0);
    for(i=0;i<3;i++)if(match.shells[i].active) {
        shell_state* s=&match.shells[i];uint8_t hit=0;
        int16_t target_x=(splash_actors[i][0]+(s->heading==0 ? splash_actors[i][3] : s->heading==8 ? -splash_actors[i][3] : 0))*16;
        int16_t target_y=(splash_actors[i][1]+(s->heading==4 ? splash_actors[i][3] : s->heading==12 ? -splash_actors[i][3] : 0))*16;
        if(s->heading==0){s->x+=96;if(s->x>=target_x){s->x=target_x;hit=1;}}
        else if(s->heading==8){s->x-=96;if(s->x<=target_x){s->x=target_x;hit=1;}}
        else if(s->heading==4){s->y+=96;if(s->y>=target_y){s->y=target_y;hit=1;}}
        else {s->y-=96;if(s->y<=target_y){s->y=target_y;hit=1;}}
        s->age++;
        if(hit) {
            effect_state* e=&match.effects[i];s->active=0;
            e->x=s->x/16;e->y=s->y/16;e->frames=30;e->kind=1;
        }
    }
    render_prepare_sprites();
}
