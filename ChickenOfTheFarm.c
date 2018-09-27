
// Controller bits
#define BUTTON_A 0x80
#define BUTTON_B 0x40
#define BUTTON_SELECT 0x20
#define BUTTON_START 0x10
#define BUTTON_UP 0x08
#define BUTTON_DOWN 0x04
#define BUTTON_LEFT 0x02
#define BUTTON_RIGHT 0x01

// Registers
#define PPU_CTRL    *((unsigned char*)0x2000)
#define PPU_MASK    *((unsigned char*)0x2001)
#define PPU_ADDRESS *((unsigned char*)0x2006)
#define PPU_DATA    *((unsigned char*)0x2007)

#pragma bss-name (push, "OAM")
unsigned char sprites[256];
#pragma bss-name (pop)

#pragma bss-name (push, "BSS")

static unsigned char gController1;
static unsigned char gPrevController1;
static unsigned char gPrevController1Change;
static unsigned char gX;
static unsigned char gY;
static unsigned long gXScroll;
static unsigned char gYScroll;
static unsigned char gYNametable;
static unsigned char devnull;
static unsigned int  i;
static unsigned int  gJumping; // 0 if not currently in the air from a jump, 1 if yes
static unsigned int  gBounceCounter;
static unsigned int  gVelocity;
static unsigned int  gVelocityDirection;
static unsigned int  gSpeed;
static unsigned int  gSpeedDirection;
static unsigned int  gStage;
static unsigned int  gCounter;
static unsigned int  gHealth;
static unsigned int  gIframes;

extern void pMusicInit(unsigned char);
extern void pMusicPlay(void);

void __fastcall__ UnRLE(const unsigned char *data);

#include "Background/Level0Bottom.h"
#include "Background/Level1Top.h"
#include "Background/Level1Bottom.h"
#include "Background/Level2Top.h"
#include "Background/Level2Bottom.h"

#pragma bss-name (pop)

#pragma data-name (push, "RODATA")

// 16 x (15 + 15)
// TODO should use a bit per block instead of byte, but this is a lot easier at the moment
unsigned char collision[480] = {
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,};


unsigned char frog[4] = {0x00,0x01,0x02,0x03,};
unsigned char bird[2] = {0x04,0x05,};

#pragma data-name (pop)

#pragma data-name ("CHARS")

unsigned char pattern[288] = {0x00,0x00,0x00,0x01,0x01,0x07,0x09,0x17,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x08, // frog
                              0x60,0xD0,0x88,0x88,0x86,0x3E,0xF3,0x7C,0x00,0x20,0x50,0x50,0x78,0xC0,0x0C,0x80, // frog
                              0x2F,0x59,0x7E,0xF6,0xEC,0x9C,0x38,0x3F,0x10,0x20,0x00,0x00,0x00,0x01,0x00,0x00, // frog
                              0x60,0xC0,0xC0,0xC0,0xE6,0x7C,0x1E,0x80,0x80,0x00,0x18,0x10,0x00,0x00,0x00,0x00, // frog
                              0x18,0x2C,0xF9,0x03,0x03,0x01,0x01,0x00,0x00,0x00,0xE6,0x04,0x00,0x00,0x00,0x00, // bird
                              0x00,0x00,0xF0,0xFE,0xFF,0xDF,0xC0,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x0E, // bird
                              0x19,0x04,0x70,0x00,0x61,0x86,0x22,0x47,0x19,0x07,0x79,0x1D,0x66,0xB1,0x29,0x40, // portal
                              0x00,0x00,0x30,0x21,0x03,0x06,0x1E,0x18,0x18,0x7E,0x7E,0xFE,0xFC,0x78,0x60,0x00, // health
                              0xFF,0x00,0x78,0x40,0x5F,0x1F,0x3F,0x00,0x00,0xFF,0xFF,0xFF,0xE0,0xE0,0xC0,0xFF,
                              0xFF,0x03,0xFF,0x0F,0xEF,0xE3,0x8F,0x3F,0x00,0xFC,0x00,0xF0,0x10,0x1C,0x70,0xC0,
                              0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                              0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                              0x3C,0x7E,0x61,0x3C,0x1E,0x43,0x3F,0x1E,0x3C,0x42,0x40,0x3C,0x02,0x42,0x3C,0x00, // S
                              0xFE,0x3F,0x18,0x18,0x18,0x18,0x18,0x08,0xFE,0x10,0x10,0x10,0x10,0x10,0x10,0x00, // T
                              0x18,0x2C,0x52,0x63,0x7F,0x7F,0x63,0x21,0x18,0x24,0x42,0x42,0x7E,0x42,0x42,0x00, // A
                              0x7C,0x7E,0x63,0x7D,0x6E,0x64,0x62,0x21,0x7C,0x42,0x42,0x7C,0x48,0x44,0x42,0x00, // R
                              };

void vblank(void)
{
    while((*((unsigned char*)0x2002) & 0x80) != 0x00);
    while((*((unsigned char*)0x2002) & 0x80) != 0x80);
}

void vblank_counter(void)
{
    do
    {
        vblank();
    }
    while( --gCounter != 0 );
}

void ppuinit(void)
{
    // from NESDev:
    // +---------+----------------------------------------------------------+
    // |  $2000  | PPU Control Register #1 (W)                              |
    // |         |                                                          |
    // |         |    D7: Execute NMI on VBlank                             |
    // |         |           0 = Disabled                                   |
    // |         |           1 = Enabled                                    |
    // |         |    D6: PPU Master/Slave Selection --+                    |
    // |         |           0 = Master                +-- UNUSED           |
    // |         |           1 = Slave               --+                    |
    // |         |    D5: Sprite Size                                       |
    // |         |           0 = 8x8                                        |
    // |         |           1 = 8x16                                       |
    // |         |    D4: Background Pattern Table Address                  |
    // |         |           0 = $0000 (VRAM)                               |
    // |         |           1 = $1000 (VRAM)                               |
    // |         |    D3: Sprite Pattern Table Address                      |
    // |         |           0 = $0000 (VRAM)                               |
    // |         |           1 = $1000 (VRAM)                               |
    // |         |    D2: PPU Address Increment                             |
    // |         |           0 = Increment by 1                             |
    // |         |           1 = Increment by 32                            |
    // |         | D1-D0: Name Table Address                                |
    // |         |         00 = $2000 (VRAM)                                |
    // |         |         01 = $2400 (VRAM)                                |
    // |         |         10 = $2800 (VRAM)                                |
    // |         |         11 = $2C00 (VRAM)                                |
    // +---------+----------------------------------------------------------+
    // +---------+----------------------------------------------------------+
    // |  $2001  | PPU Control Register #2 (W)                              |
    // |         |                                                          |
    // |         | D7-D5: Full Background Colour (when D0 == 1)             |
    // |         |         000 = None  +------------+                       |
    // |         |         001 = Green              | NOTE: Do not use more |
    // |         |         010 = Blue               |       than one type   |
    // |         |         100 = Red   +------------+                       |
    // |         | D7-D5: Colour Intensity (when D0 == 0)                   |
    // |         |         000 = None            +--+                       |
    // |         |         001 = Intensify green    | NOTE: Do not use more |
    // |         |         010 = Intensify blue     |       than one type   |
    // |         |         100 = Intensify red   +--+                       |
    // |         |    D4: Sprite Visibility                                 |
    // |         |           0 = sprites not displayed                      |
    // |         |           1 = sprites visible                            |
    // |         |    D3: Background Visibility                             |
    // |         |           0 = Background not displayed                   |
    // |         |           1 = Background visible                         |
    // |         |    D2: Sprite Clipping                                   |
    // |         |           0 = sprites invisible in left 8-pixel column   |
    // |         |           1 = No clipping                                |
    // |         |    D1: Background Clipping                               |
    // |         |           0 = BG invisible in left 8-pixel column        |
    // |         |           1 = No clipping                                |
    // |         |    D0: Display Type                                      |
    // |         |           0 = Colour display                             |
    // |         |           1 = Monochrome display                         |
    // +---------+----------------------------------------------------------+

    // Set PPU registers
    PPU_CTRL = 0x84;
    PPU_MASK = 0x0E;
}

void ppudisable(void)
{
    // Disable graphics
    PPU_CTRL = 0x00;
    PPU_MASK = 0x00;
}

void apuinit(void)
{
    // Initialize the APU to a known basic state
    *((unsigned char*)0x4015) = 0x0F;

    *((unsigned char*)0x4000) = 0x30;
    *((unsigned char*)0x4001) = 0x08;
    *((unsigned char*)0x4002) = 0x00;
    *((unsigned char*)0x4003) = 0x00;
    *((unsigned char*)0x4004) = 0x30;
    *((unsigned char*)0x4005) = 0x08;
    *((unsigned char*)0x4006) = 0x00;
    *((unsigned char*)0x4007) = 0x00;
    *((unsigned char*)0x4008) = 0x80;
    *((unsigned char*)0x4009) = 0x00;
    *((unsigned char*)0x400A) = 0x00;
    *((unsigned char*)0x400B) = 0x00;
    *((unsigned char*)0x400C) = 0x30;
    *((unsigned char*)0x400D) = 0x00;
    *((unsigned char*)0x400E) = 0x00;
    *((unsigned char*)0x400F) = 0x00;
    *((unsigned char*)0x4010) = 0x00;
    *((unsigned char*)0x4011) = 0x00;
    *((unsigned char*)0x4012) = 0x00;
    *((unsigned char*)0x4013) = 0x00;
    *((unsigned char*)0x4014) = 0x00;
    *((unsigned char*)0x4015) = 0x0F;
    *((unsigned char*)0x4016) = 0x00;
    *((unsigned char*)0x4017) = 0x40;
}

void set_scroll(void)
{
    // read 0x2002 to reset the address toggle
    devnull = *((unsigned char*)0x2002);
    // scroll a number of pixels in the X direction
    *((unsigned char*)0x2005) = gXScroll;
    // scroll a number of pixels in the Y direction
    *((unsigned char*)0x2005) = gYScroll;
}


void palattes(void)
{
    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x00;
    PPU_DATA =    0x0E;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x01;
    PPU_DATA    = 0x00;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x02;
    PPU_DATA    = 0x10;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x30;

    // Background 1
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x04;
    PPU_DATA =    0x0E;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x05;
    PPU_DATA    = 0x00;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x06;
    PPU_DATA    = 0x10;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x07;
    PPU_DATA    = 0x20;


    // Sprite 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x11;
    PPU_DATA    = 0x0A;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x12;
    PPU_DATA    = 0x1A;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x13;
    PPU_DATA    = 0x30;

    // Sprite 1
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x15;
    PPU_DATA    = 0x11;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x16;
    PPU_DATA    = 0x02;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x17;
    PPU_DATA    = 0x27;

    // Sprite 2
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x19;
    PPU_DATA    = 0x06;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x1A;
    PPU_DATA    = 0x16;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x1B;
    PPU_DATA    = 0x30;

    // Sprite 3
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x1D;
    PPU_DATA    = 0x16;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x1E;
    PPU_DATA    = 0x30;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x1F;
    PPU_DATA    = 0x30;
}

//void ppu_copy( unsigned char* addr, unsigned int len )
//{
//    for( i = 0; i < len; i++ )
//    {
//        PPU_ADDRESS = 0x20;
//        PPU_ADDRESS = i;
//        PPU_DATA = ((unsigned char*) addr)[i];
//    }
//}

void fade_out(void)
{
    PPU_MASK = 0x0E;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x00;
    PPU_DATA    = 0x0E;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x01;
    PPU_DATA    = 0x0E;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x02;
    PPU_DATA    = 0x00;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x10;

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x02;
    PPU_DATA    = 0x0E;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x00;

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x0E;

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    PPU_CTRL = 0x00;
    PPU_MASK = 0x00;
    set_scroll();
}

void fade_in(void)
{
    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x00;

    PPU_CTRL = 0x84 + gYNametable;
    PPU_MASK = 0x0E;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x02;
    PPU_DATA    = 0x00;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x10;

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    // Background 0
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x01;
    PPU_DATA    = 0x00;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x02;
    PPU_DATA    = 0x10;
    PPU_ADDRESS = 0x3F;
    PPU_ADDRESS = 0x03;
    PPU_DATA    = 0x30;

    PPU_CTRL = 0x84 + gYNametable;
    if( gStage != 0 )
    {
        PPU_MASK = 0x1E;
    }
    else
    {
        PPU_MASK = 0x0E;
    }
    set_scroll();

    gCounter = 20;
    vblank_counter();
}

void patterntables(void)
{
    //for( i = 0; i < 768; i++ )
    //{
    //    PPU_ADDRESS = 0x00;
    //    PPU_ADDRESS = 0;
    //    //PPU_DATA = j;//((unsigned char*) addr)[i];
    //    PPU_DATA = ((unsigned char*) pattern)[i];
    //    PPU_ADDRESS = 0x10;
    //    PPU_ADDRESS = 0;
    //    //PPU_DATA = j;//((unsigned char*) addr)[i];
    //    PPU_DATA = ((unsigned char*) pattern)[i];
    //}
    //PPU_ADDRESS = 0x00;
    //PPU_ADDRESS = 0;
    //for( i = 0; i < 1024; i++ )
    //{
    //    //PPU_DATA = pattern[i];
    //    PPU_DATA = i;//pattern[i];
    //}
}

void draw_health(void)
{
    for( i = 0; i < 8; i++)
    {
        sprites[40 + (i<<2)] = 0x0F + (i<<3) + i;
        sprites[41 + (i<<2)] = 0x07;
        if( gHealth > i )
        {
            sprites[42 + (i<<2)] = 0x02;
        }
        else
        {
            sprites[42 + (i<<2)] = 0x03;
        }
        sprites[43 + (i<<2)] = 0x10;
    }
}

void setup_sprites(void)
{
    unsigned int i;

    // clear sprite ram
    for( i = 0; i < 256; i++ )
    {
        sprites[i] = 0x00;
    }

    // From NESDev:
    //+------+----------+--------------------------------------+
    //+ Byte | Bits     | Description                          |
    //+------+----------+--------------------------------------+
    //|  0   | YYYYYYYY | Y Coordinate - 1. Consider the coor- |
    //|      |          | dinate the upper-left corner of the  |
    //|      |          | sprite itself.                       |
    //|  1   | IIIIIIII | Tile Index #                         |
    //|  2   | vhp000cc | Attributes                           |
    //|      |          |   v = Vertical Flip   (1=Flip)       |
    //|      |          |   h = Horizontal Flip (1=Flip)       |
    //|      |          |   p = Background Priority            |
    //|      |          |         0 = In front                 |
    //|      |          |         1 = Behind                   |
    //|      |          |   c = Upper two (2) bits of colour   |
    //|  3   | XXXXXXXX | X Coordinate (upper-left corner)     |
    //+------+----------+--------------------------------------+}

    // Frog
    sprites[0] = 0x50;
    sprites[1] = 0x00;
    sprites[2] = 0x00;
    sprites[3] = 0x50;

    sprites[4] = 0x50;
    sprites[5] = 0x01;
    sprites[6] = 0x00;
    sprites[7] = 0x58;

    sprites[8] = 0x58;
    sprites[9] = 0x02;
    sprites[10] = 0x00;
    sprites[11] = 0x50;

    sprites[12] = 0x58;
    sprites[13] = 0x03;
    sprites[14] = 0x00;
    sprites[15] = 0x58;

    gX = 0x10;
    gY = 0xD0;
    gVelocity = 0;
    gVelocityDirection = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gXScroll = 0;
    gYScroll = 0;
    gYNametable = 2;
    gJumping = 1;
    gBounceCounter = 0;
    gStage = 0;
    gHealth = 8;
    gIframes = 0;

    // bird
    sprites[16] = 0x30;
    sprites[17] = 0x04;
    sprites[18] = 0x02;
    sprites[19] = 0x50;

    sprites[20] = 0x30;
    sprites[21] = 0x05;
    sprites[22] = 0x02;
    sprites[23] = 0x58;

    draw_health();
}

void dma_sprites(void)
{
    *((unsigned char*)0x4013) = 0x00;
    *((unsigned char*)0x4014) = 0x02;
}

void input_poll(void)
{
    // Strobe 0x4016 bit 0 to gather input
    *((unsigned char*)0x4016) |= 0x01;
    *((unsigned char*)0x4016) &= 0xFE;

    gPrevController1 = gController1;
    gController1 = 0x00;

     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_A;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_B;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_SELECT;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_START;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_UP;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_DOWN;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_LEFT;
     }
     if(*((unsigned char*)0x4016) & 0x01)
     {
         gController1 |= BUTTON_RIGHT;
     }

     if( ((gController1 & BUTTON_A) == BUTTON_A && (gPrevController1 & BUTTON_A) == 0) ||
         ((gController1 & BUTTON_A) == 0 && (gPrevController1 & BUTTON_A) == BUTTON_A))
     {
         gPrevController1Change = gPrevController1;
     }
}

void small_jump(void)
{
    if(gJumping == 0) {
      gJumping = 1;
      gVelocity = 6;
      gVelocityDirection = 1;
    }
}

void big_jump(void)
{
    if(gJumping == 0) {
      gJumping = 1;
      gVelocity = 16;
      gVelocityDirection = 1;
      gPrevController1Change |= BUTTON_A;
    }
}

void update_sprites(void)
{
    if(gController1 & BUTTON_UP)
    {
    }
    if(gController1 & BUTTON_DOWN)
    {
    }
    if(gController1 & BUTTON_LEFT)
    {
        if( gSpeedDirection == 0 )
        {
            if( gSpeed < 12 )
            {
                ++gSpeed;
            }
        }
        else
        {
            if( gSpeed == 0 )
            {
                gSpeed = 1;
                gSpeedDirection = 0;

                sprites[1] = 0x01;
                sprites[2] = 0x40;
                sprites[5] = 0x00;
                sprites[6] = 0x40;
                sprites[9] = 0x03;
                sprites[10] = 0x40;
                sprites[13] = 0x02;
                sprites[14] = 0x40;
            }
            else
            {
                --gSpeed;
            }
        }

    }
    if(gController1 & BUTTON_RIGHT)
    {
        if( gSpeedDirection == 1 )
        {
            if( gSpeed < 12 )
            {
                ++gSpeed;
            }
        }
        else
        {
            if( gSpeed == 0 )
            {
                gSpeed = 1;
                gSpeedDirection = 1;

                sprites[1] = 0x00;
                sprites[2] = 0x00;
                sprites[5] = 0x01;
                sprites[6] = 0x00;
                sprites[9] = 0x02;
                sprites[10] = 0x00;
                sprites[13] = 0x03;
                sprites[14] = 0x00;
            }
            else
            {
                --gSpeed;
            }
        }
    }
    if( ((gController1 & (BUTTON_RIGHT | BUTTON_LEFT)) == 0) && (gSpeed > 0) )
    {
        --gSpeed;
    }
    if(gController1 & BUTTON_B)
    {
      // Currently does nothing
    }
    // Only if new press (else you could just hold jump to jump over and over)
    if(((gController1 & BUTTON_A) > 0) && ((gPrevController1Change & BUTTON_A) == 0))
    {
        big_jump();
    }
    if(!((gController1 & BUTTON_A) || (gController1 & BUTTON_B)))
    {
        // Stop making the sound
        //*((unsigned char*)0x4000) = 0x30;
    }

    sprites[0] = gY;
    sprites[3] = gX;
    sprites[4] = gY;
    sprites[7] = gX+8;
    sprites[8] = gY+8;
    sprites[11] = gX;
    sprites[12] = gY+8;
    sprites[15] = gX+8;

    if( gIframes > 0 )
    {
        sprites[2]  ^= 2;
        sprites[6]  ^= 2;
        sprites[10] ^= 2;
        sprites[14] ^= 2;
    }
    else
    {
        sprites[2]  &= 0xFC;
        sprites[6]  &= 0xFC;
        sprites[10] &= 0xFC;
        sprites[14] &= 0xFC;
    }
}

void load_stage(void)
{
    fade_out();

    if( gStage == 0 )
    {
        PPU_ADDRESS = 0x28; // address of nametable #2
        PPU_ADDRESS = 0x00;
        UnRLE(Level0Bottom);	// uncompresses our data
    }
    if( gStage == 1 )
    {
        PPU_ADDRESS = 0x28; // address of nametable #2
        PPU_ADDRESS = 0x00;
        UnRLE(Level1Bottom);	// uncompresses our data
        PPU_ADDRESS = 0x20; // address of nametable #2
        PPU_ADDRESS = 0x00;
        UnRLE(Level1Top);	// uncompresses our data
    }
    if( gStage == 2 )
    {
        PPU_ADDRESS = 0x28; // address of nametable #2
        PPU_ADDRESS = 0x00;
        UnRLE(Level2Bottom);	// uncompresses our data
        PPU_ADDRESS = 0x20; // address of nametable #2
        PPU_ADDRESS = 0x00;
        UnRLE(Level2Top);	// uncompresses our data
    }

    vblank();

    gX = 0x10;
    gY = 0xD0;
    gYNametable = 2;
    gVelocity = 0;
    gVelocityDirection = 0;
    gXScroll = 0;
    gYScroll = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gJumping = 1;
    gBounceCounter = 0;

    // Frog direction
    sprites[1] = 0x00;
    sprites[2] = 0x00;
    sprites[5] = 0x01;
    sprites[6] = 0x00;
    sprites[9] = 0x02;
    sprites[10] = 0x00;
    sprites[13] = 0x03;
    sprites[14] = 0x00;

    // Portal
    sprites[24] = 0x00;
    sprites[25] = 0x00;
    sprites[26] = 0x00;
    sprites[27] = 0x00;

    sprites[28] = 0x00;
    sprites[29] = 0x00;
    sprites[30] = 0x00;
    sprites[31] = 0x00;

    sprites[32] = 0x00;
    sprites[33] = 0x00;
    sprites[34] = 0x00;
    sprites[35] = 0x00;

    sprites[36] = 0x00;
    sprites[37] = 0x00;
    sprites[38] = 0x00;
    sprites[39] = 0x00;

    update_sprites();
    dma_sprites();
    set_scroll();

    fade_in();
}

void do_physics(void)
{
    if( (gController1 & BUTTON_START) != 0 )
    {
        gStage = 1;
        load_stage();
    }

    // 0-15 Frog 4 sprites
    // 16-23 Bird
    if( sprites[16] != gY )
    {
        if( sprites[16] < gY )
        {
            sprites[16] += 1;
            sprites[20] += 1;
        }
        else
        {
            sprites[16] -= 1;
            sprites[20] -= 1;
        }
    }
    if( sprites[19] != gX )
    {
        if( sprites[19] < gX )
        {
            // X
            sprites[19] += 1;
            sprites[23] += 1;
            // Tiles
            sprites[21] = 0x04;
            sprites[22] = 0x41;
            sprites[17] = 0x05;
            sprites[18] = 0x41;
        }
        else
        {
            // X
            sprites[19] -= 1;
            sprites[23] -= 1;
            // Tiles
            sprites[21] = 0x05;
            sprites[22] = 0x01;
            sprites[17] = 0x04;
            sprites[18] = 0x01;
        }
    }


    //
    // Horizontal Movement
    //

    if( gSpeedDirection == 0 )
    {
        ////TODO better checks to verify direction changed
        //if((gPrevController1 & BUTTON_LEFT) == 0) {
        //  sprites[1] = 0x01;
        //  sprites[2] = 0x40;
        //  sprites[5] = 0x00;
        //  sprites[6] = 0x40;
        //  sprites[9] = 0x03;
        //  sprites[10] = 0x40;
        //  sprites[13] = 0x02;
        //  sprites[14] = 0x40;
        //}

        for( i = 0; (i<<2) < gSpeed; i++ )
        {
            if(gX > 0x08)
            {

                if( gYNametable == 2 )
                {
                  //x = gX >> 4;
                  //y = gY >> 4
                  //index[y*16 + x]
                    if( collision[240 + (((gY+1)&0xF0)) + ((gX-1) >> 4)] == 0 &&
                        collision[240 + (((gY+0x10)&0xF0)) + ((gX-1) >> 4)] == 0 )
                    {
                        gX -= 1;
                        small_jump();
                    }
                    else
                    {
                        gSpeed = 0;
                    }
                }
                else
                {
                    if((gYScroll + gY + 1) >= 0xF0)
                    {
                        if( collision[240 + (((gYScroll + gY + 1 - 0xF0) & 0xF0) ) + ((gX-1) >> 4)] == 0 &&
                            collision[240 + (((gYScroll + gY + 0x10 - 0xF0) & 0xF0) ) + ((gX-1) >> 4)] == 0 )
                        {
                            gX -= 1;
                            small_jump();
                        }
                        else
                        {
                            gSpeed = 0;
                        }
                    }
                    else
                    {
                        if( collision[(((gYScroll + gY + 1) & 0xF0) ) + ((gX-1) >> 4)] == 0 &&
                            collision[(((gYScroll + gY + 0x10) & 0xF0) ) + ((gX-1) >> 4)] == 0 )
                        {
                            gX -= 1;
                            small_jump();
                        }
                        else
                        {
                            gSpeed = 0;
                        }
                    }
                }
            }
        }
    }
    else
    {
        ////TODO better checks to verify direction changed
        //if((gPrevController1 & BUTTON_RIGHT) == 0) {
        //  sprites[1] = 0x00;
        //  sprites[2] = 0x00;
        //  sprites[5] = 0x01;
        //  sprites[6] = 0x00;
        //  sprites[9] = 0x02;
        //  sprites[10] = 0x00;
        //  sprites[13] = 0x03;
        //  sprites[14] = 0x00;
        //}

        for( i = 0; (i<<2) < gSpeed; i++ )
        {
            if(gX < 0xE8)
            {
                if( gYNametable == 2 )
                {
                    if( collision[240 + (((gY+1)&0xF0) ) + ((gX+0x10) >> 4)] == 0 &&
                        collision[240 + (((gY+0x10)&0xF0) ) + ((gX+0x10) >> 4)] == 0 )
                    {
                        gX += 1;
                        small_jump();
                    }
                    else
                    {
                        gSpeed = 0;
                    }
                }
                else
                {
                    if((gYScroll + gY + 1) >= 0xF0)
                    {
                        if( collision[240 + (((gYScroll + gY + 1 - 0xF0) & 0xF0) ) + ((gX+0x10) >> 4)] == 0 &&
                            collision[240 + (((gYScroll + gY + 0x10 - 0xF0) & 0xF0) ) + ((gX+0x10) >> 4)] == 0 )
                        {
                            gX += 1;
                            small_jump();
                        }
                        else
                        {
                            gSpeed = 0;
                        }
                    }
                    else
                    {
                        if( collision[(((gYScroll + gY + 1) & 0xF0) ) + ((gX+0x10) >> 4)] == 0 &&
                            collision[(((gYScroll + gY + 0x10) & 0xF0) ) + ((gX+0x10) >> 4)] == 0 )
                        {
                            gX += 1;
                            small_jump();
                        }
                        else
                        {
                            gSpeed = 0;
                        }
                    }
                }
            }
        }
    }

    //
    // Vertical Movement
    //

    if( gVelocityDirection == 1 ) // moving up
    {
        for( i = 0; (i<<2) < gVelocity; i++ )
        {
            if( gYNametable == 2 )
            {
                if( collision[240 + (((gY)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gY)&0xF0) ) + ((gX+0xF) >> 4)] == 0 )
                {
                    if(gY > 0x0F)
                    {
                        gY -= 1;
                    }
                    else
                    {
                        gYScroll = 0xEF;
                        gYNametable = 0;
                    }
                }
                else
                {
                    gVelocity = 0;
                    gVelocityDirection = 0;
                    break;
                }
            }
            else if((gYScroll + gY) >= 0xF0 )
            {
                if( collision[240 + (((gYScroll + gY - 0xF0)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gYScroll + gY - 0xF0)&0xF0) ) + ((gX+0xF) >> 4)] == 0 )
                {
                    if(gY > 0x0F)
                    {
                        gY -= 1;
                    }
                    else
                    {
                        gYScroll -= 1;
                    }
                }
                else
                {
                    gVelocityDirection = 0;
                    gJumping = 0;
                    break;
                }
            }
            else
            {
                if( collision[(((gYScroll + gY - 0x100) & 0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[(((gYScroll + gY - 0x100) & 0xF0) ) + ((gX+0xF) >> 4)] == 0 )
                {
                    if(gY > 0x0F)
                    {
                        gY -= 1;
                    }
                    else
                    {
                        if( gYScroll > 0 )
                        {
                            gYScroll -= 1;
                        }
                    }
                }
                else
                {
                    gVelocity = 0;
                    gVelocityDirection = 0;
                    break;
                }
            }
        }
        if( i == ((gVelocity+3)>>2) )
        {
            if( gVelocity == 0 )
            {
                gVelocityDirection = 0;
            }
            else
            {
                gVelocity -= 1;
            }
        }
    }
    else // moving down
    {
        for( i = 0; (i<<2) < gVelocity; i++ )
        {
            if(gYNametable == 2 )
            {
                if( collision[240 + (((gY+0x11)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gY+0x11)&0xF0) ) + ((gX+0xF) >> 4)] == 0 )
                {
                    if(gY < 0xCF)
                    {
                        gY += 1;
                        gJumping = 1;
                    }
                    else
                    {
                        gVelocity = 0;
                        gJumping = 0;
                        break;
                    }
                }
                else
                {
                    gVelocity = 0;
                    gJumping = 0;
                    break;
                }
            }
            else if((gYScroll + gY + 0x11) >= 0xF0)
            {
                if( collision[240 + (((gYScroll + gY+0x11 - 0xF0)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gYScroll + gY+0x11 - 0xF0)&0xF0) ) + ((gX+0xF) >> 4)] == 0 )
                {
                    if(gY < 0xCF)
                    {
                        gY += 1;
                        gJumping = 1;
                    }
                    else
                    {
                        if( gYScroll == 0xEF )
                        {
                            gYNametable = 2;
                            gYScroll = 0;
                            gVelocity = 0;
                            gJumping = 0;
                            break;
                        }
                        else
                        {
                            gYScroll+=1;
                            gJumping = 1;
                        }
                    }
                }
                else
                {
                    gVelocity = 0;
                    gJumping = 0;
                    break;
                }
            }
            else
            {
                if( collision[(((gYScroll + gY+0x11)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[(((gYScroll + gY+0x11)&0xF0) ) + ((gX+0xF) >> 4)] == 0 )
                {
                    if(gY < 0xCF)
                    {
                        gY += 1;
                    }
                    else
                    {
                        gYScroll+=1;
                    }
                    gJumping = 1;
                }
                else
                {
                    gVelocity = 0;
                    gJumping = 0;
                    break;
                }
            }
        }

        if( i == ((gVelocity+3)>>2) )
        {
            gVelocity+=1;
        }
    }

    // Portal
    if( gYNametable == 0 && gYScroll < 0x28)
    {
        if( gYScroll < 0x20 )
        {
            sprites[24] = 0x0F - gYScroll;
            sprites[25] = 0x06;
            sprites[26] = 0x00;
            sprites[27] = 0xE0;

            sprites[28] = 0x0F - gYScroll;
            sprites[29] = 0x06;
            sprites[30] = 0x40;
            sprites[31] = 0xE8;
        }
        else
        {
            sprites[24] = 0x00;
            sprites[25] = 0x00;
            sprites[26] = 0x00;
            sprites[27] = 0x00;

            sprites[28] = 0x00;
            sprites[29] = 0x00;
            sprites[30] = 0x00;
            sprites[31] = 0x00;
        }

        sprites[32] = 0x17 - gYScroll;
        sprites[33] = 0x06;
        sprites[34] = 0x80;
        sprites[35] = 0xE0;

        sprites[36] = 0x17 - gYScroll;
        sprites[37] = 0x06;
        sprites[38] = 0xC0;
        sprites[39] = 0xE8;
    }
    else
    {
        sprites[24] = 0x00;
        sprites[25] = 0x00;
        sprites[26] = 0x00;
        sprites[27] = 0x00;

        sprites[28] = 0x00;
        sprites[29] = 0x00;
        sprites[30] = 0x00;
        sprites[31] = 0x00;

        sprites[32] = 0x00;
        sprites[33] = 0x00;
        sprites[34] = 0x00;
        sprites[35] = 0x00;

        sprites[36] = 0x00;
        sprites[37] = 0x00;
        sprites[38] = 0x00;
        sprites[39] = 0x00;
    }

    if( gYNametable == 0 && gY == 0x0F && gX == 0xE0 )
    {
        if( gStage == 1 )
        {
            gStage = 2;
            load_stage();
        }
    }

    if( sprites[16] == gY && sprites[19] == gX && gIframes == 0)
    {
        if( gHealth > 0 )
        {
            if( gStage != 0 )
            {
                --gHealth;
            }
            draw_health();
            if( gHealth == 0 )
            {
                gHealth = 8;
                gStage = 0;
                load_stage();
                draw_health();
            }
            else
            {
                gIframes = 120;
            }
        }
    }
    if( gIframes > 0 )
    {
        --gIframes;
    }
}



void main(void)
{
    gCounter = 5;
    vblank_counter();

    ppudisable();

    palattes();
    vblank();
    patterntables();

  	PPU_ADDRESS = 0x28; // address of nametable #2
  	PPU_ADDRESS = 0x00;
  	UnRLE(Level0Bottom);	// uncompresses our data


    vblank();
    setup_sprites();

    apuinit();

    pMusicInit(0x1);

    gCounter = 5;
    vblank_counter();

    gStage = 0;
    fade_in();

    while(1)
    {
        vblank();

        input_poll();

        update_sprites();

        dma_sprites();

        // set bits [1:0] to 0 for nametable
        PPU_CTRL = 0x84 + gYNametable;
        set_scroll();

        do_physics();
    }
}
