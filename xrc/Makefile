CC=gcc
CXX=g++
TARGET=libwx_xrc-2.4.a

all: $(TARGET)

# Expat compilation stuff
expat_dir = expat
EXPAT_DEFS=-I$(expat_dir)/xmlparse -I$(expat_dir)/xmltok
EXPAT_OBJS=xmlparse.o xmltok.o xmlrole.o

xmlparse.o: expat/xmlparse/xmlparse.c
	$(CC) $(EXPAT_DEFS) -c $<

xmltok.o: expat/xmltok/xmltok.c
	$(CC) $(EXPAT_DEFS) -c $<

xmlrole.o: expat/xmltok/xmlrole.c
	$(CC) $(EXPAT_DEFS) -c $<


# XRC compilation stuff

OBJECTS=$(EXPAT_OBJS) \
        xml.o xmlres.o xmlrsall.o \
        xh_bttn.o xh_chckb.o xh_chckl.o xh_choic.o xh_combo.o xh_dlg.o \
        xh_gauge.o xh_html.o xh_menu.o xh_notbk.o xh_panel.o xh_radbt.o \
        xh_radbx.o xh_sizer.o xh_slidr.o xh_spin.o xh_stbmp.o xh_sttxt.o \
        xh_text.o xh_listb.o xh_toolb.o xh_stlin.o xh_bmp.o xh_unkwn.o \
        xh_bmpbt.o xh_cald.o xh_listc.o xh_scrol.o xh_stbox.o xh_tree.o \
        xh_frame.o xh_gdctl.o xh_scwin.o xh_split.o xh_wizrd.o

CXXFLAGS=`wx-config --cxxflags` -Iinclude $(EXPAT_DEFS)

# This is for version 011 of the XRC library.
libwx_xrc-2.4.a: $(OBJECTS)
	ar cr $@ $^
	ranlib $@

clean:
	rm -f $(OBJECTS) $(TARGET)
