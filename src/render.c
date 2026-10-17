#include "render.h"
#include "arena.h"
#include "gameplay.h"
#include "audio.h"
#include <string.h>
#include "profile.h"
#include "timing.h"
gfx_context video;
static uint8_t previous_cells[ARENA_W*ARENA_H],hud_valid;
static uint32_t previous_score;
static uint8_t previous_lives,previous_left,previous_audio;
static gfx_sprite sprites[50];
static uint16_t sprite_offset_x,sprite_offset_y;
static const uint8_t blast_frames[31]={5,5,5,5,5,5,4,4,4,4,4,3,3,3,3,3,2,2,2,2,2,1,1,1,1,1,0,0,0,0,0};
void render_reset(void){memset(previous_cells,255,sizeof(previous_cells));hud_valid=0;arena_dirty=1;}
static void render_cells(void)
{
    uint8_t x,y,q;
    if(!arena_dirty)return;
    arena_dirty=0;
    for(y=0;y<ARENA_H;y++) for(x=0;x<ARENA_W;x++) {
        timing_sample();
        uint16_t pos=y*ARENA_W+x;uint8_t t=arena_cells[pos];
        if(previous_cells[pos]==t) continue;
        for(q=0;q<4;q++) {
            uint8_t tile=t;
            if(t==TILE_HQ) tile=160+q;
            else if(t>=16 && t<=31) tile=(t&(1u<<q)) ? TILE_BRICK : TILE_FLOOR;
            gfx_tilemap_place(&video,tile,0,FIELD_X/16+x*2+(q&1),FIELD_Y/16+y*2+q/2);
        }
        previous_cells[pos]=t;
    }
}
void render_arena(void)
{
    uint8_t x,y;
    render_reset();render_clear_ui();render_cells();
    for(y=1;y<29;y++) for(x=6;x<34;x++)
        if(y==1 || y==28 || x==6 || x==33)gfx_tilemap_place(&video,9,0,x,y);
}
/* Pause text touches tile rows 11 and 13 (logical rows 4 and 5).
 * Its long hint also covers both side borders; restore those tiles too. */
void render_resume(void)
{
    uint8_t y;
    memset(previous_cells+4*ARENA_W,255,2*ARENA_W);
    arena_dirty=1;render_cells();
    for(y=11;y<=13;y+=2) {
        gfx_tilemap_place(&video,9,0,6,y);
        gfx_tilemap_place(&video,9,0,33,y);
    }
}
static void put_sprite(uint8_t slot,int16_t x,int16_t y,uint16_t tile)
{
    sprites[slot].x=x+sprite_offset_x;sprites[slot].y=y+sprite_offset_y;sprites[slot].tile=(uint8_t)tile;
    sprites[slot].flags=tile>=256 ? SPRITE_TILE_BIT9 : SPRITE_NONE;
}
static void put_quad(uint8_t slot,int16_t x,int16_t y,uint16_t tile,uint8_t bank)
{
    static gfx_sprite* s;
    s=&sprites[slot];
    /* Effects and stationary actors often keep the same four records. */
    if(s->x==x+sprite_offset_x && s->y==y+sprite_offset_y &&
        s->tile==(uint8_t)tile && s->flags==((tile>=256 ? SPRITE_TILE_BIT9 : SPRITE_NONE)|bank))return;
    s->x=x+sprite_offset_x;s->y=y+sprite_offset_y;s->tile=(uint8_t)tile;
    s->flags=(tile>=256 ? SPRITE_TILE_BIT9 : SPRITE_NONE)|bank;s->options=0;
    s[1]=s[0];s[1].x+=16;s[1].tile++;
    s[2]=s[0];s[2].y+=16;s[2].tile+=2;
    s[3]=s[2];s[3].x+=16;s[3].tile++;
}
static void clear_sprites(uint8_t slot,uint8_t count)
{
    if(sprites[slot].x || sprites[slot].y)memset(&sprites[slot],0,count*sizeof(gfx_sprite));
}
void render_prepare_sprites(void)
{
    static uint8_t i,track;profile_start(6);track=match.animation_tick>=6;
    sprite_offset_x=16+(match.stage ? FIELD_X : 0);sprite_offset_y=16+(match.stage ? FIELD_Y : 0);
    for(i=0;i<TANK_COUNT;i++) {
        timing_sample();
        static tank_state* t; t=&match.tanks[i];
        static uint8_t frame; frame=t->heading*2+((t->moving || t->invulnerable) ? track : 0);
        /* A protected tank still blocks movement: never make it invisible. */
        if(t->active)
            put_quad(i*4,((uint16_t)t->x>>4)-16,((uint16_t)t->y>>4)-16,TANK_VIDEO_TILE+(uint16_t)frame*4,i ? SPRITE_PALETTE_01 : 0);
        else clear_sprites(i*4,4);
    }
    for(i=0;i<SHELL_COUNT;i++) {
        timing_sample();
        static shell_state* s; s=&match.shells[i];
        if(s->active)put_sprite(20+i,s->x/16-8,s->y/16-8,7);
        else clear_sprites(20+i,1);
        if(s->active && (s->age==1 || s->age==2))put_sprite(47+i,s->x/16-8,s->y/16-8,123+s->age);
        else clear_sprites(47+i,1);
    }
    for(i=0;i<6;i++) {
        timing_sample();
        static effect_state* e; e=&match.effects[i];
        if(!e->frames)clear_sprites(23+i*4,4);
        else if(e->kind) put_quad(23+i*4,e->x-16,e->y-16,136+blast_frames[e->frames]*4,0);
        else {clear_sprites(24+i*4,3);put_sprite(23+i*4,e->x-8,e->y-8,126+(12-e->frames)/6);}
    }
    profile_stop(6);
}
void render_present(void){profile_start(7);gfx_sprite_render_array(&video,0,sprites,50);profile_stop(7);timing_sample();}
void render_hide_sprites(void)
{
    uint8_t i;gfx_sprite s={0};
    memset(sprites,0,sizeof(sprites));
    for(i=0;i<GFX_SPRITES_COUNT;i++) gfx_sprite_render(&video,i,&s);
}
void render_clear_ui(void)
{
    uint8_t y,row[40];
    /* In native 4bpp, layer 1 contains attributes, not overlay graphics. */
    memset(row,0,sizeof(row));
    for(y=0;y<30;y++) gfx_tilemap_load(&video,row,40,1,0,y);
    memset(row,8,sizeof(row));
    for(y=0;y<30;y++) gfx_tilemap_load(&video,row,40,0,0,y);
    hud_valid=0;
}
void render_text(const char* t,uint8_t x,uint8_t y)
{
    while(*t && x<40 && y<30) {
        char c=*t++;uint8_t tile=EMPTY_TILE;
        if(c>='A' && c<='Z') tile=80+c-'A';
        else if(c>='0' && c<='9') tile=106+c-'0';
        else if(c=='-') tile=116;else if(c==':') tile=117;
        gfx_tilemap_place(&video,tile,0,x++,y);
    }
}
void render_center(const char* text,uint8_t y)
{
    uint16_t length=strlen(text);if(length>40) length=40;render_text(text,(40-length)/2,y);
}
void render_number(uint16_t value,uint8_t x,uint8_t y)
{
    char text[4];if(value>999) value=999;
    text[0]='0'+value/100;text[1]='0'+(value/10)%10;text[2]='0'+value%10;text[3]=0;
    render_text(text,x,y);
}
void render_score(uint32_t value,uint8_t x,uint8_t y)
{
    char text[7];uint8_t i=6;text[6]=0;
    do {text[--i]='0'+value%10;value/=10;}while(i);
    render_text(text,x,y);
}
void render_prepare_frame(void)
{
    uint8_t sound=audio_is_enabled(),left=ENEMIES_PER_STAGE-match.destroyed;
    render_cells();render_prepare_sprites();
    if(!hud_valid) {
        render_text("1UP",6,0);render_text("STAGE",25,0);render_number(match.stage,31,0);
        render_text("LIVES",6,29);render_text("LEFT",21,29);
    }
    if(!hud_valid || previous_score!=match.score) {render_score(match.score,10,0);previous_score=match.score;}
    if(!hud_valid || previous_lives!=match.lives) {render_number(match.lives,12,29);previous_lives=match.lives;}
    if(!hud_valid || previous_left!=left) {render_number(left,26,29);previous_left=left;}
    if(!hud_valid || previous_audio!=sound) {render_text(sound ? "ON " : "OFF",36,29);previous_audio=sound;}
    hud_valid=1;
}
void render_frame(void){render_prepare_frame();render_present();}
