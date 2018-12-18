const unsigned char ShortClimbPalette[13]={
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

#define SHORTCLIMB_ENEMY_COUNT 3
static const unsigned char Sprites_ShortClimb[SHORTCLIMB_ENEMY_COUNT * 5] = {
//   id, startX, startY, startNametable, direction,
   0x05,   0x7E,   0x0F,           0x02,      0x01,
   0x04,   0x27,   0xEF,           0x01,      0x00,
   0x06,   0xC0,   0x76,           0x02,      0x02,
};
