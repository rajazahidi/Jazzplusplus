#include "stdlib.h"
#include "string.h"
#include "portmidi.h"
#include "pminternal.h"

#define MIDI_CLOCK      0xf8
#define MIDI_ACTIVE     0xfe

#define is_empty(midi) ((midi)->tail == (midi)->head)

static int pm_initialized = FALSE;
int pm_hosterror = FALSE;
char pm_hosterror_text[PM_HOST_ERROR_MSG_LEN];

/*
====================================================================
system implementation of portmidi interface
====================================================================
*/

int descriptor_index = 0;
descriptor_node descriptors[pm_descriptor_max];

/* pm_add_device -- describe interface/device pair to library 
 *
 * This is called at intialization time, once for each 
 * interface (e.g. DirectSound) and device (e.g. SoundBlaster 1)
 * The strings are retained but NOT COPIED, so do not destroy them!
 *
 * returns pmInvalidDeviceId if device memory is exceeded
 * otherwise returns pmNoError
 */
PmError pm_add_device(char *interf, char *name, int input, 
                      void *descriptor, pm_fns_type dictionary) {
    if (descriptor_index >= pm_descriptor_max) {
        return pmInvalidDeviceId;
    }
    descriptors[descriptor_index].pub.interf = interf;
    descriptors[descriptor_index].pub.name = name;
    descriptors[descriptor_index].pub.input = input;
    descriptors[descriptor_index].pub.output = !input;

    /* default state: nothing to close (for automatic device closing) */
    descriptors[descriptor_index].pub.opened = FALSE;

    /* ID number passed to win32 multimedia API open */
    descriptors[descriptor_index].descriptor = descriptor;
    
    /* points to PmInternal, allows automatic device closing */
    descriptors[descriptor_index].internalDescriptor = NULL;

    descriptors[descriptor_index].dictionary = dictionary;
    
    descriptor_index++;
    
    return pmNoError;
}


/*
====================================================================
portmidi implementation
====================================================================
*/

int Pm_CountDevices( void ) {
    PmError err = Pm_Initialize();
    if (err) 
        return err;
    return descriptor_index;
}


const PmDeviceInfo* Pm_GetDeviceInfo( PmDeviceID id ) {
    PmError err = Pm_Initialize();
    if (err) 
        return NULL;
    if (id >= 0 && id < descriptor_index) {
        return &descriptors[id].pub;
    }
    return NULL;
}

/* pm_success_fn -- "noop" function pointer */
PmError pm_success_fn(PmInternal *midi) {
    return pmNoError;
}

/* none_write -- returns an error if called */
PmError none_write(PmInternal *midi, PmEvent *buffer, long length) {
    return pmBadPtr;
}

/* pm_fail_fn -- generic function, returns error if called */
PmError pm_fail_fn(PmInternal *midi) {
    return pmBadPtr;
}

static PmError none_open(PmInternal *midi, void *driverInfo) {
    return pmBadPtr;
}
static void none_get_host_error(PmInternal * midi, char * msg, unsigned int len) {
    strcpy(msg,"");
}
static unsigned int none_has_host_error(PmInternal * midi) {
    return FALSE;
}

#define none_abort pm_fail_fn
#define none_close pm_fail_fn

pm_fns_node pm_none_dictionary = {
    none_write, 
    none_open,
    none_abort, 
    none_close,
    none_poll,
    none_has_host_error,
    none_get_host_error 
};


const char *Pm_GetErrorText( PmError errnum ) {
    const char *msg;

    switch(errnum)
    {
    case pmNoError:                  
        msg = ""; 
        break;
    case pmHostError:                
        msg = "PortMidi: `Host error'"; 
        break;
    case pmInvalidDeviceId:          
        msg = "PortMidi: `Invalid device ID'"; 
        break;
    case pmInsufficientMemory:       
        msg = "PortMidi: `Insufficient memory'"; 
        break;
    case pmBufferTooSmall:           
        msg = "PortMidi: `Buffer too small'"; 
        break;
    case pmBadPtr:                   
        msg = "PortMidi: `Bad pointer'"; 
        break;
    case pmInternalError:            
        msg = "PortMidi: `Internal PortMidi Error'"; 
        break;
    case pmBufferOverflow:
        msg = "PortMidi: `Buffer overflow'";
        break;
    default:                         
        msg = "PortMidi: `Illegal error number'"; 
        break;
    }
    return msg;
}


void Pm_GetHostErrorText(PortMidiStream * stream, char * msg, 
                         unsigned int len) {
    PmInternal * midi = (PmInternal *) stream;
    
    if (pm_hosterror) { /* we have the string already from open or close */
        strncpy(msg, (char *) pm_hosterror_text, len);
        pm_hosterror = FALSE;
        pm_hosterror_text[0] = 0; /* clear the message; not necessary, but it
                                 might help with debugging */
    } else if (midi == NULL) {
        /* make this routine bullet-proof so we can always count on
        it running */
        strncpy(msg,"Can't print host error for bogus stream argument", len);
    } else {
        (*midi->dictionary->host_error)(midi, msg, len);
    }
    msg[len - 1] = 0; /* make sure string is terminated */
}


int Pm_HasHostError(PortMidiStream * stream) {
    if (pm_hosterror) {
        return TRUE;
    } else if (stream) {
        PmInternal * midi = (PmInternal *) stream;
        return (*midi->dictionary->has_host_error)(midi);
    } else {
        return FALSE;
    }
}


PmError Pm_Initialize( void ) {
    pm_hosterror_text[0] = 0; /* the null string */
    if (!pm_initialized) {
        pm_init();
        pm_initialized = TRUE;
    }
    return pmNoError;
}


PmError Pm_Terminate( void ) {
    if (pm_initialized) {
        pm_term();
        pm_initialized = FALSE;
    }
    return pmNoError;
}


/* Pm_Read -- read up to length longs from source into buffer */
/*
   returns number of longs actually read, or error code
 
   When the reader wants data:
     if overflow_flag:
         do not get anything
         empty the buffer (read_ptr = write_ptr)
         clear overflow_flag
         return pmBufferOverflow
     get data
     return number of messages
*/
PmError Pm_Read(PortMidiStream *stream, PmEvent *buffer, long length) {
    PmInternal *midi = (PmInternal *) stream;
    int n = 0;
    long head;
    
    /* arg checking */
    if(midi == NULL)
        return pmBadPtr;
    if(Pm_HasHostError(midi))
        return pmHostError;
    if(!descriptors[midi->device_id].pub.opened)
        return pmBadPtr;
    if(!descriptors[midi->device_id].pub.input)
        return pmBadPtr;
    
    /* First poll for data in the buffer...
     * This either simply checks for data, or attempts first to fill the buffer
     * with data from the MIDI hardware; this depends on the implementation. */
    if (Pm_Poll(midi) == 0)
        return 0;

    head = midi->head;
    while (head != midi->tail && n < length) {
        *buffer++ = midi->buffer[head++];
        if (head == midi->buffer_len) head = 0;
        n++;
    }
    midi->head = head;
    if (midi->overflow) {
        midi->head = midi->tail;
        midi->overflow = FALSE;
        return pmBufferOverflow;
    }
    return n;
}

PmError Pm_Poll( PortMidiStream *stream )
{
    PmInternal *midi = (PmInternal *) stream;
    PmError result;

    /* arg checking */
    if(midi == NULL)
        return pmBadPtr;
    if(Pm_HasHostError(midi))
        return pmHostError;
    if(!descriptors[midi->device_id].pub.opened)
        return pmBadPtr;
    if(!descriptors[midi->device_id].pub.input)
        return pmBadPtr;
    
    result = (*(midi->dictionary->poll))(midi);
    if (result != pmNoError)
        return result;
    else
        return midi->head != midi->tail;
}


PmError Pm_Write( PortMidiStream *stream, PmEvent *buffer, long length)
{
    PmInternal *midi = (PmInternal *) stream;
    
    /* arg checking */
    if(midi == NULL)
        return pmBadPtr;
    if(Pm_HasHostError(midi))
        return pmHostError;
    if(!descriptors[midi->device_id].pub.opened)
        return pmBadPtr;
    if(!descriptors[midi->device_id].pub.output)
        return pmBadPtr;
    
    return (*midi->dictionary->write)(midi, buffer, length);
}


PmError Pm_WriteShort( PortMidiStream *stream, long when, long msg)
{
    PmEvent event;
    
    event.timestamp = when;
    event.message = msg;
    return Pm_Write(stream, &event, 1);
}


PmError Pm_WriteSysEx(PortMidiStream *stream, PmTimestamp when, unsigned char *msg)
{
    /* allocate buffer space for PM_DEFAULT_SYSEX_BUFFER_SIZE bytes */
    /* each PmEvent holds sizeof(PmMessage) bytes of sysex data */
#define BUFLEN (PM_DEFAULT_SYSEX_BUFFER_SIZE / sizeof(PmMessage))
#define MIDI_EOX        0xf7
    PmEvent buffer[BUFLEN];
    /* the next byte in the buffer is represented by an index, bufx, and
       a shift in bits */
    int shift = 0;
    int bufx = 0;
    buffer[0].message = 0;
    buffer[0].timestamp = when;

    while (1) {
        /* insert next byte into buffer */
        buffer[bufx].message |= ((*msg) << shift);
        shift += 8;
        if (shift == 32) {
            shift = 0;
            bufx++;
            if (bufx == BUFLEN) {
                PmError err = Pm_Write(stream, buffer, BUFLEN);
                if (err) return err;
                /* prepare to fill another buffer */
                bufx = 0;
            }
            buffer[bufx].message = 0;
            buffer[bufx].timestamp = when;
        }
        /* keep inserting bytes until you find MIDI_EOX */
        if (*msg++ == MIDI_EOX) break;
    }

    /* we're finished sending full buffers, but there may
     * be a partial one left.
     */
    if (shift != 0) bufx++; /* add partial message to buffer len */
    if (bufx) { /* bufx is number of PmEvents to send from buffer */
        return Pm_Write(stream, buffer, bufx);
    }
    return pmNoError;
}



PmError Pm_OpenInput(PortMidiStream** stream,
                     PmDeviceID inputDevice,
                     void *inputDriverInfo,
                     long bufferSize,
                     PmTimeProcPtr time_proc,
                     void *time_info,
                     PmStream *thru) {
    PmInternal *midi, *midiThru;
    PmError err;
    pm_hosterror = FALSE;
    *stream = NULL;
    
    /* arg checking */
    if (inputDevice < 0 || inputDevice >= descriptor_index) 
        return pmInvalidDeviceId;
    if (!descriptors[inputDevice].pub.input) 
        return pmBadPtr;
    if(descriptors[inputDevice].pub.opened)
        return pmBadPtr;
        
    /* create portMidi internal data */
    midi = (PmInternal *) pm_alloc(sizeof(PmInternal)); 
    *stream = midi;
    if (!midi) 
        return pmInsufficientMemory;
    midi->device_id = inputDevice;
    midi->write_flag = FALSE;
    midi->time_proc = time_proc;
    midi->time_info = time_info;
    if (bufferSize <= 0) bufferSize = 256; /* default buffer size */
    else bufferSize++; /* buffer holds N-1 msgs, so increase request by 1 */
    midi->buffer_len = bufferSize; /* portMidi input storage */
    midi->buffer = (PmEvent *) pm_alloc(sizeof(PmEvent) * midi->buffer_len); 
    if (!midi->buffer) { 
        /* free portMidi data */
        *stream = NULL;
        pm_free(midi); 
        return pmInsufficientMemory;
    }
    midi->head = 0;
    midi->tail = 0;
    midi->latency = 0; /* not used */
    midi->overflow = FALSE;
    midi->flush = FALSE;
    midi->sysex_in_progress = FALSE;
    midi->filters = PM_FILT_ACTIVE;
    midiThru = (PmInternal *) thru;
    /* verify midi thru is acceptable device */
    if(midiThru && 
        (!descriptors[midiThru->device_id].pub.opened ||
         !descriptors[midiThru->device_id].pub.output)) {
        /* failed, release portMidi internal data */
        *stream = NULL;
        pm_free(midi->buffer); 
        pm_free(midi); 
        return pmBadPtr;
    }
    midi->thru = midiThru;
	midi->callback_thru_error = FALSE;
    midi->dictionary = descriptors[inputDevice].dictionary; 
    descriptors[inputDevice].internalDescriptor = midi;
    /* open system dependent input device */
    err = (*midi->dictionary->open)(midi, inputDriverInfo);
    if (err) {
        *stream = NULL;
        descriptors[inputDevice].internalDescriptor = NULL;
        /* free portMidi data */
        pm_free(midi->buffer);  
        pm_free(midi);
    } else {
        /* portMidi input open successful */
        descriptors[inputDevice].pub.opened = TRUE;
    }
    return err;
}


PmError Pm_OpenOutput(PortMidiStream** stream,
                      PmDeviceID outputDevice,
                      void *outputDriverInfo,
                      long bufferSize,
                      PmTimeProcPtr time_proc,
                      void *time_info,
                      long latency ) {
    PmInternal *midi;
    PmError err;
    pm_hosterror = FALSE;
    *stream =  NULL;
    
    /* arg checking */
    if (outputDevice < 0 || outputDevice >= descriptor_index) 
        return pmInvalidDeviceId;
    if (!descriptors[outputDevice].pub.output) 
        return pmBadPtr;
    if(descriptors[outputDevice].pub.opened)
        return pmBadPtr;

    /* create portMidi internal data */
    midi = (PmInternal *) pm_alloc(sizeof(PmInternal)); 
    *stream = midi;                 
    if (!midi) 
        return pmInsufficientMemory;
    midi->device_id = outputDevice;
    midi->write_flag = TRUE;
    midi->time_proc = time_proc;
    midi->time_info = time_info;
    /* when stream used, this buffer allocated and used by 
        winmm_out_open; deleted by winmm_out_close */
    midi->buffer_len = bufferSize;
    midi->buffer = NULL;
    midi->head = 0; /* unused by output */
    midi->tail = 0; /* unused by output */
    /* if latency zero, output immediate (timestamps ignored) */
    /* if latency < 0, use 0 but don't return an error */
    if (latency < 0) latency = 0;
    midi->latency = latency;
    midi->overflow = FALSE; /* not used */
    midi->flush = FALSE; /* not used */
    midi->sysex_in_progress = FALSE; /* not used */
    midi->filters = 0; /* not used for output */
    midi->thru = NULL; /* not used */
    midi->callback_thru_error = FALSE; /* not used */
	midi->dictionary = descriptors[outputDevice].dictionary;
    descriptors[outputDevice].internalDescriptor = midi;
    /* open system dependent output device */
    err = (*midi->dictionary->open)(midi, outputDriverInfo);
    if (err) {
        *stream = NULL;
        descriptors[outputDevice].internalDescriptor = NULL;
        /* free portMidi data */
        pm_free(midi); 
    } else {
        /* portMidi input open successful */
        descriptors[outputDevice].pub.opened = TRUE;
    }
    return err;
}


PmError Pm_SetFilter(PortMidiStream *stream, long filters) {
    PmInternal *midi = (PmInternal *) stream;
    
    /* arg checking */
    if (midi == NULL)
        return pmBadPtr;
    if (!descriptors[midi->device_id].pub.opened)
        return pmBadPtr;

    midi->filters = filters;
    return pmNoError;
}


PmError Pm_Close( PortMidiStream *stream ) {
    PmInternal *midi = (PmInternal *) stream;
    PmError err;

    /* arg checking */
    if (midi == NULL) /* midi must point to something */
        return pmBadPtr;
	/* if it is an open device, the device_id will be valid */
	if (midi->device_id < 0 || midi->device_id >= descriptor_index)
		return pmBadPtr;
	/* and the device should be in the opened state */
    if (!descriptors[midi->device_id].pub.opened)
        return pmBadPtr;
    
    /* close the device */
    err = (*midi->dictionary->close)(midi);
    descriptors[midi->device_id].internalDescriptor = NULL;
    descriptors[midi->device_id].pub.opened = FALSE;
    pm_free(midi->buffer);       
    pm_free(midi); 

    return err;
}


PmError Pm_Abort( PortMidiStream* stream ) {
    PmInternal *midi = (PmInternal *) stream;

    /* arg checking */
    if (midi == NULL)
        return pmBadPtr;
    if (!descriptors[midi->device_id].pub.output)
        return pmBadPtr;
    if (!descriptors[midi->device_id].pub.opened)
        return pmBadPtr;

    return (*midi->dictionary->abort)(midi);
}

#include<assert.h> /* for debugging (temporary) */

/* in win32 multimedia API (via callbacks) some of these functions used; assume never fail */
long pm_next_time(PmInternal *midi) {

    /* arg checking */
    assert(midi != NULL);
    assert(!Pm_HasHostError(midi));
    
    return midi->buffer[midi->head].timestamp;
}

/* pm_enqueue is where system-dependent MIDI input handlers
   supply MIDI data to the system-independent PortMIDI code.
   The supplier MUST obey these rules:
   1) all input messages must be sent to pm_enqueue, which
      in turn sends them on to the MIDI THRU port if any, as
      well as enqueueing them to a FIFO for the application.
   2) While a sysex message is started, 
      the flag midi->sysex_in_progress must be set
   3) After the completion of a sysex message, the flags
      midi->sysex_in_progress midi->flush must be cleared
   (Note: we don't manage the sysex_in_progress flag in 
    pm_enqueue because it would require us to look at every
	incoming byte again.)
   (Note also that the overflow flag is managed by pm_enqueue
    and Pm_Read, so the supplier should not read or write it.)
 */
/* pm_enqueue is the place where all input messages arrive from 
   system-dependent code such as pmwinmm.c. Here, the messages
   are sent onto the THRU port, if any, messages are filtered,
   and messages are entered into the PortMidi input buffer. If
   an error occurs writing to midi thru, it is recorded in
   midi->callback_thru_error. This may not be the best-chosen
   name because midi does not always arrive via a callback. In 
   any case pm_enqueue MIGHT be called asynchronously, so we have
   to be very careful not to lose error reports.
 */

 /* Algorithnm:
	forward data to THRU port immediately
	if overflow or flush, return
	if message is filtered, return
	ATOMIC:
	    enqueue data
	    if buffer overflow, set overflow
		if buffer overflow and sysex_in_progress, set flush
  */
void pm_enqueue(PmInternal *midi, PmEvent *event)
{ 
    long tail;
    int status = Pm_MessageStatus(event->message);
    /* arg checking */
    assert(midi != NULL);
    assert(!Pm_HasHostError(midi));

    /* handle thru before any filtering takes place */
    if (midi->thru) {
        midi->callback_thru_error = Pm_Write(midi->thru, event, 1);
    }
    /* don't try to do anything more in an overflow state */
    if (midi->overflow || midi->flush) return;
    if ((status == MIDI_ACTIVE) && 
        (midi->filters & PM_FILT_ACTIVE)) { 
        /* MIDI active sensing filter: do nothing */
    } else if ((status == MIDI_CLOCK) && 
               (midi->filters & PM_FILT_CLOCK)) {
        /* MIDI clock filter: do nothing */
    } else if (midi->sysex_in_progress & PM_FILT_SYSEX) {
        /* MIDI sysex filter: do nothing */
    } else {
        tail = midi->tail;
        midi->buffer[tail++] = *event;
        if (tail == midi->buffer_len) tail = 0;
        if (tail == midi->head || midi->overflow) {
            midi->overflow = TRUE;
            if (midi->sysex_in_progress) midi->flush = TRUE; 
			/* drop the rest of the message, this must be cleared 
			   by caller when EOX is received */
            return;
        }
        midi->tail = tail; /* complete the write */
    }
}

int pm_queue_full(PmInternal *midi)
{
    long tail;

    /* arg checking */
    assert(midi != NULL);
    assert(!Pm_HasHostError(midi));
    
    tail = midi->tail + 1;
    if (tail == midi->buffer_len) tail = 0;
    return tail == midi->head;
}

