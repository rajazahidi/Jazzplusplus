/*
 * Platform interface to the MacOS X CoreMIDI framework
 * 
 * Jon Parise <jparise@cmu.edu>
 * and subsequent work by Andrew Zeldis and Zico Kolter
 *
 * $Id: pmmacosxcm.c,v 1.1.1.1 2004-03-25 07:29:21 patearl Exp $
 */

#include <stdlib.h>

#include "portmidi.h"
#include "pminternal.h"
#include "porttime.h"
#include "pmmac.h"
#include "pmmacosxcm.h"

#include <stdio.h>
#include <string.h>

#include <CoreServices/CoreServices.h>
#include <CoreMIDI/MIDIServices.h>
#include <CoreAudio/HostTime.h>

#define PACKET_BUFFER_SIZE 1024

#define VERBOSE_ON 1
#define VERBOSE if (VERBOSE_ON)

static MIDIClientRef	client = NULL;	/* Client handle to the MIDI server */
static MIDIPortRef	portIn = NULL;	/* Input port handle */
static MIDIPortRef	portOut = NULL;	/* Output port handle */

extern pm_fns_node pm_macosx_in_dictionary;
extern pm_fns_node pm_macosx_out_dictionary;

/* we need a local variable, because it needs to be of type OSStatus */
OSStatus macHostError;
CFStringRef hostErrorMsg;
#define TEST_HOST_ERROR(msg) 		\
{					\
if (macHostError != noErr) { 		\
    hostErrorMsg = CFSTR(msg); 		\
    return pmHostError; 		\
}					\
}

#define SET_HOST_ERROR(msg) hostErrorMsg = CFSTR(msg);

/* private function declarations */
MIDITimeStamp timestamp_pm_to_cm(PmTimestamp timestamp);
PmTimestamp timestamp_cm_to_pm(MIDITimeStamp timestamp);

char* cm_get_full_endpoint_name(MIDIEndpointRef endpoint);


static int
midi_length(long msg)
{
    int status, high, low;
    static int high_lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1,         /* 0x00 through 0x70 */
        3, 3, 3, 3, 2, 2, 3, 1          /* 0x80 through 0xf0 */
    };
    static int low_lengths[] = {
        1, 1, 3, 2, 1, 1, 1, 1,         /* 0xf0 through 0xf8 */
        1, 1, 1, 1, 1, 1, 1, 1          /* 0xf9 through 0xff */
    };

    status = msg & 0xFF;
    high = status >> 4;
    low = status & 15;

    return (high != 0xF0) ? high_lengths[high] : low_lengths[low];
}

static PmTimestamp
get_timestamp(PmInternal *midi)
{
    PmTimeProcPtr time_proc;

    /* Set the time procedure accordingly */
    time_proc = midi->time_proc;
    if (time_proc == NULL) {
        time_proc = Pt_Time;
    }

    return (*time_proc)(midi->time_info);
}

/* called when MIDI packets are received */
static void
readProc(const MIDIPacketList *newPackets, void *refCon, void *connRefCon)
{
    PmInternal *midi;
    PmEvent event;
    MIDIPacket *packet;
    UInt64 currentTime;
    unsigned int packetIndex;

    /* Retrieve the context for this connection */
    midi = (PmInternal *) connRefCon;

    packet = (MIDIPacket *) &newPackets->packet[0];
    for (packetIndex = 0; packetIndex < newPackets->numPackets; packetIndex++) {

        /* Build the PmMessage for the PmEvent structure */
        switch (packet->length) {
            case 1:
                event.message = Pm_Message(packet->data[0], 0, 0);
                break;
            case 2:
                event.message = Pm_Message(packet->data[0], packet->data[1], 0);
                break;
            case 3:
                event.message = Pm_Message(packet->data[0], packet->data[1], packet->data[2]);
                break;
            default:
                /* Skip packets that are too large to fit in a PmMessage */
                continue;
        }

        /* Set the timestamp and dispatch this message */
        currentTime = AudioGetCurrentHostTime();
        event.timestamp = get_timestamp(midi);
        if (packet->timeStamp > currentTime) {
            event.timestamp += timestamp_cm_to_pm(packet->timeStamp - currentTime);
        } else {
            event.timestamp -= timestamp_cm_to_pm(currentTime - packet->timeStamp);
        }
        pm_enqueue(midi, &event);

        /* Advance to the next packet in the packet list */
        packet = MIDIPacketNext(packet);
    }
}


static PmError
midi_in_open(PmInternal *midi, void *driverInfo)
{
    MIDIEndpointRef endpoint;

    endpoint = (MIDIEndpointRef) descriptors[midi->device_id].descriptor;
    if (endpoint == NULL) {
        return pmInvalidDeviceId;
    }

    macHostError = MIDIPortConnectSource(portIn, endpoint, midi);
    TEST_HOST_ERROR("MIDIPortConnectSource() in midi_in_open()");

    return pmNoError;
}

static PmError
midi_in_close(PmInternal *midi)
{
    MIDIEndpointRef endpoint;

    endpoint = (MIDIEndpointRef) descriptors[midi->device_id].descriptor;
    if (endpoint == NULL) {
        return pmInvalidDeviceId;
    }

    macHostError = MIDIPortDisconnectSource(portIn, endpoint);
    TEST_HOST_ERROR("MIDIPortDisconnectSource() in midi_in_close()");

    return pmNoError;
}

static PmError
midi_out_open(PmInternal *midi, void *driverInfo)
{
    /*
     * MIDISent() only requires an output port (portOut) and a valid MIDI
     * endpoint (which we've already created and stored in the PmInternal
     * structure).  Therefore, no additional work needs to be done here to
     * open the device for output.
     */

    return pmNoError;
}

static PmError
midi_out_close(PmInternal *midi)
{
    return pmNoError;
}

static PmError
midi_abort(PmInternal *midi)
{
    return pmNoError;
}

static PmError
midi_write(PmInternal *midi, PmEvent *events, long length)
{
    Byte packetBuffer[PACKET_BUFFER_SIZE];
    MIDIEndpointRef endpoint;
    MIDIPacketList *packetList;
    MIDIPacket *packet;
    MIDITimeStamp timestamp;
    PmTimeProcPtr time_proc;
    PmEvent event;
    unsigned int pm_time;
    unsigned int eventIndex;
    unsigned int messageLength;
    Byte message[3];

    endpoint = (MIDIEndpointRef) descriptors[midi->device_id].descriptor;
    
    if (endpoint == NULL) {
        return pmInvalidDeviceId;
    }

    /* Make sure the packetBuffer is large enough */
    if (length > PACKET_BUFFER_SIZE) {
        SET_HOST_ERROR("Packet buffer too big in midi_write()");
        return pmHostError;
    }

    /*
     * Initialize the packet list. Each packet contains bytes that are to
     * be played at the same time.
     */
    packetList = (MIDIPacketList *) packetBuffer;
    if ((packet = MIDIPacketListInit(packetList)) == NULL) {
        return pmHostError;
    }

    /* Set the time procedure accordingly */
    time_proc = midi->time_proc;
    if (time_proc == NULL) {
        time_proc = Pt_Time;
    }

    /* Extract the event data and pack it into the message buffer */
    for (eventIndex = 0; eventIndex < length; eventIndex++) {
        event = events[eventIndex];

        /* Compute the timestamp */
        pm_time = (*time_proc)(midi->time_info);
        timestamp = timestamp_pm_to_cm((event.timestamp - pm_time) + midi->latency);
        timestamp += AudioGetCurrentHostTime();

        //VERBOSE printf("%d, %qu\n", pm_time, AudioGetCurrentHostTime());

        messageLength = midi_length(event.message);
        message[0] = Pm_MessageStatus(event.message);
        message[1] = Pm_MessageData1(event.message);
        message[2] = Pm_MessageData2(event.message);

        /* Add this message to the packet list */
        packet = MIDIPacketListAdd(packetList, sizeof(packetBuffer), packet,
                                   timestamp, messageLength, message);
        if (packet == NULL) {
            return pmHostError;
        }
    }

    
    macHostError = MIDISend(portOut, endpoint, packetList);
    TEST_HOST_ERROR("MIDISend() in midi_write()");

    return pmNoError;
}

static unsigned int midi_has_host_error(PmInternal *midi)
{
    return (macHostError != noErr);
}

static void midi_host_error(PmInternal *midi, char *msg, unsigned int len)
{
    CFStringRef errorString;

    /* write the error string, or don't if there isn't space */
    errorString = CFStringCreateWithFormat(NULL, NULL, CFSTR("Error %d: %@"),
                                           macHostError, hostErrorMsg);
    
    CFStringGetCString(errorString, msg, len, CFStringGetSystemEncoding());
}

MIDITimeStamp timestamp_pm_to_cm(PmTimestamp timestamp)
{
    UInt64 nanos;
    if (timestamp <= 0) {
        return (MIDITimeStamp)0;
    } else {
        nanos = (UInt64)timestamp * (UInt64)1000000;
        return (MIDITimeStamp)AudioConvertNanosToHostTime(nanos);
    }
}

PmTimestamp timestamp_cm_to_pm(MIDITimeStamp timestamp)
{
    UInt64 nanos;
    nanos = AudioConvertHostTimeToNanos(timestamp);
    return (PmTimestamp)(nanos / (UInt64)1000000);
}


char* cm_get_full_endpoint_name(MIDIEndpointRef endpoint)
{
    MIDIEntityRef entity;
    MIDIDeviceRef device;
    CFStringRef endpointName = NULL, deviceName = NULL, fullName = NULL;
    CFStringEncoding defaultEncoding;
    char* newName;

    /* get the default string encoding */
    defaultEncoding = CFStringGetSystemEncoding();

    /* get the entity and device info */
    MIDIEndpointGetEntity(endpoint, &entity);
    MIDIEntityGetDevice(entity, &device);

    /* create the nicely formated name */
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &endpointName);
    MIDIObjectGetStringProperty(device, kMIDIPropertyName, &deviceName);
    if (deviceName != NULL) {
        fullName = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@: %@"),
                                            deviceName, endpointName);
    } else {
        fullName = endpointName;
    }
    
    /* copy the string into our buffer */
    newName = (char*)malloc(CFStringGetLength(fullName) + 1);
    CFStringGetCString(fullName, newName, CFStringGetLength(fullName) + 1,
                        defaultEncoding);

    /* clean up */
    if (endpointName) CFRelease(endpointName);
    if (deviceName) CFRelease(deviceName);
    if (fullName) CFRelease(fullName);

    return newName;
}

 

pm_fns_node pm_macosx_in_dictionary = {
    none_write,
    midi_in_open,
    midi_abort,
    midi_in_close,
    success_poll,
    midi_has_host_error,
    midi_host_error,
};

pm_fns_node pm_macosx_out_dictionary = {
    midi_write,
    midi_out_open,
    midi_abort,
    midi_out_close,
    success_poll,
    midi_has_host_error,
    midi_host_error,
};


PmError pm_macosxcm_init(void)
{
    ItemCount numInputs, numOutputs, numDevices;
    MIDIEndpointRef endpoint;
    int i;

    /* Determine the number of MIDI devices on the system */
    numDevices = MIDIGetNumberOfDevices();
    numInputs = MIDIGetNumberOfSources();
    numOutputs = MIDIGetNumberOfDestinations();

    /* Return prematurely if no devices exist on the system */
    if (numDevices <= 0) {
        SET_HOST_ERROR("No valid MIDI devices.");
        return pmHostError;
    }

    /* Iterate over the MIDI input devices */
    for (i = 0; i < numInputs; i++) {
        endpoint = MIDIGetSource(i);
        if (endpoint == NULL) {
            continue;
        }

        /* set the first input we see to the default */
        if (pm_default_input_device_id == -1)
            pm_default_input_device_id = descriptor_index;
        
        /* Register this device with PortMidi */
        pm_add_device("CoreMIDI", cm_get_full_endpoint_name(endpoint),
                      TRUE, (void*)endpoint, &pm_macosx_in_dictionary);
    }

    /* Iterate over the MIDI output devices */
    for (i = 0; i < numOutputs; i++) {
        endpoint = MIDIGetDestination(i);
        if (endpoint == NULL) {
            continue;
        }

        /* set the first output we see to the default */
        if (pm_default_output_device_id == -1)
            pm_default_output_device_id = descriptor_index;

        /* Register this device with PortMidi */
        pm_add_device("CoreMIDI", cm_get_full_endpoint_name(endpoint),
                      FALSE, (void*)endpoint, &pm_macosx_out_dictionary);
    }
        

    /* Initialize the client handle */
    macHostError = MIDIClientCreate(CFSTR("PortMidi"), NULL, NULL, &client);
    TEST_HOST_ERROR("MIDIClientCreate() in pm_macosxcm_init()");

    /* Create the input port */
    macHostError = MIDIInputPortCreate(client, CFSTR("Input port"), readProc,
                                          NULL, &portIn);
    TEST_HOST_ERROR("MIDIInputPortCreate() in pm_macosxcm_init()");
        
    /* Create the output port */
    macHostError = MIDIOutputPortCreate(client, CFSTR("Output port"), &portOut);
    TEST_HOST_ERROR("MIDIOutputPortCreate() in pm_macosxcm_init()");

    return pmNoError;
}

void pm_macosxcm_term(void)
{
    if (client != NULL)		MIDIClientDispose(client);
    if (portIn != NULL)		MIDIPortDispose(portIn);
    if (portOut != NULL)	MIDIPortDispose(portOut);
}