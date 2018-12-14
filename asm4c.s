.import _TileList
.import _collision

.export _Title_UnRLE
.export _UnRLE

; not sure this is an appropriate place for these:
.segment "ZEROPAGE"
	RLE_LOW:	 .res 1
	RLE_HIGH:	 .res 1
	RLE_TAG:	 .res 1
	RLE_BYTE:	 .res 1
	CMP_TILE_LO: .res 1
	CMP_TILE_HI: .res 1
	CMP_VAL:     .res 1
	CMP_CNT:     .res 1
	CMP_TOT:     .res 1

.segment "CODE"

_Title_UnRLE:
	tay
	stx <RLE_HIGH
	lda #0
	sta <RLE_LOW

	lda (RLE_LOW),y
	sta <RLE_TAG
	iny
	bne @401
	inc <RLE_HIGH
@401:
	lda (RLE_LOW),y
	iny
	bne @411
	inc <RLE_HIGH
@411:
	cmp <RLE_TAG
	beq @402
	sta $2007
	sta <RLE_BYTE
	clc
	bcc @401
@402:
	lda (RLE_LOW),y
	beq @404
	iny
	bne @421
	inc <RLE_HIGH
@421:
	tax
	lda <RLE_BYTE
@403:
	sta $2007
	dex
	bne @403
	beq @401
@404:
	rts

_UnRLE:
    pha
    txa
    pha
    lda #0
    sta <CMP_CNT
    sta <CMP_TOT
    pla
    tax
    pla

	tay
	stx <RLE_HIGH
	lda #0
	sta <RLE_LOW

	lda (RLE_LOW),y
	sta <RLE_TAG
	iny
	bne @1
	inc <RLE_HIGH
@1:
	lda (RLE_LOW),y
	iny
	bne @11
	inc <RLE_HIGH
@11:
	cmp <RLE_TAG
	beq @2
	pha
	lda <CMP_TOT
	cmp #240
	beq @301
	inc <CMP_TOT
@302:
    pla
    ; put the tile number in CMP_VAL
    sta <CMP_VAL
	pha
	tya
	pha
	txa
	pha
	jsr _UnCMP
	pla
	tax
	pla
	tay
	pla
	clc
	bcc @303
@301:
    pla
	sta $2007
@303:
	sta <RLE_BYTE
	clc
	bcc @1
	;bne @1
@2:
	lda (RLE_LOW),y
	beq @4
	iny
	bne @21
	inc <RLE_HIGH
@21:
	tax
	lda <RLE_BYTE
@3:
	pha
	lda <CMP_TOT
	cmp #240
	beq @501
	inc <CMP_TOT
@502:
    pla
    ; put the tile number in CMP_VAL
    sta <CMP_VAL
	pha
	tya
	pha
	txa
	pha
	jsr _UnCMP
	pla
	tax
	pla
	tay
	pla
	clc
	bcc @503
@501:
    pla
	sta $2007
@503:
	dex
	bne @3
	beq @1
@4:
	rts

_UnCMP:

    ; find the set of nametable values for that tile and put the pointer in CMP_TILE
    lda #>_TileList
    sta <CMP_TILE_HI
    lda #<_TileList
    sta <CMP_TILE_LO
    lda <CMP_VAL
    asl
    bcc @100
    inc <CMP_TILE_HI
@100:
    asl
    bcc @101
    inc <CMP_TILE_HI
@101:
    clc
    adc <CMP_TILE_LO
    sta <CMP_TILE_LO
    bcc @102
    inc <CMP_TILE_HI
@102:

    ; iterate through the 4 nametables values in the tile
    lda #0
    tay

    ; keep track of which nametable tile we're on in the row
    lda <CMP_CNT
    tax

    ; put the first two values straight into the nametable
    lda (CMP_TILE_LO),y
    sta $2007
    iny
    lda (CMP_TILE_LO),y
    sta $2007
    iny

    ; put the next two values in the (overloaded) collision table
    lda (CMP_TILE_LO),y
    ldy <CMP_CNT
    sta _collision,y
    inc <CMP_CNT
    ldy #3
    lda (CMP_TILE_LO),y
    ldy <CMP_CNT
    sta _collision,y
    inc <CMP_CNT

    cpy #31
    bne @104

    ; we've reached the end of the row we need to put all of the nametable
    ; values we unpacked into the collision table area into the real nametable
    ldy #0
@103:
    lda _collision,y
    sta $2007
    iny
    cpy #32
    bne @103
    lda #0
    sta <CMP_CNT

@104:
    rts
