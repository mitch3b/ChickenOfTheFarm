C:\Users\pgrimsrud\NES\cc65\bin\cc65 -Oi ChickenOfTheFarm.c -o ChickenOfTheFarm.asm --add-source
C:\Users\pgrimsrud\NES\cc65\bin\ca65 ChickenOfTheFarm.asm
C:\Users\pgrimsrud\NES\cc65\bin\ca65 main.asm
C:\Users\pgrimsrud\NES\cc65\bin\ca65 asm4c.s
C:\Users\pgrimsrud\NES\cc65\bin\ld65 -C nes.cfg -o ChickenOfTheFarm.nes main.o test.o --mapfile ChickenOfTheFarm.map asm4c.o "C:\Users\pgrimsrud\NES\cc65\lib\nes.lib"
