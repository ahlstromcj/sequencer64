/**
 * \file          midi_alsa.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-17
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
 *
 *  The ALSA Sequencer API is based on the use of a callback function for MIDI
 *  input.
 *
 *  Thanks to Pedro Lopez-Cabanillas for help with the ALSA sequencer time
 *  stamps and other assorted fixes!!!
 *
 *  If you don't need timestamping for incoming MIDI events, define the
 *  preprocessor definition SEQ64_AVOID_TIMESTAMPING to save resources
 *  associated with the ALSA sequencer queues.
 */

#include <sstream>                      /* std::ostringstream               */
#include <pthread.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>

#include "calculations.hpp"             /* beats_per_minute_from_tempo_us() */
#include "midi_alsa.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

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
    pthread_t m_thread;
    pthread_t dummy_thread_id;
    unsigned long long lastTime;
    int queue_id;                // input queue needed to get timestamped events
    int trigger_fds[2];

    alsa_midi_data_t (snd_seq_t * s);

};

/**
 *  Convenient constructor to make sure every member is assigned to a
 *  non-random value :-D.
 */

alsa_midi_data_t::alsa_midi_data_t (snd_seq_t * s)
 :
    seq                 (s),
    portNum             (-1),
    vport               (-1),
    subscription        (nullptr),
    coder               (nullptr),
    bufferSize          (0),        // or 32 for output
    buffer              (nullptr),
    m_thread            (),         // or alsadata->dummy_thread_id for input
    dummy_thread_id     (),         // or pthread_self() for input
    lastTime            (0LL),
    queue_id            (-1),       // filled in later
    trigger_fds         ()
{
    trigger_fds[0] = -1;            // for input setup
    trigger_fds[1] = -1;            // for input setup
}

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
    rtmidi_in_data * rtindata = static_cast<rtmidi_in_data *>(ptr);
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>
    (
        rtindata->api_data()
    );
    long nBytes;
    unsigned long long time, lasttime;
    bool continueSysex = false;
    bool doDecode = false;
    midi_message message;
    int poll_fd_count;
    struct pollfd * poll_fds;
    snd_seq_event_t * ev;
    int result;
    alsadata->bufferSize = 32;
    result = snd_midi_event_new(0, &alsadata->coder);
    if (result < 0)
    {
        rtindata->do_input(false);
        errprintfunc("error initializing MIDI event parser");
        return nullptr;
    }

    midibyte * buffer = (midibyte *) malloc(alsadata->bufferSize);
    if (is_nullptr(buffer))
    {
        rtindata->do_input(false);
        snd_midi_event_free(alsadata->coder);
        alsadata->coder = 0;
        errprintfunc("error initializing buffer memory");
        return nullptr;
    }
    snd_midi_event_init(alsadata->coder);
    snd_midi_event_no_status(alsadata->coder, 1); // suppress running status

    /*
     * Can this return value ever be zero?
     */

    poll_fd_count = snd_seq_poll_descriptors_count(alsadata->seq, POLLIN) + 1;
    poll_fds = (struct pollfd *) alloca(poll_fd_count * sizeof(struct pollfd));
    snd_seq_poll_descriptors
    (
        alsadata->seq, poll_fds + 1, poll_fd_count - 1, POLLIN
    );
    poll_fds[0].fd = alsadata->trigger_fds[0];
    poll_fds[0].events = POLLIN;
    while (rtindata->do_input())
    {
        if (snd_seq_event_input_pending(alsadata->seq, 1) == 0)
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

        result = snd_seq_event_input(alsadata->seq, &ev);
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
            if (! rtindata->test_ignore_flags(0x02))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_TICK:    // 0xF9 ... MIDI timing tick
            if (! rtindata->test_ignore_flags(0x02))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_CLOCK:   // 0xF8 ... MIDI timing (clock) tick
            if (! rtindata->test_ignore_flags(0x02))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_SENSING: // Active sensing
            if (! rtindata->test_ignore_flags(0x04))
                doDecode = true;
            break;

        case SND_SEQ_EVENT_SYSEX:
            if (rtindata->test_ignore_flags(0x01))
                break;

            if (ev->data.ext.len > alsadata->bufferSize)
            {
                alsadata->bufferSize = ev->data.ext.len;
                free(buffer);
                buffer = (midibyte *) malloc(alsadata->bufferSize);
                if (buffer == NULL)
                {
                    rtindata->do_input(false);
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
                alsadata->coder, buffer, alsadata->bufferSize, ev
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
                    /*
                     * Calculate the time stamp.
                     * Method 1: Use the system time:
                     *
                     * ()gettimeofday(&tv, (struct timezone *)NULL);
                     * time = (tv.tv_sec * 1000000) + tv.tv_usec;
                     *
                     * Method 2: Use the ALSA sequencer event time data.
                     * (Thanks to Pedro Lopez-Cabanillas!)
                     */

                    message.timeStamp = 0.0;
                    time = (ev->time.time.tv_sec * 1000000) +
                        (ev->time.time.tv_nsec / 1000);

                    lasttime = time;
                    time -= alsadata->lastTime;
                    alsadata->lastTime = lasttime;
                    if (rtindata->first_message())
                        rtindata->first_message(false);
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

        if (rtindata->using_callback())
        {
            rtmidi_callback_t callback = rtindata->user_callback();
            callback(message.timeStamp, message.bytes, rtindata->user_data());
        }
        else
        {
            /*
             * As long as we haven't reached our queue size limit, push the
             * message.
             */

            (void) rtindata->queue().add(message);
        }
    }

    if (not_nullptr(buffer))
        free(buffer);

    snd_midi_event_free(alsadata->coder);
    alsadata->coder = 0;
    alsadata->m_thread = alsadata->dummy_thread_id;
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
    unsigned queuesize,
    int ppqn,
    int bpm
) :
    midi_in_api (queuesize, ppqn, bpm)
{
    initialize(clientname);             // is this a virtual function!!!???
}

/**
 *  Destructor.  Closes a connection if it exists, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_in_alsa::~midi_in_alsa ()
{
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    close_port();
    if (m_input_data.do_input())
    {
        bool f = false;
        m_input_data.do_input(false);       /* shut down the input thread   */
        (void) write(alsadata->trigger_fds[1], &f, sizeof(f));
        if (! pthread_equal(alsadata->m_thread, alsadata->dummy_thread_id))
            pthread_join(alsadata->m_thread, NULL);
    }

    if (alsadata->trigger_fds[0] != (-1))
    {
        close(alsadata->trigger_fds[0]);
        close(alsadata->trigger_fds[1]);
    }
    if (alsadata->vport >= 0)
        snd_seq_delete_port(alsadata->seq, alsadata->vport);

#ifndef SEQ64_AVOID_TIMESTAMPING
    snd_seq_free_queue(alsadata->seq, alsadata->queue_id);
#endif

    snd_seq_close(alsadata->seq);
    delete alsadata;
}

/**
 *  Initializes the ALSA input object.  The first item opened is the snd_seq_t
 *  "handle".  An alsa_midi_data_t object is allocated on the heap, and it is
 *  then stored in the m_api_data member <i>and</i> the API pointer in the
 *  m_input_data member.  The "handle" is stored in this member as well.
 *
 * \param clientname
 *      The client name to be applied to the object for visibility.
 */

void
midi_in_alsa::initialize (const std::string & clientname)
{
    snd_seq_t * seq;                        /* stored in alsa_midi_data_t   */
    int result = snd_seq_open               /* set up ALSA sequencer client */
    (
        &seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK
    );
    if (result < 0)
    {
        m_error_string = func_message("error opening ALSA sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
    else
    {
        alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>
        (
            new (std::nothrow) alsa_midi_data_t(seq)
        );

        /*
         * Same as for output up to here.
         */

        if (not_nullptr(alsadata))                  /* save API-specific info   */
        {
            snd_seq_set_client_name(seq, clientname.c_str());
            alsadata->dummy_thread_id = pthread_self();
            alsadata->m_thread = alsadata->dummy_thread_id;
            if (pipe(alsadata->trigger_fds) == -1)
            {
                m_error_string = func_message("error creating pipe objects");
                error(rterror::DRIVER_ERROR, m_error_string);
            }
            else
            {
#ifndef SEQ64_AVOID_TIMESTAMPING
                /*
                 * Create the input queue.  Tempo (mm=100, or 60000.0) and
                 * resolution (240) values, but now we've made them parameters.
                 *
                 * Alternate:
                 *
                 * snd_seq_alloc_queue(seq);
                 */

                int tempous = tempo_us_from_beats_per_minute(bpm());
                alsadata->queue_id = snd_seq_alloc_named_queue
                (
                    seq, "seq64rtmidi queue"
                );

                snd_seq_queue_tempo_t * qtempo;
                snd_seq_queue_tempo_alloca(&qtempo);
                snd_seq_queue_tempo_set_tempo(qtempo, tempous);
                snd_seq_queue_tempo_set_ppq(qtempo, ppqn());
                snd_seq_set_queue_tempo(alsadata->seq, alsadata->queue_id, qtempo);
                snd_seq_drain_output(alsadata->seq);
#endif
                m_api_data = alsadata;                      /* no cast needed */
                m_input_data.api_data(alsadata);            /* no cast needed */
            }
        }
    }
}

#define SEQ64_ALSA_PORT_CLIENT      0xFF000000
#define SEQ64_ALSA_PORT_COUNT       (-1)

/**
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
 *      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE.  We add a new
 *      value, SEQ64_ALSA_PORT_CLIENT = 0xFF000000, to indicate we only want the
 *      client value.
 *
 * \param portnumber
 *      The port number to look up. If this value is -1, then the ports are
 *      counted.
 *
 * \return
 *      Returns the port count if portnumber is -1, 1 if the port number
 *      queried is portnumber, or 0 if there is no match to portnumber.  If
 *      type is the new value, 0xFF000000, then the client value is returned
 *      without further querying of the ALSA subsystem.
 */

static unsigned
alsa_port_info
(
    snd_seq_t * seq,
    snd_seq_port_info_t * pinfo,
    unsigned type,
    int portnumber
)
{
    int client;
    int count = 0;
    snd_seq_client_info_t * cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        if (client == 0)
        {
            continue;
        }
        else if (type == SEQ64_ALSA_PORT_CLIENT)
        {
            infoprint("Returning client number");
            return client;
        }

        snd_seq_port_info_set_client(pinfo, client);    /* reset query info */
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

    /*
     * If a negative portnumber was used, return the port count.
     */

    if (portnumber < 0)
        return count;

    return 0;
}

/**
 *  Gets the input sequencer port count from ALSA.  Note that this data was
 *  already obtained ahead of time via the new rtmidi_info object, so that
 *  this call is redundant, and eventually we hope to eliminate it.
 *
 * \return
 *      Returns the result of a alsa_port_info() call.
 */

unsigned
midi_in_alsa::get_port_count ()
{
    snd_seq_port_info_t * pinfo;
    snd_seq_port_info_alloca(&pinfo);
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    return alsa_port_info
    (
        alsadata->seq, pinfo,
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SEQ64_ALSA_PORT_COUNT
    );
}

/**
 *  Returns the name of the given port number from ALSA.  Note that this data
 *  was already obtained ahead of time via the new rtmidi_info object, so that
 *  this call is redundant, and eventually we hope to eliminate it.
 *
 * \param portnumber
 *      The port number to query for the port name.
 *
 * \return
 *      Returns the port name reported by ALSA.  If an error occurs, this
 *      value is empty.
 */

std::string
midi_in_alsa::get_port_name (unsigned portnumber)
{
    std::string stringname;
    if (portnumber != SEQ64_BAD_PORT_ID)
    {
        snd_seq_client_info_t * cinfo;
        snd_seq_port_info_t * pinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);
        alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
        if
        (
            alsa_port_info
            (
                alsadata->seq, pinfo,
                SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                int(portnumber)
            )
        )
        {
            int cnum = snd_seq_port_info_get_client(pinfo);
            snd_seq_get_any_client_info(alsadata->seq, cnum, cinfo);

            /*
             * Assemble name with with full portnames added to ensure unique
             * device names.
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
            m_error_string = func_message("error looking for port name");
            error(rterror::WARNING, m_error_string);
        }
    }
    return stringname;
}

/**
 *  Opens an ALSA input port.  This code is a lot like the code for the output
 *  port, and could perhaps be folded into midi_alsa_info.
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
    /* START OF COMMON CODE */

    if (m_connected)
    {
        m_error_string = func_message("a valid connection already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }
    if (get_port_count() < 1)
    {
        m_error_string = func_message("no MIDI input sources found");
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }

    if (portnumber != SEQ64_BAD_PORT_ID)
    {
        snd_seq_port_info_t * src_pinfo;
        snd_seq_port_info_alloca(&src_pinfo);
        alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
        if
        (
            alsa_port_info
            (
                alsadata->seq, src_pinfo,
                SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, // vs WRITE
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

        /* END OF COMMON CODE */

        /*
         * We need to determine what the heck is a sender client versus receiver
         * client.  For example, on our system, the sender client is 14 (for the
         * Midi Through device), while the receiver client is 129 (which is not
         * found by the rtmidi_info object.
         */

        snd_seq_addr_t sender;
        sender.client = snd_seq_port_info_get_client(src_pinfo);
        sender.port = snd_seq_port_info_get_port(src_pinfo);

        snd_seq_addr_t receiver;
        receiver.client = snd_seq_client_id(alsadata->seq);

        snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        if (alsadata->vport < 0)
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

#ifndef SEQ64_AVOID_TIMESTAMPING
            snd_seq_port_info_set_timestamping(pinfo, 1);
            snd_seq_port_info_set_timestamp_real(pinfo, 1);
            snd_seq_port_info_set_timestamp_queue(pinfo, alsadata->queue_id);
#endif

            snd_seq_port_info_set_name(pinfo,  portname.c_str());
            alsadata->vport = snd_seq_create_port(alsadata->seq, pinfo);
            if (alsadata->vport < 0)
            {
                m_error_string = func_message("ALSA error creating input port");
                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
            alsadata->vport = snd_seq_port_info_get_port(pinfo);
        }

        receiver.port = alsadata->vport;
        if (! alsadata->subscription)
        {
            /*
             * Make a port subscription.
             */

            if (snd_seq_port_subscribe_malloc(&alsadata->subscription) < 0)
            {
                m_error_string = func_message("ALSA error port subscription");
                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
            snd_seq_port_subscribe_set_sender(alsadata->subscription, &sender);
            snd_seq_port_subscribe_set_dest(alsadata->subscription, &receiver);
            if (snd_seq_subscribe_port(alsadata->seq, alsadata->subscription))
            {
                snd_seq_port_subscribe_free(alsadata->subscription);
                alsadata->subscription = 0;
                m_error_string = func_message("ALSA error making port connection");
                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
        }

        if (! m_input_data.do_input())
        {
            // Start the input queue

#ifndef SEQ64_AVOID_TIMESTAMPING
            snd_seq_start_queue(alsadata->seq, alsadata->queue_id, NULL);
            snd_seq_drain_output(alsadata->seq);
#endif

            // Start our MIDI input thread.

            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
            m_input_data.do_input(true);
            int err = pthread_create
            (
                &alsadata->m_thread, &attr, alsa_midi_handler, &m_input_data
            );
            pthread_attr_destroy(&attr);
            if (err)
            {
                snd_seq_unsubscribe_port(alsadata->seq, alsadata->subscription);
                snd_seq_port_subscribe_free(alsadata->subscription);
                alsadata->subscription = 0;
                m_input_data.do_input(false);
                m_error_string = func_message("error starting MIDI input thread");
                error(rterror::THREAD_ERROR, m_error_string);
                return;
            }
        }
        m_connected = true;
    }
    else
    {
        std::ostringstream ost;
        ost << func_message("'portnumber' ") << portnumber << " is invalid";
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
    }
}

/**
 *  Opens a virtual ALSA input port.  This is an application-provided port
 *  to which other ALSA elements can connect.  It is similar to seq24's
 *  "manual ALSA ports" concept.
 *
 * \param portname
 *      The name of the virtual port to open.
 */

void
midi_in_alsa::open_virtual_port (const std::string & portname)
{
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    if (alsadata->vport < 0)
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

#ifndef SEQ64_AVOID_TIMESTAMPING
        snd_seq_port_info_set_timestamping(pinfo, 1);
        snd_seq_port_info_set_timestamp_real(pinfo, 1);
        snd_seq_port_info_set_timestamp_queue(pinfo, alsadata->queue_id);
#endif

        snd_seq_port_info_set_name(pinfo, portname.c_str());
        alsadata->vport = snd_seq_create_port(alsadata->seq, pinfo);
        if (alsadata->vport < 0)
        {
            m_error_string = func_message("ALSA error creating virtual port");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
        alsadata->vport = snd_seq_port_info_get_port(pinfo);
    }

    if (! m_input_data.do_input())
    {
        /*
         * Wait for old thread to stop, if still running.  Then start the
         * input queue if SEQ64_AVOID_TIMESTAMPING is not defined.  Then
         * start the MIDI input thread.
         */

        if (! pthread_equal(alsadata->m_thread, alsadata->dummy_thread_id))
            pthread_join(alsadata->m_thread, NULL);

#ifndef SEQ64_AVOID_TIMESTAMPING
        snd_seq_start_queue(alsadata->seq, alsadata->queue_id, NULL);
        snd_seq_drain_output(alsadata->seq);
#endif

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        m_input_data.do_input(true);
        int err = pthread_create
        (
            &alsadata->m_thread, &attr, alsa_midi_handler, &m_input_data
        );
        pthread_attr_destroy(&attr);
        if (err)
        {
            if (alsadata->subscription)
            {
                snd_seq_unsubscribe_port(alsadata->seq, alsadata->subscription);
                snd_seq_port_subscribe_free(alsadata->subscription);
                alsadata->subscription = 0;
            }
            m_input_data.do_input(false);
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
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    if (m_connected)
    {
        if (alsadata->subscription)
        {
            snd_seq_unsubscribe_port(alsadata->seq, alsadata->subscription);
            snd_seq_port_subscribe_free(alsadata->subscription);
            alsadata->subscription = 0;
        }

        /*
         * Stop the input queue if SEQ64_AVOID_TIMESTAMPING is not defined.
         */

#ifndef SEQ64_AVOID_TIMESTAMPING
        snd_seq_stop_queue(alsadata->seq, alsadata->queue_id, NULL);
        snd_seq_drain_output(alsadata->seq);
#endif
        m_connected = false;
    }

    /*
     * Then stop the thread to avoid triggering the callback, while the port
     * is intended to be closed.
     */

    if (m_input_data.do_input())
    {
        bool f = false;
        m_input_data.do_input(false);
        int res = write(alsadata->trigger_fds[1], &f, sizeof(f));

        /*
         * Why did RtMidi do this????
         */

        (void) res;
        if (! pthread_equal(alsadata->m_thread, alsadata->dummy_thread_id))
            pthread_join(alsadata->m_thread, NULL);
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

    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    if (alsadata->vport >= 0)
        snd_seq_delete_port(alsadata->seq, alsadata->vport);

    if (alsadata->coder)
        snd_midi_event_free(alsadata->coder);

    if (alsadata->buffer)
        free(alsadata->buffer);

    snd_seq_close(alsadata->seq);
    delete alsadata;
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
    int result = snd_seq_open
    (
        &seq, "default", SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK
    );
    if (result < 0)
    {
        m_error_string = func_message("error opening ALSA sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
        return;
    }
    snd_seq_set_client_name(seq, clientname.c_str());

    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>
    (
        new (std::nothrow) alsa_midi_data_t(seq)
    );

    /*
     * Same as for input up to here.
     */

    if (not_nullptr(alsadata))                  /* save API-specific info   */
    {
        unsigned buffsize = 32;                 /* set before using :-D     */
        int result = snd_midi_event_new(buffsize, &alsadata->coder);
        if (result < 0)
        {
            delete alsadata;
            m_error_string = func_message("error initializing MIDI event parser");
            error(rterror::DRIVER_ERROR, m_error_string);
        }
        else
        {
            midibyte * b = reinterpret_cast<midibyte *>(malloc(buffsize));
            if (not_nullptr(b))
            {
                alsadata->buffer = b;
                alsadata->bufferSize = 32;
                m_api_data = alsadata;                  /* no cast needed */
                snd_midi_event_init(alsadata->coder);
            }
            else
            {
                delete alsadata;
                m_error_string = func_message("error allocating output memory");
                error(rterror::MEMORY_ERROR, m_error_string);
            }
        }
    }
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
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    return alsa_port_info
    (
        alsadata->seq, pinfo,
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SEQ64_ALSA_PORT_COUNT
    );
}

/**
 *  Provides the ALSA output port name.
 *
 *  THIS IS COMMON CODE!!!!!!
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
    std::string stringname;
    if (portnumber != SEQ64_BAD_PORT_ID)
    {
        snd_seq_client_info_t * cinfo;
        snd_seq_port_info_t * pinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);

        alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
        if
        (
            alsa_port_info
            (
                alsadata->seq, pinfo,
                SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                int(portnumber)
            )
        )
        {
            int cnum = snd_seq_port_info_get_client(pinfo);
            snd_seq_get_any_client_info(alsadata->seq, cnum, cinfo);

            /*
             * Ensure devices are listed with full portnames added to ensure
             * unique device names.
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
            m_error_string = func_message("error looking for port name");
            error(rterror::WARNING, m_error_string);
        }
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
    /* START OF COMMON CODE */

    if (m_connected)
    {
        m_error_string = func_message("a valid connection already exists");
        error(rterror::WARNING, m_error_string);
        return;
    }

    if (get_port_count() < 1)
    {
        m_error_string = func_message("no MIDI output sources found");
        error(rterror::NO_DEVICES_FOUND, m_error_string);
        return;
    }

    if (portnumber != SEQ64_BAD_PORT_ID)
    {
        snd_seq_port_info_t * pinfo;
        snd_seq_port_info_alloca(&pinfo);
        alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
        if
        (
            alsa_port_info
            (
                alsadata->seq, pinfo,
                SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, // vs READ
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

        /* END OF COMMON CODE */

        snd_seq_addr_t receiver;
        receiver.client = snd_seq_port_info_get_client(pinfo);
        receiver.port = snd_seq_port_info_get_port(pinfo);

        snd_seq_addr_t sender;
        sender.client = snd_seq_client_id(alsadata->seq);
        if (alsadata->vport < 0)
        {
            /*
             * The "legacy" midibus::init_out() replaces the
             * SND_SEQ_PORT_CAP_SUBS_READ flag with
             * SND_SEQ_PORT_CAP_NO_EXPORT.  This code here is like the seq24's
             * midibus::init_out_sub() function.  Thus, alsadata->vport here
             * is like m_local_addr_port there.
             */

            alsadata->vport = snd_seq_create_simple_port
            (
                alsadata->seq, portname.c_str(),
                SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
            );
            if (alsadata->vport < 0)
            {
                m_error_string = func_message("ALSA error creating output port");
                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
        }
        sender.port = alsadata->vport;
        if (! alsadata->subscription)       // NEW CONDITION, VALID???
        {
            /*
             * Make subscription.  The midibus::init_out() code instead
             * used snd_seq_connect_to(port).
             */

            if (snd_seq_port_subscribe_malloc(&alsadata->subscription) < 0)
            {
                snd_seq_port_subscribe_free(alsadata->subscription);
                m_error_string = func_message("ALSA error port subscription");
                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
            snd_seq_port_subscribe_set_sender(alsadata->subscription, &sender);
            snd_seq_port_subscribe_set_dest(alsadata->subscription, &receiver);
            snd_seq_port_subscribe_set_time_update(alsadata->subscription, 1);
            snd_seq_port_subscribe_set_time_real(alsadata->subscription, 1);
            if (snd_seq_subscribe_port(alsadata->seq, alsadata->subscription))
            {
                snd_seq_port_subscribe_free(alsadata->subscription);
                m_error_string = func_message("ALSA error making port connection");
                error(rterror::DRIVER_ERROR, m_error_string);
                return;
            }
        }
        m_connected = true;
    }
    else
    {
        std::ostringstream ost;
        ost << func_message("'portnumber' ") << portnumber << " is invalid";
        m_error_string = ost.str();
        error(rterror::INVALID_PARAMETER, m_error_string);
    }
}

/**
 *  Closes the ALSA output port.
 */

void
midi_out_alsa::close_port ()
{
    if (m_connected)
    {
        alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
        snd_seq_unsubscribe_port(alsadata->seq, alsadata->subscription);
        snd_seq_port_subscribe_free(alsadata->subscription);
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
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    if (alsadata->vport < 0)
    {
        alsadata->vport = snd_seq_create_simple_port    /* m_local_addr_port    */
        (
            alsadata->seq, portname.c_str(),            /* portname, bus name   */
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
        );
        if (alsadata->vport < 0)
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
    alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    unsigned nBytes = message.size();
    if (nBytes > alsadata->bufferSize)
    {
        alsadata->bufferSize = nBytes;
        result = snd_midi_event_resize_buffer(alsadata->coder, nBytes);
        if (result != 0)
        {
            m_error_string = func_message("ALSA error resizing MIDI event");
            error(rterror::DRIVER_ERROR, m_error_string);
            return;
        }
        free(alsadata->buffer);
        alsadata->buffer = (midibyte *) malloc(alsadata->bufferSize);
        if (alsadata->buffer == NULL)
        {
            m_error_string = func_message("error allocating buffer memory");
            error(rterror::MEMORY_ERROR, m_error_string);
            return;
        }
    }

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, alsadata->vport);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    for (unsigned i = 0; i < nBytes; ++i)
        alsadata->buffer[i] = message.at(i);

    result = snd_midi_event_encode
    (
        alsadata->coder, alsadata->buffer, long(nBytes), &ev
    );
    if (result < int(nBytes))
    {
        m_error_string = func_message("event parsing error");
        error(rterror::WARNING, m_error_string);
        return;
    }
    result = snd_seq_event_output(alsadata->seq, &ev); // Send the event.
    if (result < 0)
    {
        m_error_string = func_message("error sending MIDI message to port");
        error(rterror::WARNING, m_error_string);
        return;
    }
    snd_seq_drain_output(alsadata->seq);
}

}           // namespace seq64

/*
 * midi_alsa.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

