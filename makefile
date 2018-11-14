# TODO cleaner ways to do this, but since I'm the only one probably using this for now...
CC65 = ../../cc65/bin/cc65
CA65 = ../../cc65/bin/ca65
LD65 = ../../cc65/bin/ld65
NAME = ChickenOfTheFarm

$(NAME).nes: $(NAME).o main.o asm4c.o nes.cfg
	$(LD65) -C nes.cfg --dbgfile vars.txt -o $(NAME).nes $(NAME).o --mapfile resources/Sound/Music/$(NAME).map main.o asm4c.o nes.lib

	rm *.o
	rm $(NAME).s

main.o: main.asm
	$(CA65) -g main.asm

asm4c.o: asm4c.s
	$(CA65) -g asm4c.s

$(NAME).o: $(NAME).s
	$(CA65) -g $(NAME).s

$(NAME).s: $(NAME).c
	$(CC65) -g -Oi $(NAME).c --add-source

clean: cleanGraphics
	rm -f $(NAME).nes

# Generate all the graphics files
cleanGraphics:
	rm -f resources/__pycache__/*
	rm -f resources/Background/*/*-reduced.png
	rm -f resources/Background/*/*-tmp.h
	rm -f resources/Sprite/*/*-reduced.png
	rm -f resources/Sprite/*/*-tmp.h

# TODO There's a cleaner way to do this where this can be auto-magically called only if the expected files haven't been generated yet
generateGraphics:
	python3 resources/generate_graphics.py
