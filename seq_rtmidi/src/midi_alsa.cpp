/**
 * \file          midi_alsa.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-01
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  In this refactoring, we are trying to improve the RtMidi project in the
 *  following ways:
 *
 *      -   Make it more readable.
 *      -   Partition the code into separate modules.
 *      -   Remove the usage of virtual functions in constructors.
 *      -   Fix bugs found while implementing a native JACK version of
 *          Sequencer64 (Seq64Jack).
 *
 *  API information found at:
 *
 *      - http://www.alsa-project.org/documentation.php#Library
 */

#include <sstream>

#include "midi_alsa.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 * The ALSA Sequencer API is based on the use of a callback function for MIDI
 * input.
 *
 * Thanks to Pedro Lopez-Cabanillas for help with the ALSA sequencer time
 * stamps and other assorted fixes!!!
 *
 * If you don't need timestamping for incoming MIDI events, define the
 * preprocessor definition AVOID_TIMESTAMPING to save resources associated
 * with the ALSA sequencer queues.
 */

#include <pthread.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>

/**
 *  An internal structure to hold variables related to the ALSA API
 *  implementation.
 */

struct alsa_midi_data_t
{
    snd_seq_t * seq;
    unsigned portNum;
    int vport;
    snd_seq_port_subscribe_t * subscription;
    snd_midi_event_t * coder;
    unsigned bufferSize;
    midibyte * buffer;
    pthread_t thread;
    pthread_t dummy_thread_id;
    unsigned long long lastTime;
    int queue_id;                // input queue needed to get timestamped events
    int trigger_fds[2];
};

/*
 * Doesn't seem to be used.
 *
 * #define PORT_TYPE(pinfo, bits) \
 *  ((snd_seq_port_info_get_capability(pinfo) & (bits)) == (bits))
 */

/**
 *  Provides the ALSA MIDI callback.
 *
 * \param ptr
 *      Provides, we hope, the pointer to the rtmidi_in_data
 *      object.
 *
 * \return
 *      Always returns the null pointer.
 */

static void *
alsa_midi_handler (void * ptr)
{
    rtmidi_in_data * data = static_cast<rtmidi_in_data *>(ptr);
    alsa_midi_data_t * apiData = static_cast<alsa_midi_data_t *>(data->apiData);
    long nBytes;
    unsigned long long time, lastTime;
    bool continueSysex = false;
    bool doDecode = false;
    midi_message message;
    int poll_fd_count;
    struct pollfd * poll_fds;
    snd_seq_event_t * ev;
    int result;
    apiData->bufferSize = 32;
    result = snd_midi_event_new(0, &apiData->coder);
    if (result < 0)
    {
        data->doInput = false;
        errprintfunc("error initializing MIDI event parser");
        return nullptr;
    }

    midibyte * buffer = (midibyte *) malloc(apiData->bufferSize);
    if (is_nullptr(buffer))
    {
        data->doInput = false;
        snd_midi_event_free(apiData->coder);
        apiData->coder = 0;
        errprintfunc("error initializing buffer memory");
        return nullptr;
    }
    snd_midi_event_init(apiData->coder);
    snd_midi_event_no_status(apiData->coder, 1); // suppress running status

    /*
     * Can this return value ever be zero?
     */

    poll_fd_count = snd_seq_poll_descriptors_count(apiData->seq, POLLIN) + 1;
    poll_fds = (struct pollfd *) alloca(poll_fd_count * sizeof(struct pollfd));
    snd_seq_poll_descriptors
    (
        apiData->seq, poll_fds + 1, poll_fd_count - 1, POLLIN
    );
    poll_fds[0].fd = apiData->trigger_fds[0];
    poll_fds[0].events = POLLIN;
    while (data->doInput)
    {
        if (snd_seq_event_input_pending(apiData->seq, 1) == 0)
        {
            if (poll(poll_fds, poll_fd_count, -1) >= 0) // no data pending
            {
                if (poll_fds[0].revents & POLLIN)
                {
                    bool dummy;
                    int res = read(poll_fds[0].fd, &dummy, sizeof(dummy));

                    /*
                     * Why did RtMidi do this????
                     */

                    (void) res;
                }
            }
            continue;
        }

        // If here, there should be data.

        result = snd_seq_event_input(apiData->seq, &ev);
        if (result == -ENOSPC)
        {
            errprintfunc("MIDI input buffer overrun");
            continue;
        }
        else if (result <= 0)
        {
            errprintfunc("unknown MIDI input error");
            perror("System reports");
            continue;
        }

        // This is a bit weird, but we now have to decode an ALSA MIDI
        // event (back) into MIDI bytes.  We'll ignore non-MIDI types.

        if (! continueSysex)
            message.bytes.clear();

        doDecode = false;
        switch (ev->type)
        {
        case SND_SEQ_EVENT_PORT_SUBSCRIBED:     /* port connection made     */
#ifdef SEQ64_USE_DEBUG_OUTPUT
            printf("port connection made\n");
#endif
            break;

        case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:   /* port connection closed   */
#ifdef SEQ64_USE_DEBUG_OUTPUT
            printf
            (
                "port connection closed: sender = %d:%d, dest = %d:%d\n"
                int(ev->data.connect.sender.client),
                int(ev->data.connect.sender.port),
                int(ev->data.connect.dest.client),
                int(ev->data.connect.dest.port)
            );
#endif
            break;

        case SND_SEQ_EVENT_QFRAME:  // MIDI time code
            if (!(data->ignoreFlags & 0x02))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_TICK:    // 0xF9 ... MIDI timing tick
            if (!(data->ignoreFlags & 0x02))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_CLOCK:   // 0xF8 ... MIDI timing (clock) tick
            if (!(data->ignoreFlags & 0x02))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_SENSING: // Active sensing
            if (!(data->ignoreFlags & 0x04))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_SYSEX:
            if ((data->ignoreFlags & 0x01))
                break;

            if (ev->data.ext.len > apiData->bufferSize)
            {
                apiData->bufferSize = ev->data.ext.len;
                free(buffer);
                buffer = (midibyte *) malloc(apiData->bufferSize);
                if (buffer == NULL)
                {
                    data->doInput = false;
                    errprintfunc("error resizing buffer memory");
                    break;
                }
            }

        default:
            doDecode = true;
        }

        if (doDecode)
        {
            nBytes = snd_midi_event_decode
            (
                apiData->coder, buffer, apiData->bufferSize, ev
            );
            if (nBytes > 0)
            {
                /*
                 * ALSA sequencer has a maximum buffer size for MIDI SysEx
                 * events of 256 bytes.  If a device sends SysEx messages
                 * larger than this, they are segmented into 256 byte chunks.
                 * So, we watch for this and concatenate SysEx chunks into a
                 * single SysEx message if necessary.
                 */

                if (! continueSysex)
                {
                    message.bytes.assign(buffer, &buffer[nBytes]);
                }
                else
                {
                    message.bytes.insert
                    (
                        message.bytes.end(), buffer, &buffer[nBytes]
                    );
                }

                continueSysex =
                (
                    (ev->type == SND_SEQ_EVENT_SYSEX) &&
                    (message.bytes.back() != 0xF7)
                );
                if (! continueSysex)
                {
                    message.timeStamp = 0.0;

                    // Calculate the time stamp:
                    //
                    // Method 1: Use the system time.
                    //
                    // ()gettimeofday(&tv, (struct timezone *)NULL);
                    // time = (tv.tv_sec * 1000000) + tv.tv_usec;
                    //
                    // Method 2: Use the ALSA sequencer event time data.
                    //
                    // (thanks to Pedro Lopez-Cabanillas!).

                    time = (ev->time.time.tv_sec * 1000000) +
                        (ev->time.time.tv_nsec / 1000);

                    lastTime = time;
                    time -= apiData->lastTime;
                    apiData->lastTime = lastTime;
                    if (data->firstMessage == true)
                        data->firstMessage = false;
                    else
                        message.timeStamp = time * 0.000001;
                }
                else
                {
#ifdef SEQ64_USE_DEBUG_OUTPUT
                    errprintfunc("event parsing error");
#endif
                }
            }
        }

        snd_seq_free_event(ev);
        if (message.bytes.size() == 0 || continueSysex)
            continue;

        if (data->usingCallback)
        {
            rtmidi_callback_t callback = data->userCallback;
            callback(message.timeStamp, message.bytes, data->userdata);
        }
        else
        {
            /*
             * As long as we haven't reached our queue size limit, push the
             * message.
             */

            (void) data->queue.add(message);
        }
    }

    if (not_nullptr(buffer))
        free(buffer);

    snd_midi_event_free(apiData->coder);
    apiData->coder = 0;
    apiData->thread = apiData->dummy_thread_id;
    return nullptr;
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

midi_in_alsa::midi_in_alsa
(
    const std::string & clientname,
    unsigned queuesize
)
 :
    midi_in_api (queuesize)
{
    initialize(clientname);             // is this a virtual function!!!???
}

/**
 *  Destructor.  Closes a connection if it exists, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_in_alsa::~midi_in_alsa()
{
    close_port();

    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if (m_input_data.doInput)
    {
        m_input_data.doInput = false;       /* shut down the input thread   */
        (void) write
        (
            data->trigger_fds[1], &m_input_data.doInput,
            sizeof(m_input_data.doInput)
        );
        if (! pthread_equal(data->thread, data->dummy_thread_id))
            pthread_join(data->thread, NULL);
    }

    close(data->trigger_fds[0]);
    close(data->trigger_fds[1]);
    if (data->vport >= 0)
        snd_seq_delete_port(data->seq, data->vport);

#ifndef AVOID_TIMESTAMPING
    snd_seq_free_queue(data->seq, data->queue_id);
#endif

    snd_seq_close(data->seq);
    delete data;
}

/**
 *  Initializes the ALSA input object.
 *
 * \param clientname
 *      The client name to be applied to the object for visibility.
 */

void
midi_in_alsa::initialize (const std::string & clientname)
{
    // Set up the ALSA sequencer client.

    snd_seq_t * seq;
    int result = snd_seq_open
    (
        &seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK
    );
    if (result < 0)
    {
        m_error_string = func_message("error opening ALSA sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    snd_seq_set_client_name(seq, clientname.c_str());   // set client name

    /*
     * m_queue = snd_seq_alloc_queue(m_alsa_seq);
     */

    // Save our api-specific connection information.

    alsa_midi_data_t * data = (alsa_midi_data_t *) new alsa_midi_data_t;
    if (not_nullptr(data))
    {
        data->seq = seq;
        data->portNum = -1;
        data->vport = -1;
        data->subscription = 0;
        data->dummy_thread_id = pthread_self();
        data->thread = data->dummy_thread_id;
        data->trigger_fds[0] = -1;
        data->trigger_fds[1] = -1;
        m_api_data = (void *) data;
        m_input_data.apiData = (void *) data;
        if (pipe(data->trigger_fds) == -1)
        {
            m_error_string = func_message("error creating pipe objects");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }

        // Create the input queue

#ifndef AVOID_TIMESTAMPING
        data->queue_id = snd_seq_alloc_named_queue(seq, "rtmidi Queue");

        // Set arbitrary tempo (mm=100) and resolution (240)

        snd_seq_queue_tempo_t *qtempo;
        snd_seq_queue_tempo_alloca(&qtempo);
        snd_seq_queue_tempo_set_tempo(qtempo, 600000);
        snd_seq_queue_tempo_set_ppq(qtempo, 240);
        snd_seq_set_queue_tempo(data->seq, data->queue_id, qtempo);
        snd_seq_drain_output(data->seq);
#endif
    }
}

/*
 * This function is used to count or get the pinfo structure for a given port
 * number.
 *
 * \param seq
 *      TO DO.
 *
 * \param pinfo
 *      TO DO.
 *
 * \param type
 *      The type of port to look up.  It is a one of the following masks:
 *      SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, and
 *      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE.
 *
 * \param portnumber
 *      The port number to look up. If this value is -1, then the ports are
 *      counted.
 *
 * \return
 *      Returns the port count, or 0.
 */

static unsigned
alsa_port_info
(
    snd_seq_t * seq, snd_seq_port_info_t * pinfo,
    unsigned type, int portnumber
)
{
    snd_seq_client_info_t * cinfo;
    int client;
    int count = 0;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        if (client == 0)
            continue;

        // Reset query info

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0)
        {
            unsigned atyp = snd_seq_port_info_get_type(pinfo);
            if
            (
                ((atyp & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
                ((atyp & SND_SEQ_PORT_TYPE_SYNTH) == 0)
            )
            {
                continue;
            }

            unsigned caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & type) != type)
                continue;

            if (count == portnumber)
                return 1;

            ++count;
        }
    }

    // If a negative portnumber was used, return the port count.

    if (portnumber < 0)
        return count;

    return 0;
}

/**
 *  Gets the input sequencer port count from ALSA.
 *
 * \return
 *      Returns the result of a alsa_port_info() call.
 */

unsigned
midi_in_alsa::get_port_count ()
{
    snd_seq_port_info_t * pinfo;
    snd_seq_port_info_alloca(&pinfo);
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    return alsa_port_info
    (
        data->seq, pinfo,
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, -1
    );
}

/**
 *  Returns the name of the given port number from ALSA.
 *
 * \param portnumber
 *      The port number to query for the port name.
 *
 * \return
 *      Returns the port name reported by ALSA.
 */

std::string
midi_in_alsa::get_port_name (unsigned portnumber)
{
    snd_seq_client_info_t * cinfo;
    snd_seq_port_info_t * pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    std::string stringname;
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if
    (
        alsa_port_info
        (
            data->seq, pinfo,
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            int(portnumber)
        )
    )
    {
        int cnum = snd_seq_port_info_get_client(pinfo);
        snd_seq_get_any_client_info(data->seq, cnum, cinfo);

        // These lines added to make sure devices are listed
        // with full portnames added to ensure individual device names

        std::ostringstream os;
        os
            << snd_seq_client_info_get_name(cinfo) << " "
            << snd_seq_port_info_get_client(pinfo) << ":"
            << snd_seq_port_info_get_port(pinfo)
            ;

        stringname = os.str();
    }
    else
    {
        // If we get here, we didn't find a match.

        m_error_string = func_message("error looking for port name");
        error(rterror::WARNING, m_error_string);
    }
    return stringname;
}

/**
 *  Opens an ALSA input port.
 *
 * \param portnumber
 *      The port to be opened.
 *
 * \param portname
 *      The desired name for the port.
 */

void
midi_in_alsa::open_port (unsigned portnumber, const std::string & portname)
{
    if (m_connected)
    {
        m_error_string = func_message("a valid connection already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }

    unsigned nsrc = this->get_port_count();
    if (nsrc < 1)
    {
        m_error_string = func_message("no MIDI input sources found");
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }

    snd_seq_port_info_t * src_pinfo;
    snd_seq_port_info_alloca(&src_pinfo);
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if
    (
        alsa_port_info
        (
            data->seq, src_pinfo,
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            int(portnumber)
        ) == 0
    )
    {
        std::ostringstream ost;
        ost
            << func_message("'portnumber' argument (")
            << portnumber << ") is invalid"
        ;
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
        return;
    }

    snd_seq_addr_t sender, receiver;
    sender.client = snd_seq_port_info_get_client(src_pinfo);
    sender.port = snd_seq_port_info_get_port(src_pinfo);
    receiver.client = snd_seq_client_id(data->seq);

    snd_seq_port_info_t * pinfo;
    snd_seq_port_info_alloca(&pinfo);
    if (data->vport < 0)
    {
        snd_seq_port_info_set_client(pinfo, 0);
        snd_seq_port_info_set_port(pinfo, 0);
        snd_seq_port_info_set_capability
        (
            pinfo, SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE
        );
        snd_seq_port_info_set_type
        (
            pinfo, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
        );
        snd_seq_port_info_set_midi_channels(pinfo, 16);

#ifndef AVOID_TIMESTAMPING
        snd_seq_port_info_set_timestamping(pinfo, 1);
        snd_seq_port_info_set_timestamp_real(pinfo, 1);
        snd_seq_port_info_set_timestamp_queue(pinfo, data->queue_id);
#endif

        snd_seq_port_info_set_name(pinfo,  portname.c_str());
        data->vport = snd_seq_create_port(data->seq, pinfo);
        if (data->vport < 0)
        {
            m_error_string = func_message("ALSA error creating input port");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
        data->vport = snd_seq_port_info_get_port(pinfo);
    }

    receiver.port = data->vport;
    if (! data->subscription)
    {
        // Make subscription

        if (snd_seq_port_subscribe_malloc(&data->subscription) < 0)
        {
            m_error_string = func_message("ALSA error port subscription");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
        snd_seq_port_subscribe_set_sender(data->subscription, &sender);
        snd_seq_port_subscribe_set_dest(data->subscription, &receiver);
        if (snd_seq_subscribe_port(data->seq, data->subscription))
        {
            snd_seq_port_subscribe_free(data->subscription);
            data->subscription = 0;
            m_error_string = func_message("ALSA error making port connection");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
    }

    if (m_input_data.doInput == false)
    {
        // Start the input queue

#ifndef AVOID_TIMESTAMPING
        snd_seq_start_queue(data->seq, data->queue_id, NULL);
        snd_seq_drain_output(data->seq);
#endif

        // Start our MIDI input thread.

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        m_input_data.doInput = true;
        int err = pthread_create
        (
            &data->thread, &attr, alsa_midi_handler, &m_input_data
        );
        pthread_attr_destroy(&attr);
        if (err)
        {
            snd_seq_unsubscribe_port(data->seq, data->subscription);
            snd_seq_port_subscribe_free(data->subscription);
            data->subscription = 0;
            m_input_data.doInput = false;
            m_error_string = func_message("error starting MIDI input thread");
            error(rterror::THREAD_ERROR, m_error_string);
            return;
        }
    }
    m_connected = true;
}

/**
 *  Opens a virtual ALSA input port.
 *
 *  Need to learn what that is.
 *
 * \param portname
 *      The name of the virtual port to open.
 */

void
midi_in_alsa::open_virtual_port (const std::string & portname)
{
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if (data->vport < 0)
    {
        snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_capability
        (
            pinfo, SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE
        );
        snd_seq_port_info_set_type
        (
            pinfo, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
        );
        snd_seq_port_info_set_midi_channels(pinfo, 16);

#ifndef AVOID_TIMESTAMPING
        snd_seq_port_info_set_timestamping(pinfo, 1);
        snd_seq_port_info_set_timestamp_real(pinfo, 1);
        snd_seq_port_info_set_timestamp_queue(pinfo, data->queue_id);
#endif

        snd_seq_port_info_set_name(pinfo, portname.c_str());
        data->vport = snd_seq_create_port(data->seq, pinfo);
        if (data->vport < 0)
        {
            m_error_string = func_message("ALSA error creating virtual port");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
        data->vport = snd_seq_port_info_get_port(pinfo);
    }

    if (m_input_data.doInput == false)
    {
        // Wait for old thread to stop, if still running

        if (! pthread_equal(data->thread, data->dummy_thread_id))
            pthread_join(data->thread, NULL);

        // Start the input queue

#ifndef AVOID_TIMESTAMPING
        snd_seq_start_queue(data->seq, data->queue_id, NULL);
        snd_seq_drain_output(data->seq);
#endif

        // Start our MIDI input thread.

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        m_input_data.doInput = true;
        int err = pthread_create
        (
            &data->thread, &attr, alsa_midi_handler, &m_input_data
        );
        pthread_attr_destroy(&attr);
        if (err)
        {
            if (data->subscription)
            {
                snd_seq_unsubscribe_port(data->seq, data->subscription);
                snd_seq_port_subscribe_free(data->subscription);
                data->subscription = 0;
            }
            m_input_data.doInput = false;
            m_error_string = func_message("error starting MIDI input thread");
            error(rterror::THREAD_ERROR, m_error_string);
            return;
        }
    }
}

/**
 *  Closes the ALSA input port.
 */

void
midi_in_alsa::close_port ()
{
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if (m_connected)
    {
        if (data->subscription)
        {
            snd_seq_unsubscribe_port(data->seq, data->subscription);
            snd_seq_port_subscribe_free(data->subscription);
            data->subscription = 0;
        }

        // Stop the input queue

#ifndef AVOID_TIMESTAMPING
        snd_seq_stop_queue(data->seq, data->queue_id, NULL);
        snd_seq_drain_output(data->seq);
#endif
        m_connected = false;
    }

    // Stop thread to avoid triggering the callback, while the port is
    // intended to be closed

    if (m_input_data.doInput)
    {
        m_input_data.doInput = false;
        int res = write
        (
            data->trigger_fds[1],
            &m_input_data.doInput,
            sizeof(m_input_data.doInput)
        );

        /*
         * Why did RtMidi do this????
         */

        (void) res;
        if (! pthread_equal(data->thread, data->dummy_thread_id))
            pthread_join(data->thread, NULL);
    }
}

/**
 *  Constructor.
 */

midi_out_alsa::midi_out_alsa (const std::string & clientname)
 :
    midi_out_api    ()
{
    initialize(clientname);             // is this a virtual function!!!???
}

/**
 *  Destructor and cleanup.
 */

midi_out_alsa::~midi_out_alsa ()
{
    close_port();   // Close a connection if it exists.

    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if (data->vport >= 0)
        snd_seq_delete_port(data->seq, data->vport);

    if (data->coder)
        snd_midi_event_free(data->coder);

    if (data->buffer)
        free(data->buffer);

    snd_seq_close(data->seq);
    delete data;
}

/**
 *  Opens the ALSA output port and sets up the ALSA sequencer client.
 *
 * \param clientname
 *      The name of the output port client.
 */

void
midi_out_alsa::initialize (const std::string & clientname)
{

    snd_seq_t * seq;
    int result1 = snd_seq_open
    (
        &seq, "default", SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK
    );
    if (result1 < 0)
    {
        m_error_string = func_message("error creating ALSA sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }

    snd_seq_set_client_name(seq, clientname.c_str());

    // Save our api-specific connection information.

    alsa_midi_data_t * data = (alsa_midi_data_t *) new alsa_midi_data_t;
    data->seq = seq;
    data->portNum = -1;
    data->vport = -1;
    data->bufferSize = 32;
    data->coder = 0;
    data->buffer = 0;
    int result = snd_midi_event_new(data->bufferSize, &data->coder);
    if (result < 0)
    {
        delete data;
        m_error_string = func_message("error initializing MIDI event parser");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    data->buffer = (midibyte *) malloc(data->bufferSize);
    if (is_nullptr(data->buffer))
    {
        delete data;
        m_error_string = func_message("error allocating buffer memory");
        error(rterror::MEMORY_ERROR, m_error_string);
        return;
    }
    snd_midi_event_init(data->coder);
    m_api_data = (void *) data;
}

/**
 *  Counts the number of ALSA output ports.
 *
 * \return
 *      Returns the number of output ports returned by the alsa_port_info()
 *      function.
 */

unsigned
midi_out_alsa::get_port_count ()
{
    snd_seq_port_info_t * pinfo;
    snd_seq_port_info_alloca(&pinfo);
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    return alsa_port_info
    (
        data->seq, pinfo,
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, -1
    );
}

/**
 *  Provides the ALSA output port name.
 *
 * \param portnumber
 *      Provides the output port number to query.
 *
 * \return
 *      Returns the output ALSA port name.
 */

std::string
midi_out_alsa::get_port_name (unsigned portnumber)
{
    snd_seq_client_info_t * cinfo;
    snd_seq_port_info_t * pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    std::string stringname;
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if
    (
        alsa_port_info
        (
            data->seq, pinfo,
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            int(portnumber)
        )
    )
    {
        int cnum = snd_seq_port_info_get_client(pinfo);
        snd_seq_get_any_client_info(data->seq, cnum, cinfo);

        /*
         * These lines added to make sure devices are listed
         * with full portnames added to ensure individual device names.
         */

        std::ostringstream os;
        os
            << snd_seq_client_info_get_name(cinfo) << " "
            << snd_seq_port_info_get_client(pinfo) << ":"
            << snd_seq_port_info_get_port(pinfo)
            ;
        stringname = os.str();
    }
    else
    {
        // If we get here, we didn't find a match.

        m_error_string = func_message("error looking for port name");
        error(rterror::WARNING, m_error_string);
    }
    return stringname;
}

/**
 *  Opens an ALSA output port.
 *
 *  The highlights are calling snd_seq_port_info_alloca() then calling
 *  snd_seq_create_simple_port(), as done in the "legacy" midibus::init_out().
 *
 * \param portnumber
 *      The port to be opened.
 *
 * \param portname
 *      The desired name for the port.
 */

void
midi_out_alsa::open_port (unsigned portnumber, const std::string & portname)
{
    if (m_connected)
    {
        m_error_string = func_message("a valid connection already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }

    unsigned nsrc = this->get_port_count();
    if (nsrc < 1)
    {
        m_error_string = func_message("no MIDI output sources found");
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }

    snd_seq_port_info_t * pinfo;
    snd_seq_port_info_alloca(&pinfo);
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if
    (
        alsa_port_info
        (
            data->seq, pinfo,
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            int(portnumber)
        ) == 0
    )
    {
        std::ostringstream ost;
        ost << func_message("'portnumber' argument (")
            << portnumber << ") is invalid"
            ;
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
        return;
    }

    snd_seq_addr_t sender, receiver;
    receiver.client = snd_seq_port_info_get_client(pinfo);
    receiver.port = snd_seq_port_info_get_port(pinfo);
    sender.client = snd_seq_client_id(data->seq);
    if (data->vport < 0)
    {
        /*
         * The "legacy" midibus::init_out() replaces the
         * SND_SEQ_PORT_CAP_SUBS_READ flag with SND_SEQ_PORT_CAP_NO_EXPORT.
         * This code here is like the seq24's midibus::init_out_sub()
         * function.  Thus, data->vport here is like m_local_addr_port there.
         */

        data->vport = snd_seq_create_simple_port
        (
            data->seq, portname.c_str(),
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
        );
        if (data->vport < 0)
        {
            m_error_string = func_message("ALSA error creating output port");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
    }

    sender.port = data->vport;

    /*
     * Make subscription.  The midibus::init_out() code instead
     * used snd_seq_connect_to(port).
     */

    if (snd_seq_port_subscribe_malloc(&data->subscription) < 0)
    {
        snd_seq_port_subscribe_free(data->subscription);
        m_error_string = func_message("error allocating port subscription");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    snd_seq_port_subscribe_set_sender(data->subscription, &sender);
    snd_seq_port_subscribe_set_dest(data->subscription, &receiver);
    snd_seq_port_subscribe_set_time_update(data->subscription, 1);
    snd_seq_port_subscribe_set_time_real(data->subscription, 1);
    if (snd_seq_subscribe_port(data->seq, data->subscription))
    {
        snd_seq_port_subscribe_free(data->subscription);
        m_error_string = func_message("ALSA error making port connection");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    m_connected = true;
}

/**
 *  Closes the ALSA output port.
 */

void
midi_out_alsa::close_port ()
{
    if (m_connected)
    {
        alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
        snd_seq_unsubscribe_port(data->seq, data->subscription);
        snd_seq_port_subscribe_free(data->subscription);
        m_connected = false;
    }
}

/**
 *  Opens a virtual ALSA output port.
 *
 *  Need to learn what that is.  Right now, it seems to line up with the
 *  functionality of api_init_out() in the "legacy" midibus module.
 *
 * \param portname
 *      The name of the virtual port to open.
 */

void
midi_out_alsa::open_virtual_port (const std::string & portname)
{
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    if (data->vport < 0)
    {
        data->vport = snd_seq_create_simple_port    /* m_local_addr_port    */
        (
            data->seq, portname.c_str(),            /* portname, bus name   */
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
        );
        if (data->vport < 0)
        {
            m_error_string = func_message("ALSA error creating virtual port");
            error(rterror::DRIVER_ERROR, m_error_string);
        }
    }
}

/**
 *  Sends a ALSA MIDI output message.
 *
 * \param message
 *      Provides the vector of message bytes to send.
 */

void
midi_out_alsa::send_message (const std::vector<midibyte> & message)
{
    int result;
    alsa_midi_data_t * data = static_cast<alsa_midi_data_t *>(m_api_data);
    unsigned nBytes = message.size();
    if (nBytes > data->bufferSize)
    {
        data->bufferSize = nBytes;
        result = snd_midi_event_resize_buffer(data->coder, nBytes);
        if (result != 0)
        {
            m_error_string = func_message("ALSA error resizing MIDI event");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
        free(data->buffer);
        data->buffer = (midibyte *) malloc(data->bufferSize);
        if (data->buffer == NULL)
        {
            m_error_string = func_message("error allocating buffer memory");
            error(rterror::MEMORY_ERROR, m_error_string);
            return;
        }
    }

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, data->vport);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    for (unsigned i = 0; i < nBytes; ++i)
        data->buffer[i] = message.at(i);

    result = snd_midi_event_encode(data->coder, data->buffer, long(nBytes), &ev);
    if (result < int(nBytes))
    {
        m_error_string = func_message("event parsing error");
        error(rterror::WARNING, m_error_string);
        return;
    }
    result = snd_seq_event_output(data->seq, &ev); // Send the event.
    if (result < 0)
    {
        m_error_string = func_message("error sending MIDI message to port");
        error(rterror::WARNING, m_error_string);
        return;
    }
    snd_seq_drain_output(data->seq);
}

}           // namespace seq64

/*
 * midi_alsa.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

