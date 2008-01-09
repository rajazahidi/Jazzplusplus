

# if one is there, all should be there
ps: ../trackwin.ps

../trackwin.ps: trackwin.ppm
	for i in *.ppm; do n=`basename $$i .ppm`; echo $$i; ppmtogif $$i | giftopnm | pnmtops -noturn > ../$$n.ps; done

png: ../html/trackwin.png

../html/trackwin.png: trackwin.ppm
	for i in *.ppm; do n=`basename $$i .ppm`; echo $$i; pnmtopng $$i > ../html/$$n.png; done

bmp: ../trackwin.bmp

../trackwin.bmp: trackwin.ppm
	for i in *.ppm; do n=`basename $$i .ppm`; echo $$i; ppmtobmp -windows $$i > ../$$n.bmp; done

trackwin.ppm:
	for i in *.gz; do n=`basename $$i .gz`; cat $$i | gunzip < $$i > $$n; done
	#gunzip *.gz


clean:
	rm -f *.ppm
	rm -f ../*.ps 
	rm -f ../*.bmp
	#if test -f trackwin.ppm; then gzip *.ppm; fi
