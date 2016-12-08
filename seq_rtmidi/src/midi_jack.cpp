/**
 * \file          midi_jack.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-08
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
 *  Contains the JACK MIDI API data as a kind of scratchpad for this object.
 *  This guy needs a constructor taking parameters for an rtmidi_in_data
 *  pointer.
 */

struct JackMidiData
{
    jack_client_t * client;
    jack_port_t * port;
    jack_ringbuffer_t * buffSize;
    jack_ringbuffer_t * buffMessage;
    jack_time_t lastTime;
    rtmidi_in_data * rtMidiIn;
};

/*
 * API: JACK Class Definitions: midi_in_jack
 */

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
jackProcessIn (jack_nframes_t nframes, void * arg)
{
    JackMidiData * jackdata = (JackMidiData *) arg;
    rtmidi_in_data * rtindata = jackdata->rtMidiIn;
    jack_midi_event_t event;
    jack_time_t time;
    if (jackdata->port == NULL)                 /* is port created?        */
       return 0;

    /*
     * We have MIDI events in the buffer.
     */

    void * buff = jack_port_get_buffer(jackdata->port, nframes);
    int evcount = jack_midi_get_event_count(buff);
    for (int j = 0; j < evcount; ++j)
    {
        midi_message message;
        message.bytes.clear();
        jack_midi_event_get(&event, buff, j);
        for (unsigned i = 0; i < event.size; ++i)
            message.bytes.push_back(event.buffer[i]);

        time = jack_get_time();              /* compute the delta time  */
        if (rtindata->first_message())
            rtindata->first_message(false);
        else
            message.timeStamp = (time - jackdata->lastTime) * 0.000001;

        jackdata->lastTime = time;
        if (! rtindata->continue_sysex())
        {
            if (rtindata->using_callback())
            {
                rtmidi_callback_t callback = rtindata->user_callback();
                callback(message.timeStamp, message.bytes, rtindata->user_data());
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
    JackMidiData * jackdata = new JackMidiData;
    m_api_data = jackdata;                              /* no cast needed */
    jackdata->rtMidiIn = &m_input_data;
    jackdata->port = nullptr;
    jackdata->client = nullptr;
    jackdata->buffSize = nullptr;
    jackdata->buffMessage = nullptr;
    jackdata->lastTime = 0;
    m_clientname = clientname;
    connect();
}

/**
 *  Connects the MIDI input port.  The following calls are made:
 *
 *      -   jack_client_open(), to initialize JACK client.
 *          We've added the code used in jack_assistant to get better status
 *          information.
 *      -   jack_set_process_callback(), to set jackProcessIn().
 *      -   jack_activate().
 *
 *  If the JackMidiData client member is already set, this function returns
 *  immediately.
 *
 * \todo
 *      See if the jack_client_open() call from jack_assistant is better or
 *      has useful features.
 */

void
midi_in_jack::connect ()
{
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    if (is_nullptr(jackdata->client))
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
                    (void) info_message("JACK server started now");
                else
                    (void) info_message("JACK server already started");

                if (status & JackNameNotUnique)
                    (void) info_message("JACK client-name NOT unique");
            }
            jackdata->client = result;
            jack_set_process_callback(result, jackProcessIn, jackdata);
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    close_port();
    if (jackdata->client)
        jack_client_close(jackdata->client);

    delete jackdata;
}

/**
 *  Opens the JACK MIDI input port.  It makes the following calls:
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
midi_in_jack::open_port (unsigned portnumber, const std::string & portname)
{
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->port))
    {
        jackdata->port = jack_port_register
        (
            jackdata->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
        );
    }

    if (is_nullptr(jackdata->port))
    {
        m_error_string = func_message("JACK error creating port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    std::string name = get_port_name(portnumber);
    jack_connect(jackdata->client, name.c_str(), jack_port_name(jackdata->port));
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->port))
    {
        jackdata->port = jack_port_register
        (
            jackdata->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
        );
    }
    if (is_nullptr(jackdata->port))
    {
        m_error_string = func_message("JACK error creating virtual port");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
}

/**
 *  Retrieves the number of JACK MIDI output (!) ports.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      jack_get_ports() call.
 */

unsigned
midi_in_jack::get_port_count ()
{
    unsigned count = 0;
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->client))
        return 0;

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        jackdata->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
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
midi_in_jack::get_port_name (unsigned portnumber)
{
    std::string result;
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        jackdata->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    if (is_nullptr(jackdata->port))
        return;

    jack_port_unregister(jackdata->client, jackdata->port);
    jackdata->port = nullptr;
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
jackProcessOut (jack_nframes_t nframes, void * arg)
{
    JackMidiData * jackdata = reinterpret_cast<JackMidiData *>(arg);
    if (not_nullptr(jackdata->port))                     /* is port created? */
    {
        void * buff = jack_port_get_buffer(jackdata->port, nframes);
        jack_midi_clear_buffer(buff);
        while (jack_ringbuffer_read_space(jackdata->buffSize) > 0)
        {
            int space;
            jack_ringbuffer_read
            (
                jackdata->buffSize, (char *) &space, sizeof(space)
            );
            jack_midi_data_t * md = jack_midi_event_reserve(buff, 0, space);
            char * mididata = reinterpret_cast<char *>(md);
            jack_ringbuffer_read(jackdata->buffMessage, mididata, size_t(space));
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    close_port();
    jack_ringbuffer_free(jackdata->buffSize);
    jack_ringbuffer_free(jackdata->buffMessage);
    if (jackdata->client)
        jack_client_close(jackdata->client);

    delete jackdata;
}

/**
 *  Initializes the JackMidiData structure and calls connect().
 *
 * \param clientname
 *      The name of the MIDI output port.
 */

void
midi_out_jack::initialize (const std::string & clientname)
{
    JackMidiData * jackdata = new JackMidiData;
    m_api_data = jackdata;                          /* no cast needed */
    jackdata->port = nullptr;
    jackdata->client = nullptr;
    m_clientname = clientname;
    connect();
}

/**
 *  Connects the MIDI output port.  The following calls are made:
 *
 *      -   jack_ringbuffer_create(), called twice, to initialize the
 *          output ringbuffers
 *      -   jack_client_open(), to initialize JACK client
 *      -   jack_set_process_callback(), to set jackProcessIn()
 *      -   jack_activate()
 *
 *  If the JackMidiData client member is already set, this function returns
 *  immediately.
 */

void
midi_out_jack::connect ()
{
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    if (not_nullptr(jackdata->client))
        return;

    jackdata->buffSize = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);
    jackdata->buffMessage = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);
    jackdata->client = jack_client_open             /* initialize JACK client   */
    (
        m_clientname.c_str(), JackNoStartServer, NULL
    );
    if (is_nullptr(jackdata->client))
    {
        m_error_string = func_message("JACK server not running?");
        error(rterror::WARNING, m_error_string);
        return;
    }
    jack_set_process_callback(jackdata->client, jackProcessOut, jackdata);
    jack_activate(jackdata->client);
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
midi_out_jack::open_port (unsigned portnumber, const std::string & portname)
{
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->port))
    {
        jackdata->port = jack_port_register
        (
            jackdata->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
        );
    }

    if (is_nullptr(jackdata->port))
    {
        m_error_string = func_message("JACK error creating port");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    std::string name = get_port_name(portnumber);   /* connecting to output */
    jack_connect(jackdata->client, jack_port_name(jackdata->port), name.c_str());
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(jackdata->port))
    {
        jackdata->port = jack_port_register
        (
            jackdata->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
        );
    }

    if (is_nullptr(jackdata->port))
    {
        m_error_string = func_message("JACK error creating virtual port");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
}

/**
 *  Retrieves the number of JACK MIDI input (!) ports.
 *
 * \return
 *      Returns the number of port counted in the output of the
 *      jack_get_ports() call.
 */

unsigned
midi_out_jack::get_port_count ()
{
    unsigned count = 0;
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (not_nullptr(jackdata->client))
        return 0;

    const char ** ports = jack_get_ports    /* list of available ports */
    (
        jackdata->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
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
midi_out_jack::get_port_name (unsigned portnumber)
{
    std::string result;
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    connect();
    const char ** ports = jack_get_ports        /* List of available ports  */
    (
        jackdata->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    if (is_nullptr(jackdata->port))
        return;

    jack_port_unregister(jackdata->client, jackdata->port);
    jackdata->port = nullptr;
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
    JackMidiData * jackdata = static_cast<JackMidiData *>(m_api_data);
    jack_ringbuffer_write
    (
        jackdata->buffMessage, (const char *) &message[0], message.size()
    );
    jack_ringbuffer_write(jackdata->buffSize, (char *) &nBytes, sizeof(nBytes));
}

}           // namespace seq64

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

