.export __STARTUP__ : absolute=1
.import _main

.import __STACK_START__, __STACK_SIZE__
.include "zeropage.inc"
.import initlib, copydata

.pushseg
.segment "ZEROPAGE"
.popseg

.pushseg
.segment "BSS"

_gVblank: .res 1
.export _gVblank

.popseg

.pushseg
.segment "HEADER"

; iNES header
; see http://wiki.nesdev.com/w/index.php/INES

.byte $4E, $45, $53, $1A ; "NES" EOF
.byte $02                ; PRG ROM size (16 KiB units)
.byte $01                ; CHR ROM size (8 KiB units)
.byte $00                ; horizontal mirroring
.byte $08                ; mapper 0000 (NROM)
.byte $00                ; PRG RAM size (8 KiB units)
.byte $00                ; NTSC
.byte $00                ; unused
.res 5, $00              ; zero-fill
.popseg

.pushseg
.segment "VECTORS"

.word nmi   ;$FFFA NMI
.word start ;$FFFC Reset
.word irq   ;$FFFE IRQ
.popseg

.pushseg
.segment "STARTUP"
start:
    jmp _main

nmi:
    inc _gVblank

; no handler for irq
irq:
    rti

.popseg

.pushseg
.segment "MUSIC"
.export _music
_music:
    ;.incbin "starwars.nsf", $80 ;just need the data not the header
    ;.incbin "Z2.nsf", $80 ;just need the data not the header
    ;.incbin "test.nsf" ;, $80
    .incbin "resources/Sound/Music/ChickenOfTheFarm.nsf"
.popseg

.export _pMusicInit
_pMusicInit:
    ; address of the init function (see nsf header for the offset, adjust based on where we included it in the ROM
    jmp _music + $80 ;$8000; $87F9 ;_music + $0F9 ;$80 ;$8080
    rts

.export _pMusicPlay
_pMusicPlay:
    lda #0
    ldx #0
    ; address of the play function (see nsf header for the offset, adjust based on where we included it in the ROM
    jmp _music + $83 ;$8084; $8803 ;_music + $703 ;$84 ;$8084
    rts

.pushseg
.segment "SPRITES"
.popseg

.pushseg
.segment "ONCE"
.popseg

.pushseg
.segment "CHARS"
.popseg
