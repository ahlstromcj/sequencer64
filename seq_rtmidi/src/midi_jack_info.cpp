/**
 * \file          midi_jack_info.cpp
 *
 *    A class for obtaining JACK port information.
 *
 * \author        Chris Ahlstrom
 * \date          2017-01-01
 * \updates       2017-01-20
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  This class is meant to collect a whole bunch of JACK information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating JACK midibus objects and midi_jack API objects.
 */

#include "calculations.hpp"             /* beats_per_minute_from_tempo_us() */
#include "event.hpp"                    /* seq64::event and other tokens    */
#include "jack_assistant.hpp"           /* seq64::create_jack_client()      */
#include "midi_jack_info.hpp"           /* seq64::midi_jack_info            */
#include "midibus_common.hpp"           /* from the libseq64 sub-project    */
#include "settings.hpp"                 /* seq64::rc() configuration object */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * Defined in midi_jack.cpp; used to be static.
 */

extern int jack_process_rtmidi_input (jack_nframes_t nframes, void * arg);
extern int jack_process_rtmidi_output (jack_nframes_t nframes, void * arg);

/**
 *  Provides a JACK callback function that uses the callbacks defined in the
 *  midi_jack module.
 */

static int
jack_process_io (jack_nframes_t nframes, void * arg)
{
    if (nframes > 0)
    {
        jack_process_rtmidi_input(nframes, arg);
        jack_process_rtmidi_output(nframes, arg);
    }
    return 0;

#if USE_DUMMY_CODE
    midi_jack_data * jackdata = (midi_jack_data *) arg;
    if (jackdata->m_jack_port == NULL)      /* is port created?        */
       return 0;
    else
       return int(nframes) == 0 ? int(nframes) : 0 ;    // EXPERIMENTAL
#endif
}

/**
 *  Principal constructor.
 *
 *  Note the m_multi_client member.  We may want each JACK port to have its
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

midi_jack_info::midi_jack_info
(
    const std::string & appname,
    int ppqn,
    int bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_multi_client          (false),
    m_jack_data             (),
    m_jack_client           (nullptr)               /* inited for connect() */
{
    silence_jack_info();
    m_jack_client = connect();
    if (not_nullptr(m_jack_client))                 /* created by connect() */
    {
        midi_handle(m_jack_client);                 /* void version         */
        client_handle(m_jack_client);               /* jack version         */
        m_jack_data.m_jack_client = m_jack_client;  /* port version         */
    }
}

/**
 *  Destructor.  Deactivates (disconnects and closes) any ports maintained by
 *  the JACK client, then closes the JACK client, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_jack_info::~midi_jack_info ()
{
    disconnect();
}

/**
 *  Local JACK connection for enumerating the ports.  Note that this name will
 *  be used for normal ports, so we make sure it reflects the application
 *  name.
 *
 *  Note that this function does not call jack_connect().
 */

jack_client_t *
midi_jack_info::connect ()
{
    jack_client_t * result = m_jack_client;
    if (is_nullptr(result))
    {
        const char * clientname = SEQ64_APP_NAME;
        if (multi_client())
            clientname = "midi_jack_info";

        result = create_jack_client(clientname);
        if (not_nullptr(result))
        {
            m_jack_client = result;
            int rc = jack_set_process_callback
            (
                result, jack_process_io, &m_jack_data
            );
            if (rc == 0)
            {
                /**
                 * We need to add a call to jack_on_shutdown() to set up a
                 * shutdown callback.  We also need to wait on the activation
                 * call until we have registered all the ports.  Then we
                 * (actually the mastermidibus) can call the api_connect()
                 * function to activate this JACK client and connect all the
                 * ports.
                 *
                 * jack_activate(result);
                 */
            }
            else
            {
                m_error_string = func_message("JACK can't set I/O callback");
                error(rterror::WARNING, m_error_string);
            }
        }
        else
        {
            m_error_string = func_message("JACK server not running?");
            error(rterror::WARNING, m_error_string);
        }
    }
    return result;
}

/**
 *  The opposite of connect().
 */

void
midi_jack_info::disconnect ()
{
    if (not_nullptr(m_jack_client))
    {
        jack_deactivate(m_jack_client);
        jack_client_close(m_jack_client);
        m_jack_client = nullptr;
    }
}

/**
 *  Extracts the two names from the JACK port-name format,
 *  "clientname:portname".
 */

void
midi_jack_info::extract_names
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
 * \tricky
 *      When we want to connect to a system input port, we want to use an output
 *      port to do that.  When we want to connect to a system output port, we
 *      want to use an input port to do that.  Therefore, we search for the <i>
 *      opposite </i> kind of port.
 *
 *  If in multi-client mode, then this function disconnects the JACK
 *  client afterward. At this point, we have got all the data we need, and are
 *  not providing a client to each JACK port we create.
 *
 *  If there is no system input port, or no system output port, then we add a
 *  virtual port of that type so that the application has something to work with.
 *
 *  Note that, at some pointer, we ought to consider how to deal with
 *  transitory system JACK clients and ports, and adjust for it.  A kind of
 *  miniature form of session management.
 *  Also, don't forget about the usefulness of jack_get_port_by_id() and
 *  jack_get_port_by_name().
 *
 * Error handling:
 *
 *  Not having any JACK input ports present isn't necessarily an error.  There
 *  may not be any, and there may still be at least one output port.
 *
 *      m_error_string = func_message("no JACK input ports available");
 *      error(rterror::WARNING, m_error_string);
 *
 *  Also, if there are none, we try to make a virtual port so that the
 *  application has something to work with.  The only issue is the client
 *  number.  Currently all virtual ports we create have a client number of 0.
 *
 * \return
 *      Returns the total number of ports found.  Note that 0 ports is not
 *      necessarily an error; there may be no JACK apps running with exposed
 *      ports.  If there is no JACK client, then -1 is returned.
 */

int
midi_jack_info::get_all_port_info ()
{
    int result = 0;
    if (not_nullptr(m_jack_client))
    {
        const char ** inports = jack_get_ports    /* list of JACK ports   */
        (
            m_jack_client, NULL,
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput         /* tricky   */
        );
        if (is_nullptr(inports))                  /* check port validity  */
        {
            warnprint("no JACK input port available, creating virtual port");
            int client = 0;
            std::string clientname = SEQ64_APP_NAME;
            std::string portname = SEQ64_APP_NAME " midi in 0";
            input_ports().add
            (
                client, clientname, 0, portname, SEQ64_MIDI_VIRTUAL_PORT
            );
            ++result;
        }
        else
        {
            std::vector<std::string> client_name_list;
            int client = -1;
            int count = 0;
            input_ports().clear();
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
                input_ports().add(client, clientname, count, portname);
                ++count;
            }
            jack_free(inports);
            result += count;
        }

        const char ** outports = jack_get_ports    /* list of JACK ports   */
        (
            m_jack_client, NULL,
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput    /* tricky   */
        );
        if (is_nullptr(outports))                  /* check port validity  */
        {
            /*
             * Not really an error, though perhaps we want to warn about it.
             *
             * m_error_string = func_message("no JACK outputs ports available");
             * error(rterror::WARNING, m_error_string);
             *
             * As with the input port, we create a virtual port.
             */

            warnprint("no JACK output port available, creating virtual port");
            int client = 0;
            std::string clientname = SEQ64_APP_NAME;
            std::string portname = SEQ64_APP_NAME " midi out 0";
            output_ports().add
            (
                client, clientname, 0, portname, SEQ64_MIDI_VIRTUAL_PORT
            );
            ++result;
        }
        else
        {
            std::vector<std::string> client_name_list;
            int client = -1;
            int count = 0;
            output_ports().clear();
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
                output_ports().add(client, clientname, count, portname);
                ++count;
            }
            jack_free(outports);
            result += count;
        }
    }
    else
        result = -1;

    if (multi_client())
        disconnect();

    return result;
}

/**
 *  Flushes our local queue events out into JACK.  This is also a midi_jack
 *  function.
 */

void
midi_jack_info::api_flush ()
{
    // No code yet
}

/**
 *  Goes through all the ports, represented by midibus objects, that have
 *  been created.  For any non-virtual port, the port connection is made.
 */

bool
midi_jack_info::api_connect ()
{
    bool result = not_nullptr(client_handle());
    if (result)
    {
        int rc = jack_activate(client_handle());
        result = rc == 0;
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
                    m->api_connect();
            }
        }
        else
        {
            m_error_string = func_message("JACK can't activate I/O");
            error(rterror::WARNING, m_error_string);
        }
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
midi_jack_info::api_set_ppqn (int p)
{
    midi_info::api_set_ppqn(p);
}

/**
 *  Sets the BPM numeric value, then makes JACK calls to set up the BPM
 *  tempo.
 *
 * \param b
 *      The desired new BPM value to set.
 */

void
midi_jack_info::api_set_beats_per_minute (int b)
{
    midi_info::api_set_beats_per_minute(b);
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
 *  midi_jack_info.
 *
 *  \threadsafe
 *      Quite a lot is done during the lock!
 *
 * \param client
 *      Provides the JACK client number.
 *
 * \param port
 *      Provides the JACK client port.
 */

void
midi_jack_info::api_port_start (mastermidibus & masterbus, int bus, int port)
{
    if (multi_client())
    {
        int bus_slot = masterbus.m_outbus_array.count();
        int test = masterbus.m_outbus_array.replacement_port(bus, port);
        if (test >= 0)
            bus_slot = test;

        midibus * m = new midibus(masterbus.m_midi_scratch, bus_slot);
        m->is_virtual_port(false);
        m->is_input_port(false);
        masterbus.m_outbus_array.add(m);

        bus_slot = masterbus.m_inbus_array.count();
        test = masterbus.m_inbus_array.replacement_port(bus, port);
        if (test >= 0)
            bus_slot = test;

        m = new midibus(masterbus.m_midi_scratch, bus_slot);
        m->is_virtual_port(false);
        m->is_input_port(false);
        masterbus.m_inbus_array.add(m);
    }
}

/**
 *  Grab a MIDI event.
 *
 * \param inev
 *      The event to be set based on the found input event.
 */

bool
midi_jack_info::api_get_midi_event (event * inev)
{
    bool sysex = false;
    bool result = false;
    midibyte buffer[0x1000];                /* temporary buffer for MIDI data */

    if (! rc().manual_alsa_ports())
    {
        // to do?
    }
    if (result)
        return false;

    /**
     *  We will only get EVENT_SYSEX on the first packet of MIDI data;
     *  the rest we have to poll for.  SysEx processing is currently
     *  disabled.
     */

#ifdef USE_SYSEX_PROCESSING                 /* currently disabled           */
    inev->set_sysex_size(bytes);
    if (buffer[0] == EVENT_MIDI_SYSEX)
    {
        inev->restart_sysex();              /* set up for sysex if needed   */
        sysex = inev->append_sysex(buffer, bytes);
    }
    else
    {
#endif
        /*
         *  Some keyboards send Note On with velocity 0 for Note Off, so we
         *  take care of that situation here by creating a Note Off event,
         *  with the channel nybble preserved. Note that we call
         *  event :: set_status_keep_channel() instead of using stazed's
         *  set_status function with the "record" parameter.  A little more
         *  confusing, but faster.
         */

        inev->set_data(buffer[1], buffer[2]);
        if (inev->is_note_off_recorded())
            inev->set_status_keep_channel(EVENT_NOTE_OFF);

        sysex = false;

#ifdef USE_SYSEX_PROCESSING
    }
#endif

    while (sysex)       /* sysex messages might be more than one message */
    {
        sysex = false;
    }
    return true;
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
        jack_set_info_function(jack_message_bit_bucket);
}

}           // namespace seq64

/*
 * midi_jack_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

