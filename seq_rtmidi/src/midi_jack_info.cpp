/**
 * \file          midi_jack_info.cpp
 *
 *    A class for obtaining JACK port information.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-01-01
 * \updates       2018-06-02
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  This class is meant to collect a whole bunch of JACK information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating JACK midibus objects and midi_jack API objects.
 *
 * Multi-client mode:
 *
 *  As noted in issue #73 on GitHub, JACK MIDI timing can be a bit off:
 *
 *      I always notice small "timing drops" ...  when using jack midi. There
 *      is no xrun, but just playing a regular kick with a high bpm, it's easy
 *      to notice the time between kicks is not perfectly regular. Using alsa
 *      midi, it always works as expected.  Oh, I just noticed this timing
 *      issue is highly depending on jack buffer size.  I initially had it set
 *      to 1024 (and 3 periods). Issue is subtle but can be heard.  Then I
 *      tested with 256, and couldn't really hear the issue (but then I have
 *      occasional xruns, very few but still).  Then I tested with 2048, and
 *      the issue gets really worse, really easy to hear it.  A TimeTest.midi
 *      file is provided to demonstrate this issue.
 *
 *  So we're going to try to put MIDI input and output on separate clients, as
 *  an option.
 */

#include "calculations.hpp"             /* extract_port_names()             */
#include "easy_macros.hpp"              /* C++ version of easy macros       */
#include "event.hpp"                    /* seq64::event and other tokens    */
#include "jack_assistant.hpp"           /* seq64::create_jack_client()      */
#include "midi_jack.hpp"                /* seq64::midi_jack_info            */
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
 *  midi_jack module.  This function calls both the input callback and
 *  the output callback, depending on the port type.  This may lead to
 *  delays, depending on the size of the JACK MIDI buffer.
 *
 * \param nframes
 *      The frame number from the JACK API.
 *
 * \param arg
 *      The putative pointer to the midi_jack_info structure.
 *
 * \return
 *      Always returns 0.
 */

int
jack_process_io (jack_nframes_t nframes, void * arg)
{
    if (nframes > 0)
    {
        midi_jack_info * self = reinterpret_cast<midi_jack_info *>(arg);
        if (not_nullptr(self))
        {
            /*
             * Here we want to go through the I/O ports and route the data
             * appropriately.
             */

            std::vector<midi_jack *>::iterator mi;
            for
            (
                mi = self->m_jack_ports.begin();
                mi != self->m_jack_ports.end(); ++mi
            )
            {
                midi_jack * mj = *mi;
                midi_jack_data * mjp = &mj->jack_data();
                if (mj->parent_bus().is_input_port())
                    (void) jack_process_rtmidi_input(nframes, mjp);
                else
                    (void) jack_process_rtmidi_output(nframes, mjp);
            }
        }
    }
    return 0;
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

midi_jack_info::midi_jack_info
(
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_jack_ports            (),
    m_jack_client           (nullptr),              /* inited for connect() */
    m_jack_client_2         (nullptr)
{
    silence_jack_info();
    m_jack_client = connect();
    if (not_nullptr(m_jack_client))                 /* created by connect() */
    {
        midi_handle(m_jack_client);                 /* void version         */
        client_handle(m_jack_client);               /* jack version         */
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
        /*
         * int jacksize = jack_port_name_size();
         * infoprintf("JACK PORT NAME SIZE = %d\n", jacksize); // = 320
         */

        const char * clientname = rc().app_client_name().c_str();
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
        apiprint("jack_deactivate", "info");
        apiprint("jack_client_close", "info");
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
 * JackPortIsPhysical:
 *
 *  If this flag is added, then only ports corresponding to a physical device
 *  are get detected and connected.  This might be a useful option to add at a
 *  later date.
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
        input_ports().clear();
        output_ports().clear();

        const char ** inports = jack_get_ports    /* list of JACK ports   */
        (
            m_jack_client, NULL,
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsInput                            /* tricky   */
        );
        if (is_nullptr(inports))                  /* check port validity  */
        {
            warnprint("no JACK input port available, creating virtual port");
            int clientnumber = 0;
            int portnumber = 0;
            std::string clientname = rc().app_client_name();
            std::string portname = clientname + " midi in 0";
            input_ports().add
            (
                clientnumber, clientname, portnumber, portname,
                SEQ64_MIDI_VIRTUAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                SEQ64_MIDI_INPUT_PORT
            );
            ++result;
        }
        else
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
            std::string clientname = rc().app_client_name();
            std::string portname = clientname + " midi out 0";
            output_ports().add
            (
                client, clientname, 0, portname,
                SEQ64_MIDI_VIRTUAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                SEQ64_MIDI_OUTPUT_PORT
            );
            ++result;
        }
        else
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
 *  Each JACK port's midi_jack::api_connect() function decides, based on
 *  multi-client status, whether or not to activate before making the
 *  connection.
 *
 * \return
 *      Returns true if activation succeeds.
 */

bool
midi_jack_info::api_connect ()
{
    bool result = not_nullptr(client_handle());
    if (result)
    {
        int rc = jack_activate(client_handle());
        apiprint("jack_activate", "info");
        result = rc == 0;
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
                    break;
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
midi_jack_info::api_set_ppqn (int p)
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
midi_jack_info::api_set_beats_per_minute (midibpm b)
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
 *  midi_jack_info.
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
midi_jack_info::api_port_start
(
    mastermidibus & /*masterbus*/, int /*bus*/, int /*port*/
)
{
    // no code, was multi-client code
}

/**
 *  We might be able to eliminate this function.
 */
int
midi_jack_info::api_poll_for_midi ()
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
midi_jack_info::api_get_midi_event (event * /*inev*/)
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

}           // namespace seq64

/*
 * midi_jack_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

