const unsigned char Level4Palette[4]={
BLACK,
DARK_GRAY,
GRAY,
WHITE,
};

#define LEVEL4_ENEMY_COUNT 3
static const unsigned char Sprites_Level4[LEVEL4_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, numSprites, doesTongueKill, spriteOffset,
   0x01,   0x50,   0x50,           0x02,  0x00,      0x00,       0x01,           0x00,          0x0,
   0x04,   0x78,   0xCF,           0x02,  0x00,      0x00,       0x04,           0x00,          0x4,
   0x05,   0xE0,   0x13,           0x00,  0x00,      0x01,       0x01,           0x00,         0x14,
};
