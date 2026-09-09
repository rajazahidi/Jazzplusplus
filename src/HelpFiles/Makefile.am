SUBDIRS = images

SUFFIXES= .o .tex

.tex.o:
	-if which tex2rtf > /dev/null 2>&1; then export TEXINPUTS=.; tex2rtf $< $*.html -html -twice; fi
	touch $@

noinst_LIBRARIES = libphony.a

libphony_a_SOURCES = jazz.tex

helpdir = ${prefix}/share/Jazz++/HelpFiles

make-install-dirs:
	-if test '!' -d $(helpdir); then mkdir -p $(helpdir); fi

install-data-hook: make-install-dirs
	-@ echo Installing $(helpdir) ; \
	$(INSTALL_DATA) $(srcdir)/*.gif $(helpdir) ; \
	$(INSTALL_DATA) $(srcdir)/*.png $(helpdir) ; \
	$(INSTALL_DATA) *.html $(helpdir) ; \
	$(INSTALL_DATA) *.hhc $(helpdir) ; \
	$(INSTALL_DATA) *.hhk $(helpdir) ; \
	$(INSTALL_DATA) *.hhp $(helpdir) ; \
	$(INSTALL_DATA) *.htx $(helpdir) ; \
	$(INSTALL_DATA) *.con $(helpdir) ; \
	$(INSTALL_DATA) *.ref $(helpdir)
