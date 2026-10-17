#include "arena.h"
#include "timing.h"
#include "ai.h"
uint8_t arena_cells[ARENA_W*ARENA_H];
uint8_t arena_dirty;
#include "arenas.generated.h"
void arena_load(uint8_t index)
{
    uint16_t i;if(index>=ARENA_COUNT) index=0;
    for(i=0;i<ARENA_W*ARENA_H;i++) arena_cells[i]=arena_maps[index][i];
    arena_dirty=1;
}
int16_t arena_spawn_x(uint8_t actor){return arena_spawns[actor%4][0];}
int16_t arena_spawn_y(uint8_t actor){return arena_spawns[actor%4][1];}
uint8_t arena_at(int16_t x,int16_t y)
{
    uint8_t cell,quadrant;
    if(x<0 || x>=WORLD_W || y<0 || y>=WORLD_H) return TILE_WALL;
    cell=arena_cells[(uint16_t)((uint16_t)y>>5)*ARENA_W+((uint16_t)x>>5)];
    if(cell>=16 && cell<=31) {
        quadrant=(y&16 ? 2 : 0)+(x&16 ? 1 : 0);
        return (cell&(1u<<quadrant)) ? TILE_BRICK : TILE_FLOOR;
    }
    return cell;
}
/* A terrain cell is either fully solid, a four-quadrant brick mask, or
 * passable. This helper preserves the original treatment of all tile IDs. */
static uint8_t blocked(uint8_t tile,uint8_t mask)
{
    if(tile==TILE_FLOOR)return 0;
    if((tile&240)==16)return !!(tile&mask);
    return tile==TILE_WALL || tile==TILE_HQ || tile==TILE_BRICK;
}
uint8_t arena_can_place(int16_t x,int16_t y)
{
    /* Scratch, not a cache: recomputed on every call. SDCC otherwise spills
     * the footprint across a large IX stack frame. Collision is non-reentrant. */
    static uint16_t left,top;
    static uint8_t ox,oy,xfirst,xlast,yfirst,ylast;
    static const uint8_t* cell;
    left=(uint16_t)((uint16_t)x-TANK_RADIUS);
    top=(uint16_t)((uint16_t)y-TANK_RADIUS);
    if(left>WORLD_W-TANK_DIAMETER || top>WORLD_H-TANK_DIAMETER)return 0;
    timing_sample();
    ox=(uint8_t)left&31;oy=(uint8_t)top&31;
    xfirst=ox<16 ? 15 : 10;xlast=ox<=16 ? 5 : 15;
    yfirst=oy<16 ? 15 : 12;ylast=oy<=16 ? 3 : 15;
    cell=arena_cells+(uint16_t)(top>>5)*ARENA_W+(left>>5);
    if(blocked(cell[0],xfirst&yfirst))return 0;
    if(ox && blocked(cell[1],xlast&yfirst))return 0;
    if(oy) {
        if(blocked(cell[ARENA_W],xfirst&ylast))return 0;
        if(ox && blocked(cell[ARENA_W+1],xlast&ylast))return 0;
    }
    return 1;
}

void arena_damage(int16_t x,int16_t y,uint8_t heading)
{
    uint16_t cell=((uint16_t)y>>5)*ARENA_W+((uint16_t)x>>5);
    uint8_t mask;
    if(arena_cells[cell]<16 || arena_cells[cell]>31) return;
    arena_dirty=1;
    mask=(heading==4 || heading==12) ? (y%32<16 ? 3 : 12) : (x%32<16 ? 5 : 10);
    arena_cells[cell]&=(uint8_t)~mask;
    if(!(arena_cells[cell]&15)){arena_cells[cell]=TILE_FLOOR;ai_invalidate();}
}
