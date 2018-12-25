const unsigned char CastleColumnsPalette[13]={
BLACK,
DARK_GRAY,
GRAY,
WHITE,
DARK_TAN,
DARK_GREEN,
TAN,
DARK_PURPLE,
PURPLE,
LIGHT_PURPLE,
DARK_GREEN,
GREEN,
LIGHT_GREEN,
};

#define CASTLECOLUMNS_ENEMY_COUNT 6
static const unsigned char Sprites_CastleColumns[CASTLECOLUMNS_ENEMY_COUNT * 5] = {
//   id, startX, startY, startNametable, direction,
   0x04,   0xE0,   0xCF,           0x02,      0x00,
   0x05,   0xE4,   0x13,           0x00,      0x01,
   0x03,   0x10,   0x10,           0x02,      0x01,
   0x03,   0xE0,   0x40,           0x00,      0x01,
   0x03,   0x20,   0x70,           0x00,      0x01,
   0x03,   0xD0,   0xC0,           0x00,      0x01,
};
