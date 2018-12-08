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

#define LEVELBACKANDFORTH_ENEMY_COUNT 1
static const unsigned char Sprites_LevelBackAndForth[LEVELBACKANDFORTH_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, numSprites, doesTongueKill, spriteOffset,
   0x04,   0xDC,   0x13,           0x00,  0x00,      0x00,       0x04,           0x00,         0x1C,
};
