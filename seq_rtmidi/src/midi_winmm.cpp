/**
 * \file          midi_winmm.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-18
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    API information deciphered from:
 *
 *  http://msdn.microsoft.com/library/default.asp?url=/library/
 *       en-us/multimed/htm/_win32_midi_reference.asp
 *
 *    The Windows MM API is based on the use of a callback function for MIDI
 *    input.  We convert the system specific time stamps to delta time values.
 *
 *    Thanks to Jean-Baptiste Berruchon for the SysEx code.
 *
 *  In this refactoring...
 */

#include <sstream>

#include "midi_winmm.hpp"

/*
 *
 * Windows MM MIDI header files.
 */

#include <windows.h>
#include <mmsystem.h>

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 * Some constants.
 */

#define RT_SYSEX_BUFFER_SIZE    1024
#define RT_SYSEX_BUFFER_COUNT      4

/**
 *    A structure to hold variables related to the Win-MM MIDI API
 *    implementation.  Hurray, Windows handles!  (joke)
 */

struct WinMidiData
{
    HMIDIIN inHandle;                  /* Handle to Midi Input Device   */
    HMIDIOUT outHandle;                /* Handle to Midi Output Device  */
    DWORD lastTime;
    midi_in_api::midi_message message;
    LPMIDIHDR sysexBuffer[RT_SYSEX_BUFFER_COUNT];

    /*
     * [Patrice]
     * See https://groups.google.com/forum/#!topic/mididev/6OUjHutMpEo
     */

    CRITICAL_SECTION _mutex;
};

/*
 * API: Windows MM Class Definitions: midi_in_winmm
 */

/**
 *  The Window MIDI input callback.
 *
 *  The WinMM API requires that the SysEx buffer be requeued after input of
 *  each SysEx message.  Even if we are ignoring SysEx messages, we still need
 *  to requeue the buffer in case the user decides to not ignore SysEx
 *  messages in the future.  However, it seems that WinMM calls this function
 *  with an empty SysEx buffer when an application closes and in this case, we
 *  should avoid requeueing it, else the computer suddenly reboots after one
 *  or two minutes.
 *
 * \param hmin
 *      Unused parameter.
 *
 * \param inputStatus
 *      Provides ...
 *
 * \param instancePtr
 *      Provides ...
 *
 * \param midiMessage
 *      Provides ...
 *
 * \param timestamp
 *      Provides ...
 */

static void CALLBACK
midiInputCallback
(
   HMIDIIN /*hmin*/,
   UINT inputStatus,
   DWORD_PTR instancePtr,
   DWORD_PTR midiMessage,
   DWORD timestamp
)
{
    if
    (
        inputStatus != MIM_DATA && inputStatus != MIM_LONGDATA &&
        inputStatus != MIM_LONGERROR
    )
    {
        return;
    }

    // static_cast<midi_in_api::rtmidi_in_data *>

    midi_in_api::rtmidi_in_data * data =
        (midi_in_api::rtmidi_in_data *) instancePtr;

    WinMidiData * apiData = static_cast<WinMidiData *>(data->apiData);
    if (data->firstMessage == true)         // Calculate time stamp.
    {
        apiData->message.timeStamp = 0.0;
        data->firstMessage = false;
    }
    else
        apiData->message.timeStamp = double(timestamp-apiData->lastTime) * 0.001;

    apiData->lastTime = timestamp;
    if (inputStatus == MIM_DATA)     // Channel or system message
    {
        // Make sure the first byte is a status byte.

        midibyte status = (midibyte)(midiMessage & 0x000000FF);
        if (!(status & 0x80))
            return;

        // Determine the number of bytes in the MIDI message.

        unsigned short nBytes = 1;
        if (status < 0xC0)
            nBytes = 3;
        else if (status < 0xE0)
            nBytes = 2;
        else if (status < 0xF0)
            nBytes = 3;
        else if (status == 0xF1)
        {
            if (data->ignoreFlags & 0x02)
                return;
            else
                nBytes = 2;
        }
        else if (status == 0xF2)
            nBytes = 3;
        else if (status == 0xF3)
            nBytes = 2;
        else if (status == 0xF8 && (data->ignoreFlags & 0x02))
        {
            // A MIDI timing tick message and we're ignoring it.

            return;
        }
        else if (status == 0xFE && (data->ignoreFlags & 0x04))
        {
            // A MIDI active sensing message and we're ignoring it.

            return;
        }

        // Copy bytes to our MIDI message.

        midibyte * ptr = (midibyte *) &midiMessage;
        for (int i = 0; i < nBytes; ++i)
            apiData->message.bytes.push_back(*ptr++);
    }
    else   // Sysex message ( MIM_LONGDATA or MIM_LONGERROR )
    {
        MIDIHDR * sysex = (MIDIHDR *) midiMessage;
        if (!(data->ignoreFlags & 0x01) && inputStatus != MIM_LONGERROR)
        {
            // Sysex message and we're not ignoring it

            for (int i = 0; i < (int)sysex->dwBytesRecorded; ++i)
                apiData->message.bytes.push_back(sysex->lpData[i]);
        }

        /*
         * See banner notes about the WinMM API.
         */

        if (apiData->sysexBuffer[sysex->dwUser]->dwBytesRecorded > 0)
        {
            EnterCriticalSection(&(apiData->_mutex));
            MMRESULT result = midiInAddBuffer
            (
                apiData->inHandle, apiData->sysexBuffer[sysex->dwUser],
                sizeof(MIDIHDR)
            );
            LeaveCriticalSection(&(apiData->_mutex));
            if (result != MMSYSERR_NOERROR)
                errprintfunc("error sending sysex to Midi device";

            if (data->ignoreFlags & 0x01)
                return;
        }
        else
            return;
    }

    if (data->usingCallback)
    {
        rtmidi_in::rtmidi_callback_t callback =
            (rtmidi_in::rtmidi_callback_t) data->userCallback;

        callback
        (
            apiData->message.timeStamp, &apiData->message.bytes, data->userdata
        );
    }
    else
    {
        // As long as we haven't reached our queue size limit, push the message.

        if (data->queue.size < data->queue.ringSize)
        {
            data->queue.ring[data->queue.back++] = apiData->message;
            if (data->queue.back == data->queue.ringSize)
                data->queue.back = 0;

            data->queue.size++;
        }
        else
            errprintfunc("message queue limit reached");
    }

    // Clear the vector for the next input message.

    apiData->message.bytes.clear();
}

/**
 *  Principal constructor.
 *
 * \param clientname
 *      Provides the name of the MIDI input port.
 *
 * \param queuesize
 *      Provides the upper limit of the queue size.
 */

midi_in_winmm::midi_in_winmm
(
    const std::string & clientname,
    unsigned queuesize
) :
    midi_in_api (queuesize)
{
    initialize(clientname);
}

/**
 *  Destructor.  Closes a connection if it exists, then cleans up any API
 *  resources in use.
 */

midi_in_winmm::~midi_in_winmm ()
{
    WinMidiData * data = static_cast<WinMidiData *>(m_api_data);
    close_port();
    DeleteCriticalSection(&(data->_mutex));
    delete data;
}

/**
 *  Initializes the JACK client MIDI data structure.  Then calls the connect()
 *  function.
 *
 *  We'll issue a warning here if no devices are available but not throw an
 *  error since the user can plugin something later.
 *
 * \param clientname
 *      Provides the name of the client; unused.
 */

void
midi_in_winmm::initialize (const std::string & /*clientname*/)
{
    unsigned nDevices = midiInGetNumDevs();
    if (nDevices == 0)
    {
        m_error_string =
            "midi_in_winmm::initialize(): "
            "no MIDI input devices currently available.";

        error(rterror::WARNING, m_error_string);
    }

    // Save our api-specific connection information.

    WinMidiData *data = (WinMidiData *) new WinMidiData;
    m_api_data = (void *) data;
    m_input_data.apiData = (void *) data;
    data->message.bytes.clear();  // needs to be empty for first input message
    if (!InitializeCriticalSectionAndSpinCount(&(data->_mutex), 0x00000400))
    {
        m_error_string =
            "midi_in_winmm::initialize(): "
            "InitializeCriticalSectionAndSpinCount failed.";

        error(rterror::WARNING, m_error_string);
    }
}

/**
 *  Opens the WinMM MIDI input port.
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name under which the port is registered; unused.
 */

void
midi_in_winmm::open_port (unsigned portnumber, const std::string & /*portname*/)
{
    if (m_connected)
    {
        m_error_string =
            "midi_in_winmm::open_port: a valid connection already exists";

        error(rterror::WARNING, m_error_string);
        return;
    }

    unsigned nDevices = midiInGetNumDevs();
    if (nDevices == 0)
    {
        m_error_string = "midi_in_winmm::open_port: no MIDI input sources found";
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }

    if (portnumber >= nDevices)
    {
        std::ostringstream ost;
        ost
            << "midi_in_winmm::open_port: the 'portnumber' argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
        return;
    }

    WinMidiData * data = static_cast<WinMidiData *>(m_api_data);
    MMRESULT result = midiInOpen
    (
        &data->inHandle, portnumber, (DWORD_PTR) &midiInputCallback,
        (DWORD_PTR) &m_input_data, CALLBACK_FUNCTION
    );
    if (result != MMSYSERR_NOERROR)
    {
        m_error_string =
            "midi_in_winmm::open_port: "
            "error creating Windows MM MIDI input port.";

        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    // Allocate and init the sysex buffers.

    for (int i = 0; i < RT_SYSEX_BUFFER_COUNT; ++i)
    {
        data->sysexBuffer[i] = (MIDIHDR*) new char[ sizeof(MIDIHDR) ];
        data->sysexBuffer[i]->lpData = new char[ RT_SYSEX_BUFFER_SIZE ];
        data->sysexBuffer[i]->dwBufferLength = RT_SYSEX_BUFFER_SIZE;

        // We use the dwUser parameter as buffer indicator

        data->sysexBuffer[i]->dwUser = i;
        data->sysexBuffer[i]->dwFlags = 0;
        result = midiInPrepareHeader
        (
            data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR)
        );
        if (result != MMSYSERR_NOERROR)
        {
            midiInClose(data->inHandle);
            m_error_string =
                "midi_in_winmm::open_port: "
                " error starting Windows MM MIDI input port (PrepareHeader).";

            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }

        // Register the buffer.

        result = midiInAddBuffer
        (
            data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR)
        );
        if (result != MMSYSERR_NOERROR)
        {
            midiInClose(data->inHandle);
            m_error_string =
                "midi_in_winmm::open_port: "
                "error starting Windows MM MIDI input port (AddBuffer).";

            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
    }

    result = midiInStart(data->inHandle);
    if (result != MMSYSERR_NOERROR)
    {
        midiInClose(data->inHandle);
        m_error_string =
            "midi_in_winmm::open_port: "
            "error starting Windows MM MIDI input port.";

        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    m_connected = true;
}

/**
 *  Opens a virtual WinMM MIDI port.  Actually, it does not...
 *  this function cannot be implemented for the Windows MM MIDI API.
 *
 * \param portname
 *      Provides the port name under which the virtual port is registered;
 *      unused.
 */

void
midi_in_winmm::open_virtual_port (const std::string & /*portname*/)
{
    m_error_string =
        "midi_in_winmm::open_virtual_port: "
        "cannot be implemented in Windows MM MIDI API!";

    error(rterror::WARNING, m_error_string);
}

/**
 *  Closes the MIDI input port.
 */

void midi_in_winmm::close_port ()
{
    if (m_connected)
    {
        WinMidiData *data = static_cast<WinMidiData *>(m_api_data);
        EnterCriticalSection(&(data->_mutex));
        midiInReset(data->inHandle);
        midiInStop(data->inHandle);
        for (int i = 0; i < RT_SYSEX_BUFFER_COUNT; ++i)
        {
            int result = midiInUnprepareHeader
            (
                data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR)
            );
            delete [] data->sysexBuffer[i]->lpData;
            delete [] data->sysexBuffer[i];
            if (result != MMSYSERR_NOERROR)
            {
                midiInClose(data->inHandle);
                m_error_string =
                    "midi_in_winmm::open_port: "
                    "error closing Windows MM MIDI "
                    "input port (midiInUnprepareHeader).";

                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
        }
        midiInClose(data->inHandle);
        m_connected = false;
        LeaveCriticalSection(&(data->_mutex));
    }
}

/**
 *  Retrieves the number of WinMM MIDI input ports.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      call midiInGetNumDevs().
 */

unsigned
midi_in_winmm::get_port_count ()
{
    return midiInGetNumDevs();
}

/**
 *  Retrieves the name of the desired port.
 *
 * \param portnumber
 *      The port number for which to get the name.
 *
 * \return
 *      Returns the port name as a standard C++ string.
 */

std::string
midi_in_winmm::get_port_name (unsigned portnumber)
{
    std::string stringName;
    unsigned nDevices = midiInGetNumDevs();
    if (portnumber >= nDevices)
    {
        std::ostringstream ost;
        ost
            << "midi_in_winmm::get_port_name: the 'portnumber' argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::WARNING, m_error_string);
        return stringName;
    }

    MIDIINCAPS deviceCaps;
    midiInGetDevCaps(portnumber, &deviceCaps, sizeof(MIDIINCAPS));

#if defined UNICODE || defined _UNICODE
    int length = WideCharToMultiByte
    (
        CP_UTF8, 0, deviceCaps.szPname, -1, NULL, 0, NULL, NULL
    ) - 1;
    stringName.assign(length, 0);
    length = WideCharToMultiByte
    (
        CP_UTF8, 0, deviceCaps.szPname,
        static_cast<int>(wcslen(deviceCaps.szPname)),
        &stringName[0], length, NULL, NULL
    );
#else
    stringName = std::string(deviceCaps.szPname);
#endif

    /*
     * Next lines added to add the portnumber to the name so that the device's
     * names are sure to be listed with individual names even when they have
     * the same brand name.
     */

    std::ostringstream os;
    os << " " << portnumber;
    stringName += os.str();
    return stringName;
}

/*
 *  API: Windows MM Class Definitions: midi_out_winmm
 */

/**
 *  Principal constructor.
 *
 * \param clientname
 *      The name of the MIDI output port.
 */

midi_out_winmm::midi_out_winmm (const std::string & clientname)
 :
    midi_out_api    ()
{
    initialize(clientname);
}

midi_out_winmm::~midi_out_winmm()
{
    // Close a connection if it exists.
    close_port();

    // Cleanup.
    WinMidiData *data = static_cast<WinMidiData *>(m_api_data);
    delete data;
}

/**
 *  The destructor closes the port and cleans out the API data structure.
 *  We'll issue a warning here if no devices are available but not throw an
 *  error since the user can plug something in later.
 */

void
midi_out_winmm::initialize (const std::string & /*clientname*/)
{
    unsigned nDevices = midiOutGetNumDevs();
    if (nDevices == 0)
    {
        m_error_string =
            "midi_out_winmm::initialize: "
            "no MIDI output devices currently available.";

        error(rterror::WARNING, m_error_string);
    }

    // Save our api-specific connection information.

    WinMidiData * data = (WinMidiData *) new WinMidiData;
    m_api_data = (void *) data;
}

/**
 *  Retrieves the number of WinMM MIDI output ports.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      call midiOutGetNumDevs().
 */

unsigned
midi_out_winmm::get_port_count ()
{
    return midiOutGetNumDevs();
}

/**
 *  Retrieves the name of the desired port.
 *
 * \param portnumber
 *      The port number for which to get the name.
 *
 * \return
 *      Returns the port name as a standard C++ string.
 */

std::string
midi_out_winmm::get_port_name (unsigned portnumber)
{
    std::string stringName;
    unsigned nDevices = midiOutGetNumDevs();
    if (portnumber >= nDevices)
    {
        std::ostringstream ost;
        ost
            << "midi_out_winmm::get_port_name: the 'portnumber' argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::WARNING, m_error_string);
        return stringName;
    }

    MIDIOUTCAPS deviceCaps;
    midiOutGetDevCaps(portnumber, &deviceCaps, sizeof(MIDIOUTCAPS));

#if defined UNICODE || defined _UNICODE
    int length = WideCharToMultiByte
    (
        CP_UTF8, 0, deviceCaps.szPname, -1, NULL, 0, NULL, NULL
    ) - 1;
    stringName.assign(length, 0);
    length = WideCharToMultiByte
    (
        CP_UTF8, 0, deviceCaps.szPname,
        static_cast<int>(wcslen(deviceCaps.szPname)),
        &stringName[0], length, NULL, NULL
    );
#else
    stringName = std::string(deviceCaps.szPname);
#endif

    /*
     * Next lines added to add the portnumber to the name so that the device's
     * names are sure to be listed with individual names even when they have
     * the same brand name
     */

    std::ostringstream os;
    os << " " << portnumber;
    stringName += os.str();
    return stringName;
}

/**
 *  Opens the WinMM MIDI output port.
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name under which the port is registered; unused.
 */

void
midi_out_winmm::open_port (unsigned portnumber, const std::string & /*portname*/)
{
    if (m_connected)
    {
        m_error_string =
            "midi_out_winmm::open_port: a valid connection already exists!";

        error(rterror::WARNING, m_error_string);
        return;
    }

    unsigned nDevices = midiOutGetNumDevs();
    if (nDevices < 1)
    {
        m_error_string =
            "midi_out_winmm::open_port: no MIDI output destinations found!";

        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }

    if (portnumber >= nDevices)
    {
        std::ostringstream ost;
        ost
            << "midi_out_winmm::open_port: the 'portnumber' argument ("
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
        return;
    }

    WinMidiData *data = static_cast<WinMidiData *>(m_api_data);
    MMRESULT result = midiOutOpen
    (
        &data->outHandle, portnumber, (DWORD)NULL, (DWORD)NULL, CALLBACK_NULL
    );
    if (result != MMSYSERR_NOERROR)
    {
        m_error_string =
            "midi_out_winmm::open_port: "
            "error creating Windows MM MIDI output port.";

        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    m_connected = true;
}

/**
 *  Closes the MIDI output port.
 */

void
midi_out_winmm::close_port ()
{
    if (m_connected)
    {
        WinMidiData * data = static_cast<WinMidiData *>(m_api_data);
        midiOutReset(data->outHandle);
        midiOutClose(data->outHandle);
        m_connected = false;
    }
}

/**
 *  Opens a virtual WinMM MIDI port.  Actually, it does not...
 *  this function cannot be implemented for the Windows MM MIDI API.
 *
 * \param portname
 *      Provides the port name under which the virtual port is registered;
 *      unused.
 */

void
midi_out_winmm::open_virtual_port (const std::string & /*portname*/)
{
    m_error_string =
        "midi_out_winmm::open_virtual_port: "
        "cannot be implemented in Windows MM MIDI API!";

    error(rterror::WARNING, m_error_string);
}

/**
 *  Sends a WinMM MIDI output message.
 *
 * \param message
 *      Provides the vector of message bytes to send.
 */

void
midi_out_winmm::send_message (std::vector<midibyte> * message)
{
    if (! m_connected)
        return;

    unsigned nBytes = static_cast<unsigned>(message->size());
    if (nBytes == 0)
    {
        m_error_string =
            "midi_out_winmm::send_message: message argument is empty!";

        error(rterror::WARNING, m_error_string);
        return;
    }

    MMRESULT result;
    WinMidiData * data = static_cast<WinMidiData *>(m_api_data);
    if (message->at(0) == 0xF0)     // Sysex message
    {
        // Allocate buffer for sysex data.

        char * buffer = (char *) malloc(nBytes);
        if (buffer == NULL)
        {
            m_error_string =
                "midi_out_winmm::send_message: "
                "error allocating sysex message memory!";

            error(rterror::MEMORY_ERROR, m_error_string);
            return;
        }

        // Copy data to buffer.

        for (unsigned i = 0; i < nBytes; ++i)
            buffer[i] = message->at(i);

        // Create and prepare MIDIHDR structure.

        MIDIHDR sysex;
        sysex.lpData = (LPSTR) buffer;
        sysex.dwBufferLength = nBytes;
        sysex.dwFlags = 0;
        result = midiOutPrepareHeader(data->outHandle,  &sysex, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            free(buffer);
            m_error_string =
                "midi_out_winmm::send_message: error preparing sysex header.";

            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }

        // Send the message.

        result = midiOutLongMsg(data->outHandle, &sysex, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            free(buffer);
            m_error_string =
                "midi_out_winmm::send_message: error sending sysex message.";

            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }

        // Unprepare the buffer and MIDIHDR.

        while
        (
            MIDIERR_STILLPLAYING ==
                midiOutUnprepareHeader(data->outHandle, &sysex, sizeof(MIDIHDR))
        )
        {
            Sleep(1);
        }
        free(buffer);
    }
    else   // Channel or system message.
    {
        if (nBytes > 3) // Make sure the message size isn't too big.
        {
            m_error_string =
                "midi_out_winmm::send_message: "
                "message size is greater than 3 bytes (and not sysex)!";

            error(rterror::WARNING, m_error_string);
            return;
        }

        // Pack MIDI bytes into double word.

        DWORD packet;
        midibyte * ptr = (midibyte *) &packet;
        for (unsigned i = 0; i < nBytes; ++i)
        {
            *ptr = message->at(i);
            ++ptr;
        }

        // Send the message immediately.

        result = midiOutShortMsg(data->outHandle, packet);
        if (result != MMSYSERR_NOERROR)
        {
            m_error_string =
                "midi_out_winmm::send_message: error sending MIDI message.";

            error(rterror::DRIVER_ERROR, m_error_string);
        }
    }
}

}           // namespace seq64

/*
 * midi_winmm.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

