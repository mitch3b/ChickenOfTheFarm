# TODO cleaner ways to do this, but since I'm the only one probably using this for now...
CC65 = ../../cc65/bin/cc65
CA65 = ../../cc65/bin/ca65
LD65 = ../../cc65/bin/ld65
NAME = ChickenOfTheFarm


$(NAME).nes: $(NAME).o main.o asm4c.o nes.cfg
	$(LD65) -C nes.cfg -o $(NAME).nes $(NAME).o main.o asm4c.o nes.lib

	rm *.o
	rm test.s

main.o: main.asm
	$(CA65) main.asm

asm4c.o: asm4c.s
	$(CA65) asm4c.s

$(NAME).o: $(NAME).s
	$(CA65) $(NAME).s

$(NAME).s: $(NAME).c
	$(CC65) -Oi $(NAME).c --add-source

clean:
# if linux uncomment the next line
	rm $(NAME).nes
# windows
#	del $(NAME).nes
