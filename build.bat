C:\Users\mitch3a\Downloads\cc65-snapshot-win32\bin\cc65 -Oi ChickenOfTheFarm.c -o ChickenOfTheFarm.asm --add-source
C:\Users\mitch3a\Downloads\cc65-snapshot-win32\bin\ca65 ChickenOfTheFarm.asm
C:\Users\mitch3a\Downloads\cc65-snapshot-win32\bin\ca65 main.asm
C:\Users\mitch3a\Downloads\cc65-snapshot-win32\bin\ca65 asm4c.s
C:\Users\mitch3a\Downloads\cc65-snapshot-win32\bin\ld65 -C nes.cfg -o ChickenOfTheFarm.nes main.o ChickenOfTheFarm.o --mapfile ChickenOfTheFarm.map asm4c.o "C:\Users\mitch3a\Downloads\cc65-snapshot-win32\lib\nes.lib"
