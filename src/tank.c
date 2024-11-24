#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_keyboard.h>
#include <zos_time.h>
#include <zvb_gfx.h>
#include <zvb_hardware.h>
#include "controller.h"
#include "keyboard.h"
#include "tank.h"

#define TILE_TRANSPARENT    0x1F
#define BACKGROUND_TILE     0x11
#define PLAYER_TILE         0x03
#define WIDTH               20
#define HEIGHT              15
#define TILE_SIZE           16

gfx_context vctx;
gfx_sprite player;
static uint8_t controller_mode = 1;
static uint8_t frames = 0;
static uint16_t buttons = 0;

int main(void) {
    init();

    while (1) {
        gfx_wait_vblank(&vctx);
        frames++;
        input();
        if(frames == 18) {
            frames = 0;
            update();
            draw();
        }

        gfx_wait_end_vblank(&vctx);
    }
    // return 0; // unreachable
}

void init(void) {
    zos_err_t err = keyboard_init();
    if(err != ERR_SUCCESS) {
        printf("Failed to init keyboard: %d", err);
        exit(1);
    }
    err = controller_init();
    if(err != ERR_SUCCESS) {
        printf("Failed to init controller: %d", err);
    }
    // verify the controller is actually connected
    uint16_t test = controller_read();
    // if unconnected, we'll get back 0xFFFF (all buttons pressed)
    if(test & 0xFFFF) {
        controller_mode = 0;
    }

    /* Disable the screen to prevent artifacts from showing */
    gfx_enable_screen(0);

    err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_320_8BIT, &vctx);
    if (err) exit(1);

    // Load the palette
    extern uint8_t _tank_palette_end;
    extern uint8_t _tank_palette_start;
    const size_t palette_size = &_tank_palette_end - &_tank_palette_start;
    err = gfx_palette_load(&vctx, &_tank_palette_start, palette_size, 0);
    if (err) exit(1);

    // Load the tiles
    extern uint8_t _tank_tileset_end;
    extern uint8_t _tank_tileset_start;
    const size_t sprite_size = &_tank_tileset_end - &_tank_tileset_start;
    gfx_tileset_options options = {
        .compression = TILESET_COMP_NONE,
    };
    err = gfx_tileset_load(&vctx, &_tank_tileset_start, sprite_size, &options);
    if (err) exit(1);

    // Draw the tilemap
    load_tilemap();

    // Setup the player sprite
    player.flags = SPRITE_NONE;
    gfx_sprite_set_tile(&vctx, 0, PLAYER_TILE);
    player.tile = PLAYER_TILE;

    gfx_enable_screen(1);
}

void load_tilemap(void) {
    uint8_t line[WIDTH];

    // Load the tilemap
    extern uint8_t _tank_tilemap_start;
    for (uint16_t i = 0; i < HEIGHT; i++) {
        uint16_t offset = i * WIDTH;
        memcpy(&line, &_tank_tilemap_start + offset, WIDTH);
        gfx_tilemap_load(&vctx, line, WIDTH, 0, 0, i);
    }
}

void input(void) {
    buttons = keyboard_read();
    if(controller_mode == 1) {
        buttons |= controller_read();
    }
}

uint16_t get_tile(uint8_t varx, uint8_t vary) {

          //20x15 map
        static uint16_t map[300] = {
        1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,0,1,
        1,0,0,1,0,0,0,0,0,1,0,0,1,1,1,1,1,1,1,1,
        1,0,0,1,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,
        1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,
        0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,
        1,1,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,0,0,1,
        1,1,0,1,0,0,0,0,0,1,1,1,1,0,0,1,1,1,1,1,
        0,0,0,1,0,1,1,1,1,1,0,0,0,0,0,1,0,1,0,1,
        1,1,0,1,0,1,0,0,0,1,1,1,1,0,0,1,0,1,0,1,
        1,1,1,1,0,1,0,0,0,1,0,0,1,0,0,1,0,1,0,1,
        1,0,0,1,0,1,0,0,0,1,0,0,1,1,1,1,0,1,0,1,
        1,0,0,1,0,1,1,1,1,1,1,1,1,0,0,1,0,1,0,1,
        1,0,0,1,0,1,0,0,0,0,0,0,1,0,0,1,0,1,0,1,
        1,1,1,1,1,1,1,1,1,0,2,0,1,1,1,1,1,1,1,1
        };

    return map[(vary * WIDTH) + varx];
}

void update(void) {
    static uint16_t x = 16;
    static uint16_t y = 16;

    static int8_t xd = 3;
    static int8_t yd = 3;
    int8_t tile_x = (x >> 4)-1; 
    int8_t tile_y = (y >> 4)-1;  
 
    int8_t h_direction = 0;
    int8_t v_direction = 0;

    if(buttons & SNES_RIGHT) {
        h_direction = 1;   
        //x += xd;
    }

    else if (buttons & SNES_LEFT) {
        h_direction =-1;
        //x -= xd;
    }

    else {
        h_direction = 0;  // No horizontal movement
    }

    if (buttons & SNES_UP) {
        v_direction = -1;
        //y -= yd;
    }
    
    else if (buttons & SNES_DOWN) {
        v_direction = 1;
        //y += yd;
    }
    
    else {
        v_direction = 0;  // No vertical movement
    }



    if (h_direction) x += h_direction*xd;
    else if (v_direction) y += v_direction*yd;

    if(x > 320) x = 320;
    if(x < 16) x = 16;
    if(y < 16) y = 16;
    if(y > 240) y = 240;

    player.x = x;
    player.y = y;
}

void draw(void) {
    uint8_t err = gfx_sprite_render(&vctx, 0, &player);
    if(err != 0) {
        printf("graphics error: %d", err);
        exit(1);
    }
}

void _tank_palette(void) {
    __asm__(
    "__tank_palette_start:\n"
    "    .incbin \"assets/tank.ztp\"\n"
    "__tank_palette_end:\n"
    );
}

void _tank_tileset(void) {
    __asm__(
    "__tank_tileset_start:\n"
    "    .incbin \"assets/tank.zts\"\n"
    "__tank_tileset_end:\n"
    );
}

void _tank_tilemap(void) {
    __asm__(
    "__tank_tilemap_start:\n"
    "    .incbin \"assets/tank.ztm\"\n"
    "__tank_tilemap_end:\n"
    );
}