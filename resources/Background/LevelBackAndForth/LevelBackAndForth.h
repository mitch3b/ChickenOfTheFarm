const unsigned char LevelBackAndForthPalette[13]={
BLACK,
DARK_GRAY,
GRAY,
WHITE,
DARK_BLUE,
BLUE,
LIGHT_BLUE,
DARK_PURPLE,
PURPLE,
LIGHT_PURPLE,
DARK_GREEN,
GREEN,
LIGHT_GREEN,
};

#define LEVELBACKANDFORTH_ENEMY_COUNT 2
static const unsigned char Sprites_LevelBackAndForth[LEVELBACKANDFORTH_ENEMY_COUNT * 5] = {
//   id, startX, startY, startNametable, direction,
   0x04,   0x10,   0xBF,           0x02,      0x00,
   0x05,   0xA4,   0x33,           0x00,      0x01,
};
