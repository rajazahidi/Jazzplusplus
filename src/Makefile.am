## Process this file with automake to produce Makefile.in
bin_PROGRAMS = jazz
jazz_SOURCES = \
AboutDialog.cpp \
GuitarFrame.cpp \
GuitarSettings.cpp \
GuitarSettingsDialog.cpp \
GuitarWindow.cpp \
JazzPlusPlusApplication.cpp \
PianoFrame.cpp \
ToolBar.cpp \
TrackFrame.cpp \
TrackWindow.cpp

#jazz_LDADD=-L../xrc -lwx_xrc
#jazz_CXXFLAGS=-I../xrc/include

# The following files aren't compilable under wx2
#EXTRA_DIST = \
#rhythm.cpp \
#gs_dlg1.cpp \
#gs_dlg2.cpp \
#sampldlg.cpp \
#samplwin.cpp \
#alsadrv.cpp


noinst_HEADERS = \
AboutDialog.h \
GuitarFrame.h \
GuitarSettingsDialog.h \
GuitarSettings.h \
GuitarWindow.h \
JazzPlusPlusApplication.h \
PianoFrame.h \
Resources.h \
ToolBar.h \
TrackFrame.h \
TrackWindow.h \
WxWidgets.h

# This directive defines the Jazz++ data directory.  This may be a hack, but
# other programs uyse similar techniques.
# The automake info file suggests using INCLUDES for this purpose
# and i cant even get it to expand the pkgdatadir, aargh
INCLUDES= -DJAZZ_DATADIR=\"@datadir@/@PACKAGE@\"
