#include "resources/resources.h"

//
// DEFINES
//

// Controller bits
#define BUTTON_A 0x80
#define BUTTON_B 0x40
#define BUTTON_SELECT 0x20
#define BUTTON_START 0x10
#define BUTTON_UP 0x08
#define BUTTON_DOWN 0x04
#define BUTTON_LEFT 0x02
#define BUTTON_RIGHT 0x01

// Palette addresses
#define BACKGROUND0_0            0x00
#define BACKGROUND0_1            0x01
#define BACKGROUND0_2            0x02
#define BACKGROUND0_3            0x03
#define BACKGROUND1_0            0x04
#define BACKGROUND1_1            0x05
#define BACKGROUND1_2            0x06
#define BACKGROUND1_3            0x07
#define BACKGROUND2_0            0x08
#define BACKGROUND2_1            0x09
#define BACKGROUND2_2            0x0A
#define BACKGROUND2_3            0x0B
#define BACKGROUND3_0            0x0C
#define BACKGROUND3_1            0x0D
#define BACKGROUND3_2            0x0E
#define BACKGROUND3_3            0x0F
#define SPRITE0_0                0x10
#define SPRITE0_1                0x11
#define SPRITE0_2                0x12
#define SPRITE0_3                0x13
#define SPRITE1_0                0x14
#define SPRITE1_1                0x15
#define SPRITE1_2                0x16
#define SPRITE1_3                0x17
#define SPRITE2_0                0x18
#define SPRITE2_1                0x19
#define SPRITE2_2                0x1A
#define SPRITE2_3                0x1B
#define SPRITE3_0                0x1C
#define SPRITE3_1                0x1D
#define SPRITE3_2                0x1E
#define SPRITE3_3                0x1F

// Registers
#define PPU_CTRL    *((unsigned char*)0x2000)
#define PPU_MASK    *((unsigned char*)0x2001)
#define PPU_ADDRESS *((unsigned char*)0x2006)
#define PPU_DATA    *((unsigned char*)0x2007)

// These are used to determine how much screen to show above and below frog while scrolling
#define MAX_BOTTOM_BUFFER 0xAF
#define MAX_TOP_BUFFER    0x3F

#define SET_COLOR( index, color )  PPU_ADDRESS = 0x3F; PPU_ADDRESS = index; PPU_DATA = color

//
// TYPEDEFS
//

typedef enum _GameState_t
{
    TITLE_SCREEN_STATE = 0,
    DEMO_STATE,
    GAME_RUNNING_STATE,
    ENDING_STATE,
} GameState_t;

typedef enum _FrogAnimationState_t
{
    FROG_NORMAL = 0,
    FROG_HOP,
    FROG_JUMP,
    FROG_SUSPENDED,
    FROG_FALLING,
    FROG_LANDING,
} FrogAnimationState_t;

typedef enum _TongueState_t
{
    TONGUE_NORMAL = 0,
    TONGUE_EXTENDING,
    TONGUE_EXTENDING2,
    TONGUE_OUT,
    TONGUE_RETRACTING,
    TONGUE_RETRACTING2,
}
TongueState_t;

//
// GLOBALS
//

#pragma bss-name (push, "OAM")
unsigned char sprites[256];


// 16 x (15 + 15)
// TODO should use a bit per block instead of byte, but this is a lot easier at the moment
unsigned char collision[480] = {};

#pragma bss-name (pop)

#pragma bss-name (push, "BSS")

static unsigned char        gController1;
static unsigned char        gPrevController1;
static unsigned char        gPrevController1Change;
static unsigned char        gTmpX;
static unsigned char        gX;
static unsigned char        gY;
static unsigned long        gXScroll;
static unsigned char        gYScroll;
static unsigned char        gYNametable;
static unsigned char        devnull;
static unsigned int         i;
static unsigned int         j;
static unsigned int         garbage;
// These are probably overkill, but it makes collision detection a lot cleaner
static unsigned int         x1;
static unsigned int         y1;
static unsigned int         width1;
static unsigned int         height1;
static unsigned int         x2;
static unsigned int         y2;
static unsigned int         width2;
static unsigned int         height2;
static unsigned int         gJumping; // 0 if not currently in the air from a jump, 1 if yes
static unsigned int         gBounceCounter;
static unsigned int         gVelocity;
static unsigned int         gVelocityDirection;
static unsigned int         gSpeed;
static unsigned int         gSpeedDirection;
static unsigned int         gStage;
static unsigned int         gCounter;
static unsigned int         gHealth;
static unsigned int         gIframes;
static GameState_t          gGameState;
static FrogAnimationState_t gFrogAnimationState;
static TongueState_t        gTongueState;
static unsigned int         gTongueCounter;
static unsigned int         gFade;
static const unsigned int*  gScratchPointer;

extern void pMusicInit(unsigned char);
extern void pMusicPlay(void);

void __fastcall__ UnRLE(const unsigned char *data);


#pragma bss-name (pop)

void vblank(void)
{
    while((*((unsigned char*)0x2002) & 0x80) != 0x00);
    while((*((unsigned char*)0x2002) & 0x80) != 0x80);
}

void vblank_counter(void)
{
    for( i = 0; i < gCounter; i++ )
    {
        vblank();
    }
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


// TODO not the most efficient but pulling directly from nametables
// This method goes through the top nametable then the bottom. It only needs the top left corner of
// every 2x2 block so grab every other on a row, then skip the next row entirely. Repeat.
void loadCollisionFromNametables(void)
{
  PPU_ADDRESS = 0x20; // address of nametable #0
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  i = *((unsigned char*)0x2007);

  for(i = 0 ; i < 240 ;) {
    collision[i] = (*((unsigned char*)0x2007) != PATTERN_BLANK_0) ? 0x01 : 0x00;
    j = *((unsigned char*)0x2007);

    i++;
    if(i % 16 == 0) {
      for(j = 0; j < 32 ; j++) {
        garbage = *((unsigned char*)0x2007);
      }
    }
  }

  PPU_ADDRESS = 0x28; // address of nametable #2
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  i = *((unsigned char*)0x2007);

  for(i = 0 ; i < 240 ;) {
    collision[i + 240] = (*((unsigned char*)0x2007) != PATTERN_BLANK_0) ? 0x01 : 0x00;
    j = *((unsigned char*)0x2007);

    i++;
    if(i % 16 == 0) {
      for(j = 0; j < 32 ; j++) {
        garbage = *((unsigned char*)0x2007);
      }
    }
  }
}


void palettes(void)
{
    // Sprite 0
    SET_COLOR(SPRITE0_1, DARK_GREEN);
    SET_COLOR(SPRITE0_2, GREEN);
    SET_COLOR(SPRITE0_3, WHITE);

    // Sprite 1
    SET_COLOR(SPRITE1_1, GRAY_BLUE);
    SET_COLOR(SPRITE1_2, DARK_BLUE);
    SET_COLOR(SPRITE1_3, LIGHT_ORANGE);

    // Sprite 2
    SET_COLOR(SPRITE2_1, DARK_RED);
    SET_COLOR(SPRITE2_2, RED);
    SET_COLOR(SPRITE2_3, WHITE);

    // Sprite 3
    SET_COLOR(SPRITE3_1, RED);
    SET_COLOR(SPRITE3_2, WHITE);
    SET_COLOR(SPRITE3_3, WHITE);
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

void load_palette(void)
{
    SET_COLOR(BACKGROUND0_0, Level1Palette[0]);
    SET_COLOR(BACKGROUND1_0, Level1Palette[0]);
    SET_COLOR(BACKGROUND2_0, Level1Palette[0]);
    SET_COLOR(BACKGROUND3_0, Level1Palette[0]);

    if( gFade > 0 )
    {
        SET_COLOR(BACKGROUND0_1, gScratchPointer[0]);
        SET_COLOR(BACKGROUND1_1, gScratchPointer[0]);
        SET_COLOR(BACKGROUND2_1, gScratchPointer[0]);
        SET_COLOR(BACKGROUND3_1, gScratchPointer[0]);
    }
    else
    {
        SET_COLOR(BACKGROUND0_1, gScratchPointer[1 ]);
        SET_COLOR(BACKGROUND1_1, gScratchPointer[4 ]);
        SET_COLOR(BACKGROUND2_1, gScratchPointer[7 ]);
        SET_COLOR(BACKGROUND3_1, gScratchPointer[10]);
    }

    if( gFade > 1 )
    {
        SET_COLOR(BACKGROUND0_2, gScratchPointer[0]);
        SET_COLOR(BACKGROUND1_2, gScratchPointer[0]);
        SET_COLOR(BACKGROUND2_2, gScratchPointer[0]);
        SET_COLOR(BACKGROUND3_2, gScratchPointer[0]);
    }
    else
    {
        SET_COLOR(BACKGROUND0_2, gScratchPointer[2  - gFade]);
        SET_COLOR(BACKGROUND1_2, gScratchPointer[5  - gFade]);
        SET_COLOR(BACKGROUND2_2, gScratchPointer[8  - gFade]);
        SET_COLOR(BACKGROUND3_2, gScratchPointer[11 - gFade]);
    }

    if( gFade > 2 )
    {
        SET_COLOR(BACKGROUND0_3, gScratchPointer[0]);
        SET_COLOR(BACKGROUND1_3, gScratchPointer[0]);
        SET_COLOR(BACKGROUND2_3, gScratchPointer[0]);
        SET_COLOR(BACKGROUND3_3, gScratchPointer[0]);
    }
    else
    {
        SET_COLOR(BACKGROUND0_3, gScratchPointer[3  - gFade]);
        SET_COLOR(BACKGROUND1_3, gScratchPointer[6  - gFade]);
        SET_COLOR(BACKGROUND2_3, gScratchPointer[9  - gFade]);
        SET_COLOR(BACKGROUND3_3, gScratchPointer[12 - gFade]);
    }

}

void fade_out(void)
{
    PPU_MASK = 0x0E;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 1;
    load_palette();

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 2;
    load_palette();

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 3;
    load_palette();

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
    gFade = 2;
    load_palette();

    PPU_CTRL = 0x84 + gYNametable;
    PPU_MASK = 0x0E;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 1;
    load_palette();

    PPU_CTRL = 0x84 + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 0;
    load_palette();

    PPU_CTRL = 0x84 + gYNametable;
    if( gStage == 0 || gStage > 4 )
    {
        PPU_MASK = 0x0E;
    }
    else
    {
        PPU_MASK = 0x1E;
    }
    set_scroll();

    gCounter = 20;
    vblank_counter();
}

void draw_health(void)
{
    for( i = 0; i < 8; i++)
    {
        sprites[40 + (i<<2)] = 0x0F + (i<<3) + i;
        sprites[41 + (i<<2)] = PATTERN_HEALTH_0;
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
    gY = 0xCF;
    gVelocity = 0;
    gVelocityDirection = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gXScroll = 0;
    gYScroll = 0;
    gYNametable = 2;
    gJumping = 0;
    gBounceCounter = 0;
    gStage = 0;
    gHealth = 8;
    gIframes = 0;

    // bird
    sprites[16] = 0x30;
    sprites[17] = PATTERN_BIRD_0;
    sprites[18] = 0x02;
    sprites[19] = 0x50;

    sprites[20] = 0x30;
    sprites[21] = PATTERN_BIRD_1;
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
    if(gJumping == 0 || (gJumping == 1 && gVelocityDirection == 1)) {
      gJumping = 2;
      gVelocity = 16;
      gVelocityDirection = 1;
      gPrevController1Change |= BUTTON_A;
    }
}

void update_frog_sprite(void)
{
    if( gVelocity <= 2 && gJumping != 0 )
    {
        gFrogAnimationState = FROG_SUSPENDED;
    }
    else if( gVelocityDirection == 1 && gJumping != 0 )
    {
        if( gJumping == 1 )
        {
            gFrogAnimationState = FROG_HOP;
        }
        else
        {
            gFrogAnimationState = FROG_JUMP;
        }
    }
    else if( gVelocityDirection == 0 && gJumping != 0 )
    {
        if( gJumping == 1 )
        {
            gFrogAnimationState = FROG_LANDING;
        }
        else
        {
            gFrogAnimationState = FROG_FALLING;
        }
    }
    else
    {
        gFrogAnimationState = FROG_NORMAL;
    }

    switch( gFrogAnimationState )
    {
        case FROG_HOP:
            if( gSpeedDirection == 1 )
            {
                sprites[1]  = PATTERN_FROG_HOP_0;
                sprites[5]  = PATTERN_FROG_HOP_1;
                sprites[9]  = PATTERN_FROG_HOP_2;
                sprites[13] = PATTERN_FROG_HOP_3;
            }
            else
            {
                sprites[1]  = PATTERN_FROG_HOP_1;
                sprites[5]  = PATTERN_FROG_HOP_0;
                sprites[9]  = PATTERN_FROG_HOP_3;
                sprites[13] = PATTERN_FROG_HOP_2;
            }
            break;

        case FROG_JUMP:
            if( gSpeedDirection == 1 )
            {
                sprites[1]  = PATTERN_FROG_JUMP_0;
                sprites[5]  = PATTERN_FROG_JUMP_1;
                sprites[9]  = PATTERN_FROG_JUMP_2;
                sprites[13] = PATTERN_FROG_JUMP_3;
            }
            else
            {
                sprites[1]  = PATTERN_FROG_JUMP_1;
                sprites[5]  = PATTERN_FROG_JUMP_0;
                sprites[9]  = PATTERN_FROG_JUMP_3;
                sprites[13] = PATTERN_FROG_JUMP_2;
            }
            break;

        case FROG_SUSPENDED:
            if( gSpeedDirection == 1 )
            {
                sprites[1]  = PATTERN_FROG_SUSPENDED_0;
                sprites[5]  = PATTERN_FROG_SUSPENDED_1;
                sprites[9]  = PATTERN_FROG_SUSPENDED_2;
                sprites[13] = PATTERN_FROG_SUSPENDED_3;
            }
            else
            {
                sprites[1]  = PATTERN_FROG_SUSPENDED_1;
                sprites[5]  = PATTERN_FROG_SUSPENDED_0;
                sprites[9]  = PATTERN_FROG_SUSPENDED_3;
                sprites[13] = PATTERN_FROG_SUSPENDED_2;
            }
            break;

        case FROG_FALLING:
            if( gSpeedDirection == 1 )
            {
                sprites[1]  = PATTERN_FROG_FALLING_0;
                sprites[5]  = PATTERN_FROG_FALLING_1;
                sprites[9]  = PATTERN_FROG_FALLING_2;
                sprites[13] = PATTERN_FROG_FALLING_3;
            }
            else
            {
                sprites[1]  = PATTERN_FROG_FALLING_1;
                sprites[5]  = PATTERN_FROG_FALLING_0;
                sprites[9]  = PATTERN_FROG_FALLING_3;
                sprites[13] = PATTERN_FROG_FALLING_2;
            }
            break;

        case FROG_LANDING:
            if( gSpeedDirection == 1 )
            {
                sprites[1]  = PATTERN_FROG_LANDING_0;
                sprites[5]  = PATTERN_FROG_LANDING_1;
                sprites[9]  = PATTERN_FROG_LANDING_2;
                sprites[13] = PATTERN_FROG_LANDING_3;
            }
            else
            {
                sprites[1]  = PATTERN_FROG_LANDING_1;
                sprites[5]  = PATTERN_FROG_LANDING_0;
                sprites[9]  = PATTERN_FROG_LANDING_3;
                sprites[13] = PATTERN_FROG_LANDING_2;
            }
            break;

        case FROG_NORMAL:
        default:
            if( gSpeedDirection == 1 )
            {
                sprites[1]  = PATTERN_FROG_0;
                sprites[5]  = PATTERN_FROG_1;
                sprites[9]  = PATTERN_FROG_2;
                sprites[13] = PATTERN_FROG_3;
            }
            else
            {
                sprites[1]  = PATTERN_FROG_1;
                sprites[5]  = PATTERN_FROG_0;
                sprites[9]  = PATTERN_FROG_3;
                sprites[13] = PATTERN_FROG_2;
            }
            break;
    }

    if( gSpeedDirection == 1 )
    {
        sprites[2] = 0x00;
        sprites[6] = 0x00;
        sprites[10] = 0x00;
        sprites[14] = 0x00;
    }
    else
    {
        sprites[2] = 0x40;
        sprites[6] = 0x40;
        sprites[10] = 0x40;
        sprites[14] = 0x40;
    }
}

void update_tongue_sprite(void)
{
    switch( gTongueState )
    {
        case TONGUE_EXTENDING:
            if( gTongueCounter == 0 )
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_0;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 24;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 8;
                }
                gTongueState = TONGUE_EXTENDING2;
                gTongueCounter = 2;
            }
            else
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 8;
                }
                gTongueCounter--;
            }
            break;
        case TONGUE_EXTENDING2:
            if( gTongueCounter == 0 )
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_0;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 24;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_1;
                    sprites[82] = 0x02;
                    sprites[83] = gX + 32;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 24;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 16;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_0;
                    sprites[82] = 0x42;
                    sprites[83] = gX - 8;
                }
                gTongueState = TONGUE_OUT;
                gTongueCounter = 5;
            }
            else
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_0;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 24;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 8;
                }
                gTongueCounter--;
            }
            break;
        case TONGUE_OUT:
            if( gTongueCounter == 0 )
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_0;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 24;

                    sprites[80] = 0x00;
                    sprites[81] = 0x00;
                    sprites[82] = 0x00;
                    sprites[83] = 0x00;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 8;

                    sprites[80] = 0x00;
                    sprites[81] = 0x00;
                    sprites[82] = 0x00;
                    sprites[83] = 0x00;
                }
                gTongueState = TONGUE_RETRACTING;
                gTongueCounter = 5;
            }
            else
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_0;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 24;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_1;
                    sprites[82] = 0x02;
                    sprites[83] = gX + 32;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 24;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 16;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_0;
                    sprites[82] = 0x42;
                    sprites[83] = gX - 8;
                }
                gTongueCounter--;
            }
            break;
        case TONGUE_RETRACTING:
            if( gTongueCounter == 0 )
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = 0x00;
                    sprites[77] = 0x00;
                    sprites[78] = 0x00;
                    sprites[79] = 0x00;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 8;

                    sprites[76] = 0x00;
                    sprites[77] = 0x00;
                    sprites[78] = 0x00;
                    sprites[79] = 0x00;
                }
                gTongueState = TONGUE_RETRACTING2;
                gTongueCounter = 5;
            }
            else
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_0;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 24;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 16;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 8;
                }
                gTongueCounter--;
            }
            break;
        case TONGUE_RETRACTING2:
            if( gTongueCounter == 0 )
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = 0x00;
                    sprites[73] = 0x00;
                    sprites[74] = 0x00;
                    sprites[75] = 0x00;
                }
                else
                {
                    sprites[72] = 0x00;
                    sprites[73] = 0x00;
                    sprites[74] = 0x00;
                    sprites[75] = 0x00;
                }
                gTongueState = TONGUE_NORMAL;
                gTongueCounter = 0;
            }
            else
            {
                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 8;
                }
                gTongueCounter--;
            }
            break;
        case TONGUE_NORMAL:
        default:
            if( (gController1 & BUTTON_B) != 0 && (gPrevController1 & BUTTON_B) == 0)
            {
                gTongueState = TONGUE_EXTENDING;
                gTongueCounter = 2;

                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 16;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 8;
                }
            }
            break;
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

                //sprites[1] = 0x01;
                //sprites[2] = 0x40;
                //sprites[5] = 0x00;
                //sprites[6] = 0x40;
                //sprites[9] = 0x03;
                //sprites[10] = 0x40;
                //sprites[13] = 0x02;
                //sprites[14] = 0x40;
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

                //sprites[1] = 0x00;
                //sprites[2] = 0x00;
                //sprites[5] = 0x01;
                //sprites[6] = 0x00;
                //sprites[9] = 0x02;
                //sprites[10] = 0x00;
                //sprites[13] = 0x03;
                //sprites[14] = 0x00;
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

    update_frog_sprite();
    update_tongue_sprite();

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

    switch( gStage )
    {
        case 0:
            PPU_ADDRESS = 0x28;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_TitleScreen_bottom_rle);	// uncompresses our data
            gScratchPointer = TitleScreenPalette;
            load_palette();
            break;

        case 1:
            PPU_ADDRESS = 0x28;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level1_bottom_rle);	// uncompresses our data
            PPU_ADDRESS = 0x20;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level1_top_rle);	// uncompresses our data
            gScratchPointer = Level1Palette;
            load_palette();
            break;

        case 2:
            PPU_ADDRESS = 0x28;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level2_bottom_rle);	// uncompresses our data
            PPU_ADDRESS = 0x20;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level2_top_rle);	// uncompresses our data
            gScratchPointer = Level2Palette;
            load_palette();
            break;
        case 3:
            PPU_ADDRESS = 0x28;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level3_bottom_rle);	// uncompresses our data
            PPU_ADDRESS = 0x20;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level3_top_rle);	// uncompresses our data
            gScratchPointer = Level3Palette;
            load_palette();
            break;
        case 4:
            PPU_ADDRESS = 0x28;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level4_bottom_rle);	// uncompresses our data
            PPU_ADDRESS = 0x20;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_Level4_top_rle);	// uncompresses our data
            gScratchPointer = Level4Palette;
            load_palette();
            break;
        default:
            PPU_ADDRESS = 0x28;
            PPU_ADDRESS = 0x00;
            UnRLE(Nametable_EndingScreen_bottom_rle);	// uncompresses our data
            gScratchPointer = EndingScreenPalette;
            load_palette();
            break;
    }

    loadCollisionFromNametables();

    vblank();

    gX = 0x10;
    gY = 0xCF;
    gYNametable = 2;
    gVelocity = 0;
    gVelocityDirection = 0;
    gXScroll = 0;
    gYScroll = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gJumping = 0;
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

void next_stage(void)
{
    switch( gStage )
    {
        case 4:
            gGameState = ENDING_STATE;

        default:
            gStage++;
    }
}

/**
 * Compares two rectangles for collision
 * Requires x1, y1, width1, height1, x2, y2, width2, height2 to be set bc params are apparently dangerous
 * TODO doesn't account for screen wrapping
 * TODO return values are also supposedly bad, but reads so much better in code like this
 */
int is_collision(void)
{
    return x1 + width1 > x2
        && x2 + width2 > x1
        && y1 + height1 > y2
        && y2 + height2 > y1;
}

void do_physics(void)
{
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
            sprites[21] = PATTERN_BIRD_0;
            sprites[22] = 0x41;
            sprites[17] = PATTERN_BIRD_1;
            sprites[18] = 0x41;
        }
        else
        {
            // X
            sprites[19] -= 1;
            sprites[23] -= 1;
            // Tiles
            sprites[21] = PATTERN_BIRD_1;
            sprites[22] = 0x01;
            sprites[17] = PATTERN_BIRD_0;
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
            gTmpX = gX - 1;

            if( gYNametable == 2 )
            {
              //x = gX >> 4;
              //y = gY >> 4
              //index[y*16 + x]
                if( collision[240 + (((gY+1)&0xF0)) + (gTmpX >> 4)] == 0 &&
                    collision[240 + (((gY+0x10)&0xF0)) + (gTmpX >> 4)] == 0 )
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
                    if( collision[240 + (((gYScroll + gY + 1 - 0xF0) & 0xF0) ) + (gTmpX >> 4)] == 0 &&
                        collision[240 + (((gYScroll + gY + 0x10 - 0xF0) & 0xF0) ) + (gTmpX >> 4)] == 0 )
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
                    if( collision[(((gYScroll + gY + 1) & 0xF0) ) + (gTmpX >> 4)] == 0 &&
                        collision[(((gYScroll + gY + 0x10) & 0xF0) ) + (gTmpX >> 4)] == 0 )
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
            gTmpX = gX + 0x10;

            if( gYNametable == 2 )
            {
                if( collision[240 + (((gY+1)&0xF0) ) + (gTmpX >> 4)] == 0 &&
                    collision[240 + (((gY+0x10)&0xF0) ) + (gTmpX >> 4)] == 0 )
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
                    if( collision[240 + (((gYScroll + gY + 1 - 0xF0) & 0xF0) ) + (gTmpX >> 4)] == 0 &&
                        collision[240 + (((gYScroll + gY + 0x10 - 0xF0) & 0xF0) ) + (gTmpX >> 4)] == 0 )
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
                    if( collision[(((gYScroll + gY + 1) & 0xF0) ) + (gTmpX >> 4)] == 0 &&
                        collision[(((gYScroll + gY + 0x10) & 0xF0) ) + (gTmpX >> 4)] == 0 )
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

    //
    // Vertical Movement
    //


    if( gVelocityDirection == 1 ) // moving up
    {
        for( i = 0; (i<<2) < gVelocity; i++ )
        {
            gTmpX = gX + 0xF;

            if( gYNametable == 2 )
            {
                //Bottom half of level
                if( collision[240 + (((gY)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gY)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    if(gY > MAX_TOP_BUFFER )
                    {
                        gY -= 1;
                    }
                    else
                    {
                        // Jumping out of the bottom half of screen so scroll instead of moving frog up
                        gYScroll = 0xEF;
                        gYNametable = 0;
                    }
                }
                else
                {
                    //Ran into a block so stop
                    gVelocity = 0;
                    gVelocityDirection = 0;
                    break;
                }
            }
            else if((gYScroll + gY) >= 0xF0 )
            {
                if( collision[240 + (((gYScroll + gY - 0xF0)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gYScroll + gY - 0xF0)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    if(gY > MAX_TOP_BUFFER)
                    {
                        gY -= 1;
                    }
                    else
                    {
                        //??
                        gYScroll -= 1;
                    }
                }
                else
                {
                    //Ran into block so stop
                    gVelocityDirection = 0;
                    break;
                }
            }
            else
            {
                if( collision[(((gYScroll + gY) & 0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[(((gYScroll + gY) & 0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                  //Approaching the top. Scroll until we can't anymore
                  if(gYScroll > 0) {
                    gYScroll -= 1;
                  }
                  else if(gY > 0x00) {
                    gY -= 1;
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
            gTmpX = gX + 0xF;

            if(gYNametable == 2 )
            {
                //Bottom half of level
                if( collision[240 + (((gYScroll + gY + 0x11)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gYScroll + gY + 0x11)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    if(gY < 0xFF )
                    {
                        gY += 1;
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
                    //Ran into block
                    gVelocity = 0;
                    gJumping = 0;
                    break;
                }
            }
            else if((gYScroll + gY + 0x11) >= 0xF0)
            {
                if( collision[240 + (((gYScroll + gY+0x11 - 0xF0)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gYScroll + gY+0x11 - 0xF0)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    if(gY < MAX_BOTTOM_BUFFER)
                    {
                        gY += 1;
                    }
                    else
                    {
                        if( gYScroll == 0xEF )
                        {
                            gYNametable = 2;
                            gYScroll = 0;
                            break;
                        }
                        else
                        {
                            gYScroll+=1;
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
                    collision[(((gYScroll + gY+0x11)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    if(gY < MAX_BOTTOM_BUFFER)
                    {
                        gY += 1;
                    }
                    else
                    {
                        gYScroll+=1;
                    }
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
            sprites[25] = PATTERN_PORTAL_0;
            sprites[26] = 0x00;
            sprites[27] = 0xE0;

            sprites[28] = 0x0F - gYScroll;
            sprites[29] = PATTERN_PORTAL_0;
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
        sprites[33] = PATTERN_PORTAL_0;
        sprites[34] = 0x80;
        sprites[35] = 0xE0;

        sprites[36] = 0x17 - gYScroll;
        sprites[37] = PATTERN_PORTAL_0;
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

    if( gYNametable == 0 && gYScroll == 0 && gY == 0x0F && gX == 0xE0 )
    {
        next_stage();
        load_stage();
    }

    //Tongue collision box
    x1 = sprites[75];
    y1 = sprites[72] + 1;
    width1 = (sprites[79] == 0) ? 8 : (sprites[83] == 0) ? 16 : 24;
    height1 = 6;

    //bird collision box
    x2 = sprites[19];
    y2 =  sprites[16] + 1;
    width2 = 4;
    height2 = 6;

    if(is_collision())
    {
      //Reset tongue
      sprites[72] = 0;
      sprites[75] = 0;
      sprites[76] = 0;
      sprites[79] = 0;
      sprites[80] = 0;
      sprites[83] = 0;
      gTongueState = TONGUE_NORMAL;
      gTongueCounter = 0;

      //Reset bird TODO don't just reset to top left corner
      sprites[16] = 0;
      sprites[19] = 8;
      sprites[20] = 0;
      sprites[23] = 16;
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
                gGameState = TITLE_SCREEN_STATE;
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

void init_globals(void)
{
    gGameState = TITLE_SCREEN_STATE;
}

void init_game_state(void)
{
    gX = 0x10;
    gY = 0xCF;
    gYNametable = 2;
    gVelocity = 0;
    gVelocityDirection = 0;
    gXScroll = 0;
    gYScroll = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gJumping = 0;
    gBounceCounter = 0;
    gHealth = 8;
    gFrogAnimationState = FROG_NORMAL;
    gTongueState = TONGUE_NORMAL;
    gTongueCounter = 0;
    gFade = 3;
}

void game_running_sm(void)
{
    while( gGameState == GAME_RUNNING_STATE )
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

void title_screen_sm(void)
{
    while( gGameState == TITLE_SCREEN_STATE )
    {
        vblank();

        input_poll();

        // set bits [1:0] to 0 for nametable
        PPU_CTRL = 0x84 + gYNametable;
        set_scroll();

        if( (gController1 & BUTTON_START) != 0 )
        {
            gStage = 1;
            gGameState = GAME_RUNNING_STATE;
            init_game_state();
            load_stage();
        }
    }
}

void end_screen_sm(void)
{
    while( gGameState == ENDING_STATE )
    {
        vblank();

        input_poll();

        // set bits [1:0] to 0 for nametable
        PPU_CTRL = 0x84 + gYNametable;
        set_scroll();

        if( (gController1 & BUTTON_START) != 0 )
        {
            gStage = 0;
            gGameState = TITLE_SCREEN_STATE;
            load_stage();
        }
    }
}

void main(void)
{
    init_globals();

    gCounter = 5;
    vblank_counter();

    ppudisable();

    palettes();
    vblank();

  	PPU_ADDRESS = 0x28; // address of nametable #2
  	PPU_ADDRESS = 0x00;
  	UnRLE(Nametable_TitleScreen_bottom_rle);	// uncompresses our data
    gScratchPointer = TitleScreenPalette;
    load_palette();

    loadCollisionFromNametables();

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
        switch( gGameState )
        {
            case GAME_RUNNING_STATE:
                game_running_sm();
                break;

            case ENDING_STATE:
                end_screen_sm();
                break;

            case DEMO_STATE:
            case TITLE_SCREEN_STATE:
            default:
                title_screen_sm();
                break;
        }

    }
}
