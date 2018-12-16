const unsigned char IntroPalette[13]={
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

#define INTRO_ENEMY_COUNT 2
static const unsigned char Sprites_Intro[INTRO_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, doesTongueKill,
   0x04,   0xE0,   0xBF,           0x02,  0x00,      0x00,           0x00,
   0x05,   0x7E,   0xBF,           0x02,  0x00,      0x01,           0x00,
};
