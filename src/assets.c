#include <stdint.h>
#include "assets.h"
#include "config.h"
#include "tiles.generated.h"
#ifndef HOST_CHECK
extern uint8_t tiles_start,tiles_end,palette_start,palette_end;
#else
extern uint8_t host_rle[32768],host_palette[32];
#define tiles_start host_rle[0]
#define tiles_end host_rle[TILE_PAYLOAD_SIZE]
#define palette_start host_palette[0]
#define palette_end host_palette[32]
#endif
static uint8_t tile_cache[128],cached_tile=255;
static uint8_t decode_tile(uint8_t tile)
{
    uint16_t p,end;uint8_t count,control;uint16_t written=0;
    if(tile==cached_tile) return 1;
    cached_tile=255; /* A failed decode must invalidate the partially written cache. */
    if(tile>=128) return 0;
    p=tile_offsets[tile];end=tile_offsets[tile+1];
    if(p>=end)return 0;
    control=(&tiles_start)[p++];
    if(control==1) {
        uint8_t colors,low,high,bits,n;
        if(end-p!=33)return 0;
        colors=(&tiles_start)[p++];low=colors>>4;high=colors&15;
        while(p<end) {
            bits=(&tiles_start)[p++];
            for(n=0;n<4;n++) {
                uint8_t a=(bits&128) ? high : low;bits<<=1;
                tile_cache[written++]=(a<<4)|((bits&128) ? high : low);bits<<=1;
            }
        }
        cached_tile=tile;return 1;
    }
    if(control!=0)return 0;
    while(p<end && written<128) {
        control=(&tiles_start)[p++];count=(control&127)+1;
        if(written+count>128) return 0;
        if(control&128) {
            uint8_t value;if(p>=end) return 0;value=(&tiles_start)[p++];
            while(count--) tile_cache[written++]=value;
        } else {
            if(p+count>end) return 0;
            while(count--) tile_cache[written++]=(&tiles_start)[p++];
        }
    }
    if(written!=128 || p!=end) return 0;
    cached_tile=tile;return 1;
}
uint8_t assets_pixel(uint8_t tile,uint8_t x,uint8_t y)
{
    uint8_t value;
    if(x>=16 || y>=16 || !decode_tile(tile)) return 0;
    value=tile_cache[y*8+x/2];
    return (value>>(4*((x&1)^1)))&15;
}
gfx_error assets_scale_tile(gfx_context* ctx,uint8_t source,uint16_t destination)
{
    uint8_t pixels[128],q,x,y;
    if(!decode_tile(source))return 1;
    for(q=0;q<4;q++) {
        gfx_tileset_options options={.compression=TILESET_COMP_NONE,.from_byte=(destination+q)*128};
        for(y=0;y<16;y++) for(x=0;x<16;x+=2) {
            uint8_t sx=(q&1)*8+x/2,sy=(q/2)*8+y/2;
            uint8_t packed=tile_cache[sy*8+sx/2];
            uint8_t color=(packed>>(4*((sx&1)^1)))&15;
            pixels[y*8+x/2]=(color<<4)|color;
        }
        {
            gfx_error err=gfx_tileset_load(ctx,pixels,sizeof(pixels),&options);
            if(err!=GFX_SUCCESS) return err;
        }
    }
    return GFX_SUCCESS;
}
gfx_error assets_load(gfx_context* ctx)
{
    uint8_t i,other_palette[32];
    gfx_tileset_options o={.compression=TILESET_COMP_NONE};
    gfx_error e=gfx_palette_load(ctx,&palette_start,&palette_end-&palette_start,0);
    if(e!=GFX_SUCCESS) return e;
    /* In 4bpp, background color zero is opaque. Match it to the black
     * panel so glyph backgrounds blend while sprite zero remains alpha. */
    e=gfx_palette_load(ctx,(&palette_start)+18,2,0);if(e!=GFX_SUCCESS) return e;
    for(i=0;i<16;i++) {
        uint8_t color=i==4 ? 2 : i==5 ? 13 : i==6 ? 11 : i==8 ? 14 : i;
        other_palette[i*2]=(&palette_start)[color*2];other_palette[i*2+1]=(&palette_start)[color*2+1];
    }
    other_palette[0]=(&palette_start)[18];other_palette[1]=(&palette_start)[19];
    e=gfx_palette_load(ctx,other_palette,32,16);if(e!=GFX_SUCCESS) return e;
    cached_tile=255;
    for(i=0;i<128;i++) {
        if(tile_offsets[i]==tile_offsets[i+1])continue;
        if(!decode_tile(i)) return 1;
        o.from_byte=(uint16_t)i*128;
        e=gfx_tileset_load(ctx,tile_cache,128,&o);if(e!=GFX_SUCCESS) return e;
    }
    /* Resident geometry, separate palette banks: no gameplay art uploads. */
    for(i=0;i<8;i++) {
        uint8_t frame=(i/2)*8+(i&1);
        e=assets_scale_tile(ctx,16+frame,TANK_VIDEO_TILE+(uint16_t)frame*4);if(e!=GFX_SUCCESS) return e;
    }
    for(i=0;i<6;i++) {e=assets_scale_tile(ctx,118+i,136+i*4);if(e!=GFX_SUCCESS) return e;}
    e=assets_scale_tile(ctx,6,160);if(e!=GFX_SUCCESS) return e;
    return GFX_SUCCESS;
}
#ifndef HOST_CHECK
void embedded_assets(void) __naked
{
    __asm__("_tiles_start:\n.incbin \"generated/tiles.rle\"\n_tiles_end:\n"
        "_palette_start:\n.incbin \"assets/tiles.ztp\"\n_palette_end:\n");
}
#endif
