
# .xpm are 2x2 pixel larger than .bmp files. .xpm are derived from .bmp, .bmp may be edited in ppm format

all: panic.xpm


# if one is there, all should be there
panic.xpm : panic.bmp
	for i in *.bmp; do n=`basename $$i .bmp`; echo $$i; bmptoppm $$i | pnmmargin -color rgb:c3/c3/c3 2 | ppmtoxpm -name tb_$$n > $$n.xpm; done


bmp2ppm:
	for i in *.bmp; do n=`basename $$i .bmp`; echo $$i; bmptoppm < $$i > $$n.ppm ; done

ppm2bmp:
	for i in *.ppm; do n=`basename $$i .ppm`; echo $$i; ppmtobmp < $$i > $$n.bmp; done

clean:
	rm -f *.xpm *.ppm

