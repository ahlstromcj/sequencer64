/**
 * \file          midi_core.cpp
 *
 *    A class for realtime MIDI input/output via the Mac OSX Core API.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    The CoreMIDI API is based on the use of a callback function for MIDI
 *    input.  We convert the system specific time stamps to delta time values.
 *
 *  In this refactoring...
 *
 *  API information found at:
 *
 *      - http://www.......
 */

#include <sstream>

#include <CoreMIDI/CoreMIDI.h>
#include <CoreAudio/HostTime.h>
#include <CoreServices/CoreServices.h>

#include "midi_core.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * A structure to hold variables related to the CoreMIDI API
 * implementation.
 */

struct CoreMidiData
{
    MIDIClientRef client;
    MIDIPortRef port;
    MIDIEndpointRef endpoint;
    MIDIEndpointRef destinationId;
    unsigned long long lastTime;
    MIDISysexSendRequest SysExreq;
};

/**
 *  The Core API MIDI input callback function.  From the original author:
 *
 *      My interpretation of the CoreMIDI documentation: all message types,
 *      except SysEx, are complete within a packet, and there may be several
 *      of them in a single packet.  SysEx messages can be broken across
 *      multiple packets and PacketLists but are bundled alone within each
 *      packet (these packets do not contain other message types).  If SysEx
 *      messages are split across multiple MIDIPacketLists, they must be
 *      handled by multiple calls to this function.
 *
 * \param list
 *      The MIDI packet list.
 *
 * \param procRef
 *      Provides a pointer to....
 *
 * \param srcRef
 *      Provides a ????, but this parameter is not used.
 */

static void midiInputCallback
(
   const MIDIPacketList * list,
   void * procRef,
   void * /*srcRef*/
)
{
    rtmidi_in_data * rtindata = static_cast<rtmidi_in_data *>(procRef);
    CoreMidiData * coredata = static_cast<CoreMidiData *>(rtindata->api_data());
    midibyte status;
    unsigned short nBytes, iByte, size;
    unsigned long long time;
    bool continuesysex = rtindata->continue_sysex();
    midi_message & message = rtindata->message();           // const?
    const MIDIPacket * packet = &list->packet[0];
    for (unsigned i = 0; i < list->numPackets; ++i)
    {
        /*
         * See banner notes.
         */

        nBytes = packet->length;
        if (nBytes == 0)
            continue;

        /* Calculate time stamp.    */

        if (rtindata->first_message())
        {
            message.timeStamp = 0.0;
            rtindata->first_message(false);
        }
        else
        {
            time = packet->timeStamp;

            /*
             *  Time equals 0: this happens when receiving asynchronous SysEx
             *  messages.
             */

            if (time == 0)
                time = AudioGetCurrentHostTime();

            time -= coredata->lastTime;
            time = AudioConvertHostTimeToNanos(time);
            if (! continuesysex)
                message.timeStamp = time * 0.000000001;
        }
        coredata->lastTime = packet->timeStamp;

        /*
         *  "Last time" equals 0: this happens when receiving asynchronous
         *  SysEx messages
         */

        if (coredata->lastTime == 0)
            coredata->lastTime = AudioGetCurrentHostTime();

        iByte = 0;
        if (continuesysex)
        {
            // We have a continuing, segmented SysEx message.

            if (! rtindata->test_ignore_flags(0x01))
            {
                // If we're not ignoring SysEx messages, copy the entire packet.

                for (unsigned j = 0; j < nBytes; ++j)
                    message.bytes.push_back(packet->data[j]);
            }
            continuesysex = packet->data[nBytes - 1] != 0xF7;
            rtindata->continue_sysex(continuesysex);
            if (! rtindata->test_ignore_flags(0x01) && ! continuesysex)
            {
                /*
                 * If not a continuing SysEx message, invoke the user callback
                 * function or queue the message.
                 */

                if (rtindata->using_callback())
                {
                    rtmidi_callback_t callback = rtindata->user_callback();
                    callback
                    (
                        message.timeStamp, &message.bytes,
                        rtindata->user_data()
                    );
                }
                else
                    (void) rtindata->queue().add(message);

                message.bytes.clear();
            }
        }
        else
        {
            while (iByte < nBytes)
            {
                /*
                 * We expectthat the next byte in the packet is a status byte.
                 */

                size = 0;
                status = packet->data[iByte];
                if (!(status & 0x80))
                    break;

                // Determine the number of bytes in the MIDI message.

                if (status < 0xC0)
                    size = 3;
                else if (status < 0xE0)
                    size = 2;
                else if (status < 0xF0)
                    size = 3;
                else if (status == 0xF0)
                {
                    if (rtindata->test_ignore_flags(0x01))  // MIDI SysEx
                    {
                        size = 0;
                        iByte = nBytes;
                    }
                    else
                        size = nBytes - iByte;

                    continuesysex = packet->data[nBytes - 1] != 0xF7;
                    rtindata->continue_sysex(continuesysex);
                }
                else if (status == 0xF1)
                {
                    if (rtindata->test_ignore_flags(0x02))  // MIDI time code
                    {
                        size = 0;
                        iByte += 2;
                    }
                    else
                        size = 2;
                }
                else if (status == 0xF2)
                    size = 3;
                else if (status == 0xF3)
                    size = 2;
                else if (status == 0xF8 && rtindata->test_ignore_flags(0x02))
                {
                    size = 0;                   // MIDI timing tick message ...
                    iByte += 1;                 // ...and we're ignoring it.
                }
                else if (status == 0xFE && rtindata->test_ignore_flags(0x04))
                {
                    size = 0;                   // MIDI active sensing message...
                    iByte += 1;                 // ...and we're ignoring it.
                }
                else
                    size = 1;

                if (size > 0)                   // copy the data to our vector
                {
                    message.bytes.assign
                    (
                        &packet->data[iByte], &packet->data[iByte + size]
                    );
                    if (! continuesysex)
                    {
                        /*
                         * If not a continuing SysEx message, invoke the user
                         * callback function or queue the message.
                         */

                        if (rtindata->using_callback())
                        {
                            rtmidi_callback_t callback = data->user_callback();
                            callback
                            (
                                message.timeStamp, &message.bytes,
                                rtindata->userdata
                            );
                        }
                        else
                            (void) rtindata->queue.add(message);

                        message.bytes.clear();
                    }
                    iByte += size;
                }
            }
        }
        packet = MIDIPacketNext(packet);
    }
}

/**
 *  Principal constructor.
 */

midi_in_core::midi_in_core
(
    const std::string clientname,
    unsigned queuesize
)
 :
    midi_in_api (queuesize)
{
    initialize(clientname);
}

/**
 *  Destructor.
 *
 * Close a connection if it exists, then do cleanup.
 */

midi_in_core::~midi_in_core ()
{
    close_port();
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    MIDIClientDispose(coredata->client);
    if (coredata->endpoint)
        MIDIEndpointDispose(coredata->endpoint);

    delete data;
}

/**
 *  Initializes the MIDI input.
 *
 * \param clientname
 *      The name to provide for the client.
 */

void
midi_in_core::initialize (const std::string & clientname)
{
    MIDIClientRef client;
    CFStringRef name = CFStringCreateWithCString
    (
        NULL, clientname.c_str(), kCFStringEncodingASCII
    );
    OSStatus result = MIDIClientCreate(name, NULL, NULL, &client);
    if (result != noErr)
    {
        /*
         * CA notes:  Oddly enough, I prefer to use snprintf() and a
         * character buffer :-)
         */

        std::ostringstream ost;
        ost
            << func_message("error creating OS-X MIDI client object (")
            << result << ")"
            ;
        m_error_string = ost.str();
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    // Save our api-specific connection information.

    CoreMidiData * coredata = static_cast<CoreMidiData *>
    (
        new (std::nothrow) CoreMidiData()
    );
    coredata->client = client;
    coredata->endpoint = 0;
    m_api_data = reinterpret_cast<void *>(coredata);
    m_input_data.api_data(reinterpret_cast<void *>(coredata));
    CFRelease(name);
}

/**
 *  Opens the Core MIDI input port.
 *
 * \param portnumber
 *      The port number to be opened?
 *
 * \param portname
 *      The name to assign to the port, or the name of the port to open?
 */

void
midi_in_core::open_port (unsigned portnumber, const std::string & portname)
{
    if (m_connected)
    {
        m_error_string = func_message("valid connection already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }

    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    unsigned nSrc = MIDIGetNumberOfSources();
    if (nSrc < 1)
    {
        m_error_string = func_message("no MIDI input sources found");
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }
    if (portnumber >= nSrc)
    {
        std::ostringstream ost;
        ost
            << func_message("the 'portnumber' argument (")
            << portnumber << ") is invalid"
            ;
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
        return;
    }

    MIDIPortRef port;
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    OSStatus result = MIDIInputPortCreate
    (
        coredata->client,
        CFStringCreateWithCString(NULL, portname.c_str(), kCFStringEncodingASCII),
        midiInputCallback, (void *) &m_input_data, &port
    );
    if (result != noErr)
    {
        MIDIClientDispose(coredata->client);
        m_error_string = func_message("error creating OS-X MIDI input port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    // Get the desired input source identifier.

    MIDIEndpointRef endpoint = MIDIGetSource(portnumber);
    if (endpoint == 0)
    {
        MIDIPortDispose(port);
        MIDIClientDispose(coredata->client);
        m_error_string = func_message("error getting MIDI input source");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    // Make the connection.

    result = MIDIPortConnectSource(port, endpoint, NULL);
    if (result != noErr)
    {
        MIDIPortDispose(port);
        MIDIClientDispose(coredata->client);
        m_error_string = func_message("error connecting OS-X MIDI input port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    coredata->port = port;          // Save our api-specific port information
    m_connected = true;
}

/**
 *  Opens a virtual input port.  This function creates a virtual MIDI input
 *  destination.
 *
 * \param portname
 *      The name of the port.
 */

void
midi_in_core::open_virtual_port (const std::string & portname)
{
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    MIDIEndpointRef endpoint;
    OSStatus result = MIDIDestinationCreate
    (
        coredata->client,
        CFStringCreateWithCString(NULL, portname.c_str(), kCFStringEncodingASCII),
        midiInputCallback, (void *) &m_input_data, &endpoint
    );
    if (result != noErr)
    {
        m_error_string =
            func_message("error creating virtual OS-X MIDI destination");

        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    coredata->endpoint = endpoint; // save API-specific connection information
}

/**
 *  Closes the MIDI input port.
 */

void
midi_in_core::close_port ()
{
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    if (coredata->endpoint)
        MIDIEndpointDispose(coredata->endpoint);

    if (coredata->port)
        MIDIPortDispose(coredata->port);

    m_connected = false;
}

/**
 *  Get the MIDI input port count.
 *
 * \return
 *      Returns the port count from MIDIGetNumberOfSources().
 */

unsigned
midi_in_core::get_port_count ()
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    return MIDIGetNumberOfSources();
}

/**
 *  Gets the endpoint name from an endpoint.
 *
 *  This function was submitted by Douglas Casey Tucker and apparently
 *  derived largely from PortMidi.
 *
 * \param endpoint
 *      The endpoint reference object.
 *
 * \param isExternal
 *      True if the endpoint is external?
 *
 * \return
 *      Returns a string reference object from Core API.
 */

CFStringRef
EndpointName (MIDIEndpointRef endpoint, bool isExternal)
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;

    // Begin with the endpoint's name.

    str = NULL;
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &str);
    if (str != NULL)
    {
        CFStringAppend(result, str);
        CFRelease(str);
    }

    MIDIEntityRef entity = 0;
    MIDIEndpointGetEntity(endpoint, &entity);
    if (entity == 0)                            // probably virtual
        return result;

    if (CFStringGetLength(result) == 0)
    {
        // endpoint name has zero length -- try the entity

        str = NULL;
        MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &str);
        if (str != NULL)
        {
            CFStringAppend(result, str);
            CFRelease(str);
        }
    }

    // now consider the device's name

    MIDIDeviceRef device = 0;
    MIDIEntityGetDevice(entity, &device);
    if (device == 0)
        return result;

    str = NULL;
    MIDIObjectGetStringProperty(device, kMIDIPropertyName, &str);
    if (CFStringGetLength(result) == 0)
    {
        CFRelease(result);
        return str;
    }
    if (str != NULL)
    {
        /*
         * If an external device has only one entity, throw away the endpoint
         * name and just use the device name.
         */

        if (isExternal && MIDIDeviceGetNumberOfEntities(device) < 2)
        {
            CFRelease(result);
            return str;
        }
        else
        {
            if (CFStringGetLength(str) == 0)
            {
                CFRelease(str);
                return result;
            }

            /*
             * Does the entity name already start with the device name?  (Some
             * drivers do this, though they shouldn't.) If so, do not prepend.
             * Otherwise, prepend the device name to the entity name.
             */

            if
            (
                CFStringCompareWithOptions
                (
                    result,     /* endpoint name */
                    str,        /* device name */
                    CFRangeMake(0, CFStringGetLength(str)), 0
                    ) != kCFCompareEqualTo
            )
            {
                if (CFStringGetLength(result) > 0)
                    CFStringInsert(result, 0, CFSTR(" "));

                CFStringInsert(result, 0, str);
            }
            CFRelease(str);
        }
    }
    return result;
}

/**
 *  Gets the connected endpoint name.
 *
 *  This function was submitted by Douglas Casey Tucker and apparently
 *  derived largely from PortMidi.
 *
 * \param endpoint
 *      The MIDI endpointer reference.
 *
 * \return
 *      Returns a string reference object from Core API.
 */

static CFStringRef
ConnectedEndpointName (MIDIEndpointRef endpoint)
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;
    OSStatus err;
    int i;
    CFDataRef connections = NULL;   // does the endpoint have connections?
    int nConnected = 0;
    bool anyStrings = false;
    err = MIDIObjectGetDataProperty
    (
        endpoint, kMIDIPropertyConnectionUniqueID, &connections
    );
    if (connections != NULL)
    {
        /*
         * It has connections, follow them.  Concatenate the names of all
         * connected devices.
         */

        nConnected = CFDataGetLength(connections) / sizeof(MIDIUniqueID);
        if (nConnected)
        {
            const SInt32 * pid = (const SInt32 *)(CFDataGetBytePtr(connections));
            for (i = 0; i < nConnected; ++i, ++pid)
            {
                MIDIUniqueID id = EndianS32_BtoN(*pid);
                MIDIObjectRef connObject;
                MIDIObjectType connObjectType;
                err = MIDIObjectFindByUniqueID(id, &connObject, &connObjectType);
                if (err == noErr)
                {
                    if
                    (
                        connObjectType == kMIDIObjectType_ExternalSource  ||
                        connObjectType == kMIDIObjectType_ExternalDestination
                    )
                    {
                        /*
                         * Connected to an external device's endpoint (10.3
                         * and later).
                         */

                        str = EndpointName((MIDIEndpointRef)(connObject), true);
                    }
                    else
                    {
                        /*
                         * Connected to an external device (10.2) (or
                         * something else, catch-
                         */

                        str = NULL;
                        MIDIObjectGetStringProperty
                        (
                            connObject, kMIDIPropertyName, &str
                        );
                    }
                    if (str != NULL)
                    {
                        if (anyStrings)
                            CFStringAppend(result, CFSTR(", "));
                        else
                            anyStrings = true;

                        CFStringAppend(result, str);
                        CFRelease(str);
                    }
                }
            }
        }
        CFRelease(connections);
    }
    if (anyStrings)
        return result;

    CFRelease(result);

    /*
     * Here, either the endpoint had no connections, or we failed to obtain
     * names.
     */

    return EndpointName(endpoint, false);
}

/**
 *  Gets the input ports name.
 *
 * \param portnumber
 *      The number of the port to query.
 *
 * \return
 *      Returns the port name as a standard C++ string.
 */

std::string
midi_in_core::get_port_name (unsigned portnumber)
{
    CFStringRef nameRef;
    MIDIEndpointRef portRef;
    char name[128];
    std::string stringName;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    if (portnumber >= MIDIGetNumberOfSources())
    {
        std::ostringstream ost;
        ost
            << func_message("'portnumber' argument (")
            << portnumber << ") is invalid"
            ;
        m_error_string = ost.str();
        error(rterror::WARNING, m_error_string);
        return stringName;                          /* empty */
    }

    portRef = MIDIGetSource(portnumber);
    nameRef = ConnectedEndpointName(portRef);
    CFStringGetCString(nameRef, name, sizeof(name), CFStringGetSystemEncoding());
    CFRelease(nameRef);
    return name;                // why the "return stringName = name;"???
}

/*
 * API: OS-X Class Definitions: midi_out_core
 */

/**
 *  Principal constructor.
 *
 * \param clientname
 *      The name of the client to initialize (or the name we want to give the
 *      client???)
 */

midi_out_core::midi_out_core (const std::string & clientname)
 :
    midi_out_api    ()
{
    initialize(clientname);
}

/**
 *  Destructor.  Closes the connection if it exists, and does some cleanup.
 */

midi_out_core::~midi_out_core ()
{
    close_port();
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    MIDIClientDispose(coredata->client);
    if (coredata->endpoint)
        MIDIEndpointDispose(coredata->endpoint);

    delete coredata;
}

/**
 *  Initializes the MIDI output port.
 *
 * \param clientname
 *      The name of the client.
 */

void
midi_out_core::initialize (const std::string & clientname)
{
    MIDIClientRef client;
    CFStringRef name = CFStringCreateWithCString
    (
        NULL, clientname.c_str(), kCFStringEncodingASCII
    );
    OSStatus result = MIDIClientCreate(name, NULL, NULL, &client);
    if (result != noErr)
    {
        std::ostringstream ost;
        ost
            << func_message("error creating OS-X MIDI client object (")
            << result << ")"
            ;
        m_error_string = ost.str();
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    // Save our API-specific connection information.

    CoreMidiData * coredata = new (std::nothrow) CoreMidiData();
    coredata->client = client;
    coredata->endpoint = 0;
    m_api_data = reinterpret_cast<void *>(coredata);
    CFRelease(name);
}

/**
 *  Retrieves the output port count.
 *
 * \return
 *      Returns the result of MIDIGetNumberOfDestinations().
 */

unsigned
midi_out_core::get_port_count ()
{
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    return MIDIGetNumberOfDestinations();
}

/**
 *  Retrieves the name of a MIDI output port.
 *
 * \param portnumber
 *      The port number to query.
 *
 * \return
 *      Returns the name as a standard C++ string.
 */

std::string
midi_out_core::get_port_name (unsigned portnumber)
{
    CFStringRef nameRef;
    MIDIEndpointRef portRef;
    char name[128];
    std::string stringName;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    if (portnumber >= MIDIGetNumberOfDestinations())
    {
        std::ostringstream ost;
        ost
            << func_message("'portnumber' argument (")
            << portnumber << ") is invalid"
           ;
        m_error_string = ost.str();
        error(rterror::WARNING, m_error_string);
        return stringName;
    }

    portRef = MIDIGetDestination(portnumber);
    nameRef = ConnectedEndpointName(portRef);
    CFStringGetCString(nameRef, name, sizeof(name), CFStringGetSystemEncoding());
    CFRelease(nameRef);
    return name;                        // weird: return stringName = name;
}

/**
 *  Opens a MIDI output port.
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name.
 */

void
midi_out_core::open_port (unsigned portnumber, const std::string & portname)
{
    if (m_connected)
    {
        m_error_string = func_message("valid connection already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }

    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    unsigned nDest = MIDIGetNumberOfDestinations();
    if (nDest < 1)
    {
        m_error_string = func_message("no MIDI output destinations found");
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }
    if (portnumber >= nDest)
    {
        std::ostringstream ost;
        ost
            << func_message("'portnumber' argument (")
            << portnumber << ") is invalid"
            ;
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
        return;
    }

    MIDIPortRef port;
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    OSStatus result = MIDIOutputPortCreate
    (
        coredata->client,
        CFStringCreateWithCString(NULL, portname.c_str(), kCFStringEncodingASCII),
        &port
    );
    if (result != noErr)
    {
        MIDIClientDispose(coredata->client);
        m_error_string = func_message("error creating OS-X MIDI output port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    // Get the desired output port identifier.

    MIDIEndpointRef destination = MIDIGetDestination(portnumber);
    if (destination == 0)
    {
        MIDIPortDispose(port);
        MIDIClientDispose(coredata->client);
        m_error_string =
            "midi_out_core::open_port(): "
            "error getting MIDI output destination reference";

        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    coredata->port = port;      // Save our api-specific connection information.
    coredata->destinationId = destination;
    m_connected = true;
}

/**
 *  Closes a MIDI output port.
 */

void midi_out_core::close_port ()
{
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    if (coredata->endpoint)
        MIDIEndpointDispose(coredata->endpoint);

    if (coredata->port)
        MIDIPortDispose(coredata->port);

    m_connected = false;
}

/**
 *  Open a virtual MIDI output port.
 */

void
midi_out_core::open_virtual_port (const std::string & portname)
{
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    if (coredata->endpoint)
    {
        m_error_string = func_message("virtual output port already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }

    // Create a virtual MIDI output source.

    MIDIEndpointRef endpoint;
    OSStatus result = MIDISourceCreate
    (
        coredata->client,
        CFStringCreateWithCString(NULL, portname.c_str(), kCFStringEncodingASCII),
        &endpoint
    );
    if (result != noErr)
    {
        m_error_string = func_message("error creating OS-X virtual MIDI source");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    coredata->endpoint = endpoint; // Save API-specific connection information.
}

/**
 *  Sends a MIDI message.  We use the MIDISendSysex() function to
 *  asynchronously send SysEx messages.  Otherwise, we use a single CoreMidi
 *  MIDIPacket.
 *
 * \param message
 *      The message bytes to be sent.  Why a pointer?
 */

void
midi_out_core::send_message (const std::vector<midibyte> & message)
{
    unsigned nBytes = message.size();
    if (nBytes == 0)
    {
        m_error_string = func_message("no data in message argument");
        error(rterror::WARNING, m_error_string);
        return;
    }

    MIDITimeStamp timeStamp = AudioGetCurrentHostTime();
    CoreMidiData * coredata = static_cast<CoreMidiData *>(m_api_data);
    OSStatus result;
    if (message.at(0) != 0xF0 && nBytes > 3)
    {
        m_error_string =
            func_message("message format problem ... not SysEx but > 3 bytes?");

        error(rterror::WARNING, m_error_string);
        return;
    }

    Byte buffer[nBytes + (sizeof(MIDIPacketList))];
    ByteCount listSize = sizeof(buffer);
    MIDIPacketList * packetList = (MIDIPacketList*)buffer;
    MIDIPacket * packet = MIDIPacketListInit(packetList);
    ByteCount remainingBytes = nBytes;
    while (remainingBytes && packet)
    {
        ByteCount bytesForPacket = remainingBytes > 65535 ?
            65535 : remainingBytes; // 65535 = maximum size of a MIDIPacket

        const Byte * dataStartPtr =
            (const Byte *) &message.at(nBytes - remainingBytes);

        packet = MIDIPacketListAdd
        (
            packetList, listSize, packet, timeStamp, bytesForPacket, dataStartPtr
        );
        remainingBytes -= bytesForPacket;
    }

    if (is_nullptr(packet))
    {
        m_error_string = func_message("could not allocate packet list");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    if (coredata->endpoint) // send to destinations that may have connected
    {
        result = MIDIReceived(coredata->endpoint, packetList);
        if (result != noErr)
        {
            m_error_string =
                func_message("error sending MIDI to virtual destinations");

            error(rterror::WARNING, m_error_string);
        }
    }

    if (m_connected)        // send to explicit destination port if connected
    {
        result = MIDISend(coredata->port, coredata->destinationId, packetList);
        if (result != noErr)
        {
            m_error_string = func_message("error sending MIDI message to port");
            error(rterror::WARNING, m_error_string);
        }
    }
}

}           // namespace seq64

/*
 * midi_core.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

