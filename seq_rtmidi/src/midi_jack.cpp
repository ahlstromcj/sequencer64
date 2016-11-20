/**
 * \file          midi_jack.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-18
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
 */

struct JackMidiData
{
    jack_client_t * client;
    jack_port_t * port;
    jack_ringbuffer_t * buffSize;
    jack_ringbuffer_t * buffMessage;
    jack_time_t lastTime;
    midi_in_api::rtmidi_in_data * rtMidiIn;
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
jackProcessIn (jack_nframes_t nframes, void *arg)
{
    JackMidiData * jData = (JackMidiData *) arg;
    midi_in_api::rtmidi_in_data * rtData = jData->rtMidiIn;
    jack_midi_event_t event;
    jack_time_t time;
    if (jData->port == NULL)                 /* is port created?        */
       return 0;

    void * buff = jack_port_get_buffer(jData->port, nframes);

    /*
     * We have MIDI events in the buffer.
     */

    int evCount = jack_midi_get_event_count(buff);
    for (int j = 0; j < evCount; ++j)
    {
        midi_in_api::midi_message message;
        message.bytes.clear();
        jack_midi_event_get(&event, buff, j);
        for (unsigned i = 0; i < event.size; ++i)
            message.bytes.push_back(event.buffer[i]);

        time = jack_get_time();              /* compute the delta time  */
        if (rtData->firstMessage == true)
            rtData->firstMessage = false;
        else
            message.timeStamp = (time - jData->lastTime) * 0.000001;

        jData->lastTime = time;
        if (! rtData->continueSysex)
        {
            if (rtData->usingCallback)
            {
                rtmidi_in::rtmidi_callback_t callback =
                    (rtmidi_in::rtmidi_callback_t) rtData->userCallback;

                callback(message.timeStamp, &message.bytes, rtData->userdata);
            }
            else
            {
                /*
                 * As long as we haven't reached our queue size limit, push
                 * the message.
                 */

                if (rtData->queue.size < rtData->queue.ringSize)
                {
                    rtData->queue.ring[rtData->queue.back++] = message;
                    if (rtData->queue.back == rtData->queue.ringSize)
                        rtData->queue.back = 0;

                    rtData->queue.size++;
                }
                else
                    errprintfunc("message queue limit reached");
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
    midi_in_api(queuesize)
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
    JackMidiData *data = new JackMidiData;
    m_api_data = (void *) data;
    data->rtMidiIn = &m_input_data;
    data->port = NULL;
    data->client = NULL;
    this->clientname = clientname;
    connect();
}

/**
 *  Connects the MIDI input port.  The following calls are made:
 *
 *      -   jack_client_open(), to initialize JACK client
 *      -   jack_set_process_callback(), to set jackProcessIn()
 *      -   jack_activate()
 *
 *  If the JackMidiData client member is already set, this function returns
 *  immediately.
 */

void
midi_in_jack::connect ()
{
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    if (data->client)
        return;

    data->client = jack_client_open(clientname.c_str(), JackNoStartServer, NULL);
    if (is_nullptr(data->client))
    {
        m_error_string = "midi_in_jack::initialize(): JACK server not running?";
        error(rterror::WARNING, m_error_string);
        return;
    }
    jack_set_process_callback(data->client, jackProcessIn, data);
    jack_activate(data->client);
}

/**
 *  Destructor.  Closes the port, closes the JACK client, and cleans up the
 *  API data structure.
 */

midi_in_jack::~midi_in_jack()
{
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    close_port();
    if (data->client)
        jack_client_close(data->client);

    delete data;
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(data->port))
    {
        data->port = jack_port_register
        (
            data->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
        );
    }

    if (is_nullptr(data->port))
    {
        m_error_string = "midi_in_jack::open_port(): JACK error creating port";
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    std::string name = get_port_name(portnumber);
    jack_connect(data->client, name.c_str(), jack_port_name(data->port));
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

void midi_in_jack::open_virtual_port(const std::string portname)
{
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(data->port))
    {
        data->port = jack_port_register
        (
            data->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
        );
    }
    if (is_nullptr(data->port))
    {
        m_error_string = "midi_in_jack::open_virtual_port(): "
                         "JACK error creating virtual port";

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
    int count = 0;
    JackMidiData *data = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (!data->client)
        return 0;

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        data->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    std::string result;
    connect();

    const char ** ports = jack_get_ports        /* list of available ports  */
    (
        data->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput
    );
    if (is_nullptr(ports))                      /* Check port validity      */
    {
        m_error_string = "midi_in_jack::get_port_name(): no ports available!";
        error(rterror::WARNING, m_error_string);
        return result;
    }

    if (is_nullptr(ports[portnumber]))
    {
        std::ostringstream ost;
        ost
            << "midi_in_jack::get_port_name(): "
               "the 'portnumber' argument ("
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
 *  Closes the MIDI input port by calling jack_port_unregister() and
 *  nullifying the port pointer.
 */

void
midi_in_jack::close_port ()
{
    JackMidiData *data = static_cast<JackMidiData *>(m_api_data);
    if (is_nullptr(data->port))
        return;

    jack_port_unregister(data->client, data->port);
    data->port = nullptr;
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
    JackMidiData * data = (JackMidiData *) arg;

    if (is_nullptr(data->port))                     /* is port created? */
        return 0;

    void * buff = jack_port_get_buffer(data->port, nframes);
    jack_midi_data_t * mididata;
    int space;
    jack_midi_clear_buffer(buff);
    while (jack_ringbuffer_read_space(data->buffSize) > 0)
    {
        jack_ringbuffer_read
        (
            data->buffSize, (char *) &space, (size_t) sizeof(space)
        );
        mididata = jack_midi_event_reserve(buff, 0, space);
        jack_ringbuffer_read(data->buffMessage, (char *) mididata, size_t(space));
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
    midi_out_api ()
{
    initialize(clientname);
}

/**
 *  The destructor closes the port and cleans out the API data structure.
 */

midi_out_jack::~midi_out_jack ()
{
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    close_port();
    jack_ringbuffer_free(data->buffSize);
    jack_ringbuffer_free(data->buffMessage);
    if (data->client)
        jack_client_close(data->client);

    delete data;
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
    JackMidiData * data = new JackMidiData;
    m_api_data = (void *) data;
    data->port = nullptr;
    data->client = nullptr;
    this->clientname = clientname;
    connect();
}

/**
 *  Connects the MIDI output port.  The following calls are made:
 *
 *      -   jack_ringbuffer_create(), called twice
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    if (not_nullptr(data->client))
        return;

    // Initialize output ringbuffers

    data->buffSize = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);
    data->buffMessage = jack_ringbuffer_create(JACK_RINGBUFFER_SIZE);

    // Initialize JACK client

    data->client = jack_client_open(clientname.c_str(), JackNoStartServer, NULL);
    if (is_nullptr(data->client))
    {
        m_error_string = "midi_out_jack::initialize(): JACK server not running?";
        error(rterror::WARNING, m_error_string);
        return;
    }

    jack_set_process_callback(data->client, jackProcessOut, data);
    jack_activate(data->client);
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(data->port))
    {
        data->port = jack_port_register
        (
            data->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
        );
    }

    if (is_nullptr(data->port))
    {
        m_error_string = "midi_out_jack::open_port(): JACK error creating port";
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    std::string name = get_port_name(portnumber);   /* connecting to output */
    jack_connect(data->client, jack_port_name(data->port), name.c_str());
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (is_nullptr(data->port))
    {
        data->port = jack_port_register
        (
            data->client, portname.c_str(),
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0
        );
    }

    if (is_nullptr(data->port))
    {
        m_error_string = "midi_out_jack::open_virtual_port(): "
                         "JACK error creating virtual port";

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
    int count = 0;
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    connect();
    if (not_nullptr(data->client))
        return 0;

    // List of available ports
    const char ** ports = jack_get_ports
    (
        data->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    std::string result;

    connect();
    const char ** ports = jack_get_ports        /* List of available ports  */
    (
        data->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput
    );

    if (is_nullptr(ports))
    {
        m_error_string = "midi_out_jack::get_port_name(): no ports available";
        error(rterror::WARNING, m_error_string);
        return result;
    }

    if (is_nullptr(ports[portnumber]))
    {
        std::ostringstream ost;
        ost
            << "midi_out_jack::get_port_name(): "
               "the 'portnumber' argument ("
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
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    if (is_nullptr(data->port))
        return;

    jack_port_unregister(data->client, data->port);
    data->port = nullptr;
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
midi_out_jack::send_message (std::vector<midibyte> * message)
{
    int nBytes = message->size();
    JackMidiData * data = static_cast<JackMidiData *>(m_api_data);
    jack_ringbuffer_write
    (
        data->buffMessage, (const char *) &(*message)[0], message->size()
    );
    jack_ringbuffer_write(data->buffSize, (char *) &nBytes, sizeof(nBytes));
}

}           // namespace seq64

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

