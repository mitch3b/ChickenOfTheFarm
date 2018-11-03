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

#define ARROW_SPEED 3
#define MAX_NUM_SPRITES 10        // need to reserve enough space for max sprites
#define FIRST_ENEMY_SPRITE 84
#define LAST_ENEMY_SPRITE 124 //Reserve 10 sprites for now

#define INVALID_ID   0
#define MINI_FROG_ID 1
#define ARROW_ID     2
#define BIRD_ID      3
#define ID_COUNT     4


#define SET_COLOR( index, color )  PPU_ADDRESS = 0x3F; PPU_ADDRESS = index; PPU_DATA = color

#define TONGUE_EXTEND_DELAY  0
#define TONGUE_RETRACT_DELAY 4

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
    TONGUE_CLEANUP,
}
TongueState_t;


//According to nesdev, better to have arrays of attributes than arrays of objects: https://forums.nesdev.com/viewtopic.php?f=2&t=17465&start=0
typedef struct {
  unsigned char id[MAX_NUM_SPRITES];
  unsigned char startX[MAX_NUM_SPRITES];
  unsigned char startY[MAX_NUM_SPRITES];
  unsigned char startNametable[MAX_NUM_SPRITES];
  unsigned char state[MAX_NUM_SPRITES];
  unsigned char direction[MAX_NUM_SPRITES]; //Also palette color, bit 7 is direction
  unsigned char numSprites[MAX_NUM_SPRITES]; //Not using this yet... not sure if we need it
  unsigned char doesTongueKill[MAX_NUM_SPRITES];
  unsigned char spriteStart[MAX_NUM_SPRITES];
} sprites_t;
#define SPRITES_T_MEMBER_COUNT 9

void invalid_ai_handler(void);
void arrow_ai_handler(void);
void bird_ai_handler(void);
void item_ai_handler(void);

void invalid_collision_handler(void);
void enemy_collision_handler(void);
void mini_frog_collision_handler(void);

typedef struct {
  unsigned char pattern;
  void          (*ai_handler)(void);
  void          (*collision_handler)(void);
} sprite_properties_t;

sprite_properties_t spriteProperties[ID_COUNT] = {
    {PATTERN_AAA_INVALID_0,  &invalid_ai_handler, &invalid_collision_handler  }, //INVALID_ID
    {  PATTERN_MINI_FROG_0,     &item_ai_handler, &mini_frog_collision_handler}, //MINI_FROG_ID
    {      PATTERN_ARROW_0,    &arrow_ai_handler, &enemy_collision_handler    }, //ARROW_ID
    {       PATTERN_BIRD_0,     &bird_ai_handler, &enemy_collision_handler    }, //BIRD_ID
};

//
// GLOBALS
//

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
#pragma bss-name (push, "OAM")
unsigned char sprites[256];


#pragma bss-name (pop)

#pragma bss-name (push, "BSS")

// 16 x (15 + 15)
// TODO should use a bit per block instead of byte, but this is a lot easier at the moment
unsigned char collision[496];


static unsigned char*       gScratchPointer2;
static unsigned char        gController1;
static unsigned char        gPrevController1;
static unsigned char        gPrevController1Change;
static unsigned char        gFrameCounter;
static unsigned char        gTmpX;
static unsigned char        gX;
static unsigned char        gY;
static unsigned long        gXScroll;
static unsigned char        gYScroll;
static unsigned char        gYPrevScroll;
static unsigned char        gYNametable;
static unsigned char        gYPrevNametable;
static unsigned char        numMiniFrogs;
static unsigned char        devnull;
static unsigned int         i;
static unsigned int         j;
static unsigned char        k;
static unsigned char        gTmp;
static unsigned char        gTmp2;
static unsigned char        gTmp3;
static unsigned char        gTmp4;
static unsigned char        gTmp5;
static unsigned char        gTmp6;
static unsigned char        gTmp7;

// These are probably overkill, but it makes collision detection a lot cleaner
static unsigned char        x1;
static unsigned char        y1;
static unsigned char        width1;
static unsigned char        height1;
static unsigned char        x2;
static unsigned char        y2;
static unsigned char        width2;
static unsigned char        height2;
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
static sprites_t            gSpriteTable;
static unsigned char        numSprites;
static GameState_t          gGameState;
static FrogAnimationState_t gFrogAnimationState;
static TongueState_t        gTongueState;
static unsigned int         gTongueCounter;
static unsigned int         gFade;
static unsigned int         gLives;
static unsigned int         gDisplayLives;
static const unsigned char* gScratchPointer;

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
    //Could put this only when its set, but this is the safest place
    gYPrevScroll = gYScroll;
    gYPrevNametable = gYNametable;
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
    gTmp4 = *((unsigned char*)0x2007);
    collision[i] =  (gTmp4 != PATTERN_BLANK_0) ? 0x01 : 0x00;
    j = *((unsigned char*)0x2007);

    i++;
    if(i % 16 == 0) {
      for(j = 0; j < 32 ; j++) {
        gTmp4 = *((unsigned char*)0x2007);
      }
    }
  }

  PPU_ADDRESS = 0x28; // address of nametable #2
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  i = *((unsigned char*)0x2007);

  for(i = 0 ; i < 240 ;) {
    gTmp4 = *((unsigned char*)0x2007);
    collision[i + 240] =  (gTmp4 != PATTERN_BLANK_0) ? 0x01 : 0x00;
    j = *((unsigned char*)0x2007);

    i++;
    if(i % 16 == 0) {
      for(j = 0; j < 32 ; j++) {
        gTmp4 = *((unsigned char*)0x2007);
      }
    }
  }

  for(i = 0; i < 16; i++)
  {
      collision[480 + i] = 0;
  }
}

void ClearSprites(void)
{
    // clear sprite ram
    for( i = 0; i < 256; i++ )
    {
        sprites[i] = 0x00;
    }

    gScratchPointer2 = (unsigned char*)sprites;
    for(gTmp2 = 0; gTmp2 < (MAX_NUM_SPRITES * SPRITES_T_MEMBER_COUNT); gTmp2++)
    {
        gScratchPointer2 = 0;
        gScratchPointer2++;
    }
}

void LoadSprites(void)
{
    numMiniFrogs = 0;
    for(gTmp2 = 0; gTmp2 < numSprites; gTmp2++)
    {
        gTmp = *gScratchPointer2++;
        gSpriteTable.id[gTmp2] =             gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.startX[gTmp2] =         gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.startY[gTmp2] =         gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.startNametable[gTmp2] = gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.state[gTmp2] =          gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.direction[gTmp2] =      gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.numSprites[gTmp2] =     gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.doesTongueKill[gTmp2] = gTmp;
        gTmp = *gScratchPointer2++;
        gSpriteTable.spriteStart[gTmp2] =    gTmp;

        if(gSpriteTable.id[gTmp2] == MINI_FROG_ID) {
          numMiniFrogs++;
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
    if( gStage == 0 || gStage > 4 || gDisplayLives == 1)
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

//40, 44, 48, 52, 54, 58, 62, 64
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
      gVelocity = 13;
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
                gTongueCounter = TONGUE_EXTEND_DELAY;
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
                gTongueCounter = TONGUE_RETRACT_DELAY;
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
                gTongueCounter = TONGUE_RETRACT_DELAY;
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
                gTongueCounter = TONGUE_RETRACT_DELAY;
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
                sprites[72] = 0x00;
                sprites[73] = 0x00;
                sprites[74] = 0x00;
                sprites[75] = 0x00;
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
        case TONGUE_CLEANUP:
            for( i = 72; i < 84; i++)
            {
                sprites[i] = 0x00;
            }
            gTongueState = TONGUE_NORMAL;
            break;
        case TONGUE_NORMAL:
        default:
            if( (gController1 & BUTTON_B) != 0 && (gPrevController1 & BUTTON_B) == 0)
            {
                gTongueState = TONGUE_EXTENDING;
                gTongueCounter = TONGUE_EXTEND_DELAY;

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

    if(gIframes > 0 && (gIframes & 0x4) == 0)
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

    gYNametable = 2;
    gYScroll = 0;

    if( gDisplayLives == 1)
    {
        PPU_ADDRESS = 0x28;
        PPU_ADDRESS = 0x00;
        UnRLE(Nametable_Lives_bottom_rle);	// uncompresses our data

        PPU_ADDRESS = 0x29;
        PPU_ADDRESS = 0xED;
        PPU_DATA = PATTERN_NUMBERS_0 + gLives;

        gScratchPointer = LivesPalette;
        load_palette();

        vblank();

        fade_in();

        gCounter = 30;
        vblank_counter();

        fade_out();

        gDisplayLives = 0;
    }

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

            ClearSprites();
            numSprites = LEVEL1_ENEMY_COUNT;
            gScratchPointer2 = (unsigned char*)Sprites_Level1;
            LoadSprites();
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

            ClearSprites();
            numSprites = LEVEL2_ENEMY_COUNT;
            gScratchPointer2 = (unsigned char*)Sprites_Level2;
            LoadSprites();
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

            ClearSprites();
            numSprites = LEVEL3_ENEMY_COUNT;
            gScratchPointer2 = (unsigned char*)Sprites_Level3;
            LoadSprites();
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

            ClearSprites();
            numSprites = LEVEL4_ENEMY_COUNT;
            gScratchPointer2 = (unsigned char*)Sprites_Level4;
            LoadSprites();
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

    draw_health();

    update_sprites();
    dma_sprites();
    set_scroll();

    vblank();

    fade_in();
}

void next_stage(void)
{
    switch( gStage )
    {
        case 4:
            gGameState = ENDING_STATE;

        default:
            gTongueState = TONGUE_CLEANUP;
            gTongueCounter = 0;
            update_tongue_sprite();

            gStage++;
    }
}

void death(void)
{
    gHealth = 8;

    if( gLives == 0 )
    {
        gStage = 0;
        gGameState = TITLE_SCREEN_STATE;
    }
    else
    {
        gLives--;
        gDisplayLives = 1;
    }

    draw_health();
    load_stage();
}

/**
 * Compares two rectangles for collision
 * Requires x1, y1, width1, height1, x2, y2, width2, height2 to be set bc params are apparently dangerous
 * TODO doesn't account for screen wrapping left to right
 * TODO return values are also supposedly bad, but reads so much better in code like this
 */
int is_collision(void)
{
    return x1 + width1 > x2
        && x2 + width2 > x1
        && y1 + height1 > y2
        && y2 + height2 > y1;
}

/**
 * Compares rectangle sprite to background
 * Requires x1, y1, width1, height1
 * TODO doesn't account for screen wrapping left to right
 * TODO return values are also supposedly bad, but reads so much better in code like this
 */
int is_background_collision(void) {
  if( gYNametable == 2 )
  {
      if( collision[240 + (((y1)&0xF0)) + (x1 >> 4)] == 0 &&
          collision[240 + (((y1 + height1)&0xF0)) + (x1 >> 4)] == 0 &&
          collision[240 + (((y1)&0xF0)) + ((x1 + width1) >> 4)] == 0 &&
              collision[240 + (((y1 + height1)&0xF0)) + ((x1 + width1) >> 4)] == 0)
      {
          return 0;
      }
      else
      {
          return 1;
      }
  }
  else
  {
      if((gYScroll + y1 + 1) >= 0xF0)
      {
          if( collision[240 + (((gYScroll + y1 - 0xF0) & 0xF0) ) + (x1 >> 4)] == 0 &&
              collision[240 + (((gYScroll + y1 + height1 - 0xF0) & 0xF0) ) + (x1 >> 4)] == 0 &&
              collision[240 + (((gYScroll + y1 - 0xF0) & 0xF0) ) + ((x1 + width1) >> 4)] == 0 &&
              collision[240 + (((gYScroll + y1 + height1 - 0xF0) & 0xF0) ) + ((x1 + width1) >> 4)] == 0 )
          {
              return 0;
          }
          else
          {
              return 1;
          }
      }
      else
      {
          if( collision[(((gYScroll + y1) & 0xF0) ) + (x1 >> 4)] == 0 &&
              collision[(((gYScroll + y1 + height1) & 0xF0) ) + (x1 >> 4)] == 0 &&
              collision[(((gYScroll + y1) & 0xF0) ) + ((x1 + width1) >> 4)] == 0 &&
              collision[(((gYScroll + y1 + height1) & 0xF0) ) + ((x1 + width1) >> 4)] == 0 )
          {
              return 0;
          }
          else
          {
              return 1;
          }
      }
  }
}

void put_i_in_collision2_vars(void) {
  switch(gSpriteTable.id[i])
  {
    case ARROW_ID:
      x2 = sprites[j + 3];
      y2 =  sprites[j] + 4;
      width2 = 8;
      height2 = 1;
      break;

    case BIRD_ID:
      x2 = sprites[j+3];
      y2 =  sprites[j];
      width2 = 16;
      height2 = 8;
      break;

    case MINI_FROG_ID:
    default:
      x2 = sprites[j + 3];
      y2 = sprites[j];
      width2 = 8;
      height2 = 8;
      break;
  }
}

void take_hit(void)
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
          death();
      }
      else
      {
          gIframes = 120;
      }
      //if(gSpeed != 0)
      {
          gSpeed = 16;
          gSpeedDirection ^= 1;
      }
  }
}

/**
 * Overwrites:
 * gTmp4
 * gTmp
 * gTmp2
 * k
 */
void spawn_check(void)
{
  // check to see if the sprite has already spawned first
  if(sprites[j+1] == 0 && gSpriteTable.id[i] != INVALID_ID)
  {
    if(gSpriteTable.startNametable[i] == 2) {
      //bottom half
      if(gYNametable == 0) {
        gTmp4 = gSpriteTable.startY[i];
        if(gYScroll > gTmp4) {
            //should now be displayed
            gTmp4 = gSpriteTable.startY[i];
            gTmp = gSpriteTable.numSprites[i];
            for(k = 0; k < gTmp; k++)
            {
                sprites[j + (k << 2)] =  gTmp4 - gYScroll - 0x10;
                gTmp2 = spriteProperties[gSpriteTable.id[i]].pattern + k;
                sprites[j + (k << 2) + 1] = gTmp2;
                sprites[j + (k << 2) + 2] = gSpriteTable.direction[i];
                if(sprites[j + (k << 2) + 3] == 0) {
                  sprites[j + (k << 2) + 3] = gSpriteTable.startX[i] + ((k & 1) << 3);
                }
            }
        }
        else {
          //moved offscreen so kill it
          gTmp = gSpriteTable.numSprites[i];
          for(k = 0; k < gTmp; k++)
          {
              sprites[j + (k << 2)] = 0x00;
              sprites[j + (k << 2) + 1] = 0x00;
              sprites[j + (k << 2) + 2] = 0x00;
              sprites[j + (k << 2) + 3] = 0x00;
          }
        }
      }
      else {
        gTmp = gSpriteTable.numSprites[i];
        for(k = 0; k < gTmp; k++)
        {
          //The way we're doing scrolling, for nametable 0, we'll never have a scroll value
          sprites[j + (k << 2)] = gSpriteTable.startY[i];
          gTmp2 = spriteProperties[gSpriteTable.id[i]].pattern + k;
          sprites[j + (k << 2) + 1] = gTmp2;
          sprites[j + (k << 2) + 2] = gSpriteTable.direction[i];
          if(sprites[j + (k << 2) + 3] == 0) {
            gTmp2 = gSpriteTable.startX[i] + ((k & 1) << 3);
            sprites[j + (k << 2) + 3] = gTmp2;
          }
          //gTmp++; //TODO for whatever reason, without something here, the line above doesn't work correctly
        }
      }
    }
    else {
      //top half
      if(gYNametable == 0) {
        gTmp4 = gSpriteTable.startY[i];
        if(gYScroll < gTmp4) {
          gTmp = gSpriteTable.numSprites[i];
          for(k = 0; k < gTmp; k++)
          {
            //should now be displayed
            gTmp4 = gSpriteTable.startY[i];
            sprites[j + (k << 2)] = gTmp4 - gYScroll + ((k >> 1) << 3);
            gTmp2 = spriteProperties[gSpriteTable.id[i]].pattern + k;
            sprites[j + (k << 2) + 1] = gTmp2;
            sprites[j + (k << 2) + 2] = gSpriteTable.direction[i];
            if(sprites[j + (k << 2) + 3] == 0) {
              sprites[j + (k << 2) + 3] = gSpriteTable.startX[i] + ((k & 1) << 3);
            }
          }
        }
        else {
          gTmp = gSpriteTable.numSprites[i];
          for(k = 0; k < gTmp; k++)
          {
            //moved offscreen so kill it
            sprites[j + (k << 2)] = 0;
            sprites[j + (k << 2) + 1] = 0;
            sprites[j + (k << 2) + 2] = 0;
            sprites[j + (k << 2) + 3] = 0;
          }
          gSpriteTable.state[i] = 0;
        }
      }
      else {
        ////The way we're doing scrolling, for nametable 0, we'll never have a scroll value
        gTmp = gSpriteTable.numSprites[i];
        for(k = 0; k < gTmp; k++)
        {
          //moved offscreen so kill it
          sprites[j + (k << 2)] = 0;
          sprites[j + (k << 2) + 1] = 0;
          sprites[j + (k << 2) + 2] = 0;
          sprites[j + (k << 2) + 3] = 0;
        }
      }
    }
  }
}

/**
 * Expects:
 * j to be set to first sprite id
 * i to be set to bird index in gSpriteTable
 * Resets:
 * gTmp
 * gTmp4
 * gTmp5
 * k
 */
void sprite_maintain_y_position(void) {
  //Put scroll shift in gTmp (TODO could do this just once)
  if(gYPrevNametable == gYNametable) {
    gTmp = gYScroll - gYPrevScroll;
  }
  else{
    if(gYNametable == 2) {
      //basically hit the bottom
      gTmp = 0xF0 - gYPrevScroll;
    }
    else {
      //Moved from very bottom to scrolled top screen
      gTmp = gYScroll - 0xF0;
    }
  }

  k = j;

  gTmp5 = gSpriteTable.numSprites[i];
  for(gTmp4 = 0 ; gTmp4 < gTmp5 ; gTmp4++) {
    gTmp6 = sprites[k];
    sprites[k] = gTmp6 - gTmp;
    k += 4;
  }
}

void arrow_ai_handler(void)
{
  // check to see if the sprite has spawned first
  if(sprites[j+1] != 0)
  {
    //Update Y
    sprite_maintain_y_position();

    //Update X
    if(gSpriteTable.state[i] == 2 || gSpriteTable.state[i] == 1)
    {
        //Arrow is in flight
        //if arrow is just long enough to stick into a wall
        x1 = sprites[j + 3];
        y1 = sprites[j] + 4;
        height1 = 2;
        width1 = 8; //Don't count the tip
        if(gSpriteTable.state[i] == 2 && is_background_collision())
        {
          gSpriteTable.state[i] = 3;
        }
        else
        {
          if((gSpriteTable.direction[i] & 0x40) == 0x00)
          {
            //moving right
            sprites[j + 3] += ARROW_SPEED;
            //Doing this in line didn't work
            gTmp4 = sprites[j + 3];
            gTmp4 -= gSpriteTable.startX[i];

            if(gTmp4 > 8) {
              gSpriteTable.state[i] = 2;
            }
          }
          else {
            //moving left
            sprites[j + 3] -= ARROW_SPEED;
            //Doing this in line didn't work
            gTmp4 = gSpriteTable.startX[i];
            gTmp4 -= sprites[j + 3];

            if(gTmp4 > 8) {
              gSpriteTable.state[i] = 2;
            }
          }
        }
    }
    else if(gSpriteTable.state[i] == 0)
    {
      //Arrow isn't moving/waiting for trigger
      if(sprites[j] >= sprites[0] && sprites[j] <= sprites[8])
      {
        //Start moving if frog becomes in path
        gSpriteTable.state[i] = 1;
      }
    }
    else
    {
        //Arrow is stuck in wall
        gTmp4 = gSpriteTable.state[i];
        gSpriteTable.state[i] = gTmp4 + 1;
        //Check if its been stuck long enough to respawn
        if(gSpriteTable.state[i] > 120) {
          gSpriteTable.state[i] = 0;
          gTmp4 = gSpriteTable.startX[i];
          sprites[j + 3] = gTmp4;
        }
    }
  }
}

/**
 * Expects:
 * j to be set to first sprite id
 * i to be set to bird index in gSpriteTable
 * Resets:
 * gTmp4
 * Increments:
 * gTmp
 */
void bird_ai_handler(void)
{
  if(sprites[j+3] != 0) {
    sprite_maintain_y_position();

    //Bird in general moves towards the frog
    //Update Y
    if( sprites[j] != sprites[0])
    {
        if( sprites[j] < sprites[0])
        {
            sprites[j] += 1;
            sprites[j+4] += 1;

            x1 = sprites[j + 3];
            y1 = sprites[j] + 1;
            height1 = 8;
            width1 = 16; //Don't count the tip
            if(is_background_collision()) {
              sprites[j] -= 1;
              sprites[j+4] -= 1;
            }
        }
        else
        {
            sprites[j] -= 1;
            sprites[j+4] -= 1;

            x1 = sprites[j + 3];
            y1 = sprites[j] + 1;
            height1 = 8;
            width1 = 16; //Don't count the tip
            if(is_background_collision()) {
              sprites[j] += 1;
              sprites[j+4] += 1;
            }
        }
    }

    //Update X
    if( sprites[j+3] != sprites[3])
    {
        if( sprites[j+3] < sprites[3])
        {
            // X
            sprites[j+3] += 1;
            sprites[j+7] += 1;

            x1 = sprites[j + 3];
            y1 = sprites[j] + 1;
            height1 = 8;
            width1 = 16; //Don't count the tip
            if(is_background_collision()) {
              sprites[j+3] -= 1;
              sprites[j+7] -= 1;
            }

            // Tiles
            sprites[j+1] = PATTERN_BIRD_1;
            sprites[j+2] = 0x41;
            sprites[j+5] = PATTERN_BIRD_0;
            sprites[j+6] = 0x41;
        }
        else
        {
            // X
            sprites[j+3] -= 1;
            sprites[j+7] -= 1;

            x1 = sprites[j + 3];
            y1 = sprites[j] + 1;
            height1 = 8;
            width1 = 16; //Don't count the tip
            if(is_background_collision()) {
              sprites[j+3] += 1;
              sprites[j+7] += 1;
            }

            // Tiles
            sprites[j+1] = PATTERN_BIRD_0;
            sprites[j+2] = 0x01;
            sprites[j+5] = PATTERN_BIRD_1;
            sprites[j+6] = 0x01;
        }
    }
  }
}

void item_ai_handler(void)
{
  // check to see if the sprite has spawned first
  if(sprites[j+1] != 0)
  {
    //Update Y
    sprite_maintain_y_position();
  }
}

void enemy_collision_handler(void)
{
  if(gIframes == 0)
  {
    take_hit();
  }
}

void mini_frog_collision_handler(void)
{
  //remove the item
  gSpriteTable.id[i] = INVALID_ID;
  gTmp = gSpriteTable.numSprites[i];
  for(k = 0; k < gTmp; k++)
  {
    sprites[j + (k << 2)] = 0;
    sprites[j + (k << 2) + 1] = 0;
    sprites[j + (k << 2) + 2] = 0;
    sprites[j + (k << 2) + 3] = 0;
    gTmp2++;
  }

  if( gHealth < 8 )
  {
    gHealth++;
    draw_health();
  }

  numMiniFrogs--;
}

void invalid_ai_handler(void)
{
}

void invalid_collision_handler(void)
{
}

void do_physics(void)
{
    //
    // Horizontal Movement
    //

    if( gSpeedDirection == 0 )
    {
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
                if( collision[(((gYScroll + gY - 0x100) & 0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[(((gYScroll + gY - 0x100) & 0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                  //Approaching the top. Scroll until we can't anymore
                  if(gY > MAX_TOP_BUFFER || gYScroll == 0) {
                    gY -= 1;
                  }
                  else {
                    gYScroll -= 1;
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

        if( gVelocity == 0 )
        {
            gVelocityDirection = 0;
        }
        else
        {
            if((gController1 & BUTTON_A) != BUTTON_A || (gFrameCounter & 1))
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
                if( collision[240 + (((gY + 0x11)&0xF0) ) + ((gX) >> 4)] == 0 &&
                    collision[240 + (((gY + 0x11)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    if(gY == 0xF0)
                    {
                        death();
                    }
                    else
                    {
                        gY += 1;
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
          //Had to put the lock in front of portal so move one of portal tiles back to 128.
          //Need to do an overhaul of reserved sprites at some point
            sprites[128] = 0x0F - gYScroll;
            sprites[129] = PATTERN_PORTAL_0;
            sprites[130] = 0x00;
            sprites[131] = 0xE0;

            sprites[28] = 0x0F - gYScroll;
            sprites[29] = PATTERN_PORTAL_0;
            sprites[30] = 0x40;
            sprites[31] = 0xE8;
        }
        else
        {
            sprites[128] = 0x00;
            sprites[129] = 0x00;
            sprites[130] = 0x00;
            sprites[131] = 0x00;

            sprites[28] = 0x00;
            sprites[29] = 0x00;
            sprites[30] = 0x00;
            sprites[31] = 0x00;

            //lock
            sprites[24] = 0x00;
            sprites[25] = 0x00;
            sprites[26] = 0x00;
            sprites[27] = 0x00;
        }

        sprites[32] = 0x17 - gYScroll;
        sprites[33] = PATTERN_PORTAL_0;
        sprites[34] = 0x80;
        sprites[35] = 0xE0;

        sprites[36] = 0x17 - gYScroll;
        sprites[37] = PATTERN_PORTAL_0;
        sprites[38] = 0xC0;
        sprites[39] = 0xE8;

        //lock
        if(numMiniFrogs > 0) {
          sprites[24] = 0x14 - gYScroll;
          sprites[25] = PATTERN_LOCK_0;
          sprites[26] = 0x00;
          sprites[27] = 0xE4;
        }
        else {
          sprites[24] = 0x00;
          sprites[25] = 0x00;
          sprites[26] = 0x00;
          sprites[27] = 0x00;
        }
    }
    else
    {
        sprites[128] = 0x00;
        sprites[129] = 0x00;
        sprites[130] = 0x00;
        sprites[131] = 0x00;

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

        sprites[24] = 0x00;
        sprites[25] = 0x00;
        sprites[26] = 0x00;
        sprites[27] = 0x00;
    }

    //Update enemies with movements
    for(i = 0; i < numSprites; i++) {
      j = gSpriteTable.spriteStart[i];
      gTmp3 = sprites[j];

      gTmp2 = gSpriteTable.id[i];

      spriteProperties[gTmp2].ai_handler();
      // In case item/enemy needs to be respawned
      spawn_check();

      // Check if we scrolled this offscreen
      gTmp7 = sprites[j];
      if((gTmp3 > gTmp7 && (gTmp3 - gTmp7) > 200)
       || (gTmp7 > gTmp3 && (gTmp7 - gTmp3) > 200)) {
        sprites[j] = 0;
        sprites[j+1] = 0;
        sprites[j+2] = 0;
        sprites[j+3] = 0;

        //TODO use numSprites
        if(gTmp2 == BIRD_ID) {
          sprites[j+4] = 0;
          sprites[j+5] = 0;
          sprites[j+6] = 0;
          sprites[j+7] = 0;
        }
      }

      //Tongue collision box
      if(gTongueState != TONGUE_NORMAL) {
        x1 = sprites[75];
        y1 = sprites[72] + 1;
        width1 = (sprites[79] == 0) ? 8 : (sprites[83] == 0) ? 16 : 24;
        height1 = 6;

        put_i_in_collision2_vars();

        if(is_collision()) {
          //Reset tongue
          sprites[72] = 0;
          sprites[75] = 0;
          sprites[76] = 0;
          sprites[79] = 0;
          sprites[80] = 0;
          sprites[83] = 0;
          gTongueState = TONGUE_NORMAL;
          gTongueCounter = 0;

          gTmp = gSpriteTable.numSprites[i];
          for(k = 0; k < gTmp; k++)
          {
            //moved offscreen so kill it
            sprites[j + (k << 2)] = 0;
            sprites[j + (k << 2) + 1] = 0;
            sprites[j + (k << 2) + 2] = 0;
            sprites[j + (k << 2) + 3] = 0;
          }

        }
      }

      //Frog collision box
      x1 = sprites[3] + 1;
      y1 = sprites[0] + 1;
      width1 = 14;
      height1 = 15;

      put_i_in_collision2_vars();

      if(is_collision())
      {
          spriteProperties[gSpriteTable.id[i]].collision_handler();
      }

    }

    if( gYNametable == 0 && gYScroll == 0 && gY == 0x0F && gX == 0xE0 && numMiniFrogs == 0)
    {
        next_stage();
        load_stage();
    }

    if( gIframes > 0 )
    {
        --gIframes;
    }
}

void init_globals(void)
{
    gGameState = TITLE_SCREEN_STATE;
    gDisplayLives = 0;
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
    gIframes = 0;
    gHealth = 8;
    draw_health();
    gFrogAnimationState = FROG_NORMAL;
    gTongueState = TONGUE_CLEANUP;
    gTongueCounter = 0;
    update_tongue_sprite();
    gFade = 3;
    gFrameCounter = 0;
    gLives = 2;
    gDisplayLives = 1;
}

void game_running_sm(void)
{
    while( gGameState == GAME_RUNNING_STATE )
    {
        vblank();

        gFrameCounter++;

        input_poll();

        //pMusicPlay();

        if((gController1 & BUTTON_START) == BUTTON_START)
        {
            do
            {
             vblank();
             input_poll();
            }
            while((gController1 & BUTTON_START) == BUTTON_START);

            do
            {
             vblank();
             input_poll();
            }
            while((gController1 & BUTTON_START) != BUTTON_START);

            do
            {
             vblank();
             input_poll();
            }
            while((gController1 & BUTTON_START) == BUTTON_START);
        }

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
        gYScroll = 0;
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
    init_game_state();

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

    apuinit();

    pMusicInit(0x0);

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
