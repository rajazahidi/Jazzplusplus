## Process this file with automake to produce Makefile.in

ACLOCAL_FLAGS=-I config

SUBDIRS = src conf

# Here are some extra things to distribute.
# things to fix:
# docs are mising(because they are so large)
EXTRA_DIST = bootstrap wxwin2_port.txt song scripts midinetd mswin contrib bitmaps rebuild

really-clean: maintainer-clean
	cd bitmaps; make clean
	rm -f configure Makefile.in src/Makefile.in conf/Makefile.in aclocal.m4 src/config.h.in

htmldocs:
	doxygen jazz.doxy
