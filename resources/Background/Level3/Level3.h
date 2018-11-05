const unsigned char Level3Palette[4]={
BLACK,
DARK_GRAY,
GRAY,
WHITE,
};

#define LEVEL3_ENEMY_COUNT 3
static const unsigned char Sprites_Level3[LEVEL3_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, numSprites, doesTongueKill, spriteOffset,
   0x01,   0x50,   0x50,           0x02,  0x00,      0x00,       0x01,           0x00,          0x0,
   0x04,   0xE0,   0x0F,           0x00,  0x00,      0x00,       0x04,           0x00,          0x4,
   0x05,   0x13,   0xF3,           0x00,  0x00,      0x01,       0x01,           0x00,          0x8,};
