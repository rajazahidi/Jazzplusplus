## Process this file with automake to produce Makefile.in
SUBDIRS = xrc src conf

#some extra things to distribute
# things to fix:
# docs are mising(because they are so large)
# CVS dirs are included 
# bitmaps are both bmp and xpm, seems redundant
EXTRA_DIST = bootstrap wxwin2_port.txt song  scripts midinetd mswin contrib bitmaps rebuild

really-clean: maintainer-clean
	cd bitmaps; make clean
	rm -f configure Makefile.in aclocal.m4
