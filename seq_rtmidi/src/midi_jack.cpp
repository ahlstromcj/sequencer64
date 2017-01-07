/**
 * \file          midi_jack.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2017-01-07
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  Written primarily by Alexander Svetalkin, with updates for delta time by
 *  Gary Scavone, April 2011.
 *
 *  In this refactoring, we have to warp the RtMidi model, where ports are
 *  opened directly by the application, to the Sequencer64 midibus model,
 *  where the port object is created, but initialized after creation.  This
 *  proved very challenging -- it took a long time to get the midi_alsa
 *  implementation working.
 *
 *  There is an additional issue with JACK ports.  First, think of our ALSA
 *  implementation.  We have two modes:  manual (virtual) and real (normal)
 *  ports.  In ALSA, the manual mode exposes Sequencer64 ports (1 input port,
 *  16 output ports) to which other applications can connect.  The real/normal
 *  mode, via a midi_alsa_info object, determines the list of existing ALSA
 *  ports in the system, and then Sequencer64 ports are created (via the
 *  midibus) that are local, but connect to these "destination" system ports.
 *
 *  In JACK, we can do something similar.  Use the manual/virtual mode to
 *  allow Sequencer64 (seq64) to be connected manually via something like
 *  QJackCtl or a session manager.  Use the real/normal mode to connect
 *  automatically to whatever is already present.  Currently, though, new
 *  devices that appear in the system won't be accessible until a restart.
 *
 */

#include <sstream>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include "jack_assistant.hpp"           /* seq64::jack_status_pair_t    */
#include "midi_jack.hpp"
#include "settings.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Delimits the size of the JACK ringbuffer.
 */

#define JACK_RINGBUFFER_SIZE 16384      /* default size for ringbuffer  */

/**
 *    Provides the JACK process input callback.
 *
 * \param nframes
 *    The frame number to be processed.
 *
 * \param arg
 *    A pointer to the JackMIDIData structure to be processed.
 *
 * \return
 *    Returns 0.
 */

static int
jack_process_input (jack_nframes_t nframes, void * arg)
{
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    rtmidi_in_data * rtindata = jackdata->m_jack_rtmidiin;
    jack_midi_event_t event;
    jack_time_t time;
    if (jackdata->m_jack_port == NULL)      /* is port created?        */
       return 0;

    /*
     * We have MIDI events in the buffer.
     */

    void * buff = jack_port_get_buffer(jackdata->m_jack_port, nframes);
    int evcount = jack_midi_get_event_count(buff);
    for (int j = 0; j < evcount; ++j)
    {
        midi_message message;
        jack_midi_event_get(&event, buff, j);
        for (int i = 0; i < event.size; ++i)
            message.push(event.buffer[i]);

        time = jack_get_time();              /* compute the delta time  */
        if (rtindata->first_message())
            rtindata->first_message(false);
        else
            message.timestamp((time - jackdata->m_jack_lasttime) * 0.000001);

        jackdata->m_jack_lasttime = time;
        if (! rtindata->continue_sysex())
        {
            /*
             * We probably need this callback to interface with the
             * midibus-based code.
             */

            if (rtindata->using_callback())
            {
                rtmidi_callback_t callback = rtindata->user_callback();
                callback(message, rtindata->user_data());
            }
            else
            {
                /*
                 * As long as we haven't reached our queue size limit, push
                 * the message.
                 */

                (void) rtindata->queue().add(message);
            }
        }
    }
    return 0;
}

/**
 *  Defines the JACK output process callback.
 *
 * \param nframes
 *    The frame number to be processed.
 *
 * \param arg
 *    A pointer to the JackMIDIData structure to be processed.
 *
 * \return
 *    Returns 0.
 */

static int
jack_process_output (jack_nframes_t nframes, void * arg)
{
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    if (not_nullptr(jackdata->m_jack_port))             /* is port created? */
    {
        void * buff = jack_port_get_buffer(jackdata->m_jack_port, nframes);
        jack_midi_clear_buffer(buff);
        while (jack_ringbuffer_read_space(jackdata->m_jack_buffsize) > 0)
        {
            int space;
            jack_ringbuffer_read
            (
                jackdata->m_jack_buffsize, (char *) &space, sizeof(space)
            );
            jack_midi_data_t * md = jack_midi_event_reserve(buff, 0, space);
            char * mididata = reinterpret_cast<char *>(md);
            jack_ringbuffer_read
            (
                jackdata->m_jack_buffmessage, mididata, size_t(space)
            );
        }
    }
    return 0;
}

/*
 * MIDI JACK base class.
 *
 * We still need to figure out if we want a "master" client handle, or a handle
 * to each port.  Same for the JACK data item.
 */

midi_jack::midi_jack (midi_info & masterinfo, int index)
 :
    midi_api        (masterinfo, index),
    m_jack_data     ()
{
    client_handle((reinterpret_cast<jack_client_t *>(masterinfo.midi_handle())));
}

/**
 *  This could be a rote empty destructor if we offload this destruction to the
 *  midi_jack_data structure.  However, other than the initialization, this
 *  structure is "dumb".
 */

midi_jack::~midi_jack ()
{
    close_port();
    close_client();
    if (not_nullptr(m_jack_data.m_jack_buffsize))
        jack_ringbuffer_free(m_jack_data.m_jack_buffsize);

    if (not_nullptr(m_jack_data.m_jack_buffmessage))
        jack_ringbuffer_free(m_jack_data.m_jack_buffmessage);
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual ports" option is not in force.  This code is basically what was
 *  done by midi_out_jack::open_port() in RtMidi.
 *
\verbatim
    master_info().connect_name(get_bus_index())
    master_info().get_bus_name(get_bus_index())
    master_info().get_port_name(get_bus_index())
\endverbatim
 *
 *  For jack_connect(), the first port-name is the source port, and the source
 *  port must include the JackPortIsOutput flag.  The second port-name is the
 *  destination port, and the destination port must include the
 *  JackPortIsInput flag.  For this function, the source/output port is this
 *  port, and the destination/input is....
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midi_jack::api_init_out ()
{
    master_midi_mode(SEQ64_MIDI_OUTPUT);
    int portid = master_info().get_port_id(get_bus_index());
    std::string sourceportname = api_get_port_name();
    std::string destportname = master_info().get_port_name(get_bus_index());
//  std::string busname = master_info().get_bus_name(get_bus_index());
    bool result = connect_port(SEQ64_MIDI_OUTPUT, destportname, sourceportname);
    if (result)
        set_port_open();

    return result;
}

/**
 *
 *  We can't use the API port name here at this time, because it comes up
 *  empty.  It comes up empty because we haven't yet registered the ports,
 *  including the source ports.
 *
 *  So we register it first; connect_port() will not try to register it again.
 *
 * \return
 *      Returns true if the function was successful, and sets the flag
 *      indicating that the port is open.
 */

bool
midi_jack::api_init_in ()
{
    master_midi_mode(SEQ64_MIDI_INPUT);
    int portid = master_info().get_port_id(get_bus_index());
    std::string sourceportname = api_get_port_name();
    std::string destportname = master_info().get_port_name(get_bus_index());
//  std::string busname = master_info().get_bus_name(get_bus_index());
    bool result = connect_port(SEQ64_MIDI_INPUT, destportname, sourceportname);
    if (result)
        set_port_open();

    return result;
}

/*
 *
 *  This initialization is like the "open_virtual_port()" function of the
 *  RtMidi library...
 */

bool
midi_jack::api_init_out_sub ()
{
    master_midi_mode(SEQ64_MIDI_OUTPUT);
    int portid = master_info().get_port_id(get_bus_index());
    std::string portname = master_info().get_port_name(get_bus_index());
    bool result = register_port(SEQ64_MIDI_OUTPUT, portname);
    if (result)
        set_port_open();

    return result;
}

bool
midi_jack::api_init_in_sub ()
{
    master_midi_mode(SEQ64_MIDI_INPUT);
    int portid = master_info().get_port_id(get_bus_index());
    std::string portname = master_info().get_port_name(get_bus_index());
    bool result = register_port(SEQ64_MIDI_INPUT, portname);
    if (result)
        set_port_open();

    return result;
}

bool
midi_jack::api_deinit_in ()
{
    return true;
}

void
midi_jack::api_play (event * e24, midibyte channel)
{
}

void
midi_jack::api_sysex (event * e24)
{
}

void
midi_jack::api_flush ()
{
}

void
midi_jack::api_continue_from (midipulse tick, midipulse beats)
{
}

void
midi_jack::api_start ()
{
}

void
midi_jack::api_stop ()
{
}

void
midi_jack::api_clock (midipulse tick)
{
}

void
midi_jack::api_set_ppqn (int ppqn)
{
}

void
midi_jack::api_set_beats_per_minute (int bpm)
{
}

/**
 *  Gets the name of the current port via jack_port_name().  This is different
 *  from get_port_name(index), which simply looks up the port-name in the
 *  attached midi_info object.
 *
 * \return
 *      Returns the full port name ("clientname:portname") if the port has
 *      already been opened/registered; otherwise an empty string is returned.
 */

std::string
midi_jack::api_get_port_name ()
{
    std::string result;
    if (not_nullptr(m_jack_data.m_jack_port))
        result = std::string(jack_port_name(m_jack_data.m_jack_port));

    return result;
}

/**
 *  Opens input or output JACK clients, sets up the input or output callback,
 *  and actives the JACK client.  This code is combined from the former
 *  versions of the midi_in_jack::connect() and midi_out_jack::connect()
 *  functions for better readability and re-use in the input and output
 *  open_client() functions.
 *
 *  For input, it connects the MIDI input port.  The following calls are made:
 *
 *      -   jack_client_open(), to initialize JACK client.
 *      -   jack_set_process_callback(), to set jack_process_input() or
 *          jack_process_output().
 *      -   jack_activate().
 *
 *  For output, connects the MIDI output port.  The following calls are made:
 *
 *      -   jack_ringbuffer_create(), called twice, to initialize the
 *          output ringbuffers
 *      -   jack_client_open(), to initialize JACK client
 *      -   jack_set_process_callback(), to set jack_process_inpu()
 *      -   jack_activate().  This function is called only if every other step
 *          succeeds.
 *
 *  If the midi_jack_data client member is already set, this function returns
 *  immediately.  Only one client needs to be open for each midi_jack object.
 *
 *  Let's replace JackNullOption with JackNoStartServer.  We might also want to
 *  OR in the JackUseExactName option.
 *
 *  The former name of this function was a bit of a misnomer, since it does not
 *  actually call jack_connect().  That call is made in other functions.
 *
 *  Which "client" name?  Let's start with the full name, connect_name().
 *  Is UUID an output-only, input-only option, or both?
 *
\verbatim
    const char * name = master_info().get_bus_name(get_bus_index()).c_str();
    const char * name = master_info().get_port_name(get_bus_index()).c_str();
\endverbatim
 *
 * \todo
 *      See if the jack_client_open() call from jack_assistant is better or
 *      has useful features.
 *
 * \param input
 *      True if an input connection is to be made, and false if an output
 *      connection is to be made.
 */

bool
midi_jack::open_client_impl (bool input)
{
    bool result = true;
    master_midi_mode(input);
    if (is_nullptr(client_handle()))
    {
        const char * name = master_info().connect_name(get_bus_index()).c_str();
        jack_client_t * clipointer = nullptr;
        jack_status_t status;
        jack_status_t * pstatus = &status;
        jack_options_t options = JackNoStartServer;
        if (rc().jack_session_uuid().empty())
        {
            clipointer = jack_client_open(name, options, pstatus);
        }
        else
        {
            const char * uuid = rc().jack_session_uuid().c_str();
            clipointer = jack_client_open(name, options, pstatus, uuid);
        }
        if (is_nullptr(clipointer))
        {
            m_error_string = func_message("JACK server not running?");
            error(rterror::WARNING, m_error_string);
        }
        else
        {
            client_handle(clipointer);
            if (not_nullptr(pstatus))
            {
                if (status & JackServerStarted)
                    (void) info_message("JACK server started");
                else
                    (void) info_message("JACK server already started");

                if (status & JackNameNotUnique)
                    (void) info_message("JACK client-name not unique");

                jack_assistant::show_statuses(status);
            }
            if (input)
            {
                int rc = jack_set_process_callback
                (
                    clipointer, jack_process_input, &m_jack_data
                );
                if (rc != 0)
                {
                    m_error_string = func_message
                    (
                        "JACK error setting process-input callback"
                    );
                    error(rterror::WARNING, m_error_string);
                }
                else
                    jack_activate(clipointer);
            }
            else
            {
                bool iserror = false;
                jack_ringbuffer_t * rb =
                    jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);

                if (not_nullptr(rb))
                {
                    m_jack_data.m_jack_buffsize = rb;
                    rb = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);
                    if (not_nullptr(rb))
                        m_jack_data.m_jack_buffmessage = rb;
                    else
                        iserror = true;
                }
                else
                    iserror = true;

                if (iserror)
                {
                    m_error_string = func_message("JACK ringbuffer error");
                    error(rterror::WARNING, m_error_string);
                }
                else
                {
                    int rc = jack_set_process_callback
                    (
                        clipointer, jack_process_output, &m_jack_data
                    );
                    if (rc != 0)
                    {
                        m_error_string = func_message
                        (
                            "JACK error setting process-output callback"
                        );
                        error(rterror::WARNING, m_error_string);
                    }
                    else
                        jack_activate(clipointer);
                }
            }
        }
    }
    else
    {
        (void) info_message("JACK client already open");
    }
    return result;
}

/**
 *  Closes the JACK client handle.
 */

void
midi_jack::close_client ()
{
    if (not_nullptr(client_handle()))
    {
        int rc = jack_client_close(client_handle());
        if (rc != 0)
        {
            int index = get_bus_index();
            int id = master_info().get_port_id(index);
            m_error_string = func_message("JACK closing port #");
            m_error_string += std::to_string(index);
            m_error_string += " (id ";
            m_error_string += std::to_string(id);
            m_error_string += ")";
            error(rterror::DRIVER_ERROR, m_error_string);
        }
    }
}

/**
 *  Connects two named JACK ports.
 *
 * \param input
 *      Indicates true if the port to register and connect is an input port,
 *      and false if the port is an output port.  Useful macros for readability:
 *      SEQ64_MIDI_INPUT and SEQ64_MIDI_OUTPUT.
 *
 * \param sourceportname
 *      Provides the destination port-name for the connection.  For input, this
 *      should be the name associated with the JACK client handle; it is the
 *      port that gets registered.  For output, this should be the name of the
 *      port that was enumerated at start-up.  The JackPortFlags of the
 *      source_port must include JackPortIsOutput.
 *
 * \param destportname
 *      For input, this should be name of port that was enumerated at start-up.
 *      For output, this should be the name associated with the JACK client
 *      handle; it is the port that gets registered.  The JackPortFlags of the
 *      destination_port must include JackPortIsInput.
 */

bool
midi_jack::connect_port
(
    bool input,
    const std::string & sourceportname,
    const std::string & destportname
)
{
    bool result = register_port(input, sourceportname);
    if (result)
    {
        int rc = jack_connect
        (
            client_handle(), sourceportname.c_str(), destportname.c_str()
        );
        result = rc == 0;
        if (! result)
        {
            m_error_string = func_message("JACK error connecting port ");
            m_error_string += input ? "input '" : "output '";
            m_error_string += sourceportname;
            m_error_string += "' to '";
            m_error_string += destportname;
            m_error_string += "'";
            error(rterror::DRIVER_ERROR, m_error_string);
        }
    }
    return result;
}

/**
 *  Registers a named JACK port.  We made this function to encapsulate some
 *  otherwise cut-and-paste functionality.
 *
 *  For jack_port_register(), the port-name must be the actual port-name, not
 *  the full-port name ("busname:portname").  Not really sure if this is true!!!
 *
 * \param input
 *      Indicates true if the port to register input port, and false if the port
 *      is an output port.  Two macros can be used for this purpose:
 *      SEQ64_MIDI_INPUT and SEQ64_MIDI_OUTPUT.
 *
 * \param portname
 *      Provides the name of the port.  We think that this can either be just
 *      the port name or the full connect-name of the port,
 *      "clientname:portname".
 */

bool
midi_jack::register_port (bool input, const std::string & portname)
{
    bool result = not_nullptr(m_jack_data.m_jack_port);
    if (! result)
    {
        jack_port_t * p = jack_port_register
        (
            client_handle(), portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE,
            input ? JackPortIsInput : JackPortIsOutput,
            0               /* buffer size of non-built-in port type, ignored */
        );
        if (not_nullptr(p))
        {
            m_jack_data.m_jack_port = p;
            result = true;
        }
        else
        {
            m_error_string = func_message("JACK error registering port");
            m_error_string += " ";
            m_error_string += portname;
            error(rterror::DRIVER_ERROR, m_error_string);
        }
    }
    return result;
}

/**
 *  Closes the MIDI port by calling jack_port_unregister() and
 *  nullifying the port pointer.
 */

void
midi_jack::close_port ()
{
    if (not_nullptr(port_handle()))
    {
        jack_port_unregister(client_handle(), port_handle());
        port_handle(nullptr);
    }
}

/*
 * MIDI JACK input class.
 */

/**
 *  Principal constructor.  For Sequencer64, we don't current need to create
 *  a midi_in_jack object; all that is needed is created via the
 *  api_init_in*() functions.  Also, this constructor still needs to do
 *  something with queue size.
 *
 * \param clientname
 *      The name to give the client.
 *
 * \param queuesize
 *      Provides the limit of the size of the MIDI input queue.
 */

midi_in_jack::midi_in_jack
(
    midi_info & masterinfo, int index,
    const std::string & clientname,
    unsigned /*queuesize*/
) :
    midi_jack       (masterinfo, index)
{
    (void) initialize(clientname);
}

/**
 *  Destructor.  Closes the port, closes the JACK client, and cleans up the
 *  API data structure.
 */

midi_in_jack::~midi_in_jack()
{
}

/**
 *  Initializes the JACK client MIDI data structure.  Then calls the connect()
 *  function.
 *
 * \param clientname
 *      Provides the name of the client.
 */

bool
midi_in_jack::initialize (const std::string & clientname)
{
    m_clientname = clientname;
    return open_client_impl(SEQ64_MIDI_INPUT);  /* open_client() is virtual */
}

/**
 *  Opens the JACK MIDI input port.  It makes the following calls:
 *
 *      -   open_client(), which operates only if not called already.
 *      -   connect_port(), which registers a new port and then connects it.
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name under which the port is to be registered.
 */

bool
midi_in_jack::open_port (int portnumber, const std::string & portname)
{
    bool result = open_client();
    if (result)
    {
//      std::string sourceportname = get_port_name(portnumber);
//      std::string destportname = api_get_port_name();
        master_midi_mode(SEQ64_MIDI_INPUT);
        std::string destportname = master_info().get_port_name(portnumber);
        result = connect_port(SEQ64_MIDI_INPUT, portname, destportname);
    }
    return result;
}

/**
 *  Opens a virtual JACK MIDI port.  It calls:
 *
 *      -   open_client()
 *      -   register_port()
 *
 *  The only difference between a JACK virtual port and a JACK normal port
 *  is that the latter is also connected after registration.
 *
 * \param portname
 *      Provides the port name under which the virtual port is registered.
 */

bool
midi_in_jack::open_virtual_port (const std::string & portname)
{
    bool result = open_client();
    if (result)
        result = register_port(SEQ64_MIDI_INPUT, portname);

    return result;
}

#if 0

/**
 *  Retrieves the name of the desired port.  It gets it by iterating through
 *  the available ports to get the name.
 *
 *  CAN WE REPLACE THIS WITH A midi_info LOOKUP?
 *
 * \param portnumber
 *      The port number for which to get the name.
 *
 * \return
 *      Returns the port name as a standard C++ string.
 */

std::string
midi_in_jack::get_port_name (int portnumber)
{
    std::string result;
    open_client();

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        client_handle(), NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
    );
    if (is_nullptr(ports))                      /* Check port validity      */
    {
        m_error_string = func_message("no ports available");
        error(rterror::WARNING, m_error_string);
        return result;
    }

    if (is_nullptr(ports[portnumber]))
    {
        std::ostringstream ost;
        ost
            << func_message("'portnumber' argument (")
            << portnumber << ") is invalid"
            ;
        m_error_string = ost.str();
        error(rterror::WARNING, m_error_string);
    }
    else
        result.assign(ports[portnumber]);

    free(ports);
    return result;
}

#endif

/*
 * API: JACK Class Definitions: midi_out_jack
 */

/**
 *  Principal constructor.  For Sequencer64, we don't current need to create
 *  a midi_out_jack object; all that is needed is created via the
 *  api_init_out*() functions.
 *
 * \param clientname
 *      The name of the MIDI output port.
 */

midi_out_jack::midi_out_jack
(
    midi_info & masterinfo, int index,
    const std::string & clientname
) :
    midi_jack       (masterinfo, index),
    m_clientname    ()
{
    (void) initialize(clientname);
}

/**
 *  The destructor closes the port and cleans out the API data structure.
 */

midi_out_jack::~midi_out_jack ()
{
}

/**
 *  Initializes the midi_jack_data structure and calls open_client().
 *
 * \param clientname
 *      The name of the MIDI output port.
 */

bool
midi_out_jack::initialize (const std::string & clientname)
{
    m_clientname = clientname;
    return open_client_impl(SEQ64_MIDI_OUTPUT); /* open_client() is virtual */
}

/**
 *  Opens the JACK MIDI output port.  It makes the following calls:
 *
 *      -   open_client()
 *      -   connect_port(), which registers a new port and then connects it.
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name under which the port is registered.
 */

bool
midi_out_jack::open_port (int portnumber, const std::string & portname)
{
    bool result = open_client();
    if (result)
    {
        master_midi_mode(SEQ64_MIDI_OUTPUT);
        std::string sourceportname = master_info().get_port_name(portnumber);
        result = connect_port(SEQ64_MIDI_OUTPUT, sourceportname, portname);
    }
    return result;
}

/**
 *  Opens a virtual JACK MIDI port.  It calls:
 *
 *      -   open_client()
 *      -   register_port()
 *
 *  Note that a virtual port is not connected.
 *
 * \param portname
 *      Provides the port name under which the virtual port is registered.
 */

bool
midi_out_jack::open_virtual_port (const std::string & portname)
{
    bool result = open_client();
    if (result)
        result = register_port(SEQ64_MIDI_OUTPUT, portname);

    return result;
}

#if 0

/**
 *  Retrieves the name of the desired port.  It gets it by iterating through
 *  the available ports to get the name.
 *
 * \param portnumber
 *      The port number for which to get the name.
 *
 * \return
 *      Returns the port name as a standard C++ string.
 */

std::string
midi_out_jack::get_port_name (int portnumber)
{
    std::string result;
    open_client();
    const char ** ports = jack_get_ports        /* List of available ports  */
    (
        client_handle(), NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
    );

    if (is_nullptr(ports))
    {
        m_error_string = func_message("no ports available");
        error(rterror::WARNING, m_error_string);
        return result;
    }

    if (is_nullptr(ports[portnumber]))
    {
        std::ostringstream ost;
        ost
            << func_message("'portnumber' argument (")
            << portnumber << ") is invalid."
            ;
        m_error_string = ost.str();
        error(rterror::WARNING, m_error_string);
    }
    else
        result.assign(ports[portnumber]);

    free(ports);
    return result;
}

#endif

/**
 *  Sends a JACK MIDI output message.
 *  It writes the  full message size and the message itself to the JACK ring
 *  buffer.
 *
 * \param message
 *      Provides the vector of message bytes to send.
 */

bool
midi_out_jack::send_message (const std::vector<midibyte> & message)
{
    int nbytes = message.size();
    int count1 = jack_ringbuffer_write
    (
        m_jack_data.m_jack_buffmessage, (const char *) &message[0], message.size()
    );
    int count2 = jack_ringbuffer_write
    (
        m_jack_data.m_jack_buffsize, (char *) &nbytes, sizeof(nbytes)
    );
    return (count1 > 0) && (count2 > 0);
}

}           // namespace seq64

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

