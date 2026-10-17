#ifndef ZTANK_RENDER_H
#define ZTANK_RENDER_H
#include <zvb_gfx.h>
extern gfx_context video;
void render_prepare_frame(void);void render_prepare_sprites(void);void render_present(void);
void render_resume(void);void render_arena(void);void render_frame(void);void render_hide_sprites(void);
void render_text(const char* text,uint8_t x,uint8_t y);
void render_number(uint16_t value,uint8_t x,uint8_t y);
void render_score(uint32_t value,uint8_t x,uint8_t y);
void render_center(const char* text,uint8_t y);
void render_clear_ui(void);void render_reset(void);
#endif
