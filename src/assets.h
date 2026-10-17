#ifndef ZTANK_ASSETS_H
#define ZTANK_ASSETS_H
#include <zvb_gfx.h>
gfx_error assets_load(gfx_context* ctx);
uint8_t assets_pixel(uint8_t tile,uint8_t x,uint8_t y);
gfx_error assets_scale_tile(gfx_context* ctx,uint8_t source,uint16_t destination);
#endif
