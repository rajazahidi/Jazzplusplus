## Process this file with automake to produce Makefile.in
bin_PROGRAMS = jazz
jazz_SOURCES = \
                pianowin.cpp \
                resdlg.cpp \
                maction.cpp \
                eventwin.cpp \
                trackwin.cpp \
                toolbar.cpp \
                jazz.cpp track.cpp \
                synth.cpp \
                events.cpp  \
                song.cpp \
                mstdfile.cpp \
                util.cpp \
                player.cpp \
                dynarray.cpp \
                about.cpp \
                audio.cpp \
                sample.cpp \
                freqtab.cpp \
                harmonyp.cpp \
                slidrwin.cpp \
                eventlst.cpp \
                command.cpp \
                filter.cpp \
                random.cpp \
                arpeggio.cpp\
                genmeldp.cpp \
                genmeldy.cpp \
                mapper.cpp \
                audiodrv.cpp\
                proplistdlg.cpp\
                alsaplay.cpp \
                alsathru.cpp \
                harmony.cpp \
                hbanalyz.cpp \
                ctrledit.cpp  \
                sig2.cpp \
                samplcmd.cpp \
                guitar.cpp \
                dialogs.cpp \
                shuffle.cpp \
                dialogs/copyDialog.cpp \
                dialogs/copyright.cpp \
                dialogs/metronomeSettings.cpp \
                dialogs/midiThruDialog.cpp \
                dialogs/midiTiming.cpp \
                dialogs/songSettings.cpp \
                dialogs/synthSettings.cpp \
                commands/copyCommand.cpp



#the following files arent compilable yet(under wx2)

EXTRA_DIST=	rhythm.cpp \
            gs_dlg1.cpp \
            gs_dlg2.cpp \
            sampldlg.cpp \
            samplwin.cpp	\
            alsadrv.cpp


noinst_HEADERS=about.h     command.h   eventwin.h  harmony.h   midinet.h      rhythm.h    signali.h   trackwin.h \
alsadrv.h   config.h    filter.h    harmonyp.h  midi_p.h       samplcmd.h  slidrwin.h  util.h \
alsaplay.h  ctrledit.h  freqtab.h   hbanalyz.h  mstdfile.h     sampldlg.h  song.h      winaudio.h \
alsathru.h  dialogs.h   genmeldp.h  jazz.h      pianowin.h     sample.h    splcmplx.h \
arpeggio.h  dynarray.h  genmeldy.h  maction.h   player.h       samplwin.h  synth.h \
audiodrv.h  eventlst.h  gs_dlgs.h   mapper.h    proplistdlg.h  shuffle.h   toolbar.h \
audio.h     events.h    guitar.h    mascfile.h  random.h       sig2.h      track.h


#here i define the jazz data dir. i think its a hack, but other progs do like this(it would be nicer in config.h but i cant make that happen)
#also, the automake info file suggests using INCLUDES for this purpose
# and i cant even get it to expand the pkgdatadir, aargh
INCLUDES= -DJAZZ_DATADIR=\"@datadir@/@PACKAGE@\"

