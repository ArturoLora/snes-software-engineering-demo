ifeq ($(strip $(PVSNESLIB_HOME)),)
$(error "Please create an environment variable PVSNESLIB_HOME by following this guide: https://github.com/alekmaul/pvsneslib/wiki/Installation")
endif

# source lives in source/, not the devkitsnes default of src/
SRC := source

include ${PVSNESLIB_HOME}/devkitsnes/snes_rules

.PHONY: bitmaps all

#---------------------------------------------------------------------------------
# ROMNAME is used in snes_rules file
export ROMNAME := apotris

all: bitmaps $(ROMNAME).sfc

clean: cleanBuildRes cleanRom cleanGfx

#---------------------------------------------------------------------------------
pvsneslibfont.pic: pvsneslibfont.png
	@echo convert font with no tile reduction ... $(notdir $@)
	$(GFXCONV) -s 8 -o 16 -u 16 -p -e 0 -i $<

#---------------------------------------------------------------------------------
# playfield tileset + map (Story 1.2): -m also generates the .map from the
# source PNG, so the 10x20 static layout comes straight from the image.
playfield.pic: playfield.png
	@echo convert playfield tileset+map ... $(notdir $@)
	$(GFXCONV) -s 8 -o 16 -u 16 -e 0 -p -m -i $<

#---------------------------------------------------------------------------------
# active piece sprite tile (Story 4.1): single 8x8 solid-color placeholder,
# no map needed (sprites use oamInitGfxSet, not a BG tilemap).
piece.pic: piece.png
	@echo convert piece sprite tile ... $(notdir $@)
	$(GFXCONV) -s 8 -o 16 -u 16 -e 0 -p -i $<

bitmaps : pvsneslibfont.pic playfield.pic piece.pic

#---------------------------------------------------------------------------------
# Story 4.6 fix: data.asm embeds these via .incbin - Make doesn't parse
# .incbin, so without this prerequisite it only reassembles data.obj when
# data.asm itself changes, silently linking stale graphics into the ROM when
# only a .pic/.map/.pal asset changes. Prerequisites-only (no recipe) - the
# actual compile recipe for data.obj still comes from snes_rules, untouched.
data.obj: pvsneslibfont.pic pvsneslibfont.pal playfield.pic playfield.map playfield.pal piece.pic piece.pal
