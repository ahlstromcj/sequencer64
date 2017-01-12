/**
 * \file          midi_jack_info.cpp
 *
 *    A class for obtaining JACK port information.
 *
 * \author        Chris Ahlstrom
 * \date          2017-01-01
 * \updates       2017-01-11
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  API information found at:
 *
 *      - ....
 *
 *  This class is meant to collect a whole bunch of JACK information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating JACK midibus objects and midi_jack API objects.
 */

#include "calculations.hpp"             /* beats_per_minute_from_tempo_us() */
#include "event.hpp"                    /* seq64::event and other tokens    */
#include "midi_jack_info.hpp"           /* seq64::midi_jack_info            */
#include "midibus_common.hpp"           /* from the libseq64 sub-project    */
#include "settings.hpp"                 /* seq64::rc() configuration object */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param clientname
 *      Provides the name of the MIDI input port.
 *
 * \param queuesize
 *      Provides the upper limit of the queue size.
 */

midi_jack_info::midi_jack_info
(
    const std::string & appname,
    int ppqn,
    int bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_jack_data             (),
    m_jack_client           (connect())
{
    silence_jack_info();
    if (not_nullptr(m_jack_client))
    {
        /*
         * Save the JACK "handle".  Set the client's name for JACK.  Then set
         * up the JACK client queue.
         */

        midi_handle(m_jack_client);
        m_jack_data.m_jack_client = m_jack_client;
    }
}

/**
 *  Destructor.  Deactivates (disconnects and closes) any ports maintained by
 *  the JACK client, then closes the JACK client, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_jack_info::~midi_jack_info ()
{
    if (not_nullptr(m_jack_client))
    {
        jack_deactivate(m_jack_client);
        jack_client_close(m_jack_client);
    }
}

/**
 *  Provides a dummy JACK callback function to use when only enumerating the
 *  existing JACK ports.
 */

static int
jack_process_dummy (jack_nframes_t nframes, void * arg)
{
    midi_jack_data * jackdata = (midi_jack_data *) arg;
    if (jackdata->m_jack_port == NULL)      /* is port created?        */
       return 0;
    else
       return int(nframes);
}

/**
 *  Local JACK connection for enumerating the ports.
 */

jack_client_t *
midi_jack_info::connect ()
{
    jack_client_t * result = m_jack_client;
    if (is_nullptr(result))
    {
        result = jack_client_open("dummy", JackNoStartServer, NULL);
        if (not_nullptr(result))
        {
            m_jack_client = result;
            jack_set_process_callback(result, jack_process_dummy, &m_jack_data);
            jack_activate(result);
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
    clientname.clear();
    portname.clear();
    if (! fullname.empty())
    {
        std::size_t colonpos = fullname.find_first_of(":");
        if (colonpos != std::string::npos)
        {
            clientname = fullname.substr(0, colonpos);
            portname = fullname.substr(colonpos+1);
        }
        else
            portname = fullname;
    }
}

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another container.
 *
 *  Don't forget about the usefulness of jack_get_port_by_id() and
 *  jack_get_port_by_name().
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
            m_jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
        );
        if (is_nullptr(inports))                  /* check port validity  */
        {
            /*
             * Not having any JACK input ports present isn't necessarily an
             * error.  There may not be any, and there may still be at least
             * one output port.
             *
             * m_error_string = func_message("no JACK input ports available");
             * error(rterror::WARNING, m_error_string);
             */

            warnprint("no JACK input ports available");
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
            m_jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
        );
        if (is_nullptr(outports))                  /* check port validity  */
        {
            /*
             * m_error_string = func_message("no JACK outputs ports available");
             * error(rterror::WARNING, m_error_string);
             * return 0;
             */

            warnprint("no JACK output ports available");
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

    return result;
}

#if USE_THIS_CODE

/**
 *  Retrieves the number of JACK MIDI output (!) ports.  Note that this
 *  function gets the kind of port opposite its enclosing class.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      jack_get_ports() call.
 */

int
midi_jack_info::get_port_count ()
{
    int count = 0;
//  connect();
//  if (is_nullptr(m_jack_client))
//      return 0;

}

#endif

/**
 *  Flushes our local queue events out into JACK.  This is also a midi_jack
 *  function.
 */

void
midi_jack_info::api_flush ()
{
    // snd_seq_drain_output(m_jack_seq);
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
    if (1)
    {
        int bus_slot = masterbus.m_outbus_array.count();
        int test = masterbus.m_outbus_array.replacement_port(bus, port);
        if (test >= 0)
            bus_slot = test;

        midibus * m = new midibus
        (
            masterbus.m_midi_scratch, SEQ64_APP_NAME, bus_slot // index, port
        );
        masterbus.m_outbus_array.add(m, false, false);  /* out, nonvirt */
    }
    if (1)
    {
        int bus_slot = masterbus.m_inbus_array.count();
        int test = masterbus.m_inbus_array.replacement_port(bus, port);
        if (test >= 0)
            bus_slot = test;

        midibus * m = new midibus
        (
            masterbus.m_midi_scratch, SEQ64_APP_NAME, bus_slot // index, port
        );
        masterbus.m_inbus_array.add(m, false, false);  /* out, nonvirt */
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
#if 0
        switch (ev->type)
        {
        case SND_SEQ_EVENT_PORT_START:
        {
            /*
            port_start(masterbus, ev->data.addr.client, ev->data.addr.port);
             */

            result = true;
            break;
        }
        case SND_SEQ_EVENT_PORT_EXIT:
        {
            /*
            port_exit(masterbus, ev->data.addr.client, ev->data.addr.port);
             */

            result = true;
            break;
        }
        case SND_SEQ_EVENT_PORT_CHANGE:
        {
            result = true;
            break;
        }
        default:
            break;
        }
#endif
    }
    if (result)
        return false;

//  inev->set_timestamp(ev->time.tick);
//  inev->set_status_keep_channel(buffer[0]);

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
//      snd_seq_event_input(m_jack_seq, &ev);
//      long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
//      if (bytes > 0)
//          sysex = inev->append_sysex(buffer, bytes);
//      else
            sysex = false;
    }
//  snd_midi_event_free(midi_ev);
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
 *  good to silence this output, but let's provide the option.
 */

void
silence_jack_errors (bool silent)
{
    if (silent)
    {
        jack_set_error_function(jack_message_bit_bucket);
    }
}

/**
 *  This function silences JACK info output to the console.
 */

void
silence_jack_info (bool silent)
{
    if (silent)
    {
        jack_set_info_function(jack_message_bit_bucket);
    }
}

}           // namespace seq64

/*
 * midi_jack_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

