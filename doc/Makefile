all : html ps win xlp

dist: html xlp
	rm -f jazz.aux jazz.con jazz.dvi jazz.idx jazz.ilg jazz.ind jazz.log jazz.ref jazz.toc
	for i in *.bmp; do n=`basename $$i .bmp`; rm -f $$n.bmp $$n.ps; done
	rm -rf images

win: jazz.rtf
jazz.rtf: jazz.tex
	(cd images; make bmp)
	tex2rtf jazz.tex jazz.rtf -winhelp -twice

ps: jazz.ps
jazz.ps : jazz.dvi
	dvips jazz.dvi -o jazz.ps

jazz.dvi: jazz.tex
	(cd images; make ps)
	latex jazz
	latex jazz
	latex jazz

html: html/jazz_contents.html
html/jazz_contents.html : jazz.tex
	rm -rf html
	mkdir html
	#if ! test -d html; then mkdir html; fi
	(cd images; make png)
	(cd html; tex2rtf ../jazz.tex jazz -html -twice)

xlp: jazz.xlp
jazz.xlp: jazz.tex
	tex2rtf jazz.tex jazz.xlp -xlp -twice

clean:
	rm -f jazz.aux jazz.con jazz.dvi jazz.idx jazz.ilg jazz.ind jazz.log jazz.ref jazz.toc
	rm -rf html
	(cd images; make clean)
	rm -f jazz.ps
	rm -f jazz.xlp
	rm -f jazz.rtf


