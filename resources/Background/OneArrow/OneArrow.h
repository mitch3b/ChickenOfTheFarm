const unsigned char OneArrowPalette[13]={
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

#define ONEARROW_ENEMY_COUNT 3
static const unsigned char Sprites_OneArrow[ONEARROW_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, numSprites, doesTongueKill, spriteOffset,
   0x05,   0x7E,   0x7F,           0x02,  0x00,      0x01,       0x01,           0x00,         0x0,
   0x04,   0xE0,   0xBF,           0x02,  0x00,      0x00,       0x04,           0x00,         0x4,
   0x02,   0xF0,   0xA7,           0x02,  0x00,      0x40,       0x01,           0x00,         0x14,
};
