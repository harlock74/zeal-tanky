#include <string.h>
#include "gameplay.h"
#include "arena.h"
#include "physics.h"
#include "audio.h"
#include "ai.h"
#include "profile.h"
#include "timing.h"
match_state match;
#define CLOSE(x,y,ox,oy,d) ((uint16_t)((x)-(ox)+(d)-1)<(uint16_t)((d)*2-1) && (uint16_t)((y)-(oy)+(d)-1)<(uint16_t)((d)*2-1))
static uint8_t effect_near[SHELL_COUNT];
static void effect(int16_t x,int16_t y,uint8_t kind)
{
    uint8_t i;
    for(i=0;i<6;i++) if(!match.effects[i].frames) {
        match.effects[i].x=x;match.effects[i].y=y;
        match.effects[i].frames=kind ? 30 : 12;match.effects[i].kind=kind;
        if(kind) {
            uint8_t j;
            for(j=0;j<SHELL_COUNT;j++)if(match.shells[j].active &&
                CLOSE((uint16_t)match.shells[j].x>>4,(uint16_t)match.shells[j].y>>4,x,y,16+SHELL_SPEED))effect_near[j]=1;
        }
        return;
    }
}
uint8_t game_spawnable(int16_t x,int16_t y,uint8_t ignore)
{
    uint8_t i;
    if(!arena_can_place(x,y)) return 0;
    for(i=0;i<TANK_COUNT;i++) if(i!=ignore && match.tanks[i].active &&
        physics_overlap(x,y,((uint16_t)match.tanks[i].x>>4),((uint16_t)match.tanks[i].y>>4),TANK_DIAMETER)) return 0;
    return 1;
}
static void reset_stage(void)
{
    uint8_t i;
    match.arena=(match.stage-1)%ARENA_COUNT;
    arena_load(match.arena);
    memset(match.tanks,0,sizeof(match.tanks));memset(match.shells,0,sizeof(match.shells));
    memset(match.effects,0,sizeof(match.effects));
    match.elapsed=0;match.animation_tick=0;match.spawned=match.destroyed=match.over=match.respawn_wait=0;
    match.spawn_wait=45;match.hq_alive=1;match.previous_controls=0;
    for(i=0;i<TANK_COUNT;i++) match.tanks[i].heading=i ? 4 : 12;
    match.tanks[0].x=arena_spawn_x(0)*16;match.tanks[0].y=arena_spawn_y(0)*16;
    match.tanks[0].active=1;match.tanks[0].invulnerable=90;
    ai_reset();ai_prepare(); /* Build routes before the first visible play frame. */
}
void game_start(uint8_t difficulty)
{
    memset(&match,0,sizeof(match));match.stage=1;match.lives=3;
    match.difficulty=difficulty>2 ? 1 : difficulty;reset_stage();
}
void game_next_stage(void)
{
    if(match.over!=GAME_CLEAR) return;
    if(match.stage<999) match.stage++;
    reset_stage();
}
uint16_t game_hit_score(uint8_t shot,uint8_t heading,uint16_t distance)
{
    static const uint16_t front[4]={800,300,150,60},other[4]={300,150,60,30};
    uint8_t band=distance<64 ? 0 : distance<128 ? 1 : distance<192 ? 2 : 3;
    return heading==((shot+8)&15) ? front[band] : other[band];
}
static void hit_tank(uint8_t id,uint8_t shot)
{
    tank_state* t=&match.tanks[id];
    if(!t->active || t->invulnerable) return;
    t->active=0;t->moving=0;effect(((uint16_t)t->x>>4),((uint16_t)t->y>>4),1);audio_play(SFX_EXPLOSION);
    if(!id) {
        if(match.lives) match.lives--;
        match.respawn_wait=90;
        if(!match.lives) match.over=GAME_LOST;
    } else {
        int16_t dx=((uint16_t)t->x>>4)-((uint16_t)match.tanks[0].x>>4),dy=((uint16_t)t->y>>4)-((uint16_t)match.tanks[0].y>>4);
        uint16_t distance;
        if(dx<0) dx=-dx;if(dy<0) dy=-dy;distance=dx>dy ? dx : dy;
        match.score+=game_hit_score(shot,t->heading,distance);
        if(match.score>999999UL) match.score=999999UL;
        match.destroyed++;
        if(!match.bonus_awarded && match.score>=20000UL) {
            match.bonus_awarded=1;if(match.lives<9) match.lives++;
            audio_play(SFX_START);
        }
    }
}
static void shoot(uint8_t tank,uint8_t slot)
{
    tank_state* t=&match.tanks[tank];shell_state* s=&match.shells[slot];
    s->x=t->x+(t->heading==0 ? 256 : t->heading==8 ? -256 : 0);
    s->y=t->y+(t->heading==4 ? 256 : t->heading==12 ? -256 : 0);
    s->heading=t->heading;s->owner=tank;s->active=1;s->age=0;
    audio_play(SFX_FIRE);
}
static void spawn_enemy(void)
{
    uint8_t i,side;
    if(match.spawned>=ENEMIES_PER_STAGE) return;
    for(i=1;i<TANK_COUNT;i++) if(!match.tanks[i].active) {
        for(side=0;side<2;side++) {
            uint8_t spawn=1+(match.spawned+side)%2;
            int16_t x=arena_spawn_x(spawn),y=arena_spawn_y(spawn);
            if(game_spawnable(x,y,i)) {
                tank_state* t=&match.tanks[i];memset(t,0,sizeof(*t));
                t->x=x*16;t->y=y*16;t->heading=4;t->active=1;t->invulnerable=30;
                t->fire_wait=45+i*7;match.spawned++;return;
            }
        }
        return;
    }
}
static uint8_t player_direction(uint8_t controls)
{
    uint8_t vertical=controls&(CMD_UP|CMD_DOWN),horizontal=controls&(CMD_LEFT|CMD_RIGHT);
    uint8_t fresh=controls&~match.previous_controls;
    match.previous_controls=controls;
    if(vertical==(CMD_UP|CMD_DOWN)) vertical=0;
    if(horizontal==(CMD_LEFT|CMD_RIGHT)) horizontal=0;
    if(vertical && horizontal) {
        if(fresh&(CMD_UP|CMD_DOWN)) horizontal=0;
        else if(fresh&(CMD_LEFT|CMD_RIGHT)) vertical=0;
        else if(match.tanks[0].heading==0 || match.tanks[0].heading==8) vertical=0;
        else horizontal=0;
    }
    return vertical|horizontal;
}
static uint8_t absorbed(int16_t x,int16_t y)
{
    uint8_t i;
    for(i=0;i<6;i++) if(match.effects[i].frames && match.effects[i].kind &&
        CLOSE(x,y,match.effects[i].x,match.effects[i].y,16)) return 1;
    return 0;
}
/* Keep preparation separate from the hot substep loop: SDCC otherwise
 * spills substantially more state after the signed boundary checks. */
static uint8_t victims[SHELL_COUNT],terrain_cache[SHELL_COUNT];
static int16_t tank_x[TANK_COUNT],tank_y[TANK_COUNT],shell_x[SHELL_COUNT],shell_y[SHELL_COUNT];
static int16_t terrain_x[SHELL_COUNT],terrain_y[SHELL_COUNT];
static int16_t shell_pixel(int16_t position)
{
    if(position<0)return -(int16_t)((uint16_t)(-position)>>4);
    return (uint16_t)position>>4;
}
static void prepare_shells(void)
{
    uint8_t i,j;
    for(j=0;j<TANK_COUNT;j++) {tank_x[j]=(uint16_t)match.tanks[j].x>>4;tank_y[j]=(uint16_t)match.tanks[j].y>>4;}
    for(i=0;i<SHELL_COUNT;i++) {
        timing_sample();
        effect_near[i]=victims[i]=0;terrain_x[i]=terrain_y[i]=-1;
        shell_x[i]=shell_pixel(match.shells[i].x);shell_y[i]=shell_pixel(match.shells[i].y);
        if(match.shells[i].active) {
            int16_t x=shell_x[i],y=shell_y[i];
            for(j=0;j<6;j++)if(match.effects[j].frames && match.effects[j].kind &&
                CLOSE(x,y,match.effects[j].x,match.effects[j].y,16+SHELL_SPEED))effect_near[i]=1;
            for(j=match.shells[i].owner ? 0 : 1;j<(match.shells[i].owner ? 1 : TANK_COUNT);j++)
                if(match.tanks[j].active && CLOSE(x,y,tank_x[j],tank_y[j],TANK_RADIUS+2+SHELL_SPEED))victims[i]|=1u<<j;
        }
    }
}
static void update_shells(void)
{
    static uint8_t step,i,j;
    prepare_shells();
    for(step=0;step<SHELL_SPEED/SHELL_STEP && match.over!=GAME_LOST;step++) for(i=0;i<SHELL_COUNT && match.over!=GAME_LOST;i++) {
        timing_sample();
        static shell_state* s;static int16_t x,y;static uint8_t terrain;
        s=&match.shells[i];
        if(!s->active) continue;
        if(s->heading==0)shell_x[i]+=SHELL_STEP;
        else if(s->heading==8)shell_x[i]-=SHELL_STEP;
        else if(s->heading==4)shell_y[i]+=SHELL_STEP;
        else shell_y[i]-=SHELL_STEP;
        x=shell_x[i];y=shell_y[i];
        /* Three-pixel interleaved steps are below the 8px shell contact width,
         * 16px smallest brick and 28px tank contact width: no tunnelling. */
        /* Slot zero belongs to the player; slots one and two to enemies.
         * Friendly shells do not cancel one another. */
        for(j=i ? 0 : 1;j<(i ? 1 : SHELL_COUNT);j++) if(match.shells[j].active &&
            CLOSE(x,y,shell_x[j],shell_y[j],4)) {
            match.shells[j].active=s->active=0;effect(x,y,0);break;
        }
        if(!s->active) continue;
        if(effect_near[i] && absorbed(x,y)) {s->active=0;continue;}
        /* No terrain is added during a tick. Reuse a clear quadrant until
         * the shell crosses its edge; brick contacts stop the shell. */
        if(terrain_x[i]!=(x&~15) || terrain_y[i]!=(y&~15)) {
            terrain_x[i]=x&~15;terrain_y[i]=y&~15;terrain_cache[i]=arena_at(x,y);
        }
        terrain=terrain_cache[i];
        if(terrain==TILE_HQ) {
            match.hq_alive=0;match.over=GAME_LOST;s->active=0;
            effect(arena_spawn_x(3),arena_spawn_y(3),1);audio_play(SFX_EXPLOSION);break;
        }
        if(terrain==TILE_WALL || terrain==TILE_BRICK) {
            if(terrain==TILE_BRICK) arena_damage(x,y,s->heading);
            effect(x,y,0);audio_play(SFX_IMPACT);s->active=0;continue;
        }
        if(victims[i])for(j=s->owner ? 0 : 1;j<(s->owner ? 1 : TANK_COUNT);j++) if((victims[i]&(1u<<j)) && match.tanks[j].active &&
            CLOSE(x,y,tank_x[j],tank_y[j],TANK_RADIUS+2)) {
            hit_tank(j,s->heading);s->active=0;break;
        }
    }
    /* Collision uses the interleaved pixel positions above. Commit Q4 only
     * once, preserving fractional offsets from higher-stage enemy movement. */
    for(i=0;i<SHELL_COUNT;i++)if(match.shells[i].active) {
        match.shells[i].x=shell_x[i]*16+(match.shells[i].x&15);
        match.shells[i].y=shell_y[i]*16+(match.shells[i].y&15);
        if(++match.shells[i].age==255)match.shells[i].active=0;
    }
}
void game_tick(uint8_t controls)
{
    uint8_t i,direction,fire_pressed;
    if(match.over) return;
    if(match.elapsed<65535) match.elapsed++;
    if(++match.animation_tick==12)match.animation_tick=0;
    for(i=0;i<6;i++) if(match.effects[i].frames) match.effects[i].frames--;
    if(match.respawn_wait) match.respawn_wait--;
    if(!match.tanks[0].active && match.lives && !match.respawn_wait &&
        game_spawnable(arena_spawn_x(0),arena_spawn_y(0),0)) {
        match.tanks[0].x=arena_spawn_x(0)*16;match.tanks[0].y=arena_spawn_y(0)*16;
        match.tanks[0].heading=12;match.tanks[0].active=1;match.tanks[0].invulnerable=90;
    }
    if(match.spawn_wait) match.spawn_wait--;
    if(!match.spawn_wait) {spawn_enemy();match.spawn_wait=60;}
    profile_start(3);ai_tick();profile_stop(3);
    /* Consume a press once, even if the shell slot is busy or the player is
     * dead. Holding fire must not create a replacement after an impact. */
    fire_pressed=(controls&CMD_FIRE) && !(match.previous_controls&CMD_FIRE);
    direction=player_direction(controls);
    profile_start(4);
    for(i=0;i<TANK_COUNT;i++) {
        timing_sample();
        tank_state* t=&match.tanks[i];uint8_t c;
        uint8_t move=0,heading=t->heading,speed=i ? 16 : 32;
        if(!t->active) continue;
        c=i ? ai_controls(i) : direction;
        timing_sample();
        if(t->invulnerable) t->invulnerable--;
        if(i && match.stage>8) speed=20;
        if(c&CMD_UP) {heading=12;move=1;}
        else if(c&CMD_DOWN) {heading=4;move=1;}
        else if(c&CMD_LEFT) {heading=8;move=1;}
        else if(c&CMD_RIGHT) {heading=0;move=1;}
        t->heading=heading;
        t->moving=move ? physics_move_tank(i,
            heading==0 ? speed : heading==8 ? -(int16_t)speed : 0,
            heading==4 ? speed : heading==12 ? -(int16_t)speed : 0) : 0;
        if(!i && fire_pressed && !match.shells[0].active) shoot(0,0);
        if(i && !t->invulnerable && (c&CMD_FIRE)) {
            uint8_t slot;
            for(slot=1;slot<SHELL_COUNT;slot++) if(!match.shells[slot].active) {shoot(i,slot);break;}
        }
    }
    profile_stop(4);profile_start(5);update_shells();profile_stop(5);
    if(!match.over && match.destroyed==ENEMIES_PER_STAGE) {
        match.over=GAME_CLEAR;audio_play(SFX_RESULT);
    }
}

/* Terminal animation advances effects only: no enemies, shots or new damage. */
void game_end_tick(void)
{
    uint8_t i;
    for(i=0;i<TANK_COUNT;i++)match.tanks[i].moving=0;
    for(i=0;i<6;i++)if(match.effects[i].frames)match.effects[i].frames--;
}
