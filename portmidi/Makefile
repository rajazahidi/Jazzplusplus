# MAKEFILE FOR PORTMIDI AND PORTTIME

# Use this for linux alsa (0.9x) version
versions = pm_linux/pmlinuxalsa.o
ALSALIB = -lasound
VFLAGS = -DPMALSA

# Use this for null (a dummy implementation for no Midi I/O:
# versions = pmlinuxnull.o
# ALSALIB = 
# VFLAGS = -DPMNULL

pmlib = pm_linux/libportmidi.a

ptlib = porttime/libporttime.a

CC = gcc $(VFLAGS) -g -Ipm_common -Iporttime

pmobjects = pm_common/pmutil.o $(versions) pm_linux/pmlinux.o  \
	pm_common/portmidi.o pm_linux/pmlinuxalsa.o

ptobjects = porttime/porttime.o porttime/ptlinux.o 

current: all

all: $(pmlib) $(ptlib) pm_test/test pm_test/sysex pm_test/midithread \
	pm_test/latency

$(pmlib): Makefile $(pmobjects)
	ar -cr $(pmlib) $(pmobjects)

$(ptlib): Makefile $(ptobjects)
	ar -cr $(ptlib) $(ptobjects)

pm_linux/pmlinuxalsa.o: Makefile pm_linux/pmlinuxalsa.c pm_linux/pmlinuxalsa.h
	$(CC) -c pm_linux/pmlinuxalsa.c -o pm_linux/pmlinuxalsa.o

pm_test/test: Makefile pm_test/test.o $(pmlib) $(ptlib)
	$(CC) pm_test/test.c -o pm_test/test $(pmlib) $(ptlib) $(ALSALIB)

pm_test/sysex: Makefile pm_test/sysex.o $(pmlib) $(ptlib)
	$(CC) pm_test/sysex.c -o pm_test/sysex $(pmlib) $(ptlib) $(ALSALIB)

pm_test/midithread: Makefile pm_test/midithread.o $(pmlib) $(ptlib)
	$(CC) pm_test/midithread.c -o pm_test/midithread $(pmlib) $(ptlib) $(ALSALIB)

pm_test/latency: Makefile $(ptlib) $(pmlib) pm_test/latency.o 
	$(CC) pm_test/latency.c -o pm_test/latency $(ptlib) $(pmlib) $(ALSALIB) -lpthread -lm

porttime/ptlinux.o: Makefile porttime/ptlinux.c
	$(CC) -c porttime/ptlinux.c -o porttime/ptlinux.o

clean:
	 rm -f *.o *~ core* */*.o */*~ */core*

cleaner: clean

cleanest: cleaner
	rm -f $(pmlib) $(ptlib) pm_test/test pm_test/sysex pm_test/midithread
	rm -f pm_test/latency

