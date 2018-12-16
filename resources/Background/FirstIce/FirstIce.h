const unsigned char FirstIcePalette[13]={
BLACK,
VERY_LIGHT_GRAY_BLUE,
WHITE,
LIGHT_GRAY_BLUE,
VERY_LIGHT_GRAY_BLUE,
VERY_LIGHT_GRAY_BLUE,
WHITE,
VERY_LIGHT_GRAY_BLUE,
LIGHT_BLUE,
LIGHT_GRAY_BLUE,
VERY_LIGHT_GRAY_BLUE,
LIGHT_BLUE,
WHITE,
};

#define FIRSTICE_ENEMY_COUNT 3
static const unsigned char Sprites_FirstIce[FIRSTICE_ENEMY_COUNT * 7] = {
//   id, startX, startY, startNametable, state, direction, doesTongueKill,
   0x05,   0xD4,   0xB4,           0x02,  0x00,      0x01,           0x00,
   0x04,   0x00,   0x9F,           0x02,  0x00,      0x00,           0x00,
   0x02,   0xF0,   0xA3,           0x02,  0x00,      0x40,           0x00,
};
