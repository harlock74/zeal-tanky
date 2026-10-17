/* Fixed splash actors, prompt and headquarters settings. Tile artwork stays in assets/splash.tmx. */
static const int16_t splash_actors[3][5]={{224,368,12,48,0},{80,368,0,80,15},{560,368,8,64,15}};
#define SPLASH_FLASH_X 10
#define SPLASH_FLASH_Y 26
#define SPLASH_FLASH_LENGTH 19
static const uint8_t splash_flash_on[]={95,97,84,98,98,15,98,99,80,97,99,15,99,94,15,95,91,80,104};
static const uint8_t splash_flash_off[]={8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};

#define SPLASH_HQ_X 19
#define SPLASH_HQ_Y 22
extern const uint8_t tank_splash_map[], tank_splash_end[];
