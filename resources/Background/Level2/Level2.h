const unsigned char Level2Palette[13]={
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

#define LEVEL2_ENEMY_COUNT 7
static const unsigned char Sprites_Level2[LEVEL2_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, numSprites, doesTongueKill, spriteStart,
   0x02,   0x08,   0x0F,           0x00,  0x00,      0x00,       0x01,           0x00,        0x54,
   0x03,   0x20,   0x30,           0x00,  0x00,      0x00,       0x02,           0x01,        0x58,
   0x02,   0x08,   0x5F,           0x00,  0x00,      0x00,       0x01,           0x00,        0x60,
   0x02,   0xF0,   0xE7,           0x00,  0x00,      0x40,       0x01,           0x00,        0x64,
   0x02,   0xF0,   0x27,           0x02,  0x00,      0x40,       0x01,           0x00,        0x68,
   0x02,   0x08,   0x7F,           0x02,  0x00,      0x00,       0x01,           0x00,        0x6C,
   0x03,   0xCF,   0xC8,           0x02,  0x00,      0x00,       0x02,           0x01,        0x70,};
