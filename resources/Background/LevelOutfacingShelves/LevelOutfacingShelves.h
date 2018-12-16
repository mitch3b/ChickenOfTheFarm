const unsigned char LevelOutfacingShelvesPalette[4]={
BLACK,
DARK_GRAY,
GRAY,
WHITE,
};

#define LEVELOUTFACINGSHELVES_ENEMY_COUNT 15
static const unsigned char Sprites_LevelOutfacingShelves[LEVELOUTFACINGSHELVES_ENEMY_COUNT * 9] = {
//   id, startX, startY, startNametable, state, direction, doesTongueKill,
   0x02,   0x08,   0x47,           0x00,  0x00,      0x00,           0x00,   //10 right arrows
   0x02,   0x08,   0x4F,           0x00,  0x00,      0x00,           0x00,
   0x02,   0x08,   0xA7,           0x00,  0x00,      0x00,           0x00,
   0x02,   0x08,   0xAF,           0x00,  0x00,      0x00,           0x00,
   0x02,   0x08,   0x47,           0x00,  0x00,      0x00,           0x00,
   0x02,   0x08,   0x17,           0x02,  0x00,      0x00,           0x00,
   0x02,   0x08,   0x17,           0x02,  0x00,      0x00,           0x00,
   0x02,   0x08,   0x1F,           0x02,  0x00,      0x00,           0x00,
   0x02,   0x08,   0x77,           0x02,  0x00,      0x00,           0x00,
   0x02,   0x08,   0x7F,           0x02,  0x00,      0x00,           0x00,
   0x05,   0x28,   0x28,           0x00,  0x00,      0x01,           0x00,  //keys
   0x05,   0x28,   0x88,           0x00,  0x00,      0x01,           0x00,
   0x05,   0x28,   0x57,           0x02,  0x00,      0x01,           0x00,
   0x05,   0x28,   0xB7,           0x02,  0x00,      0x01,           0x00,
   0x04,   0x78,   0xC7,           0x02,  0x00,      0x01,           0x00,  //Portal
};
