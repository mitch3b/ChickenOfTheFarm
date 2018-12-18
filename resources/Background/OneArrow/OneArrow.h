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
static const unsigned char Sprites_OneArrow[ONEARROW_ENEMY_COUNT * 5] = {
//   id, startX, startY, startNametable, direction,
   0x05,   0x7E,   0x7F,           0x02,      0x01,
   0x04,   0xE0,   0xBF,           0x02,      0x00,
   0x02,   0xF0,   0x9F,           0x02,      0x40,
};
