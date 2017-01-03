/**
 * \file          midi_jack.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2017-01-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    Written primarily by Alexander Svetalkin, with updates for delta time by
 *    Gary Scavone, April 2011.
 *
 *  In this refactoring...
 *
 *  API information found at:
 *
 *      - http://www....
 */

#include <sstream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

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
    midi_jack_data * jackdata = (midi_jack_data *) arg;
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
 *  Principal constructor.
 *
 * \param clientname
 *      The name to give the client.
 *
 * \param queuesize
 *      Provides the limit of the size of the MIDI input queue.
 */

midi_in_jack::midi_in_jack
(
    const std::string & clientname,
    unsigned queuesize
) :
    midi_in_api     (queuesize),
    m_clientname    (clientname)
{
    initialize(clientname);
}

/**
 *  Initializes the JACK client MIDI data structure.  Then calls the connect()
 *  function.
 *
 * \param clientname
 *      Provides the name of the client.
 */

void
midi_in_jack::initialize (const std::string & clientname)
{
    midi_jack_data * jackdata = new midi_jack_data;
    m_api_data = jackdata;                              /* no cast needed */
    m_clientname = clientname;
    connect();
}

/**
 *  Connects the MIDI input port.  The following calls are made:
 *
 *      -   jack_client_open(), to initialize JACK client.
 *          We've added the code used in jack_assistant to get better status
 *          information.
 *      -   jack_set_process_callback(), to set jack_process_input().
 *      -   jack_activate().
 *
 *  If the midi_jack_data client member is already set, this function returns
 *  immediately.  Only one client needs to be open.
 *
 * \todo
 *      See if the jack_client_open() call from jack_assistant is better or
 *      has useful features.
 */

void
midi_in_jack::connect ()
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    if (is_nullptr(jackdata->m_jack_client))
    {
        jack_client_t * result = nullptr;
        jack_status_t status;
        jack_status_t * pstatus = &status;
        const char * name = m_clientname.c_str();
        if (rc().jack_session_uuid().empty())
        {
            /*
             * Let's replace JackNullOption.
             */

            result = jack_client_open(name, JackNoStartServer, pstatus);
        }
        else
        {
            const char * uuid = rc().jack_session_uuid().c_str();
            result = jack_client_open(name, JackNoStartServer, pstatus, uuid);
        }
        if (is_nullptr(result))
        {
            m_error_string = func_message("JACK server not running?");
            error(rterror::WARNING, m_error_string);
            return;
        }
        else
        {
            if (not_nullptr(pstatus))
            {
                if (status & JackServerStarted)
                    (void) info_message("JACK server started");
                else
                    (void) info_message("JACK server already started");

                if (status & JackNameNotUnique)
                    (void) info_message("JACK client-name NOT unique");
            }
            jackdata->m_jack_client = result;
            jack_set_process_callback(result, jack_process_input, jackdata);
            jack_activate(result);
        }
    }
    else
    {
        (void) info_message("JACK server already started");
    }
}

/**
 *  Destructor.  Closes the port, closes the JACK client, and cleans up the
 *  API data structure.
 */

midi_in_jack::~midi_in_jack()
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    close_port();
    if (not_nullptr(jackdata->m_jack_client))
        jack_client_close(jackdata->m_jack_client);

    delete jackdata;
}

/**
 *  Opens the JACK MIDI input port.  It makes the following calls:
 *
 *      -   connect(), which operates only if not called already.
 *      -   jack_port_register(), which creates a new port.
 *      -   jack_connect(), to connect to the output (?)
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name under which the port is registered.
 */

void
midi_in_jack::open_port (int portnumber, const std::string & portname)
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->m_jack_port))
    {
        jackdata->m_jack_port = jack_port_register
        (
            jackdata->m_jack_client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
        );
    }
    if (is_nullptr(jackdata->m_jack_port))
    {
        m_error_string = func_message("JACK error creating port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    std::string name = get_port_name(portnumber);
    jack_connect
    (
        jackdata->m_jack_client, name.c_str(),
        jack_port_name(jackdata->m_jack_port)
    );
}

/**
 *  Opens a virtual JACK MIDI port.  It calls:
 *
 *      -   connect()
 *      -   jack_port_register()
 *
 * \param portname
 *      Provides the port name under which the virtual port is registered.
 */

void
midi_in_jack::open_virtual_port (const std::string & portname)
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->m_jack_port))
    {
        jackdata->m_jack_port = jack_port_register
        (
            jackdata->m_jack_client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
        );
    }
    if (is_nullptr(jackdata->m_jack_port))
    {
        m_error_string = func_message("JACK error creating virtual port");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
}

/**
 *  Retrieves the number of JACK MIDI output (!) ports.  Note that this
 *  function gets the kind of port opposite its enclosing class.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      jack_get_ports() call.
 */

int
midi_in_jack::get_port_count ()
{
    int count = 0;
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->m_jack_client))
        return 0;

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        jackdata->m_jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
    );
    if (is_nullptr(ports))
        return 0;

    while (not_nullptr(ports[count]))
        ++count;

    free(ports);
    return count;
}

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
midi_in_jack::get_port_name (int portnumber)
{
    std::string result;
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        jackdata->m_jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
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

/**
 *  Closes the MIDI input port by calling jack_port_unregister() and
 *  nullifying the port pointer.
 */

void
midi_in_jack::close_port ()
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    if (is_nullptr(jackdata->m_jack_port))
        return;

    jack_port_unregister(jackdata->m_jack_client, jackdata->m_jack_port);
    jackdata->m_jack_port = nullptr;
}

/*
 * API: JACK Class Definitions: midi_out_jack
 */

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

/**
 *  Principal constructor.
 *
 * \param clientname
 *      The name of the MIDI output port.
 */

midi_out_jack::midi_out_jack (const std::string & clientname)
 :
    midi_out_api    (),
    m_clientname    ()
{
    initialize(clientname);
}

/**
 *  The destructor closes the port and cleans out the API data structure.
 */

midi_out_jack::~midi_out_jack ()
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    close_port();
    jack_ringbuffer_free(jackdata->m_jack_buffsize);
    jack_ringbuffer_free(jackdata->m_jack_buffmessage);
    if (jackdata->m_jack_client)
        jack_client_close(jackdata->m_jack_client);

    delete jackdata;
}

/**
 *  Initializes the midi_jack_data structure and calls connect().
 *
 * \param clientname
 *      The name of the MIDI output port.
 */

void
midi_out_jack::initialize (const std::string & clientname)
{
    midi_jack_data * jackdata = new midi_jack_data;
    m_api_data = jackdata;                          /* no cast needed */
    m_clientname = clientname;
    connect();
}

/**
 *  Connects the MIDI output port.  The following calls are made:
 *
 *      -   jack_ringbuffer_create(), called twice, to initialize the
 *          output ringbuffers
 *      -   jack_client_open(), to initialize JACK client
 *      -   jack_set_process_callback(), to set jack_process_inpu()
 *      -   jack_activate()
 *
 *  If the midi_jack_data client member is already set, this function returns
 *  immediately.
 */

void
midi_out_jack::connect ()
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    if (not_nullptr(jackdata->m_jack_client))
        return;

    jackdata->m_jack_buffsize = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);
    jackdata->m_jack_buffmessage = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);
    jackdata->m_jack_client = jack_client_open  /* initialize JACK client   */
    (
        m_clientname.c_str(), JackNoStartServer, NULL
    );
    if (is_nullptr(jackdata->m_jack_client))
    {
        m_error_string = func_message("JACK server not running?");
        error(rterror::WARNING, m_error_string);
        return;
    }
    jack_set_process_callback
    (
        jackdata->m_jack_client, jack_process_output, jackdata
    );
    jack_activate(jackdata->m_jack_client);
}

/**
 *  Opens the JACK MIDI output port.  It makes the following calls:
 *
 *      -   connect()
 *      -   jack_port_register(), which creates a new port.
 *      -   jack_connect(), to connect to the output (?)
 *
 * \param portnumber
 *      Provides the port number.
 *
 * \param portname
 *      Provides the port name under which the port is registered.
 */

void
midi_out_jack::open_port (int portnumber, const std::string & portname)
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->m_jack_port))
    {
        jackdata->m_jack_port = jack_port_register
        (
            jackdata->m_jack_client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
        );
    }

    if (is_nullptr(jackdata->m_jack_port))
    {
        m_error_string = func_message("JACK error creating port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    std::string name = get_port_name(portnumber);   /* connecting to output */
    jack_connect
    (
        jackdata->m_jack_client, jack_port_name(jackdata->m_jack_port),
        name.c_str()
    );
}

/**
 *  Opens a virtual JACK MIDI port.  It calls:
 *
 *      -   connect()
 *      -   jack_port_register()
 *
 * \param portname
 *      Provides the port name under which the virtual port is registered.
 */

void
midi_out_jack::open_virtual_port (const std::string & portname)
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->m_jack_port))
    {
        jackdata->m_jack_port = jack_port_register
        (
            jackdata->m_jack_client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
        );
    }

    if (is_nullptr(jackdata->m_jack_port))
    {
        m_error_string = func_message("JACK error creating virtual port");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
}

/**
 *  Retrieves the number of JACK MIDI input (!) ports.  Note that this function
 *  gets the kind of port opposite its enclosing class.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      jack_get_ports() call.
 */

int
midi_out_jack::get_port_count ()
{
    int count = 0;
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    if (not_nullptr(jackdata->m_jack_client))
        return 0;

    const char ** ports = jack_get_ports    /* list of available ports */
    (
        jackdata->m_jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
    );
    if (is_nullptr(ports))
        return 0;

    while (not_nullptr(ports[count]))
        ++count;

    free(ports);
    return count;
}

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
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    connect();
    const char ** ports = jack_get_ports        /* List of available ports  */
    (
        jackdata->m_jack_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
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

/**
 *  Closes the MIDI output port by calling jack_port_unregister() and
 *  nullifying the port pointer.
 */

void
midi_out_jack::close_port ()
{
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    if (is_nullptr(jackdata->m_jack_port))
        return;

    jack_port_unregister(jackdata->m_jack_client, jackdata->m_jack_port);
    jackdata->m_jack_port = nullptr;
}

/**
 *  Sends a JACK MIDI output message.
 *  It writes the  full message size and the message itself to the JACK ring
 *  buffer.
 *
 * \param message
 *      Provides the vector of message bytes to send.
 */

void
midi_out_jack::send_message (const std::vector<midibyte> & message)
{
    int nBytes = message.size();
    midi_jack_data * jackdata = static_cast<midi_jack_data *>(m_api_data);
    jack_ringbuffer_write
    (
        jackdata->m_client_buffmessage, (const char *) &message[0], message.size()
    );
    jack_ringbuffer_write
    (
        jackdata->m_jack_buffsize, (char *) &nBytes, sizeof(nBytes)
    );
}

}           // namespace seq64

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

