/**
 * \file          midi_win_info.cpp
 *
 *    A class for obtaining Windows MM subsystem port information.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-08-20
 * \updates       2018-06-02
 * \license       See the rtexmidi.lic file.  Too big.
 *
 * \deprecated
 *      We have decided to use the PortMidi re-implementation for Sequencer64
 *      for Windows.
 *
 *  This class is meant to collect a whole bunch of Windows MM information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating Windows MM midibus objects and midi_win API objects.
 *
 * Windows notes:
 *
 *      -   The function midiOutGetNumDevs() is a Windows MM API call
 *          returning a UINT value specifying the number of MIDI output
 *          devices found on the system.
 *      -   The function midiInGetNumDevs() returns a UINT value specifying
 *          the number of MIDI input devices found on the system.
 */

#error Internal RtMidi for Windows obsolete, use internal PortMidi instead.

#include "calculations.hpp"             /* extract_port_names()             */
#include "event.hpp"                    /* seq64::event and other tokens    */
#include "midi_win.hpp"                 /* seq64::midi_win                  */
#include "midi_win_info.hpp"            /* seq64::midi_win_info             */
#include "midibus_common.hpp"           /* from the libseq64 sub-project    */
#include "settings.hpp"                 /* seq64::rc() configuration object */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Windows MM MIDI input callback.  See RtMidi's midiInputCallback()
 *  function.
 *
 *  The Windows MM API is based on the use of a callback function for MIDI
 *  input.  We convert the system specific time stamps to delta time values.
 */

void CALLBACK
win_process_rtmidi_input
(
    HMIDIIN /*hmin*/,
    UINT instatus,
    DWORD_PTR instance,
    DWORD_PTR midimsg,
    DWORD timestamp
)
{
    if
    (
        instatus != MIM_DATA && instatus != MIM_LONGDATA &&
        instatus != MIM_LONGERROR
    )
    {
        return;
    }

    rtmidi_in_data * data = (midi_in_api::rtmidi_in_data *) instance;
    midi_win_data * api_data = reinterpret_cast<midi_win_data *>
    (
        data->m_api_data
    );

    /*
     * Calculate time stamp.
     */

    if (data->first_message())
    {
        api_data->message.timestamp(0.0);
        data->first_message(false);
    }
    else
    {
        double difftime = double(timestamp - api_data->m_win_lasstime) * 0.001;
        api_data->message.timetamp(difftime);
    }
    api_data->m_win_lasttime = timestamp;

    if (instatus == MIM_DATA)           /* channel or system message    */
    {
        // Make sure the first byte is a status byte.  If not, return.

        unsigned char status = (unsigned char)(midimsg & 0x000000FF);
        if (! (status & 0x80))
            return;

        // Determine the number of bytes in the MIDI message.

        unsigned short nbytes = 1;
        if (status < 0xC0)
            nbytes = 3;
        else if (status < 0xE0)
            nbytes = 2;
        else if (status < 0xF0)
            nbytes = 3;
        else if (status == 0xF1)
        {
            if ( data->ignore_flags() & 0x02 )
                return;
            else
                nbytes = 2;
        }
        else if (status == 0xF2)
            nbytes = 3;
        else if (status == 0xF3)
            nbytes = 2;
        else if (status == 0xF8 && data->test_ignore_flags(0x02))
        {
            return;         // MIDI timing tick message; we ignore it
        }
        else if (status == 0xFE && data->test_ignore_flags(0x04))
        {
            return;         // MIDI active-sensing message; we ignore it
        }

        unsigned char * ptr = (unsigned char *) &midimsg;
        midi_message & mm = api_data->message();
        for (int i = 0; i < nbytes; ++i)    // copy bytes to MIDI message
            mm.push(*ptr++);
    }
    else
    {
        // Sysex message (MIM_LONGDATA or MIM_LONGERROR)

        MIDIHDR * sysex = (MIDIHDR *) midimsg;
        if (! data->test_ignore_flags(0x01) && instatus != MIM_LONGERROR)
        {
            // Sysex message and we're not ignoring it

            midi_message & mm = api_data->message();
            for (int i = 0; i < int(sysex->dwBytesRecorded); ++i)
                mm.push(sysex->lpData[i]);
        }
    }

    /*
     * The WinMM API requires that the SysEx buffer be requeued after input of
     * each SysEx message.  Even if we are ignoring SysEx messages, we still
     * need to requeue the buffer, in case the user decides to not ignore
     * SysEx messages in the future.  However, it seems that WinMM calls this
     * function with an empty SysEx buffer when an application closes and, in
     * this case, we should avoid requeueing it, otherwise the computer
     * suddenly reboots after one or two minutes.  :-D
     */

    if (api_data->m_sysex_buffer[sysex->dwUser]->dwBytesRecorded > 0)
    {
        EnterCriticalSection(&(api_data->m_win_mutex));
        MMRESULT result = midiInAddBuffer
        (
            api_data->m_win_in_handle, api_data->m_sysex_buffer[sysex->dwUser],
            sizeof MIDIHDR
        );
        LeaveCriticalSection(&(api_data->m_win_mutex));
        if (result != MMSYSERR_NOERROR)
        {
            m_error_string = func_message("error sending SysEx to MIDI device");
            error(rterror::DRIVER_ERROR, m_error_string);
        }
        if (data->test_ignore_flags(0x01))
            return;
    }

    if (data->using_callback())
    {
        rtmidi_callback_t callback = (rtmidi_callback_t) data->user_callback();
        callback
        (
            api_data->message.timeStamp,
            &api_data->message.bytes,
            data->userData
        );
    }
    else
    {
        // As long as we haven't reached our queue size limit, push the message.

        if (data->queue.size < data->queue.ringSize)
        {
            data->queue.ring[data->queue.back++] = api_data->message;
            if (data->queue.back == data->queue.ringSize)
                data->queue.back = 0;

            data->queue.size++;     //
        }
        else
            std::cerr << "\nRtMidiIn: message queue limit reached!!\n\n";
    }

    // Clear the vector for the next input message.

    api_data->message.bytes.clear();
}

/**
 *  I/O callback.  Compare it to jack_process_io().
 */

datatype_unknown
win_mm_process_io ()
{
    // might be feckless
}

/**
 *  Principal constructor.
 *
 * \param appname
 *      Provides the name of the application.
 *
 * \param ppqn
 *      Provides the desired value of the PPQN (pulses per quarter note).
 *
 * \param bpm
 *      Provides the desired value of the BPM (beats per minute).
 */

midi_win_info::midi_win_info
(
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_win_handles           ()
{
    if (m_win_handles.is_error())
    {
        m_error_string = func_message("error opening Win MM sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
    else
    {
        midi_handle(&m_win_handles);                /* void version         */
        // client_handle(win_xxxxx);                /* winmm version        */
    }
}

/**
 *  Destructor.  Deactivates (disconnects and closes) any ports maintained by
 *  the Windows MM client, then closes the Windows MM client, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_win_info::~midi_win_info ()
{
    disconnect();
}

/**
 *  Local Windows MM connection for enumerating the ports.  Note that this
 *  name will be used for normal ports, so we make sure it reflects the
 *  application name.
 */

void
midi_win_info::connect ()
{
#if 0
    if (is_nullptr(result))
    {
        const char * clientname = rc().app_client_name().c_str();
        result = create_win_client(clientname);
        if (not_nullptr(result))
        {
            int rc = set_process_callback(...);
            m_win_client = result;
            if (rc == 0)
            {
            }
            else
            {
                m_error_string = func_message("can't set I/O callback");
                error(rterror::WARNING, m_error_string);
            }
        }
        else
        {
            m_error_string = func_message("Windows MM server not running?");
            error(rterror::WARNING, m_error_string);
        }
    }
    return result;
#endif
}

/**
 *  The opposite of connect().
 */

void
midi_win_info::disconnect ()
{
#if 0
    if (not_nullptr(m_jack_client))
    {
        // jack_deactivate(m_jack_client);
        // jack_client_close(m_jack_client);
        // m_win_client = nullptr;
    }
#endif
}

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another midi_info container.
 *
 * \return
 *      Returns the total number of ports found.  Note that 0 ports is not
 *      necessarily an error; there may be no apps running with exposed
 *      ports.  If there is no client, then -1 is returned.
 */

int
midi_win_info::get_all_port_info ()
{
    int result = 0;
    if (not_nullptr(m_win_client))
    {
        unsigned numdevices = unsigned(midiInGetNumDevs());  /* Win MM API   */
        input_ports().clear();
        if (numdevices > 0)
        {
            int count = 0;
            for (int in = 0; in < numdevices; ++in)
            {
                MIDIINCAPS incaps;
                MMRESULT mmr = midiInGetDevCaps(in, &incaps, sizeof incaps);
                if (mmr == MMSYSERR_NOERROR)
                {
                    std::string clientname = incaps.szPname; /* product name */
                    std::string portname = std::to_string(in);
                    input_ports().add
                    (
                        in, clientname, count, portname,
                        SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                        SEQ64_MIDI_INPUT_PORT
                    );
                    ++count;
                }
                else
                {
                    // ERROR
                }
            }
            result += count;
        }

        numdevices = unsigned(midiOutGetNumDevs());          /* Win MM API   */
        output_ports().clear();
        if (numdevices > 0)
        {
            int count = 0;
            for (int out = 0; out < numdevices; ++out)
            {
                MIDIOUTCAPS outcaps;
                MMRESULT mmr = midiOutGetDevCaps(out, &outcaps, sizeof outcaps);
                if (mmr == MMSYSERR_NOERROR)
                {
                    std::string clientname = outcaps.szPname;   /* prod name */
                    std::string portname = std::to_string(out);
                    output_ports().add
                    (
                        out, clientname, count, portname,
                        SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                        SEQ64_MIDI_OUTPUT_PORT
                    );
                    ++count;
                }
                else
                {
                    // ERROR
                }
            }
            result += count;
        }
    }
    else
        result = -1;

    return result;
}

/**
 *  Flushes our ...
 *
 *      midiOutPrepareHeader() prepares a SysEx or stream buffer for output.
 */

void
midi_win_info::api_flush ()
{
#if 0
    // No code yet; write-flush adapted from PortMidi.

    MMRESULT mmr = midiOutPrepareHeader
    (
        m_win_handles.m_win_out_handle, m_win_handles.m_sysex_buffer,
        sizeof MIDIHDR // [WIN_RT_SYSEX_BUFFER_COUNT] ????
    );

    // There seems to be more to it than this, though.
#endif
}

/**
 *  Sets up all of the ports, represented by midibus objects, that have
 *  been created.
 *
 *  If multi-client usage has been specified, each non-virtual port that has
 *  been set up (with its own JACK client pointer) is activated, and then
 *  connected to its corresponding remote system port.
 *
 *  Otherwise, the main JACK client is activated, and then all non-virtual
 *  ports are simply connected.
 *
 *  Each JACK port's midi_win::api_connect() function decides, based on
 *  multi-client status, whether or not to activate before making the
 *  connection.
 *
 * \return
 *      Returns true if activation succeeds.
 */

bool
midi_win_info::api_connect ()
{
#if 0
    bool result = not_nullptr(client_handle());
    if (result)
    {
        for
        (
            std::vector<midibus *>::iterator it = bus_container().begin();
            it != bus_container().end(); ++it
        )
        {
            midibus * m = *it;
            if (! m->is_virtual_port())
            {
                result = m->api_connect();
                if (! result)
                {
                    break;
                }
            }
        }
    }
    if (! result)
    {
        m_error_string = func_message("JACK can't activate and connect I/O");
        error(rterror::WARNING, m_error_string);
    }
    return result;
#else
    return true;
#endif
}

/**
 *  Sets the PPQN numeric value.
 *
 * \param p
 *      The desired new PPQN value to set.
 */

void
midi_win_info::api_set_ppqn (int p)
{
    midi_info::api_set_ppqn(p);
}

/**
 *  Sets the BPM numeric value (tempo).
 *
 * \param b
 *      The desired new BPM value to set.
 */

void
midi_win_info::api_set_beats_per_minute (midibpm b)
{
    midi_info::api_set_beats_per_minute(b);
}

/**
 *  Start the given MIDI port.
 *
 * \param masterbus
 *      Provides the object needed to get access to the array of input and
 *      output buss objects.
 *
 * \param bus
 *      Provides the bus/client number.
 *
 * \param port
 *      Provides the client port.
 */

void
midi_win_info::api_port_start (mastermidibus & masterbus, int bus, int port)
{
    // no code
}

/**
 *
 */

int
midi_win_info::api_poll_for_midi ()
{
    millisleep(1);
    return 0;
}

/**
 *  Grab a MIDI event.
 *
 * \param inev
 *      The event to be set based on the found input event.  We should make
 *      this value a reference someday.  Not used here.
 *
 * \return
 *      Always returns false.  Will eventually delete this function.
 */

bool
midi_win_info::api_get_midi_event (event * /*inev*/)
{
    return false;
}

/*
 * Utility functions for the Window MM API.
 */

/**
 *
\verbatim
	MMRESULT midiInOpen
	(
	   LPHMIDIIN lphMidiIn,
	   UINT      uDeviceID,
	   DWORD_PTR dwCallback,
	   DWORD_PTR dwCallbackInstance,
	   DWORD     dwFlags
	);
\endverbatim
 *
 *  -	lphMidiIn: Pointer to an HMIDIIN handle, it is filled with a handle
 *      identifying the opened MIDI input device. The handle is used to
 *      identify the device in calls to other MIDI input functions.
 *  -   uDeviceID: Identifier of the MIDI input device to be opened.  This can
 *      be an externally-specified port number.
 *  -   dwCallback: Pointer to a callback function, a thread identifier, or a
 *      handle of a window called with information about incoming MIDI
 *      messages.  For information on the callback, see MidiInProc.
 *  -   dwCallbackInstance: User instance data passed to the callback
 *      function. This parameter is not used in window callback functions or
 *      threads.
 *  -   dwFlags: Callback flag for opening the device and, optionally, a
 *      status flag that helps regulate rapid data transfers. It can be the
 *      following values: ....
 *
 *  This function just does the work, it doesn't check for the circumstances
 *  in which it is called.
 *
 * \param data
 *      The Windows MM data structure that holds critical values for the
 *      Windows MM ports.
 *
 * \param portnumber
 *      The number to be used in the creation of the port. It must range from
 *      0 to less than the number of input devices found in the system.
 *
 * \return
 *      Returns true if the MMRESULT value is a value of MMSYSERR_NOERROR,
 *      which indicates success.
 */

bool
open_win_input_port (midi_win_data & data, unsigned portnumber)
{
    unsigned ndevices = unsigned(midiInGetNumDevs());
    bool result = portnumber < ndevices;            // 0 to ndevices - 1
    if (result)
    {
        MMRESULT mmresult = midiInOpen
        (
            data->m_win_in_handle, portnumber,
            (DWORD_PTR) &win_process_rtmidi_input,
            (DWORD_PTR) &input_data,                    // what data is this???
            CALLBACK_FUNCTION
        );
        if (mmresult != MMSYSERR_NOERROR)
        {
            m_error_string = func_message("error creating MIDI input port");
            error(rterror::DRIVER_ERROR, m_error_string);
        }

        // Allocate and init the sysex buffers.
        // We use the dwUser parameter as a buffer indicator.

        for (int i = 0; i < WIN_RT_SYSEX_BUFFER_COUNT; ++i)
        {
            data->m_sysex_buffer[i] = (MIDIHDR *) new char[sizeof(MIDIHDR)];
            data->m_sysex_buffer[i]->lpData = new char[WIN_RT_SYSEX_BUFFER_SIZE];
            data->m_sysex_buffer[i]->dwBufferLength = WIN_RT_SYSEX_BUFFER_SIZE;
            data->m_sysex_buffer[i]->dwUser = i;
            data->m_sysex_buffer[i]->dwFlags = 0;
            mmresult = midiInPrepareHeader
            (
                data->m_win_in_handle, data->m_sysex_buffer[i], sizeof MIDIHDR
            );
            if (mmresult != MMSYSERR_NOERROR)
            {
                close_win_input_on_error(data, "MIDI in prepare-header failed");
                return;
            }
            mmresult = midiInAddBuffer        // Register the SysEx buffer.
            (
                data->m_win_in_handle, data->m_sysex_buffer[i], sizeof MIDIHDR
            );
            if (mmresult != MMSYSERR_NOERROR)
            {
                close_win_input_on_error(data, "MIDI in buffer register failed");
                return;
            }
        }
        mmresult = midiInStart(data->m_win_in_handle);
        if (mmresult != MMSYSERR_NOERROR)
        {
            close_win_input_on_error(data, "MIDI input start failed");
            return;
        }
        result = mmresult == MMSYSERR_NOERROR;
    }
    else
    {
        m_error_string = func_message("MIDI input port number does not exist");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
    return result;
}

/**
 *      Opens an output MIDI port in the Windows MM API.
 *
 * \param data
 *      The Windows MM data structure that holds critical values for the
 *      Windows MM ports.
 *
 * \param portnumber
 *      The number to be used in the creation of the port. It must range from
 *      0 to less than the number of input devices found in the system.
 *
 * \return
 *      Returns true if the MMRESULT value is a value of MMSYSERR_NOERROR,
 *      which indicates success.
 */

bool
open_win_output_port (midi_win_data & data, unsigned portnumber)
{
    bool result = not_nullptr(data->m_win_out_handle);  // or -1 ???
    if (result)
    {
        MMRESULT rc = midiOutOpen
        (
            data->m_win_out_handle, portnumber,
            (DWORD_PTR) NULL,
            (DWORD_PTR) NULL,
            CALLBACK_NULL
        );
        if (rc != MMSYSERR_NOERROR)
        {
            m_error_string = func_message("error creating MIDI output port");
            error(rterror::DRIVER_ERROR, m_error_string);
        }
        result = rc == MMSYSERR_NOERROR;
    }
    return result;
}

/**
 *
 *  The midiOutReset() call marks all the output buffers as being done.  If
 *  this function is not called, then the midiOutClose() call will fail.
 */

bool
close_win_output_port (midi_win_data & data)
{
    bool result = not_nullptr(data->m_win_out_handle);  // or -1 ???
    if (result)
    {
        MMRESULT rc = midiOutReset(data->m_win_out_handle);
        if (rc == MMSYSERR_NOERROR)
        {
            rc = midiOutClose(data->m_win_out_handle);
            if (rc != MMSYSERR_NOERROR)
            {
                m_error_string = func_message("error in midiOutClose()");
                error(rterror::DRIVER_ERROR, m_error_string);
            }
        }
        else
        {
            m_error_string = func_message("error in midiOutReset()");
            error(rterror::DRIVER_ERROR, m_error_string);
        }
        result = rc == MMSYSERR_NOERROR;
    }
    return result;
}

/**
 *  Error-handling wrapper.
 */

void
close_win_input_on_error (midi_win_data & data, const std::string & msg)
{
    (void) midiInClose(data->m_win_out_handle);
    m_error_string = func_message(msg);
    error(rterror::DRIVER_ERROR, m_error_string);
}

/**
 *  Error-handling wrapper.
 */

void
close_win_output_on_error (midi_win_data & data, const std::string & msg)
{
    (void) midiOutClose(data->m_win_out_handle);
    m_error_string = func_message(msg);
    error(rterror::DRIVER_ERROR, m_error_string);
}

}           // namespace seq64

/*
 * midi_win_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

