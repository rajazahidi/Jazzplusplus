/* pmwin.c -- PortMidi os-dependent code */

/* This file only needs to implement:
       pm_init(), which calls various routines to register the 
           available midi devices,
       Pm_GetDefaultInputDeviceID(), and
       Pm_GetDefaultOutputDeviceID().
   This file must
   be separate from the main portmidi.c file because it is system
   dependent, and it is separate from, say, pmwinmm.c, because it
   might need to register devices for winmm, directx, and others.
 */

#include "stdlib.h"
#include "portmidi.h"
#include "pminternal.h"
#include "pmwinmm.h"
#ifdef USE_DLL_FOR_CLEANUP
#include "pmdll.h" /* used to close ports on exit */
#endif
#ifdef DEBUG
#include "stdio.h"
#endif

void pm_init(void)
{
#ifdef USE_DLL_FOR_CLEANUP
    /* we were hoping a DLL could offer more robust cleanup after errors,
       but the DLL does not seem to run after crashes. Thus, the atexit()
       mechanism is just as powerful, and simpler to implement.
     */
    pm_set_close_function(pm_term);
#ifdef DEBUG
	printf("registered pm_term with cleanup DLL\n");
#endif
#else
    atexit(pm_term);
#ifdef DEBUG
	printf("registered pm_term with atexit()\n");
#endif
#endif
	pm_winmm_init();
}


void pm_term(void) {
	pm_winmm_term();
}


PmDeviceID Pm_GetDefaultInputDeviceID() {
    /* This routine should check the environment and the registry
       as specified in portmidi.h, but for now, it just returns
       the first device of the proper input/output flavor.
     */
    int i;
	Pm_Initialize(); /* make sure descriptors exist! */
    for (i = 0; i < descriptor_index; i++) {
        if (descriptors[i].pub.input) {
            return i;
        }
    }
    return pmNoDevice;
}

PmDeviceID Pm_GetDefaultOutputDeviceID() {
    /* This routine should check the environment and the registry
       as specified in portmidi.h, but for now, it just returns
       the first device of the proper input/output flavor.
     */
    int i;
	Pm_Initialize(); /* make sure descriptors exist! */
    for (i = 0; i < descriptor_index; i++) {
        if (descriptors[i].pub.output) {
            return i;
        }
    }
    return pmNoDevice;
    return 0;
}

#include "stdio.h" 

void *pm_alloc(size_t s) {
	return malloc(s); 
}


void pm_free(void *ptr) { 
	free(ptr); 
}

