const unsigned char BirdClimbPalette[13]={
BLACK,
DARK_GRAY,
GRAY,
WHITE,
DARK_TAN,
DARK_GREEN,
GREEN,
DARK_TAN,
DARK_GREEN,
TAN,
GREEN,
GRAY,
WHITE,
};

#define BIRDCLIMB_ENEMY_COUNT 3
static const unsigned char Sprites_BirdClimb[BIRDCLIMB_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, numSprites, doesTongueKill, spriteOffset,
   0x05,   0x7E,   0x7F,           0x00,  0x00,      0x01,       0x01,           0x00,         0x0,
   0x04,   0x40,   0x1F,           0x00,  0x00,      0x00,       0x04,           0x00,         0x4,
   0x03,   0xE0,   0xA0,           0x02,  0x00,      0x00,       0x02,           0x01,         0x14,
};
