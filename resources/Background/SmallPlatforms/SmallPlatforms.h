const unsigned char SmallPlatformsPalette[13]={
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
DARK_GREEN,
TAN,
GREEN,
};

#define SMALLPLATFORMS_ENEMY_COUNT 5
static const unsigned char Sprites_SmallPlatforms[SMALLPLATFORMS_ENEMY_COUNT * 5] = {
//   id, startX, startY, startNametable, direction,
   0x05,   0x5C,   0x0F,           0x00,      0x01,
   0x05,   0xBC,   0x0F,           0x00,      0x01,
   0x03,   0x70,   0x80,           0x02,      0x00,
   0x03,   0x30,   0x10,           0x00,      0x00,
   0x04,   0x08,   0x1F,           0x02,      0x00,
};
