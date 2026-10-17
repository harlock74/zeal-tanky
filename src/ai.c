#include <string.h>
#include "ai.h"
#include "arena.h"
#include "gameplay.h"
#include "physics.h"
#include "timing.h"
#define FLOW_INF 0x3fff
#define FLOW_N (ARENA_W*ARENA_H)
static uint16_t distances[FLOW_N],working[FLOW_N],rng;
static uint8_t queue[256],queued[(FLOW_N+7)/8],head,tail,busy,pending,ready;
static const int8_t steps[4]={1,ARENA_W,-1,-ARENA_W};
static const uint8_t commands[4]={CMD_RIGHT,CMD_DOWN,CMD_LEFT,CMD_UP};
static uint8_t random_byte(void){rng^=rng<<7;rng^=rng>>9;rng^=rng<<8;return (uint8_t)rng;}
static uint8_t neighbour(uint8_t cell,uint8_t dir)
{
    if((dir==0 && cell%ARENA_W==ARENA_W-1) || (dir==2 && cell%ARENA_W==0) ||
        (dir==1 && cell>=FLOW_N-ARENA_W) || (dir==3 && cell<ARENA_W))return 255;
    return cell+steps[dir];
}
static void enqueue(uint8_t cell)
{
    uint8_t mask=1u<<(cell&7);
    if(!(queued[cell>>3]&mask)){queued[cell>>3]|=mask;queue[tail++]=cell;}
}
void ai_invalidate(void){pending=1;}
void ai_reset(void)
{
    rng=0x6d2b+(uint16_t)match.stage*257;busy=ready=0;pending=1;
}
static void start_flow(void)
{
    uint8_t i,goal=(arena_spawn_y(3)/CELL_SIZE)*ARENA_W+arena_spawn_x(3)/CELL_SIZE;
    for(i=0;i<FLOW_N;i++){if(!(i&7))timing_sample();working[i]=FLOW_INF;}
    memset(queued,0,sizeof(queued));head=tail=0;busy=1;pending=0;
    working[goal]=0;enqueue(goal);
}
void ai_tick(void)
{
    uint8_t budget=4;
    if(!busy && pending)start_flow();
    while(busy && budget--) {
        uint8_t current,n;
        if(head==tail){timing_sample();memcpy(distances,working,sizeof(distances));timing_sample();busy=0;ready=1;break;}
        current=queue[head++];queued[current>>3]&=(uint8_t)~(1u<<(current&7));
        for(n=0;n<4;n++) {
            uint8_t next=neighbour(current,n),tile;uint16_t cost;
            timing_sample();
            if(next==255 || arena_cells[next]==TILE_WALL)continue;
            tile=arena_cells[next];cost=working[current]+1;
            if((tile>=16 && tile<=31) || tile==TILE_BRICK)cost+=4;
            if(cost<working[next]){working[next]=cost;enqueue(next);}
        }
    }
}
void ai_prepare(void)
{
    /* Full first build happens while the stage is hidden. Later changes are
     * rebuilt in bounded slices, retaining the last completed field. */
    while(pending || busy)ai_tick();
}
uint8_t ai_clear_line(int16_t x,int16_t y,int16_t tx,int16_t ty)
{
    int16_t dx=tx-x,dy=ty-y;
    /* Shots are cardinal. Visit each crossed 16px terrain quadrant once;
     * a pixel-by-pixel ray for each enemy used most of a frame on Z80. */
    if(!dx || !dy) {
        int16_t first=(!dx ? y : x)/16,last=(!dx ? ty : tx)/16;
        int8_t step=first<=last ? 1 : -1;
        while(1) {
            timing_sample();
            uint8_t tile=arena_at(!dx ? x : first*16+8,!dx ? first*16+8 : y);
            if(tile==TILE_WALL || tile==TILE_BRICK)return 0;
            if(first==last)return 1;
            first+=step;
        }
    }
    return 0; /* Cardinal weapons cannot use a diagonal sight line. */
}
static uint8_t occupied(uint8_t id,int16_t x,int16_t y)
{
    uint8_t i;tank_state* other=match.tanks;
    int16_t qx=x*16,qy=y*16;
    for(i=0;i<TANK_COUNT;i++,other++)if(i!=id && other->active &&
        (uint16_t)(qx-other->x+TANK_DIAMETER*16-1)<TANK_DIAMETER*32-1 &&
        (uint16_t)(qy-other->y+TANK_DIAMETER*16-1)<TANK_DIAMETER*32-1)return 1;
    return 0;
}
uint8_t ai_controls(uint8_t id)
{
    tank_state* t=&match.tanks[id];uint8_t fire=0,centre,aim=255,front;
    int16_t x=(uint16_t)t->x>>4,y=(uint16_t)t->y>>4;
    uint8_t cell=((uint16_t)y>>5)*ARENA_W+((uint16_t)x>>5);
    if(t->invulnerable)return 0;
    if(t->fire_wait)t->fire_wait--;
    if(t->plan_wait)t->plan_wait--;
    if(t->moving)t->stuck=0;else if(t->stuck<255)t->stuck++;
    centre=(t->x&511)==256 && (t->y&511)==256;
    if(centre && !t->plan_wait) {
        uint8_t n,best=t->heading/4,start=(id+random_byte())&3;
        uint16_t minimum=65535;
        for(n=0;n<4;n++) {
            uint8_t dir=(n+start)&3,next=neighbour(cell,dir),tile;
            int16_t nx=x+(dir==0 ? 32 : dir==2 ? -32 : 0),ny=y+(dir==1 ? 32 : dir==3 ? -32 : 0);
            uint16_t score;
            timing_sample();
            if(next==255 || arena_cells[next]==TILE_WALL)continue;
            tile=arena_cells[next];score=ready ? distances[next] : FLOW_INF;
            if(score==FLOW_INF)continue;
            if(dir==((t->heading/4+2)&3))score+=3;
            if(occupied(id,nx,ny))score+=16;
            /* Do not drill forever through the already-cleared half of a
             * brick. Prefer another approach to the remaining quadrants. */
            if(tile>=16 && tile<=31 && !(tile&((dir&1) ? 10 : 12)))score+=32;
            if(score<minimum || (score==minimum && dir==t->heading/4)){minimum=score;best=dir;}
        }
        t->heading=best*4;t->plan_wait=12;
    }
    /* Back out of traffic or a dead end, rather than rotate off-grid and
     * scrape through a wall. Full-footprint physics still checks every step. */
    if(!centre && t->stuck>=24){t->heading=(t->heading+8)&15;t->stuck=0;t->plan_wait=12;}
    if(match.tanks[0].active && !match.tanks[0].invulnerable &&
        (centre || ((match.elapsed+id)&3)==0)) {
        int16_t px=(uint16_t)match.tanks[0].x>>4,py=(uint16_t)match.tanks[0].y>>4;
        int16_t dx=px-x,dy=py-y;
        if(dx>-12 && dx<12 && dy && ai_clear_line(x,y,x,py))aim=dy>0 ? 4 : 12;
        else if(dy>-12 && dy<12 && dx && ai_clear_line(x,y,px,y))aim=dx>0 ? 0 : 8;
        if(aim!=255 && centre)t->heading=aim;
    }
    front=arena_at(x+(t->heading==0 ? 17 : t->heading==8 ? -17 : 0),
                   y+(t->heading==4 ? 17 : t->heading==12 ? -17 : 0));
    if(front==TILE_BRICK || front==TILE_HQ)t->stuck=0;
    if(!t->fire_wait && (!match.shells[1].active || !match.shells[2].active) &&
        (front==TILE_BRICK || front==TILE_HQ || aim==t->heading)) {
        uint16_t pressure=(match.stage-1)/8;
        uint8_t cooldown=match.difficulty==0 ? 110 : match.difficulty==1 ? 75 : 50;
        if(pressure>4)pressure=4;
        if(match.elapsed>1800 || match.destroyed>=16)cooldown-=15;
        cooldown-=pressure*5;t->fire_wait=cooldown+random_byte()%20;fire=CMD_FIRE;
    }
    return commands[t->heading/4]|fire;
}
