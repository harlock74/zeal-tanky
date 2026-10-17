#include <stdint.h>
#include <string.h>
#include <zos_keyboard.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zgdk/input.h>
#include "config.h"
#include "input.h"
#include "timing.h"
static uint8_t storage[127], *buffer, release_pending, keyboard_owner;
static uint8_t leases[7];
static uint16_t keys, previous, pad_previous, edges;
static uint8_t key_index(uint8_t key)
{
    switch(key) {
    case KB_UP_ARROW:return 0; case KB_DOWN_ARROW:return 1;
    case KB_LEFT_ARROW:return 2; case KB_RIGHT_ARROW:return 3;
    case KB_KEY_Z:return 4; case KB_KEY_SPACE:return 5;
    case KB_KEY_ENTER:return 6; case KB_RIGHT_SHIFT:return 7;
    case KB_KEY_P:return 8; case KB_KEY_X:return 9;
    default:return 255;
    }
}
zos_err_t tank_input_init(void)
{
    zos_err_t err;
    buffer=(uint8_t*)((uintptr_t)(storage+63)&~(uintptr_t)63);
    keys=previous=pad_previous=edges=0;release_pending=keyboard_owner=0;
    memset(leases,0,sizeof(leases));
    err=input_init(1);
    if(err!=ERR_SUCCESS) err=input_init(0);
    return err;
}
void tank_input_age(uint8_t frames)
{
    uint8_t i;
    for(i=0;i<7;i++)if(leases[i]) {
        if(leases[i]<=frames){leases[i]=0;keys&=(uint16_t)~(1u<<i);}
        else leases[i]-=frames;
    }
}
void tank_input_poll(input_frame* f)
{
    uint8_t batch,i; uint16_t pad, pressed;
    memset(f,0,sizeof(*f));edges=0;
    for(batch=0;batch<4;batch++) {
        timing_sample();
        uint16_t size=64;
        if(read(DEV_STDIN,buffer,&size)!=ERR_SUCCESS) {keys=0;release_pending=0;break;}
        if(!size) break;
        for(i=0;i<size;i++) {
            uint8_t n;uint16_t bit;
            timing_sample();
            if(buffer[i]==KB_RELEASED) {release_pending=1;continue;}
            n=key_index(buffer[i]);
            if(n!=255) {
                bit=1u<<n;
                if(n<7) {
                    leases[n]=release_pending ? 0 : (keys&bit) ? 8 : 45;
                    if(n<6) keyboard_owner=1;
                }
                if(release_pending) keys&=(uint16_t)~bit;
                else {if(!(keys&bit)) edges|=bit;keys|=bit;}
            }
            release_pending=0;
        }
    }
    pad=controller_read();
    if((pad&0xf000) || pad==0x0fff) pad=0;
    pressed=pad&~pad_previous;
    if(pressed&BUTTON_START) {
        keyboard_owner=0;keys&=0xffc0;
        for(i=0;i<6;i++) leases[i]=0;
    }
    pad_previous=pad;
    for(i=0;i<4;i++) {
        if(keys&(1u<<i)) f->controls[0]|=1u<<i;
    }
    if((keys|edges)&0x30) f->controls[0]|=CMD_FIRE;
    if(!keyboard_owner) {
        if(pad&BUTTON_UP) f->controls[0]|=CMD_FORWARD;
        if(pad&BUTTON_DOWN) f->controls[0]|=CMD_REVERSE;
        if(pad&BUTTON_LEFT) f->controls[0]|=CMD_LEFT;
        if(pad&BUTTON_RIGHT) f->controls[0]|=CMD_RIGHT;
        if(pad&BUTTON_B) f->controls[0]|=CMD_FIRE;
    }
    edges|=keys&~previous;previous=keys;
    f->start=(uint8_t)(!!(edges&(1u<<6)) || !!(pressed&BUTTON_START));
    f->quit=(uint8_t)(!!(edges&(1u<<7)) || !!(pressed&BUTTON_SELECT));
    f->pause=(uint8_t)(!!(edges&(1u<<8)) || !!(pressed&BUTTON_A));
    f->mute=(uint8_t)(!!(edges&(1u<<9)) || !!(pressed&BUTTON_X));
    f->left=(uint8_t)(!!(edges&4) || !!(pressed&BUTTON_LEFT));
    f->right=(uint8_t)(!!(edges&8) || !!(pressed&BUTTON_RIGHT));
    f->up=(uint8_t)(!!(edges&1) || !!(pressed&BUTTON_UP));
    f->down=(uint8_t)(!!(edges&2) || !!(pressed&BUTTON_DOWN));
    f->confirm=(uint8_t)(!!(edges&0x30) || !!(pressed&BUTTON_B) || f->start);
    f->accept_held=!!(keys&0x70) || !!(pad&(BUTTON_B|BUTTON_START));
}
