File: PortMidi Win32 Readme
Author: Belinda Thom, June 16 2002
Revised by: Roger Dannenberg, June 2002

=============================================================================
USING PORTMIDI:
=============================================================================

PortMidi has been created using a DLL because the Win32 MMedia API doesn't 
handle midiInput properly in the debugger. Specifically, it doesn't clean up
after itself if the user (i.e. you, a PortMidi application) hasn't explicitly
closed all open midi input devices. This lack of cleanup can lead to much
pain and agony, including the blue-screen-of-death. This situation becomes
increasingly unacceptable when you are debugging your code, so a portMidi DLL
seemed to be the only elegant solution.

We suggest using PortMidi in the Debug release version, and call it from a
console application. The "console" suggestion allows you to receive debug
information about why devices may or may not open using stdio. Of course,
this is only possible for testing. PortMidi is designed to run without a
console and should work perfectly well within a graphical user interface
application. When there is a problem, you will always be prompted to enter 
a key before your application (and any related error messages) disappears. 
This will help you identify when you are using portMidi incorrectly, and 
help us to help you should an internal PortMidi error arise.

The "Debug release" suggestion allows your code to verify the assert
statements that we have added to the code to verify PortMidi's correct
operation. This logic also will help us debug PortMidi should the need arise.

Read the portmidi.h file for PortMidi API details on using the PortMidi API.
See <...>\pm_dll_test\test.c or <...>\multithread\test.c for usage examples.
Important: to aid in debugging your client application, use the Pm_Debug and
Pm_DebugStream commands to check status of any PortMidi calls that you make.
The former should be used on PortMidi calls that do not operate on a
PortMidiStream argument, the latter on PortMidi calls that do.

=============================================================================
TO INSTALL PORTMIDI:
=============================================================================
1)  download portmidi.zip

2)  unzip portmidi.zip into directory: <...>\portmidi

=============================================================================
TO COMPILE PORTMIDI:
=============================================================================

3)  go to this directory

4)  click on the portmidi.dsw workspace

5)  the following projects exist within this workspace:
    - portmidi (the PortMidi library)
	- pm_dll (the dll library used to close midi ports on program exit)
	- porttime (a small portable library implementing timer facilities)
	- test (simple midi I/O testing)
	- multithread (more complicated midi I/O transformation based on timers;
        midi output is timer-based, i.e. client application based, with
        no latency)
	- sysex (simple sysex message I/O testing)
	- latency (uses porttime to measure system latency)

6)  verify that all project settings are for Win32 Debug release:
	- hit Alt^F7
	- highlight all three projects in left part of Project Settings window; 
	- "Settings For" should say "Win32 Debug"

7)  set pm_dll as the active project (e.g. Project->Select Active Project)

8)  rebuild all (Build->Rebuild All) to build the PortMidi DLL library

9)  The settings for these projects were distributed in the zip file, so
    compile should just work. In output window you should see:

	Linking...
	   Creating library Debug/pm_dll.lib and object Debug/pm_dll.exp

	pm_dll.dll - 0 error(s), 0 warning(s)

10) important! in order to be able to use DLL from the test project and set
    breakpoints, copy following files from <...>\pm_dll\Debug into 
    <...>\pm_dll_test\Debug and	<...>\multithread\Debug directories:
		pm_dll.lib
		pm_dll.dll
    each time you rebuild pm_dll, these copies must be redone!

11) set pm_dll_test as active project

12) build this project (F7)

13) run test project; use the menu that shows up from the command prompt to
    test that portMidi works on your system. tests include: 
		- verify midi output works
		- verify midi input works
		- verify midi input w/midi thru works

14) repeat steps 11 -> 13 for multithread and other projects if you wish

============================================================================
TO CREATE YOUR OWN PORTMIDI CLIENT APPLICATION:
============================================================================

The easiest way is to start a new project w/in the portMidi workspace:

1) To open new porject: 
	- File->New->Projects
	- Location: <...>\portmidi\<yourProjectName>
	- check Add to current workspace
	- select Win32 Console Application (recommended for now)
	- do *NOT* select the "make dependency" box (you will explicitly do this
      in the next step)
	- Click OK
	- Select "An Empty Project" and click Finish

2) Now this project will be the active project. Make it explicitly depend
   on PortMidi dll:
	- Project->Dependencies
	- Click pm_dll

3) Important! in order to be able to use portMidi DLL from your new project
   and set breakpoints,	copy following files from <...>\pm_dll\Debug into 
   <...>\<yourProjectName>\Debug directory:
		pm_dll.lib
		pm_dll.dll
    each time you rebuild pm_dll, these copies must be redone!

4) add whatever files you wish to add to your new project, using portMidi
   calls as desired (see USING PORTMIDI at top of this readme)

5) when you include portMidi files, do so like this:
	- #include "..\pm_dll\portmidi.h"
	- etc.

6) build and run your project

============================================================================
DESIGN NOTES
============================================================================

The DLL is used so that PortMidi can (usually) close open devices when the
program terminates. Failure to close input devices under WinNT, Win2K, and
probably later systems causes the OS to crash.

One way to do this would be to make a .LIB/.DLL pair, linking to the .LIB
in order to access functions in the .DLL. I'm not sure how to do this with
VC++, and it seems simple enough to create a DLL that does nothing but
call a function when the program terminates. To make this work, we need to
pass in a pointer to funciton; otherwise, I think the function will be
copied into the DLL, which we do not want.

So the structure will be: PortMidi for Win32 exists as a simple library,
with Win32-specific code in pmwin.c and MM-specific code in pmwinmm.c.
pmwin.c will use a DLL in pmdll.c to call Pm_Terminate() when the program
exits to make sure that all MIDI ports are closed.

Orderly cleanup after errors are encountered is based on a fixed order of
steps and state changes to reflect each step. Here's the order:

To open input:
    initialize return value to NULL
    - allocate the PmInternal strucure (representation of PortMidiStream)
    return value is (non-null) PmInternal structure
    - allocate midi buffer
    set buffer field of PmInternal structure
    - call system-dependent open code
        - allocate midiwinmm_type for winmm dependent data
        set descriptor field of PmInternal structure
        - open device
        set handle field of midiwinmm_type structure
        - allocate buffer 1 for sysex
        buffer is added to input port
        - allocate buffer 2 for sysex
        buffer is added to input port
        - return
    - return



