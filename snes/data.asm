.include "hdr.asm"

.section ".rodata1" superfree

tilfont:
.incbin "pvsneslibfont.pic"

palfont:
.incbin "pvsneslibfont.pal"

.ends

.section ".rodata2" superfree

playfieldtiles:
.incbin "playfield.pic"
playfieldtiles_end:

playfieldmap:
.incbin "playfield.map"
playfieldmap_end:

playfieldpal:
.incbin "playfield.pal"
playfieldpal_end:

.ends

.section ".rodata3" superfree

piecetiles:
.incbin "piece.pic"
piecetiles_end:

piecepal:
.incbin "piece.pal"
piecepal_end:

.ends
