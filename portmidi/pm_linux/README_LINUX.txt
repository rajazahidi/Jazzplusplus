README_LINUX.txt for PortMidi
Roger Dannenberg
6 April 2003

This code has not been carefully tested; however, 
all test programs in pm_test seem to run properly.

The error reporting in PortMidi, particularly pmHostError
handling, is difficult to write and difficult to test.

CHANGELOG

12-Apr-2003 Roger B. Dannenberg
      Fixed pm_test/test.c to filter clocks and active messages.
      Integrated changes from Clemens Ladisch:
          cleaned up pmlinuxalsa.c
          record timestamp on sysex input
          deallocate some resources previously left open
