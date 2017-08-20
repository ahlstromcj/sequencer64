/**
 * \file          midi_win_info.cpp
 *
 *    A class for obtaining JACK port information.
 *
 * \author        Chris Ahlstrom
 * \date          2017-08-20
 * \updates       2017-08-20
 * \license       See the rtexmidi.lic file.  Too big.
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
    UINT inputStatus,
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

        // Copy bytes to our MIDI message.

        unsigned char * ptr = (unsigned char *) &midimsg;
        midi_message & mm = api_data->message();
        for (int i = 0; i < nbytes; ++i)
            mm.push(*ptr++);

//          api_data->message.bytes.push_back(*ptr++);
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

    if (api_data->sysexBuffer[sysex->dwUser]->dwBytesRecorded > 0)
    {
        EnterCriticalSection(&(api_data->m_win_mutex));
        MMRESULT result = midiInAddBuffer
        (
            api_data->m_win_in_handle, api_data->sysexBuffer[sysex->dwUser],
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

            data->queue.size++;
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
 *  Note the m_multi_client member.  We may want each Windows MM port to have its
 *  own client, as in the original RtMidi implementation.
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
{
    // open the handle(s) and then save them
    if (bad!)
    {
        m_error_string = func_message("error opening ALSA sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
    else
    {
    //
    // midi_handle(m_win_xxxxx);                 /* void version         */
    // client_handle(win_xxxxx);               /* winmm version         */
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
    if (is_nullptr(result))
    {
        /*
         * int jacksize = jack_port_name_size();
         * infoprintf("Windows MM PORT NAME SIZE = %d\n", jacksize); // = 320
         */

        const char * clientname = rc().app_client_name().c_str();
        if (multi_client())
        {
            /*
             * We may be changing the potential usage of multi-client.
             */

            clientname = "midi_win_info";
        }

        result = create_jack_client(clientname);
        if (not_nullptr(result))
        {
            int rc = jack_set_process_callback(result, jack_process_io, this);
            m_jack_client = result;
            if (rc == 0)
            {
                /**
                 * We need to add a call to jack_on_shutdown() to set up a
                 * shutdown callback.  We also need to wait on the activation
                 * call until we have registered all the ports.  Then we
                 * (actually the mastermidibus) can call the api_connect()
                 * function to activate this Windows MM client and connect all the
                 * ports.
                 *
                 * jack_activate(result);
                 */
            }
            else
            {
                m_error_string = func_message("Windows MM can't set I/O callback");
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
}

/**
 *  The opposite of connect().
 */

void
midi_win_info::disconnect ()
{
    if (not_nullptr(m_jack_client))
    {
        jack_deactivate(m_jack_client);
        jack_client_close(m_jack_client);
        m_jack_client = nullptr;
        apiprint("jack_deactivate", "info");
        apiprint("jack_client_close", "info");
    }
}

/**
 *  Extracts the two names from the Windows MM port-name format,
 *  "clientname:portname".
 */

void
midi_win_info::extract_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    (void) extract_port_names(fullname, clientname, portname);
}

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another midi_info container.
 *
 * \return
 *      Returns the total number of ports found.  Note that 0 ports is not
 *      necessarily an error; there may be no JACK apps running with exposed
 *      ports.  If there is no JACK client, then -1 is returned.
 */

int
midi_win_info::get_all_port_info ()
{
    int result = 0;
    if (not_nullptr(m_win_client))
    {
        input_ports().clear();
        output_ports().clear();

        unsigned numdevices = unsigned(midiInGetNumDevs()); /* Win MM API   */
        if (numdevices > 0)
        {
            std::vector<std::string> client_name_list;
            int client = -1;
            int count = 0;
            while (not_nullptr(inports[count]))
            {
                std::string fullname = inports[count];
                std::string clientname;
                std::string portname;
                extract_names(fullname, clientname, portname);
                if (client == -1 || clientname != client_name_list.back())
                {
                    client_name_list.push_back(clientname);
                    ++client;
                }
                input_ports().add
                (
                    client, clientname, count, portname,
                    SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                    SEQ64_MIDI_INPUT_PORT
                );
                ++count;
            }
            jack_free(inports);
            result += count;
        }

        const char ** outports = jack_get_ports    /* list of JACK ports   */
        (
            m_jack_client, NULL,
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsOutput                       /* tricky   */
        );

        numdevices = unsigned(midiOutGetNumDevs()); /* Win MM API   */
        if (numdevices > 0)
        {
            std::vector<std::string> client_name_list;
            int client = -1;
            int count = 0;
            while (not_nullptr(outports[count]))
            {
                std::string fullname = outports[count];
                std::string clientname;
                std::string portname;
                extract_names(fullname, clientname, portname);
                if (client == -1 || clientname != client_name_list.back())
                {
                    client_name_list.push_back(clientname);
                    ++client;
                }
                output_ports().add
                (
                    client, clientname, count, portname,
                    SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                    SEQ64_MIDI_OUTPUT_PORT
                );
                ++count;
            }
            jack_free(outports);
            result += count;
        }
    }
    else
        result = -1;

    if (multi_client())
    {
        /*
         * We may be changing the potential usage of multi-client.
         */

        disconnect();
    }
    return result;
}

/**
 *  Flushes our local queue events out into JACK.  This is also a midi_win
 *  function.
 */

void
midi_win_info::api_flush ()
{
    // No code yet
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
    bool result = true;
    if (! multi_client())           /* CAREFUL IF WE ENABLE IT! */
    {
        result = not_nullptr(client_handle());
        if (result)
        {
            int rc = jack_activate(client_handle());
            apiprint("jack_activate", "info");
            result = rc == 0;
        }
    }
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
}

/**
 *  Sets the PPQN numeric value, then makes JACK calls to set up the PPQ
 *  tempo.
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
 *  Sets the BPM numeric value, then makes JACK calls to set up the BPM
 *  tempo.  These calls might need to be done in a JACK callback.
 *
 * \param b
 *      The desired new BPM value to set.
 */

void
midi_win_info::api_set_beats_per_minute (midibpm b)
{
    midi_info::api_set_beats_per_minute(b);

    // Need JACK specific tempo-setting here if applicable.
}

/**
 *  Start the given JACK MIDI port.  This function is called by
 *  api_get_midi_event() when an JACK event SND_SEQ_EVENT_PORT_START is
 *  received.
 *
 *  -   Get the API's client and port information.
 *  -   Do some capability checks.
 *  -   Find the client/port combination among the set of input/output busses.
 *      If it exists and is not active, then mark it as a replacement.  If it
 *      is not a replacement, it will increment the number of input/output
 *      busses.
 *
 *  We can simplify this code a bit by using elements already present in
 *  midi_win_info.
 *
 * \param masterbus
 *      Provides the object needed to get access to the array of input and
 *      output buss objects.
 *
 * \param bus
 *      Provides the JACK bus/client number.
 *
 * \param port
 *      Provides the JACK client port.
 */

void
midi_win_info::api_port_start (mastermidibus & masterbus, int bus, int port)
{
    if (multi_client())
    {
        /*
         * We may be changing the potential usage of multi-client.
         */

        int bus_slot = masterbus.m_outbus_array.count();
        int test = masterbus.m_outbus_array.replacement_port(bus, port);
        if (test >= 0)
            bus_slot = test;

        midibus * m = new midibus(masterbus.m_midi_master, bus_slot);
        m->is_virtual_port(false);
        m->is_input_port(false);
        masterbus.m_outbus_array.add(m, e_clock_off);

        bus_slot = masterbus.m_inbus_array.count();
        test = masterbus.m_inbus_array.replacement_port(bus, port);
        if (test >= 0)
            bus_slot = test;

        m = new midibus(masterbus.m_midi_master, bus_slot);
        m->is_virtual_port(false);
        m->is_input_port(false);
        masterbus.m_inbus_array.add(m, false);
    }
}

/**
 *  MUCH TO DO!
 */

int
midi_win_info::api_poll_for_midi ()
{
    millisleep(1);
    return 0;       // TODO TODO TODO
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

/**
 *  This function merely eats the string passed as a parameter.
 */

static void
jack_message_bit_bucket (const char *)
{
    // Into the bit-bucket with ye ya scalliwag!
}

/**
 *  This function silences JACK error output to the console.  Probably not
 *  good to silence this output, but let's provide the option, for the sake of
 *  symmetry, consistency, what have you.
 */

void
silence_jack_errors (bool silent)
{
    if (silent)
        jack_set_error_function(jack_message_bit_bucket);
}

/**
 *  This function silences JACK info output to the console.  We were getting
 *  way too many informational message, to the point of obscuring the debug
 *  and error output.
 */

void
silence_jack_info (bool silent)
{
    if (silent)
    {
#ifndef SEQ64_SHOW_API_CALLS
        jack_set_info_function(jack_message_bit_bucket);
#endif
    }
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
    MMRESULT result = midiInOpen
    (
        data->m_win_in_handle, portnumber,
        (DWORD_PTR) &win_process_rtmidi_input,
        (DWORD_PTR) &input_data,                    // what data is this???
        CALLBACK_FUNCTION
    );
    if (result != MMSYSERR_NOERROR)
    {
        m_error_string = func_message("error creating Win MM MIDI input port");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
    return result == MMSYSERR_NOERROR;
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
            m_error_string =
                func_message("error creating Win MM MIDI output port");

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
                m_error_string =
                    func_message("error in Win MM MIDI API midiOutClose()");

                error(rterror::DRIVER_ERROR, m_error_string);
            }

        }
        else
        {
            m_error_string =
                func_message("error in Win MM MIDI API midiOutReset()");

            error(rterror::DRIVER_ERROR, m_error_string);
        }
        result = rc == MMSYSERR_NOERROR;
    }
    return result;

}

}           // namespace seq64

/*
 * midi_win_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

