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
#define MAX_NUM_SPRITES 21        // need to reserve enough space for max sprites
#define FIRST_ENEMY_SPRITE 84
//#define LAST_ENEMY_SPRITE 124 //Reserve 10 sprites for now

#define INVALID_ID   0
#define MINI_FROG_ID 1
#define ARROW_ID     2
#define BIRD_ID      3
#define PORTAL_ID    4
#define KEY_ID       5
#define SNAKE_ID     6
#define HEART_ID     7
#define ID_COUNT     8


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
  unsigned char direction[MAX_NUM_SPRITES]; //Also palette color, bit 7 is direction
} sprites_t;
#define SPRITES_T_MEMBER_COUNT 9

static unsigned char SpriteSize[ID_COUNT] = {
    0,  //INVALID_ID   0
    1,  //MINI_FROG_ID 1
    1,  //ARROW_ID     2
    2,  //BIRD_ID      3
    4,  //PORTAL_ID    4
    1,  //KEY_ID       5
    2,  //SNAKE_ID     6
    1,  //HEART_ID     7
};

void spawn_1_by_1_sprite(void);
void spawn_2_by_1_sprite(void);
void spawn_portal_sprite(void);

void despawn_1_sprite(void);
void despawn_2_sprite(void);
void despawn_portal_sprite(void);

void invalid_ai_handler(void);
void arrow_ai_handler(void);
void bird_ai_handler(void);
void item_ai_handler(void);
void snake_ai_handler(void);

void invalid_collision_handler(void);
void enemy_collision_handler(void);
void mini_frog_collision_handler(void);
void heart_collision_handler(void);
void portal_collision_handler(void);
void key_collision_handler(void);

void PlaySoundEffects(void);
void init_physics(void);

typedef struct {
  unsigned char pattern;
  void          (*spawn)(void);
  void          (*despawn)(void);
  void          (*ai_handler)(void);
  void          (*collision_handler)(void);
} sprite_properties_t;

sprite_properties_t spriteProperties[ID_COUNT] = {
    {PATTERN_AAA_INVALID_0,  &spawn_1_by_1_sprite,      &despawn_1_sprite, &invalid_ai_handler, &invalid_collision_handler  }, //INVALID_ID
    {  PATTERN_MINI_FROG_0,  &spawn_1_by_1_sprite,      &despawn_1_sprite,    &item_ai_handler, &mini_frog_collision_handler}, //MINI_FROG_ID
    {      PATTERN_ARROW_0,  &spawn_1_by_1_sprite,      &despawn_1_sprite,   &arrow_ai_handler, &enemy_collision_handler    }, //ARROW_ID
    {       PATTERN_BIRD_0,  &spawn_2_by_1_sprite,      &despawn_2_sprite,    &bird_ai_handler, &enemy_collision_handler    }, //BIRD_ID
    {     PATTERN_PORTAL_0,  &spawn_portal_sprite, &despawn_portal_sprite,    &item_ai_handler, &portal_collision_handler   }, //PORTAL_ID
    {        PATTERN_KEY_0,  &spawn_1_by_1_sprite,      &despawn_1_sprite,    &item_ai_handler, &key_collision_handler      }, //KEY_ID
    {      PATTERN_SNAKE_0,  &spawn_2_by_1_sprite,      &despawn_2_sprite,   &snake_ai_handler, &enemy_collision_handler    }, //SNAKE_ID
    {      PATTERN_HEART_0,  &spawn_1_by_1_sprite,      &despawn_1_sprite,    &item_ai_handler, &heart_collision_handler    }, //HEART_ID
};

typedef struct {
    const unsigned char* bottom_rle;
    const unsigned char* top_rle;
    const unsigned char* palette;
    const unsigned char* sprites;
    unsigned char        numSprites;
    unsigned char        music;
} level_properties_t;

typedef struct {
    unsigned char        FrogStartX;
    unsigned char        FrogStartY;
    unsigned char        world;
} level_additional_properties_t;


#define NUM_LEVELS 25
level_properties_t LevelTable[NUM_LEVELS] = {
    {Nametable_TitleScreen_bottom_rle,           0,                                       TitleScreenPalette,  0,                             0,                                 0,},
    {Nametable_SmallPlatforms_bottom_rle,        Nametable_SmallPlatforms_top_rle,        GrassPalette,        Sprites_SmallPlatforms,        SMALLPLATFORMS_ENEMY_COUNT,        2,},
    {Nametable_ShortClimb_bottom_rle,            Nametable_ShortClimb_top_rle,            GrassPalette,        Sprites_ShortClimb,            SHORTCLIMB_ENEMY_COUNT,            2,},
    {Nametable_OpenPit_bottom_rle,               Nametable_OpenPit_top_rle,               GrassPalette,        Sprites_OpenPit,               OPENPIT_ENEMY_COUNT,               2,},
    {Nametable_OneArrow_bottom_rle,              Nametable_OneArrow_top_rle,              GrassPalette,        Sprites_OneArrow,              ONEARROW_ENEMY_COUNT,              2,},
    {Nametable_Intro_bottom_rle,                 Nametable_Intro_top_rle,                 GrassPalette,        Sprites_Intro,                 INTRO_ENEMY_COUNT,                 2,},
    {Nametable_BirdClimb_bottom_rle,             Nametable_BirdClimb_top_rle,             GrassPalette,        Sprites_BirdClimb,             BIRDCLIMB_ENEMY_COUNT,             2,},
    {Nametable_ArrowClimb_bottom_rle,            Nametable_ArrowClimb_top_rle,            GrassPalette,        Sprites_ArrowClimb,            ARROWCLIMB_ENEMY_COUNT,            2,},
    {Nametable_TwoBirdClimb_bottom_rle,          Nametable_TwoBirdClimb_top_rle,          GrassPalette,        Sprites_TwoBirdClimb,          TWOBIRDCLIMB_ENEMY_COUNT,          2,},
    {Nametable_IceStairs_bottom_rle,             Nametable_IceStairs_top_rle,             IcePalette,          Sprites_IceStairs,             ICESTAIRS_ENEMY_COUNT,             2,},
    {Nametable_KeyRescue_bottom_rle,             Nametable_KeyRescue_top_rle,             IcePalette,          Sprites_KeyRescue,             KEYRESCUE_ENEMY_COUNT,             2,},
    {Nametable_IceRun_bottom_rle,                Nametable_IceRun_top_rle,                IcePalette,          Sprites_IceRun,                ICERUN_ENEMY_COUNT,                2,},
    {Nametable_FirstIce_bottom_rle,              Nametable_FirstIce_top_rle,              IcePalette,          Sprites_FirstIce,              FIRSTICE_ENEMY_COUNT,              2,},
    {Nametable_ClimbOver_bottom_rle,             Nametable_ClimbOver_top_rle,             IcePalette,          Sprites_ClimbOver,             CLIMBOVER_ENEMY_COUNT,             2,},
	{Nametable_SnakeClimb_bottom_rle,            Nametable_SnakeClimb_top_rle,            CastlePalette,       Sprites_SnakeClimb,            SNAKECLIMB_ENEMY_COUNT,            2,},
	{Nametable_LevelUpAndDown_bottom_rle,        Nametable_LevelUpAndDown_top_rle,        CastlePalette,       Sprites_LevelUpAndDown,        LEVELUPANDDOWN_ENEMY_COUNT,        2,},
	{Nametable_LevelOutfacingShelves_bottom_rle, Nametable_LevelOutfacingShelves_top_rle, CastlePalette,       Sprites_LevelOutfacingShelves, LEVELOUTFACINGSHELVES_ENEMY_COUNT, 2,},
	{Nametable_LevelBackAndForth_bottom_rle,     Nametable_LevelBackAndForth_top_rle,     CastlePalette,       Sprites_LevelBackAndForth,     LEVELBACKANDFORTH_ENEMY_COUNT,     2,},
	{Nametable_CastleColumns_bottom_rle,         Nametable_CastleColumns_top_rle,         CastlePalette,       Sprites_CastleColumns,         CASTLECOLUMNS_ENEMY_COUNT,         2,},
	{Nametable_RaveTwoTowers_bottom_rle,         Nametable_RaveTwoTowers_top_rle,         CastlePalette,       Sprites_RaveTwoTowers,         RAVETWOTOWERS_ENEMY_COUNT,         2,},
	{Nametable_RaveSnakeStairs_bottom_rle,       Nametable_RaveSnakeStairs_top_rle,       CastlePalette,       Sprites_RaveSnakeStairs,       RAVESNAKESTAIRS_ENEMY_COUNT,       2,},
	{Nametable_RaveSmallGaps_bottom_rle,         Nametable_RaveSmallGaps_top_rle,         CastlePalette,       Sprites_RaveSmallGaps,         RAVESMALLGAPS_ENEMY_COUNT,         2,},
	{Nametable_RavePit_bottom_rle,               Nametable_RavePit_top_rle,               CastlePalette,       Sprites_RavePit,               RAVEPIT_ENEMY_COUNT,               2,},
	{Nametable_FirstRave_bottom_rle,             Nametable_FirstRave_top_rle,             CastlePalette,       Sprites_FirstRave,             FIRSTRAVE_ENEMY_COUNT,             2,},
    {Nametable_EndingScreen_bottom_rle,          0,                                       CastlePalette,       0,                             0,                                 0,},
};

level_additional_properties_t LevelProperties[NUM_LEVELS] = {
    {0x10, 0xBF, 0},
    {0x10, 0xBF, 1},
    {0x10, 0xBF, 1},
    {0x10, 0xBF, 1},
    {0x10, 0xBF, 1},
    {0x10, 0xBF, 1},
    {0x10, 0xBF, 1},
    {0x78, 0xBF, 1},
    {0x78, 0xBF, 1},
    {0x10, 0xBF, 2},
    {0x80, 0x4F, 2},
    {0x10, 0xBF, 2},
    {0x20, 0xBF, 2},
    {0x10, 0xBF, 2},
    {0x20, 0xCF, 3},
    {0x10, 0xCF, 3},
    {0x10, 0xCF, 3},
    {0x10, 0xCF, 3},
    {0xE0, 0xCF, 3},
    {0x10, 0x9F, 4},
    {0x70, 0xBF, 4},
    {0x10, 0xBF, 4},
    {0x10, 0xBF, 4},
    {0x10, 0xBF, 4},
    {0x10, 0xCF, 0},
};

#define TONGUE_SOUND_ID     0
#define TONGUE_SOUND_LENGTH 12
unsigned char tongueSound400C[TONGUE_SOUND_LENGTH] = {0x39, 0x37, 0x35, 0x34, 0x34, 0x35, 0x37, 0x39, 0x30, 0x30, 0x3F, 0x30};
unsigned char tongueSound400E[TONGUE_SOUND_LENGTH] = {0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0A, 0x08, 0x05, 0x00, 0x00, 0x09, 0x00};

#define DAMAGE_SOUND_ID     1
#define DAMAGE_SOUND_LENGTH 8
unsigned char damageSound400C[DAMAGE_SOUND_LENGTH] = {0x3F, 0x30, 0x3C, 0x3A, 0x36, 0x3A, 0x3C, 0x30};
unsigned char damageSound400E[DAMAGE_SOUND_LENGTH] = {0x0F, 0x08, 0x09, 0x0B, 0x0D, 0x0E, 0x0F, 0x0E};

#define HOP_SOUND_ID     2
#define HOP_SOUND_LENGTH 6
unsigned char hopSound4008[HOP_SOUND_LENGTH] = {0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
unsigned char hopSound400A[HOP_SOUND_LENGTH] = {0x80, 0x5C, 0x3A, 0xDF, 0x67, 0x67};
unsigned char hopSound400B[HOP_SOUND_LENGTH] = {0x02, 0x02, 0x02, 0x01, 0x01, 0x01};

#define JUMP_SOUND_ID     3
#define JUMP_SOUND_LENGTH 6
unsigned char jumpSound4008[JUMP_SOUND_LENGTH] = {0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
unsigned char jumpSound400A[JUMP_SOUND_LENGTH] = {0xA8, 0x93, 0x7C, 0x3F, 0xEF, 0xEF};
unsigned char jumpSound400B[JUMP_SOUND_LENGTH] = {0x01, 0x01, 0x01, 0x01, 0x00, 0x00};

#define PAUSE_SOUND_ID     4
#define PAUSE_SOUND_LENGTH 12
unsigned char pauseSound4008[PAUSE_SOUND_LENGTH] = {0x81, 0x81, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x81, 0x81};
unsigned char pauseSound400A[PAUSE_SOUND_LENGTH] = {0x23, 0x31, 0x5E, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0x23, 0x31, 0x5E};
unsigned char pauseSound400B[PAUSE_SOUND_LENGTH] = {0x00, 0x00, 0x00, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x00, 0x00, 0x00};

#define ITEM_SOUND_ID     5
#define ITEM_SOUND_LENGTH 12
unsigned char itemSound4000[ITEM_SOUND_LENGTH] = {0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F};
unsigned char itemSound4001[ITEM_SOUND_LENGTH] = {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};
unsigned char itemSound4002[ITEM_SOUND_LENGTH] = {0x6A, 0x6A, 0x6A, 0x59, 0x6A, 0x4B, 0x59, 0x3F, 0x4B, 0x34, 0x34, 0x34};

#define PORTAL_SOUND_ID     6
#define PORTAL_SOUND_LENGTH 12
#define PORTAL_NOISE_DELAY  6
unsigned char portalSound4008[PORTAL_SOUND_LENGTH] = {0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81};
unsigned char portalSound400A[PORTAL_SOUND_LENGTH] = {0x80, 0x5C, 0x3A, 0xDF, 0x67, 0x67, 0xC4, 0x80, 0xDF, 0xBF, 0xFB, 0xF3};
unsigned char portalSound400B[PORTAL_SOUND_LENGTH] = {0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x03, 0x01, 0x05};
unsigned char portalSound400C[PORTAL_SOUND_LENGTH] = {0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F};
unsigned char portalSound400E[PORTAL_SOUND_LENGTH] = {0x0C, 0x09, 0x07, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x01, 0x01, 0x01, 0x01};

#define MAX_FROG_SPEED 8

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

#pragma bss-name (push, "ZEROPAGE")
static sprites_t            gSpriteTable;
static unsigned char        gSpriteState[MAX_NUM_SPRITES];
static unsigned char        gSpriteOffset[MAX_NUM_SPRITES];
static unsigned char*       gScratchPointer2;
static unsigned char        gController1;
static unsigned char        gPrevController1;
static unsigned char        gPrevController1Change;
static unsigned char        gFrameCounter;
static unsigned char        gTmpX;
static unsigned char        gTmpX2;
static unsigned char        gX;
static unsigned char        gY;
static unsigned long        gXScroll;
static unsigned char        gYScroll;
static unsigned char        gYPrevScroll;
static unsigned char        gYNametable;
static unsigned char        gYPrevNametable;
static unsigned char        devnull;
static unsigned char        i;
static unsigned char        j;
static unsigned char        k;
static unsigned char        gTmp;
static unsigned char        gTmp2;
static unsigned char        gTmp3;
static unsigned char        gTmp4;
static unsigned char        gTmp5;
static unsigned char        gTmp6;
static unsigned char        gTmp7;
static unsigned char        gTmp8;
static unsigned char        gTmp9;
static unsigned char        x1;
static unsigned char        y1;
static unsigned char        width1;
static unsigned char        height1;
static unsigned char        x2;
static unsigned char        y2;
static unsigned char        width2;
static unsigned char        height2;
static unsigned char        gVelocity;
static FrogAnimationState_t gFrogAnimationState;

#pragma bss-name (pop)

#pragma bss-name (push, "BSS")

// 16 x (15 + 15)
// TODO should use a bit per block instead of byte, but this is a lot easier at the moment
unsigned char collision[496];


static unsigned char        numKeys;
static unsigned char        gCollisionRight;
static unsigned char        gJumping; // 0 if not currently in the air from a jump, 1 if yes
static unsigned char        gSpeed;
static unsigned char        gSpeedDirection;
static TongueState_t        gTongueState;
static unsigned char        gTongueCounter;
static unsigned char        gBirdMovement;
static unsigned char        gSnakeMovement;
static unsigned char        gVelocityDirection;
static unsigned char        gSpeedCounter;
static unsigned char        gVelocityCounter;

// These are probably overkill, but it makes collision detection a lot cleaner
static unsigned char        gStage;
static unsigned char        gWorld;
static unsigned char        gTmpWorld;
static unsigned char        gCounter;
static unsigned char        gHealth;
static unsigned char        gIframes;
static unsigned char        gNumSprites;
static GameState_t          gGameState;
static unsigned char        gFade;
static unsigned char        gLives;
static unsigned char        gDisplayLives;
static const unsigned char* gScratchPointer;
static unsigned char        gVblankPrevious;
static unsigned char        gPpuCtrlBase;
static unsigned char        gTitleScreenColor;
static unsigned char        gMusicOn;
static unsigned char        gSoundEffectCounter;
static const unsigned char* gSoundEffect0;
static const unsigned char* gSoundEffect1;
static const unsigned char* gSoundEffect2;
static unsigned char        gSoundEffectLength;
static unsigned char        gCurrentSoundEffect;
static unsigned char        gCurrentMusic;
static unsigned char        gCurrentMusicTmp;
static unsigned char        gColorTimer;
static unsigned char        gColorTimer2;
static unsigned char        gColorTimerLimit;
static unsigned char        gTmpDirection;
static unsigned char        gContinue;
static unsigned char        gMiniFrogCount;
static unsigned char        gMiniFrogCollected;
extern unsigned char        gVblank;

extern void pMusicInit(unsigned char);
extern void pMusicPlay(void);

void __fastcall__ UnRLE(const unsigned char *data);
void __fastcall__ Title_UnRLE(const unsigned char *data);


#pragma bss-name (pop)

void vblank(void)
{
    if( gMusicOn == 1 )
    {
        pMusicPlay();
        PlaySoundEffects();
    }
    gVblankPrevious = gVblank;
    while(gVblank == gVblankPrevious);
}

void vblank_counter(void)
{
    for( i = 0; i < gCounter; i++ )
    {
        vblank();
    }
}

void PlaySoundEffects(void)
{
    if( gSoundEffectCounter != 0xFF )
    {
        switch( gCurrentSoundEffect )
        {
            case TONGUE_SOUND_ID:
                gSoundEffect0 = tongueSound400C;
                gSoundEffect1 = tongueSound400E;
                gSoundEffectLength = TONGUE_SOUND_LENGTH;
                break;

            case DAMAGE_SOUND_ID:
            default:
                gSoundEffect0 = damageSound400C;
                gSoundEffect1 = damageSound400E;
                gSoundEffectLength = DAMAGE_SOUND_LENGTH;
                break;

            case HOP_SOUND_ID:
                gSoundEffect0 = hopSound4008;
                gSoundEffect1 = hopSound400A;
                gSoundEffect2 = hopSound400B;
                gSoundEffectLength = HOP_SOUND_LENGTH;
                break;

            case JUMP_SOUND_ID:
                gSoundEffect0 = jumpSound4008;
                gSoundEffect1 = jumpSound400A;
                gSoundEffect2 = jumpSound400B;
                gSoundEffectLength = JUMP_SOUND_LENGTH;
                break;

            case PAUSE_SOUND_ID:
                gSoundEffect0 = pauseSound4008;
                gSoundEffect1 = pauseSound400A;
                gSoundEffect2 = pauseSound400B;
                gSoundEffectLength = PAUSE_SOUND_LENGTH;
                break;

            case ITEM_SOUND_ID:
                *((unsigned char*)0x4000) = itemSound4000[gSoundEffectCounter];
                *((unsigned char*)0x4001) = itemSound4001[gSoundEffectCounter];
                *((unsigned char*)0x4002) = itemSound4002[gSoundEffectCounter];
                *((unsigned char*)0x4003) = 0;
                gSoundEffectLength = ITEM_SOUND_LENGTH;
                break;

            case PORTAL_SOUND_ID:
                *((unsigned char*)0x4008) = portalSound4008[gSoundEffectCounter];
                *((unsigned char*)0x400A) = portalSound400A[gSoundEffectCounter];
                *((unsigned char*)0x400B) = portalSound400B[gSoundEffectCounter];
                if( gSoundEffectCounter >= PORTAL_NOISE_DELAY )
                {
                    *((unsigned char*)0x400C) = portalSound400C[gSoundEffectCounter - PORTAL_NOISE_DELAY];
                    *((unsigned char*)0x400E) = portalSound400E[gSoundEffectCounter - PORTAL_NOISE_DELAY];
                    *((unsigned char*)0x400F) = 0;
                }
                gSoundEffectLength = ITEM_SOUND_LENGTH + PORTAL_NOISE_DELAY;
                break;
        }

        if( gCurrentSoundEffect < 2 )
        {
            *((unsigned char*)0x400C) = gSoundEffect0[gSoundEffectCounter];
            *((unsigned char*)0x400E) = gSoundEffect1[gSoundEffectCounter];
            *((unsigned char*)0x400F) = 0;
        }
        else if( gCurrentSoundEffect < 5 )
        {
            *((unsigned char*)0x4008) = gSoundEffect0[gSoundEffectCounter];
            *((unsigned char*)0x400A) = gSoundEffect1[gSoundEffectCounter];
            *((unsigned char*)0x400B) = gSoundEffect2[gSoundEffectCounter];
        }

        gSoundEffectCounter++;

        if(gSoundEffectCounter == gSoundEffectLength)
        {
            gSoundEffectCounter = 0xFF;
        }
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
    PPU_CTRL = 0x80;
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
    for( i = 0; i < 255; i++ )
    {
        sprites[i] = 0x00;
    }
    sprites[255] = 0x00;

    gScratchPointer2 = (unsigned char*)sprites;
    for(gTmp9 = 0; gTmp9 < (MAX_NUM_SPRITES * SPRITES_T_MEMBER_COUNT); gTmp9++)
    {
        gScratchPointer2 = 0;
        gScratchPointer2++;
    }
}

void LoadSprites(void)
{
    numKeys = 0;
    for(gTmp2 = 0; gTmp2 < gNumSprites; gTmp2++)
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
        gSpriteTable.direction[gTmp2] =      gTmp;

        if( gSpriteTable.id[gTmp2] == SNAKE_ID && ((gSpriteTable.direction[gTmp2] & 0x10) == 0x10))
        {
            gSpriteTable.direction[gTmp2] = gSpriteTable.direction[gTmp2] | 0x20;
        }

        gSpriteState[gTmp2] = 0;

        if( gTmp2 == 0 )
        {
            gSpriteOffset[gTmp2] = 0;
        }
        else
        {
            gSpriteOffset[gTmp2] = gSpriteOffset[gTmp2-1] + (SpriteSize[gSpriteTable.id[gTmp2-1]] << 2);
        }

        if(gSpriteTable.id[gTmp2] == KEY_ID) {
          numKeys++;
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

void load_palette(void)
{
    if( gDisplayLives == 1 || LevelProperties[gStage].world != 4 )
    {
        SET_COLOR(BACKGROUND0_0, gScratchPointer[0]);
        SET_COLOR(BACKGROUND1_0, gScratchPointer[4]);
        SET_COLOR(BACKGROUND2_0, gScratchPointer[7]);
        SET_COLOR(BACKGROUND3_0, gScratchPointer[10]);

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
    else
    {
        SET_COLOR(BACKGROUND0_0, BLACK);
        SET_COLOR(BACKGROUND0_1, BLACK);
        SET_COLOR(BACKGROUND0_2, DARK_GRAY);
        SET_COLOR(BACKGROUND0_3, BLACK);
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

    PPU_CTRL = gPpuCtrlBase + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 2;
    load_palette();

    PPU_CTRL = gPpuCtrlBase + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 3;
    load_palette();

    PPU_CTRL = gPpuCtrlBase + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    PPU_CTRL = 0x80;
    PPU_MASK = 0x00;
    set_scroll();
}

void fade_in(void)
{
    gFade = 2;
    load_palette();

    PPU_CTRL = gPpuCtrlBase + gYNametable;
    PPU_MASK = 0x0E;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 1;
    load_palette();

    PPU_CTRL = gPpuCtrlBase + gYNametable;
    set_scroll();

    gCounter = 20;
    vblank_counter();

    gFade = 0;
    load_palette();

    PPU_CTRL = gPpuCtrlBase + gYNametable;
    if( gStage == 0 || gStage > (NUM_LEVELS-2) || gDisplayLives == 1 || gContinue == 1 )
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
    if( ((gController1 & (BUTTON_RIGHT | BUTTON_LEFT)) != 0) && (gJumping == 0) )
    {
      gJumping = 1;
      gVelocity = 6;
      gVelocityDirection = 1;

      if( gSoundEffectCounter == 0xFF )
      {
          gCurrentSoundEffect = HOP_SOUND_ID;
          gSoundEffectCounter = 0;
      }

      //pMusicInit(11);
    }
}

void big_jump(void)
{
    if(gJumping == 0 || (gJumping == 1 && gVelocityDirection == 1)) {
      gJumping = 2;
      gVelocity = 13;
      gVelocityDirection = 1;
      gPrevController1Change |= BUTTON_A;

      if( gSoundEffectCounter == 0xFF )
      {
          gCurrentSoundEffect = JUMP_SOUND_ID;
          gSoundEffectCounter = 0;
      }
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
                    sprites[75] = gX + 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 23;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 7;
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
                    sprites[75] = gX + 15;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 7;
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
                    sprites[75] = gX + 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 23;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_1;
                    sprites[82] = 0x02;
                    sprites[83] = gX + 31;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 23;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 15;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_0;
                    sprites[82] = 0x42;
                    sprites[83] = gX - 7;
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
                    sprites[75] = gX + 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 23;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 7;
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
                    sprites[75] = gX + 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 23;

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
                    sprites[75] = gX - 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 7;

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
                    sprites[75] = gX + 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 23;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_1;
                    sprites[82] = 0x02;
                    sprites[83] = gX + 31;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 23;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 15;

                    sprites[80] = gY + 4;
                    sprites[81] = PATTERN_TONGUE_0;
                    sprites[82] = 0x42;
                    sprites[83] = gX - 7;
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
                    sprites[75] = gX + 15;

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
                    sprites[75] = gX - 7;

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
                    sprites[75] = gX + 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_1;
                    sprites[78] = 0x02;
                    sprites[79] = gX + 23;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 15;

                    sprites[76] = gY + 4;
                    sprites[77] = PATTERN_TONGUE_0;
                    sprites[78] = 0x42;
                    sprites[79] = gX - 7;
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
                    sprites[75] = gX + 15;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 7;
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
                //pMusicInit(6);

                gCurrentSoundEffect = TONGUE_SOUND_ID;
                gSoundEffectCounter = 0;

                gTongueState = TONGUE_EXTENDING;
                gTongueCounter = TONGUE_EXTEND_DELAY;

                if( gSpeedDirection == 1 )
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x02;
                    sprites[75] = gX + 15;
                }
                else
                {
                    sprites[72] = gY + 4;
                    sprites[73] = PATTERN_TONGUE_1;
                    sprites[74] = 0x42;
                    sprites[75] = gX - 7;
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
            if( gSpeed < MAX_FROG_SPEED )
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
            if( gSpeed < MAX_FROG_SPEED )
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
            }
            else
            {
                --gSpeed;
            }
        }
    }
    if( ((gController1 & (BUTTON_RIGHT | BUTTON_LEFT)) == 0) && (gSpeed > 0) )
    {
        // do this for ice physics
        if( (LevelProperties[gStage].world != 2) || (gSpeedCounter == gSpeed) || (gSpeedCounter == 0x04) )
        {
            --gSpeed;
            gSpeedCounter = 0;
        }
        else
        {
            ++gSpeedCounter;
        }
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

    init_physics();

    if( gGameState == TITLE_SCREEN_STATE)
    {
        gPpuCtrlBase |= 0x10; // switch to the 2nd page of patterns for the title screen
    }
    else
    {
        gPpuCtrlBase &= 0xEF; // switch to the 1st page of patterns for all non-title screens
    }

    gYNametable = 2;
    gYScroll = 0;

    if( gDisplayLives == 1 )
    {
        gCurrentMusic = 4;
        pMusicInit(4);

        PPU_ADDRESS = 0x28;
        PPU_ADDRESS = 0x00;
        UnRLE(Nametable_Lives_bottom_rle);	// uncompresses our data

        vblank();

        PPU_ADDRESS = 0x29;
        PPU_ADDRESS = 0xB3;
        PPU_DATA = PATTERN_NUMBERS_0 + LevelProperties[gStage].world;

        // make the frog area green
        PPU_ADDRESS = 0x2B;
        PPU_ADDRESS = 0xE3;
        PPU_DATA = 0xFF;
        PPU_DATA = 0xFF;
        PPU_DATA = 0xFF;
        PPU_DATA = 0xFF;

        PPU_ADDRESS = 0x2A;
        PPU_ADDRESS = 0x0C;
        PPU_DATA = PATTERN_FROG_0;
        PPU_DATA = PATTERN_FROG_1;
        PPU_ADDRESS = 0x2A;
        PPU_ADDRESS = 0x2C;
        PPU_DATA = PATTERN_FROG_2;
        PPU_DATA = PATTERN_FROG_3;

        if( gLives > 0 )
        {
            PPU_ADDRESS = 0x2A;
            PPU_ADDRESS = 0x0F;
            PPU_DATA = PATTERN_FROG_0;
            PPU_DATA = PATTERN_FROG_1;
            PPU_ADDRESS = 0x2A;
            PPU_ADDRESS = 0x2F;
            PPU_DATA = PATTERN_FROG_2;
            PPU_DATA = PATTERN_FROG_3;
        }

        if( gLives > 1 )
        {
            PPU_ADDRESS = 0x2A;
            PPU_ADDRESS = 0x12;
            PPU_DATA = PATTERN_FROG_0;
            PPU_DATA = PATTERN_FROG_1;
            PPU_ADDRESS = 0x2A;
            PPU_ADDRESS = 0x32;
            PPU_DATA = PATTERN_FROG_2;
            PPU_DATA = PATTERN_FROG_3;
        }

        gScratchPointer = CastlePalette;
        load_palette();

        vblank();

        fade_in();

        gCounter = 15;
        vblank_counter();

        PPU_ADDRESS = 0x2A;
        PPU_ADDRESS = 0x0C + gLives + gLives + gLives;
        PPU_DATA = 0;
        set_scroll();

        gCounter = 5;
        vblank_counter();

        PPU_ADDRESS = 0x2A;
        PPU_ADDRESS = 0x0D + gLives + gLives + gLives;
        PPU_DATA = 0;
        set_scroll();

        gCounter = 5;
        vblank_counter();

        PPU_ADDRESS = 0x2A;
        PPU_ADDRESS = 0x2C + gLives + gLives + gLives;
        PPU_DATA = 0;
        set_scroll();

        gCounter = 5;
        vblank_counter();

        PPU_ADDRESS = 0x2A;
        PPU_ADDRESS = 0x2D + gLives + gLives + gLives;
        PPU_DATA = 0;
        set_scroll();

        vblank();

        fade_out();

        gDisplayLives = 0;
    }

    if (LevelTable[gStage].top_rle != 0)
    {
        PPU_ADDRESS = 0x20;
        PPU_ADDRESS = 0x00;
        UnRLE(LevelTable[gStage].top_rle);	// uncompresses our data
        vblank();
    }

    PPU_ADDRESS = 0x28;
    PPU_ADDRESS = 0x00;
    if( gStage == 0 )
    {
        Title_UnRLE(LevelTable[gStage].bottom_rle);	// uncompresses our data
    }
    else
    {
        UnRLE(LevelTable[gStage].bottom_rle);	// uncompresses our data
    }
    vblank();

    gScratchPointer = LevelTable[gStage].palette;
    load_palette();
    vblank();

    if( gGameState == ENDING_STATE)
    {
        // add the number of mini frogs collected
        PPU_ADDRESS = 0x28;
        PPU_ADDRESS = 0xEC;

        if( gMiniFrogCount >= 20 )
        {
            gMiniFrogCount = gMiniFrogCount - 20;
            PPU_DATA = PATTERN_NUMBERS_2;
        }
        else if( gMiniFrogCount >= 10 )
        {
            gMiniFrogCount = gMiniFrogCount - 10;
            PPU_DATA = PATTERN_NUMBERS_1;
        }
        else
        {
            PPU_DATA = PATTERN_NUMBERS_0;
        }
        PPU_DATA = PATTERN_NUMBERS_0 + gMiniFrogCount;
    }

    if( LevelTable[gStage].numSprites != 0 )
    {
        ClearSprites();
        gNumSprites = LevelTable[gStage].numSprites;
    }

    if( LevelTable[gStage].sprites != 0 )
    {
        gScratchPointer2 = (unsigned char*)LevelTable[gStage].sprites;
        LoadSprites();
    }

    gCurrentMusicTmp = LevelTable[gStage].music;
    if( gCurrentMusicTmp != 0 && gCurrentMusicTmp != gCurrentMusic )
    {
        gCurrentMusic = LevelTable[gStage].music;
        pMusicInit(LevelTable[gStage].music);
    }

    loadCollisionFromNametables();

    gX = LevelProperties[gStage].FrogStartX;
    gY = LevelProperties[gStage].FrogStartY;

    gYNametable = 2;
    gVelocity = 0;
    gVelocityDirection = 0;
    gXScroll = 0;
    gYScroll = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gJumping = 0;

    // Frog direction
    sprites[1] = 0x00;
    sprites[2] = 0x00;
    sprites[5] = 0x01;
    sprites[6] = 0x00;
    sprites[9] = 0x02;
    sprites[10] = 0x00;
    sprites[13] = 0x03;
    sprites[14] = 0x00;

    vblank();

    draw_health();

    update_sprites();
    dma_sprites();
    set_scroll();

    vblank();

    vblank();

    fade_in();
}

void next_stage(void)
{
    switch( gStage )
    {
        case NUM_LEVELS-2:
            gGameState = ENDING_STATE;

        default:
            gTongueState = TONGUE_CLEANUP;
            gTongueCounter = 0;
            update_tongue_sprite();

            gTmpWorld = LevelProperties[gStage+1].world;
            if(gWorld != gTmpWorld && gTmpWorld != 0)
            {
                gDisplayLives = 1;
                gWorld = LevelProperties[gStage+1].world;
            }

            ++gStage;
    }
}

void death(void)
{
    gHealth = 8;

    if( gLives == 0 )
    {
        fade_out();

        gCurrentMusic = 4;
        pMusicInit(4);

        PPU_ADDRESS = 0x24;
        PPU_ADDRESS = 0x00;
        UnRLE(Nametable_Lives_top_rle);	// uncompresses our data

        gYNametable = 0;
        PPU_CTRL = gPpuCtrlBase + gYNametable;

        vblank();

        // make the frog area green
        PPU_ADDRESS = 0x27;
        PPU_ADDRESS = 0xDB;
        PPU_DATA = 0x22;
        PPU_ADDRESS = 0x27;
        PPU_ADDRESS = 0xE3;
        PPU_DATA = 0x22;

        gScratchPointer = CastlePalette;
        load_palette();

        vblank();

        gContinue = 1;

        fade_in();

        PPU_ADDRESS = 0x21;
        PPU_ADDRESS = 0xAC;
        PPU_DATA = PATTERN_MINI_FROG_0;
        gXScroll = 0;
        gYScroll = 0;
        gYNametable = 0;
        PPU_CTRL = gPpuCtrlBase + gYNametable;
        set_scroll();

        while(1)
        {
            vblank();
            input_poll();

            if(gController1 == gPrevController1)
            {
                continue;
            }

            if((gController1 & BUTTON_START) == BUTTON_START)
            {
                if(gContinue == 1)
                {
                    while(LevelProperties[gStage-1].world == gWorld)
                    {
                        --gStage;
                    }
                    gMiniFrogCount = 0;
                    gLives = 2;
                    gDisplayLives = 1;
                }
                else
                {
                    gDisplayLives = 0;

                    gStage = 0;
                    gGameState = TITLE_SCREEN_STATE;
                }
                gContinue = 0;
                break;
            }

            if(gController1 != 0)
            {
                gContinue = gContinue ^ 1;

                if(gContinue == 1)
                {
                    PPU_ADDRESS = 0x21;
                    PPU_ADDRESS = 0xAC;
                }
                else
                {
                    PPU_ADDRESS = 0x22;
                    PPU_ADDRESS = 0x2C;
                }
                PPU_DATA = PATTERN_MINI_FROG_0;

                if(gContinue == 1)
                {
                    PPU_ADDRESS = 0x22;
                    PPU_ADDRESS = 0x2C;
                }
                else
                {
                    PPU_ADDRESS = 0x21;
                    PPU_ADDRESS = 0xAC;
                }
                PPU_DATA = 0;

                set_scroll();
            }
        }

        //fade_out();

    }
    else
    {
        if( gMiniFrogCollected == 1 )
        {
            --gMiniFrogCount;
        }
        gLives--;
        gDisplayLives = 1;
    }

    draw_health();
    init_physics();
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
  gCollisionRight = x1 + width1;
  if( gYNametable == 2 )
  {
      if( collision[240 + (((y1)&0xF0)) + (x1 >> 4)] == 0 &&
          collision[240 + (((y1 + height1)&0xF0)) + (x1 >> 4)] == 0 &&
          collision[240 + (((y1)&0xF0)) + (gCollisionRight >> 4)] == 0 &&
          collision[240 + (((y1 + height1)&0xF0)) + (gCollisionRight >> 4)] == 0)
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
              collision[240 + (((gYScroll + y1 - 0xF0) & 0xF0) ) + (gCollisionRight >> 4)] == 0 &&
              collision[240 + (((gYScroll + y1 + height1 - 0xF0) & 0xF0) ) + (gCollisionRight >> 4)] == 0 )
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
              collision[(((gYScroll + y1) & 0xF0) ) + (gCollisionRight >> 4)] == 0 &&
              collision[(((gYScroll + y1 + height1) & 0xF0) ) + (gCollisionRight >> 4)] == 0 )
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

    case PORTAL_ID:
      x2 = sprites[j + 3];
      y2 = sprites[j];
      width2 = 16;
      height2 = 16;
      break;

    case MINI_FROG_ID:
    case HEART_ID:
    case KEY_ID:
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
        gCurrentSoundEffect = DAMAGE_SOUND_ID;
        gSoundEffectCounter = 0;

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
          gSpeed = MAX_FROG_SPEED;
          gSpeedDirection ^= 1;
      }
  }
}

/**
 * Depends on:
 * j as an index into sprites
 * i as an index into gSpriteTable
 * Overwrites:
 * gTmp2
 * gTmp8
 */
void spawn_1_by_1_sprite(void)
{
    sprites[j] = gTmp8;
    gTmp2 = spriteProperties[gSpriteTable.id[i]].pattern;
    sprites[j + 1] = gTmp2;
    sprites[j + 2] = gSpriteTable.direction[i];
    if(sprites[j + 3] == 0) {
      gTmp8 = gSpriteTable.startX[i];
      sprites[j + 3] = gTmp8;
    }
}

/**
 * Depends on:
 * j as an index into sprites
 */
void despawn_1_sprite(void)
{
    sprites[j] = 0;
    sprites[j + 1] = 0;
    sprites[j + 2] = 0;
    sprites[j + 3] = 0;
}

/**
 * Depends on:
 * j as an index into sprites
 * i as an index into gSpriteTable
 * Overwrites:
 * gTmp2
 * gTmp8
 */
void spawn_2_by_1_sprite(void)
{
    if( gSpriteState[i] != 0xFF )
    {
        if( gSpriteState[i] != 0 )
        {
            --gSpriteState[i];
        }
        else
        {
            if( gSpriteTable.id[i] == SNAKE_ID )
            {
                gTmpDirection = ((gSpriteTable.direction[i] & 0x20) >> 1);
                gSpriteTable.direction[i] = gSpriteTable.direction[i] & 0xEF;
                gSpriteTable.direction[i] = gSpriteTable.direction[i] | gTmpDirection;
            }

            sprites[j] = gTmp8;
            sprites[j + 4] = gTmp8;
            gTmp2 = spriteProperties[gSpriteTable.id[i]].pattern;
            if((gSpriteTable.direction[i] & 0x10) == 0x10 )
            {
                sprites[j + 5] = gTmp2;
                sprites[j + 1] = gTmp2 + 1;
                sprites[j + 2] = gSpriteTable.direction[i] | 0x40;
                sprites[j + 6] = gSpriteTable.direction[i] | 0x40;
            }
            else
            {
                sprites[j + 1] = gTmp2;
                sprites[j + 5] = gTmp2 + 1;
                sprites[j + 2] = gSpriteTable.direction[i];
                sprites[j + 6] = gSpriteTable.direction[i];
            }
            if(sprites[j + 3] == 0) {
              gTmp8 = gSpriteTable.startX[i];
              sprites[j + 3] = gTmp8;
              gTmp8 = gSpriteTable.startX[i] + 0x8;
              sprites[j + 7] = gTmp8;
            }
        }
    }
}

/**
 * Depends on:
 * j as an index into sprites
 */
void despawn_2_sprite(void)
{
    sprites[j] = 0;
    sprites[j + 1] = 0;
    sprites[j + 2] = 0;
    sprites[j + 3] = 0;
    sprites[j + 4] = 0;
    sprites[j + 5] = 0;
    sprites[j + 6] = 0;
    sprites[j + 7] = 0;
}

void spawn_portal_sprite(void)
{
    if( numKeys == 0 )
    {
        sprites[j] = gTmp8;
        sprites[j + 4] = gTmp8;
        sprites[j + 8] = gTmp8 + 8;
        sprites[j + 12] = gTmp8 + 8;

        gTmp2 = spriteProperties[gSpriteTable.id[i]].pattern;
        sprites[j + 1] = gTmp2;
        sprites[j + 5] = gTmp2;
        sprites[j + 9] = gTmp2;
        sprites[j + 13] = gTmp2;

        sprites[j + 2] = 0x00;
        sprites[j + 6] = 0x40;
        sprites[j + 10] = 0x80;
        sprites[j + 14] = 0xC0;

        gTmp8 = gSpriteTable.startX[i];
        sprites[j + 3] = gTmp8;
        sprites[j + 11] = gTmp8;
        gTmp8 = gSpriteTable.startX[i] + 0x8;
        sprites[j + 7] = gTmp8;
        sprites[j + 15] = gTmp8;
    }
}

void despawn_portal_sprite(void)
{
    sprites[j] = 0;
    sprites[j + 1] = 0;
    sprites[j + 2] = 0;
    sprites[j + 3] = 0;
    sprites[j + 4] = 0;
    sprites[j + 5] = 0;
    sprites[j + 6] = 0;
    sprites[j + 7] = 0;
    sprites[j + 8] = 0;
    sprites[j + 9] = 0;
    sprites[j + 10] = 0;
    sprites[j + 11] = 0;
    sprites[j + 12] = 0;
    sprites[j + 13] = 0;
    sprites[j + 14] = 0;
    sprites[j + 15] = 0;
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
            gTmp8 = gTmp4 - gYScroll - 0x10;
            spriteProperties[gSpriteTable.id[i]].spawn();
        }
        else {
          //moved offscreen so kill it
          spriteProperties[gSpriteTable.id[i]].despawn();
        }
      }
      else {
        gTmp8 = gSpriteTable.startY[i];
        spriteProperties[gSpriteTable.id[i]].spawn();
      }
    }
    else {
      //top half
      if(gYNametable == 0) {
        gTmp4 = gSpriteTable.startY[i];
        if(gYScroll < gTmp4) {
          gTmp8 = gSpriteTable.startY[i] - gYScroll;
          spriteProperties[gSpriteTable.id[i]].spawn();
        }
        else {
          spriteProperties[gSpriteTable.id[i]].despawn();
          gSpriteState[i] = 0;
        }
      }
      else {
        ////The way we're doing scrolling, for nametable 0, we'll never have a scroll value
        spriteProperties[gSpriteTable.id[i]].despawn();
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

  gTmp5 = SpriteSize[gSpriteTable.id[i]];
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
    if(gSpriteState[i] == 2 || gSpriteState[i] == 1)
    {
        //Arrow is in flight
        //if arrow is just long enough to stick into a wall
        x1 = sprites[j + 3];
        y1 = sprites[j] + 4;
        height1 = 2;
        width1 = 8; //Don't count the tip
        if(gSpriteState[i] == 2 && is_background_collision())
        {
          gSpriteState[i] = 3;
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
              gSpriteState[i] = 2;
            }
          }
          else {
            //moving left
            sprites[j + 3] -= ARROW_SPEED;
            //Doing this in line didn't work
            gTmp4 = gSpriteTable.startX[i];
            gTmp4 -= sprites[j + 3];

            if(gTmp4 > 8) {
              gSpriteState[i] = 2;
            }
          }
        }
    }
    else if(gSpriteState[i] == 0)
    {
      //Arrow isn't moving/waiting for trigger
      if(sprites[j] >= sprites[0] && sprites[j] <= sprites[8])
      {
        //Start moving if frog becomes in path
        gSpriteState[i] = 1;
      }
    }
    else
    {
        //Arrow is stuck in wall
        gTmp4 = gSpriteState[i];
        gSpriteState[i] = gTmp4 + 1;
        //Check if its been stuck long enough to respawn
        if(gSpriteState[i] > 120) {
          gSpriteState[i] = 0;
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
  gBirdMovement = 0;

  if(sprites[j+1] != 0) {
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
            width1 = 15; //Don't count the tip
            if(is_background_collision()) {
              sprites[j] -= 1;
              sprites[j+4] -= 1;
            }
        }
        else
        {
            sprites[j] -= 1;
            sprites[j+4] -= 1;

            if((gFrameCounter & 0x8) != 0)
            {
                gBirdMovement = 1;
            }

            x1 = sprites[j + 3];
            y1 = sprites[j] + 1;
            height1 = 8;
            width1 = 15; //Don't count the tip
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
            width1 = 15; //Don't count the tip
            if(is_background_collision()) {
              sprites[j+3] -= 1;
              sprites[j+7] -= 1;
            }

            gBirdMovement += 0x10;

        }
        else
        {
            // X
            sprites[j+3] -= 1;
            sprites[j+7] -= 1;

            x1 = sprites[j + 3];
            y1 = sprites[j] + 1;
            height1 = 8;
            width1 = 15; //Don't count the tip
            if(is_background_collision()) {
              sprites[j+3] += 1;
              sprites[j+7] += 1;
            }
        }
    }

    // Tiles
    if( (gBirdMovement & 0x10) == 0x10 )
    {
        if( (gBirdMovement & 1) == 1)
        {
            sprites[j+1] = PATTERN_BIRD2_1;
        }
        else
        {
            sprites[j+1] = PATTERN_BIRD_1;
        }
        sprites[j+2] = 0x41;
        sprites[j+5] = PATTERN_BIRD_0;
        sprites[j+6] = 0x41;
    }
    else
    {
        sprites[j+1] = PATTERN_BIRD_0;
        sprites[j+2] = 0x01;
        if( (gBirdMovement & 1) == 1)
        {
            sprites[j+5] = PATTERN_BIRD2_1;
        }
        else
        {
            sprites[j+5] = PATTERN_BIRD_1;
        }
        sprites[j+6] = 0x01;
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

void snake_ai_handler(void)
{
  gSnakeMovement = 0;

  if(gSpriteState[i] == 0xFF)
  {
      despawn_2_sprite();
  }

  if(sprites[j+1] != 0)
  {
    sprite_maintain_y_position();

    gTmp = gSpriteState[i];

    (*(unsigned char*) 0x2F0) = gSpriteState[i];

    //Update Y
    sprites[j] = sprites[j] +     gTmp;
    sprites[j+4] = sprites[j+4] + gTmp;

    if((gFrameCounter & 0x8) != 0)
    {
        gSnakeMovement = 1;
    }

    x1 = sprites[j + 3];
    y1 = sprites[j];
    height1 = 8;
    width1 = 15; //Don't count the tip
    if(is_background_collision()) {
      sprites[j] =   sprites[j] -   gTmp;
      sprites[j+4] = sprites[j+4] - gTmp;

      // reset the snake falling speed
      gSpriteState[i] = 1;
    }
    else
    {
        if( gSpriteState[i] < 15 && (gFrameCounter & 1) == 0 )
        {
            ++gSpriteState[i];
        }
    }

    //Update X
    if( (gSpriteTable.direction[i] & 0x10) == 0x10 )
    {
        sprites[j+3] += 1;
        sprites[j+7] += 1;

        x1 = sprites[j + 3];
        y1 = sprites[j];
        height1 = 8;
        width1 = 15; //Don't count the tip
        if(is_background_collision()) {
          sprites[j+3] -= 1;
          sprites[j+7] -= 1;
          gSpriteTable.direction[i] ^= 0x10;
        }

        gSnakeMovement += 0x10;
    }
    else
    {
        sprites[j+3] = sprites[j+3] - 1;
        sprites[j+7] = sprites[j+7] - 1;

        x1 = sprites[j + 3];
        y1 = sprites[j];
        height1 = 8;
        width1 = 15; //Don't count the tip
        if(is_background_collision()) {
          sprites[j+3] += 1;
          sprites[j+7] += 1;
          gSpriteTable.direction[i] ^= 0x10;
        }
    }

    // Tiles
    if( (gSnakeMovement & 0x10) == 0x10 )
    {
        if( (gSnakeMovement & 1) == 1)
        {
            sprites[j+1] = PATTERN_SNAKE2_1;
        }
        else
        {
            sprites[j+1] = PATTERN_SNAKE_1;
        }
        sprites[j+2] = 0x42;
        sprites[j+5] = PATTERN_SNAKE_0;
        sprites[j+6] = 0x42;
    }
    else
    {
        sprites[j+1] = PATTERN_SNAKE_0;
        sprites[j+2] = 0x02;
        if( (gSnakeMovement & 1) == 1)
        {
            sprites[j+5] = PATTERN_SNAKE2_1;
        }
        else
        {
            sprites[j+5] = PATTERN_SNAKE_1;
        }
        sprites[j+6] = 0x02;
    }
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
  sprites[j] = 0;
  sprites[j + 1] = 0;
  sprites[j + 2] = 0;
  sprites[j + 3] = 0;

  ++gMiniFrogCount;
  gMiniFrogCollected = 1;

  gCurrentSoundEffect = ITEM_SOUND_ID;
  gSoundEffectCounter = 0;
}

void heart_collision_handler(void)
{
  //remove the item
  gSpriteTable.id[i] = INVALID_ID;
  sprites[j] = 0;
  sprites[j + 1] = 0;
  sprites[j + 2] = 0;
  sprites[j + 3] = 0;

  if( gHealth < 8 )
  {
    ++gHealth;
    draw_health();
  }

  gCurrentSoundEffect = ITEM_SOUND_ID;
  gSoundEffectCounter = 0;
}

void invalid_ai_handler(void)
{
}

void invalid_collision_handler(void)
{
}

void portal_collision_handler(void)
{
    gCurrentSoundEffect = PORTAL_SOUND_ID;
    gSoundEffectCounter = 0;

    next_stage();
    load_stage();
}

void key_collision_handler(void)
{
  gSpriteTable.id[i] = INVALID_ID;
  sprites[j] = 0;
  sprites[j + 1] = 0;
  sprites[j + 2] = 0;
  sprites[j + 3] = 0;

  numKeys--;

  gCurrentSoundEffect = ITEM_SOUND_ID;
  gSoundEffectCounter = 0;
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
            gTmpX = gX + 0xE;
            gTmpX2 = gX + 1;

            if( gYNametable == 2 )
            {
                //Bottom half of level
                if( collision[240 + (((gY)&0xF0) ) + ((gTmpX2) >> 4)] == 0 &&
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
                if( collision[240 + (((gYScroll + gY - 0xF0)&0xF0) ) + ((gTmpX2) >> 4)] == 0 &&
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
                if( collision[(((gYScroll + gY - 0x100) & 0xF0) ) + ((gTmpX2) >> 4)] == 0 &&
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
            if((gController1 & BUTTON_A) != BUTTON_A || (gVelocityCounter & 1))
            {
                gVelocity -= 1;
                gVelocityCounter = 0;
            }
            else
            {
                ++gVelocityCounter;
            }
        }
    }
    else // moving down
    {
        for( i = 0; (i<<2) < gVelocity; i++ )
        {
            gTmpX = gX + 0xE;
            gTmpX2 = gX + 1;

            if(gYNametable == 2 )
            {
                if(gY == 0xEF)
                {
                    gController1 = 0; // A may have been buffered which would cause a jump after the respawn
                    death();
                    return;
                }
                //Bottom half of level
                if( collision[240 + (((gY + 0x11)&0xF0) ) + ((gTmpX2) >> 4)] == 0 &&
                    collision[240 + (((gY + 0x11)&0xF0) ) + (gTmpX >> 4)] == 0 )
                {
                    gY += 1;
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
                if( collision[240 + (((gYScroll + gY+0x11 - 0xF0)&0xF0) ) + ((gTmpX2) >> 4)] == 0 &&
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
                if( collision[(((gYScroll + gY+0x11)&0xF0) ) + ((gTmpX2) >> 4)] == 0 &&
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

        if( gVelocity != 0 )
        {
            gJumping = 1;
        }

        if( i == ((gVelocity+3)>>2) )
        {
            gVelocity+=1;
        }
    }

    //Update enemies with movements
    for(i = 0; i < gNumSprites; i++) {
      j = FIRST_ENEMY_SPRITE + gSpriteOffset[i];
      gTmp3 = sprites[j];

      gTmp2 = gSpriteTable.id[i];

      spriteProperties[gTmp2].ai_handler();

      // Check if we scrolled this offscreen
      gTmp7 = sprites[j];
      if((gTmp3 > gTmp7 && (gTmp3 - gTmp7) > 200)
       || (gTmp7 > gTmp3 && (gTmp7 - gTmp3) > 200)
       || (gTmp7 == 0xFF)) {
        spriteProperties[gTmp2].despawn();
      }

      // In case item/enemy needs to be respawned
      spawn_check();

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
          sprites[73] = 0;
          sprites[74] = 0;
          sprites[75] = 0;
          sprites[76] = 0;
          sprites[77] = 0;
          sprites[78] = 0;
          sprites[79] = 0;
          sprites[80] = 0;
          sprites[81] = 0;
          sprites[82] = 0;
          sprites[83] = 0;
          gTongueState = TONGUE_NORMAL;
          gTongueCounter = 0;
          if( gSpriteTable.id[i] == SNAKE_ID )
          {
              gSpriteState[i] = 0xFF; // kill the snake
          }

          if( gSpriteTable.id[i] == BIRD_ID )
          {
              gSpriteState[i] = 0x5A; // set the respawn time to 1.5s
          }

          gTmp = SpriteSize[gSpriteTable.id[i]];
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
      x1 = sprites[3] + 2;
      y1 = sprites[0] + 1;
      width1 = 12;
      height1 = 15;

      put_i_in_collision2_vars();

      if(is_collision())
      {
          spriteProperties[gSpriteTable.id[i]].collision_handler();
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
    gDisplayLives = 0;
    gPpuCtrlBase = 0x94;
    gMusicOn = 0;
    gSoundEffectCounter = 0xFF;
}

void init_physics(void)
{
    gVelocity = 0;
    gVelocityDirection = 0;
    gSpeed = 0;
    gSpeedDirection = 1;
    gJumping = 0;
    gIframes = 0;
    gFrogAnimationState = FROG_NORMAL;
    gTongueState = TONGUE_CLEANUP;
    gTongueCounter = 0;
    gColorTimer = (gHealth << 3);
    gColorTimer2 = 0;
    gTitleScreenColor = 0x11;
    gMiniFrogCollected = 0;
    update_tongue_sprite();
}

void init_game_state(void)
{
    gX = 0x10;
    gY = 0xCF;
    gXScroll = 0;
    gYScroll = 0;
    gYNametable = 2;
    gHealth = 8;
    draw_health();
    init_physics();
    gFade = 3;
    gFrameCounter = 0;
    gLives = 2;
    gDisplayLives = 1;
    gContinue = 0;
    gMiniFrogCount = 0;
}

void game_running_sm(void)
{
    while( gGameState == GAME_RUNNING_STATE )
    {
        vblank();

        gFrameCounter++;

        if( LevelProperties[gStage].world == 4 )
        {
            gColorTimerLimit = (gHealth << 3);
            if( gColorTimer >= gColorTimerLimit )
            {

                // Flashing colors
                //PPU_MASK = 0x00;
                SET_COLOR(BACKGROUND0_1, BLACK);
                SET_COLOR(BACKGROUND0_2, BLACK);
                SET_COLOR(BACKGROUND0_3, BLACK);
                //SET_COLOR((BACKGROUND0_1 + gColorTimer2), (gTitleScreenColor + (gColorTimer2 << 4)));
                SET_COLOR((BACKGROUND0_1 + gColorTimer2), gTitleScreenColor);
                //SET_COLOR((BACKGROUND0_1 + gColorTimer2), gTitleScreenColor);

                gColorTimer = 0;

                if( gColorTimer2 == 2 )
                {
                    gTitleScreenColor++;
                    if( gTitleScreenColor == 0x1D )
                    {
                        gTitleScreenColor = 0x11;
                    }

                    gColorTimer2 = 0;
                }
                else
                {
                    ++gColorTimer2;
                }
            }
            else
            {
                ++gColorTimer;
            }
        }

        input_poll();

        if((gController1 & BUTTON_START) == BUTTON_START)
        {
            (*(unsigned char*) 0x4015) = 0xC; // turn off the square wave channels

            gCurrentSoundEffect = PAUSE_SOUND_ID;
            gSoundEffectCounter = 0;

            //Paused so wait until button is released and then pushed again
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

            gCurrentSoundEffect = PAUSE_SOUND_ID;
            gSoundEffectCounter = 0;

            (*(unsigned char*) 0x4015) = 0xF; // restore off the square wave channels
        }

        update_sprites();

        dma_sprites();

        // set bits [1:0] to 0 for nametable
        PPU_CTRL = gPpuCtrlBase + gYNametable;
        set_scroll();

        do_physics();
    }
}

void title_screen_sm(void)
{
    //gTitleScreenColor = TitleScreenPalette[4];
    gTitleScreenColor = 0x21;
    gFrameCounter = 0;

    while( gGameState == TITLE_SCREEN_STATE )
    {
        vblank();

        // Flashing colors
        PPU_MASK = 0x00;
        SET_COLOR(BACKGROUND2_3, gTitleScreenColor);

        if( (gFrameCounter & 0x40) == 0 )
        {
            SET_COLOR(BACKGROUND1_1, LIGHT_TAN);
        }
        else
        {
            SET_COLOR(BACKGROUND1_1, TAN);
        }
        PPU_MASK = 0x0E;

        PPU_ADDRESS = 0x28;
        PPU_ADDRESS = 0x00;
        set_scroll();

        if( (gFrameCounter & 0x1) == 0 )
        {
            gTitleScreenColor++;
            if( gTitleScreenColor == 0x2C )
            {
                gTitleScreenColor = 0x21;
            }
        }


        input_poll();

        // set bits [1:0] to 0 for nametable
        PPU_CTRL = gPpuCtrlBase + gYNametable;
        gYScroll = 0;
        set_scroll();

        gFrameCounter++;

        if( (gController1 & BUTTON_START) != 0 )
        {
            gStage = 1;
            gWorld = 1;
            gGameState = GAME_RUNNING_STATE;
            init_game_state();
            load_stage();
        }
    }
}

void end_screen_sm(void)
{
    gCurrentMusic = 3;
    pMusicInit(3);

    while( gGameState == ENDING_STATE )
    {
        vblank();

        input_poll();

        // set bits [1:0] to 0 for nametable
        PPU_CTRL = gPpuCtrlBase + gYNametable;
        set_scroll();

        if( (gController1 & BUTTON_START) != 0 )
        {
            gStage = 0;
            gGameState = TITLE_SCREEN_STATE;
            //pMusicInit(0);
            load_stage();
        }
    }
}

void main(void)
{
    PPU_CTRL = 0;
    PPU_MASK = 0;

    ppudisable();

    init_globals();
    init_game_state();


    palettes();

    PPU_CTRL = 0x80;

    vblank();
    //gCounter = 10;
    //vblank_counter();

  	PPU_ADDRESS = 0x28; // address of nametable #2
  	PPU_ADDRESS = 0x00;
  	Title_UnRLE(Nametable_TitleScreen_bottom_rle);	// uncompresses our data
  	vblank();
    //gCounter = 10;
    //vblank_counter();

    gScratchPointer = TitleScreenPalette;
    load_palette();

    loadCollisionFromNametables();

    vblank();
    //gCounter = 10;
    //vblank_counter();

    apuinit();
    gCurrentMusic = 0;
    pMusicInit(0);
    //apuinit();

    gCounter = 5;
    vblank_counter();

    gStage = 0;
    PPU_CTRL = gPpuCtrlBase;
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
                //vblank();
                //apuinit();
                gCurrentMusic = 0;
                pMusicInit(0);
                gMusicOn = 1;
                //gCounter = 5;
                //vblank_counter();
                title_screen_sm();
                break;
        }

    }
}
