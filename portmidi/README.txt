README for PortMidi
Roger Dannenberg
6 April 2003

For Windows, please see also README_WIN.txt and debugging_dlls.txt
in pm_win.

For Linux, please see also README_LINUX.txt in pm_linux.

POINTERS VS DEVICE NUMBERS

When you open a MIDI port, PortMidi allocates a structure to
maintain the state of the open device. Since every device is
also listed in a table, you might think it would be simpler to
use the table index rather than a pointer to identify a device.
This would also help with error checking (it's hard to make
sure a pointer is valid). PortMidi's design parallels that of
PortAudio.

ERROR HANDLING

Error handling turned out to be much more complicated than expected.
PortMidi functions return error codes that the caller can check.
In addition, errors may occur asynchronously due to MIDI input. In
this case, the error code is transferred to the next call to
Pm_Read. Furthermore, an error can arise during a MIDI THRU
operation that is also invoked asynchronously when MIDI input
arrives.

Ordinarily, the caller checks for an error code. If the error is
system-dependent, pmHostError is returned and the caller can
call Pm_GetHostErrorText to get a text description of the error.

Host errors are recorded in the system-specific data allocated for
each open MIDI port. However, if an error occurs on open or close,
we cannot store the error with the device because there will be
no device data (assuming PortMidi cleans up after devices that
are not open). For open and close, we will store the host error
in a global variable. The PortMidi is smart enough to look here
first when the user asks for ErrorText.

Another problem is that when an error occurs in a MIDI THRU
operation, the caller must go to the output port to retrieve
the error, even though the initial indication of error will be
on the input port.

