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

#define SHORTCLIMB_ENEMY_COUNT 2
static const unsigned char Sprites_ShortClimb[SHORTCLIMB_ENEMY_COUNT * 7] = {
//   id, startX, startY, startNametable, state, direction, doesTongueKill,
   0x05,   0x7E,   0x0F,           0x02,  0x00,      0x01,           0x00,
   0x04,   0x27,   0xEF,           0x01,  0x00,      0x00,           0x00,
};
