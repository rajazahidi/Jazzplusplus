# The .xpm files are created automatically from the .bmp files.  A 2 pixel
# transparent border is added around each image.  Any pixels with the #c0c0c0
# color will be converted to transparent pixels in the finished image.

BITMAPS=$(shell ls *.bmp)
XPMS=$(patsubst %.bmp,%.xpm,$(BITMAPS))

all: $(XPMS)

%.xpm: %.ppm
	pnmmargin -color rgb:c0/c0/c0 2 $< > border_$<
	ppmcolormask rgb:c0/c0/c0 border_$< > trans_$<
	ppmtoxpm -alphamask=trans_$< -name=$(subst .,_,$@) border_$< > $@
	rm -f border_$< trans_$<

%.ppm: %.bmp
	bmptoppm $< > $@

#%.bmp: %.ppm
#	ppmtobmp -bpp=4 $< > $@

clean:
	rm -f *.xpm *.ppm border_* trans_*
