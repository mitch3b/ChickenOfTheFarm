.export __STARTUP__ : absolute=1
.import _main

.import __STACK_START__, __STACK_SIZE__
.include "zeropage.inc"
.import initlib, copydata

.pushseg
.segment "ZEROPAGE"
.popseg

.pushseg
.segment "HEADER"

; iNES header
; see http://wiki.nesdev.com/w/index.php/INES

.byte $4E, $45, $53, $1A ; "NES" EOF
.byte $02                ; PRG ROM size (16 KiB units)
.byte $01                ; CHR ROM size (8 KiB units)
.byte $00                ; horizontal mirroring
.byte $00                ; mapper 0000 (NROM)
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

; no handlers for nmi or irq
nmi:
irq:
    rti

.popseg

.pushseg
.segment "MUSIC"
.export _music
_music:
    ;.incbin "starwars.nsf", $80 ;just need the data not the header
    ;.incbin "Z2.nsf", $80 ;just need the data not the header
    .incbin "test.nsf"
.popseg

.export _pMusicInit
_pMusicInit:
    jmp _music + $80 ;$8080
    rts

.export _pMusicPlay
_pMusicPlay:
    lda #0
    ldx #0
    jmp _music + $84 ;$8084
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
