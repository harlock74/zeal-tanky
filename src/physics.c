#include "physics.h"
#include "arena.h"
#include "gameplay.h"
#include "timing.h"
uint8_t physics_move_tank(uint8_t tank,int16_t dx,int16_t dy)
{
    tank_state* t=&match.tanks[tank];tank_state* other=match.tanks;uint8_t i;
    int16_t nx=t->x+dx,ny=t->y+dy;
    /* Later-stage fractional speeds must land on turn centres instead of
     * skipping them. This also prevents wall penetration at exact-fit lanes. */
    if(tank) {
        if(dx>0 && (t->x&511)<256 && (nx&511)>=256)nx=(t->x&~511)+256;
        else if(dx<0 && (t->x&511)>256 && (nx&511)<=256)nx=(t->x&~511)+256;
        if(dy>0 && (t->y&511)<256 && (ny&511)>=256)ny=(t->y&~511)+256;
        else if(dy<0 && (t->y&511)>256 && (ny&511)<=256)ny=(t->y&~511)+256;
    }
    int16_t px=(uint16_t)nx>>4,py=(uint16_t)ny>>4;
    timing_sample();
    if(!arena_can_place(px,py)) {
        int16_t cross=dx ? t->y : t->x;
        int16_t centre=(cross&~511)+256,offset=centre-cross;
        int16_t speed=dx ? (dx<0 ? -dx : dx) : (dy<0 ? -dy : dy);
        /* Ease into a narrow lane only if its centre really is open. Spend
         * this frame's movement on alignment; never snap or move diagonally. */
        if(!offset || offset>128 || offset<-128 ||
            !arena_can_place(dx ? px : centre>>4,dx ? centre>>4 : py))return 0;
        if(offset>speed)offset=speed;else if(offset<-speed)offset=-speed;
        nx=t->x+(dy ? offset : 0);ny=t->y+(dx ? offset : 0);
        px=(uint16_t)nx>>4;py=(uint16_t)ny>>4;
        if(!arena_can_place(px,py))return 0;
    }
    for(i=0;i<TANK_COUNT;i++,other++) if(other!=t && other->active &&
        (uint16_t)(nx-other->x+TANK_DIAMETER*16-1)<TANK_DIAMETER*32-1 &&
        (uint16_t)(ny-other->y+TANK_DIAMETER*16-1)<TANK_DIAMETER*32-1)return 0;
    t->x=nx;t->y=ny;return 1;
}
uint8_t physics_overlap(int16_t x,int16_t y,int16_t ox,int16_t oy,uint8_t d)
{
    int16_t dx=x-ox,dy=y-oy;
    return dx>-(int16_t)d && dx<d && dy>-(int16_t)d && dy<d;
}
