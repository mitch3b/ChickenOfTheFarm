CC65 = ../../../../../cc65/bin/cc65
CA65 = ../../../../../cc65/bin/ca65
LD65 = ../../../../../cc65/bin/ld65
NAME = ChickenOfTheFarm

  $(CA65) nsf_wrap.s -D INC_MUSIC -D RELOCATE_MUSIC
  $(LD65) -C nsf.cfg -o ChickenOfTheFarm.nsf nsf_wrap.o --mapfile music.map
