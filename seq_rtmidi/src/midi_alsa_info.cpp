/**
 * \file          midi_alsa_info.cpp
 *
 *    A class for obtaining ALSA information.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2018-06-02
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  API information found at:
 *
 *      - http://www.alsa-project.org/documentation.php#Library
 *
 *  This class is meant to collect a whole bunch of ALSA information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating ALSA midibus objects and midi_alsa API objects.
 *
 *  This was to be a function to create an ALSA "announce" bus.  But it turned
 *  out to be feasible and simpler to add it as a special input port in the
 *  get_all_port_info() function.  Still, the discussion here is useful.
 *
 *  A sequencer core has two pre-defined system ports on the system client
 *  SND_SEQ_CLIENT_SYSTEM: SND_SEQ_PORT_SYSTEM_TIMER and
 *  SND_SEQ_PORT_SYSTEM_ANNOUNCE. The SND_SEQ_PORT_SYSTEM_TIMER is the system
 *  timer port, and SND_SEQ_PORT_SYSTEM_ANNOUNCE is the system announce port.
 *
 * Timer:
 *
 *  In order to control a queue from a client, client should send a
 *  queue-control event like start, stop and continue queue, change tempo,
 *  etc. to the system timer port. Then the sequencer system handles the queue
 *  according to the received event. This port supports subscription. The
 *  received timer events are broadcasted to all subscribed clients.  From
 *  SND_SEQ_PORT_SYSTEM_TIMER, one may receive SND_SEQ_EVENT_START events.
 *
 * Announce:
 *
 *  The SND_SEQ_PORT_SYSTEM_ANNOUNCE port does not receive messages, but
 *  supports subscription. When each client or port is attached, detached or
 *  modified, an announcement is sent to subscribers from this port.  From
 *  SND_SEQ_PORT_SYSTEM_ANNOUNCE, one may receive
 *  SND_SEQ_EVENT_PORT_SUBSCRIBED events.
 *
 * Capability bits (FYI):
 *
 *      SND_SEQ_PORT_CAP_READ           0x01
 *      SND_SEQ_PORT_CAP_WRITE          0x02
 *      SND_SEQ_PORT_CAP_SYNC_READ      0x04
 *      SND_SEQ_PORT_CAP_SYNC_WRITE     0x08
 *      SND_SEQ_PORT_CAP_DUPLEX         0x10
 *      SND_SEQ_PORT_CAP_SUBS_READ      0x20
 *      SND_SEQ_PORT_CAP_SUBS_WRITE     0x40
 *      SND_SEQ_PORT_CAP_NO_EXPORT      0x80
 */

#include "calculations.hpp"             /* seq64::tempo_us_from_bpm()       */
#include "easy_macros.hpp"              /* C++ version of easy macros       */
#include "event.hpp"                    /* seq64::event and other tokens    */
#include "midi_alsa_info.hpp"           /* seq64::midi_alsa_info            */
#include "midibus_common.hpp"           /* from the libseq64 sub-project    */
#include "settings.hpp"                 /* seq64::rc() configuration object */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * Initialization of static members.
 */

unsigned midi_alsa_info::sm_input_caps =
    SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

unsigned midi_alsa_info::sm_output_caps =
    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

/**
 *  Principal constructor.
 *
 * \param appname
 *      Provides the name of the application.
 *
 * \param ppqn
 *      Provides the PPQN value needed by this object.
 *
 * \param bpm
 *      Provides the beats/minute value needed by this object.
 */

midi_alsa_info::midi_alsa_info
(
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_alsa_seq              (nullptr),
    m_num_poll_descriptors  (0),            /* from ALSA mastermidibus      */
    m_poll_descriptors      (nullptr)       /* ditto                        */
{
    snd_seq_t * seq;                        /* point to member              */
    int result = snd_seq_open               /* set up ALSA sequencer client */
    (
        &seq, "default", SND_SEQ_OPEN_DUPLEX, 0 // SND_SEQ_NONBLOCK
    );
    if (result < 0)
    {
        m_error_string = func_message("error opening ALSA sequencer client");
        error(rterror::DRIVER_ERROR, m_error_string);
    }
    else
    {
        /*
         * Save the ALSA "handle".  Set the client's name for ALSA.  Then set
         * up the ALSA client queue.  No LASH support included.
         */

        m_alsa_seq = seq;
        midi_handle(seq);
        snd_seq_set_client_name(m_alsa_seq, rc().application_name().c_str());
        global_queue(snd_seq_alloc_queue(m_alsa_seq));

        /*
         * Get the number of MIDI input poll file descriptors.  Allocate the
         * poll-descriptors array.  Then get the input poll-descriptors into
         * the array.  Finally, set the input and output buffer sizes.  Can we
         * do this before creating all the MIDI busses?  If not, we'll put
         * them in a separate function to call later.
         */

        m_num_poll_descriptors = snd_seq_poll_descriptors_count
        (
            m_alsa_seq, POLLIN
        );
        m_poll_descriptors = new pollfd[m_num_poll_descriptors];
        snd_seq_poll_descriptors
        (
            m_alsa_seq, m_poll_descriptors, m_num_poll_descriptors, POLLIN
        );
        snd_seq_set_output_buffer_size(m_alsa_seq, c_midibus_output_size);
        snd_seq_set_input_buffer_size(m_alsa_seq, c_midibus_input_size);
    }
}

/**
 *  Destructor.  Closes a connection if it exists, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_alsa_info::~midi_alsa_info ()
{
    if (not_nullptr(m_alsa_seq))
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);                          /* memset it to 0   */
        snd_seq_stop_queue(m_alsa_seq, global_queue(), &ev);
        snd_seq_free_queue(m_alsa_seq, global_queue());
        snd_seq_close(m_alsa_seq);                      /* close client     */
        (void) snd_config_update_free_global();         /* more cleanup     */
        if (not_nullptr(m_poll_descriptors))
        {
            delete [] m_poll_descriptors;
            m_poll_descriptors = nullptr;
        }
    }
}

#define SEQ64_PORT_CLIENT      0xFF000000
#define SEQ64_PORT_COUNT       (-1)

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another container.  For ALSA
 *  input, the first item added is the ALSA MIDI system "announce" buss.
 *  It has the client:port value of "0:1", denoted by the ALSA macros
 *  SND_SEQ_CLIENT_SYSTEM:SND_SEQ_PORT_SYSTEM_ANNOUNCE.
 *  The information obtained is:
 *
 *      -   Client name
 *      -   Port number
 *      -   Port name
 *      -   Port capabilities
 *
 * \return
 *      Returns the total number of ports found.  For an ALSA setup, finding
 *      no ALSA ports can be considered an error.  However, finding no ports
 *      for other APIS may be fine.  So, we set the result to -1 to flag a
 *      true error.
 */

int
midi_alsa_info::get_all_port_info ()
{
    int count = 0;
    if (not_nullptr(m_alsa_seq))
    {
        snd_seq_port_info_t * pinfo;                    /* point to member  */
        snd_seq_client_info_t * cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        input_ports().clear();
        output_ports().clear();
        input_ports().add
        (
            SND_SEQ_CLIENT_SYSTEM, "system",
            SND_SEQ_PORT_SYSTEM_ANNOUNCE, "announce",
            SEQ64_MIDI_NORMAL_PORT, global_queue(), true /* system port */,
            SEQ64_MIDI_INPUT_PORT
        );
        ++count;
        while (snd_seq_query_next_client(m_alsa_seq, cinfo) >= 0)
        {
            int client = snd_seq_client_info_get_client(cinfo);
            if (client == SND_SEQ_CLIENT_SYSTEM)        /* i.e. 0 in seq.h  */
            {
                /*
                 * Client 0 won't have ports (timer and announce) that match
                 * the MIDI-generic and Synth types checked below.
                 */

                continue;
            }

            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, client); /* reset query info */
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(m_alsa_seq, pinfo) >= 0)
            {
                unsigned alsatype = snd_seq_port_info_get_type(pinfo);
                if
                (
                    ((alsatype & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
                    ((alsatype & SND_SEQ_PORT_TYPE_SYNTH) == 0)
                )
                {
                    continue;
                }

                unsigned caps = snd_seq_port_info_get_capability(pinfo);
                std::string clientname = snd_seq_client_info_get_name(cinfo);
                std::string portname = snd_seq_port_info_get_name(pinfo);
                int portnumber = snd_seq_port_info_get_port(pinfo);
                if ((caps & sm_input_caps) == sm_input_caps)
                {
                    input_ports().add
                    (
                        client, clientname, portnumber, portname,
                        SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                        SEQ64_MIDI_INPUT_PORT, global_queue()
                    );
                    ++count;
                }
                if ((caps & sm_output_caps) == sm_output_caps)
                {
                    output_ports().add
                    (
                        client, clientname, portnumber, portname,
                        SEQ64_MIDI_NORMAL_PORT, SEQ64_MIDI_NORMAL_PORT,
                        SEQ64_MIDI_OUTPUT_PORT
                    );
                    ++count;
                }
                else
                {
                    /*
                     * When VMPK is running, we get this message for a
                     * client-name of 'VMPK Output'.
                     */

                    infoprintf("Non-I/O port '%s'\n", clientname.c_str());
                }
            }
        }
    }
    if (count == 0)
        count = -1;

    return count;
}

/**
 *  Flushes our local queue events out into ALSA.  This is also a midi_alsa
 *  function.
 */

void
midi_alsa_info::api_flush ()
{
    snd_seq_drain_output(m_alsa_seq);
}

/**
 *  Sets the PPQN numeric value, then makes ALSA calls to set up the PPQ
 *  tempo.
 *
 * \param p
 *      The desired new PPQN value to set.
 */

void
midi_alsa_info::api_set_ppqn (int p)
{
    midi_info::api_set_ppqn(p);

    int queue = global_queue();
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);             /* allocate tempo struct */
    snd_seq_get_queue_tempo(m_alsa_seq, queue, tempo);
    snd_seq_queue_tempo_set_ppq(tempo, p);
    snd_seq_set_queue_tempo(m_alsa_seq, queue, tempo);
}

/**
 *  Sets the BPM numeric value, then makes ALSA calls to set up the BPM
 *  tempo.
 *
 * \param b
 *      The desired new BPM value to set.
 */

void
midi_alsa_info::api_set_beats_per_minute (midibpm b)
{
    midi_info::api_set_beats_per_minute(b);

    int queue = global_queue();
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);          /* allocate tempo struct */
    snd_seq_get_queue_tempo(m_alsa_seq, queue, tempo);
    snd_seq_queue_tempo_set_tempo(tempo, unsigned(tempo_us_from_bpm(b)));
    snd_seq_set_queue_tempo(m_alsa_seq, queue, tempo);
}

/**
 *  Polls for any ALSA MIDI information using a timeout value of 1000
 *  milliseconds.  Identical to seq_alsamidi's mastermidibus ::
 *  api_poll_for_midi(), which waits 1 millisecond if no input is pending.
 *
 * \return
 *      Returns the result of the call to poll() on the global ALSA poll
 *      descriptors.
 */

int
midi_alsa_info::api_poll_for_midi ()
{
    int result = poll(m_poll_descriptors, m_num_poll_descriptors, 1000);
    if (result == 0)
        millisleep(1);

    return result;
}

/*
 * Definitions copped from the seq_alsamidi/src/mastermidibus.cpp module.
 */

/**
 *  Macros to make capabilities-checking more readable.
 */

#define CAP_READ(cap)       (((cap) & SND_SEQ_PORT_CAP_SUBS_READ) != 0)
#define CAP_WRITE(cap)      (((cap) & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0)

/**
 *  These checks need both bits to be set.  Intermediate macros used for
 *  readability.
 */

#define CAP_R_BITS      (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ)
#define CAP_W_BITS      (SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE)

#define CAP_FULL_READ(cap)  (((cap) & CAP_R_BITS) == CAP_R_BITS)
#define CAP_FULL_WRITE(cap) (((cap) & CAP_W_BITS) == CAP_W_BITS)

#define ALSA_CLIENT_CHECK(pinfo) \
    (snd_seq_client_id(m_alsa_seq) != snd_seq_port_info_get_client(pinfo))

/**
 *  Start the given ALSA MIDI port.  This function is called by
 *  api_get_midi_event() when an ALSA event SND_SEQ_EVENT_PORT_START is
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
 *  midi_alsa_info.
 *
 * \param masterbus
 *      Provides the object that is need to get access to the busses that need
 *      to be started.
 *
 * \param bus
 *      Provides the ALSA bus/client number.
 *
 * \param port
 *      Provides the ALSA client port.
 */

void
midi_alsa_info::api_port_start (mastermidibus & masterbus, int bus, int port)
{
    snd_seq_client_info_t * cinfo;                      /* get bus info       */
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_get_any_client_info(m_alsa_seq, bus, cinfo);
    snd_seq_port_info_t * pinfo;                        /* get port info      */
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_any_port_info(m_alsa_seq, bus, port, pinfo);

#ifdef SEQ64_SHOW_API_CALLS
    printf("midi_alsa_info::port_start(%d:%d)\n", bus, port);
#endif

    int cap = snd_seq_port_info_get_capability(pinfo);  /* get its capability */
    if (ALSA_CLIENT_CHECK(pinfo))
    {
        if (CAP_FULL_WRITE(cap) && ALSA_CLIENT_CHECK(pinfo)) /* outputs */
        {
            int bus_slot = masterbus.m_outbus_array.count();
            int test = masterbus.m_outbus_array.replacement_port(bus, port);
            if (test >= 0)
                bus_slot = test;

            midibus * m = new midibus(masterbus.m_midi_master, bus_slot);
            m->is_virtual_port(false);
            m->is_input_port(false);
            masterbus.m_outbus_array.add(m, e_clock_off);   /* disabled? */
        }
        if (CAP_FULL_READ(cap) && ALSA_CLIENT_CHECK(pinfo)) /* inputs */
        {
            int bus_slot = masterbus.m_inbus_array.count();
            int test = masterbus.m_inbus_array.replacement_port(bus, port);
            if (test >= 0)
                bus_slot = test;

            midibus * m = new midibus(masterbus.m_midi_master, bus_slot);
            m->is_virtual_port(false);
            m->is_input_port(true);                  // was false BEWARE BREAKAGE
            masterbus.m_inbus_array.add(m, false);
        }
    }                                           /* end loop for clients */

    /*
     * Get the number of MIDI input poll file descriptors.
     */

    m_num_poll_descriptors = snd_seq_poll_descriptors_count(m_alsa_seq, POLLIN);
    m_poll_descriptors = new pollfd[m_num_poll_descriptors]; /* allocate info */
    snd_seq_poll_descriptors                        /* get input descriptors */
    (
        m_alsa_seq, m_poll_descriptors, m_num_poll_descriptors, POLLIN
    );
}

/**
 *  Grab a MIDI event.  First, a rather large buffer is allocated on the stack
 *  to hold the MIDI event data.  Next, if the --alsa-manual-ports option is
 *  not in force, then we check to see if the event is a port-start,
 *  port-exit, or port-change event, and we prcess it, and are done.
 *
 *  Otherwise, we create a "MIDI event parser" and decode the MIDI event.
 *
 *  We've beefed up the error-checking in this function due to crashes we got
 *  when connected to VMPK and suddenly getting a rush of ghost notes, then a
 *  seqfault.  This also occurs in legacy seq24.  To reproduce, run VMPK and
 *  make it the input source.  Open a new pattern, turn on recording, and
 *  start the ALSA transport.  Record one note.  Then activate the button for
 *  "dump input to MIDI bus".  You will here the note through VMPK, then ghost
 *  notes start appearing and seq64/seq24 eventually crash.  A bug in VMPK, or
 *  our processing?  At any rate, we catch the bug now, and don't crash, but
 *  eventually processing gets swamped until we kill VMPK.  And we now have a
 *  note sounding even though neither app is running.  Really screws up ALSA!
 *
 * Events:
 *
 *      -  SND_SEQ_EVENT_PORT_START
 *      -  SND_SEQ_EVENT_PORT_EXIT
 *      -  SND_SEQ_EVENT_PORT_CHANGE
 *      -  SND_SEQ_EVENT_PORT_SUBSCRIBED (not handled)
 *      -  SND_SEQ_EVENT_PORT_UNSUBSCRIBED (not handled)
 *
 * \todo
 *      Also, we need to consider using the new remcount return code to loop
 *      on receiving events as long as we are getting them.
 *
 * \param inev
 *      The event to be set based on the found input event.  It is the
 *      destination for the incoming event.
 *
 * \return
 *      This function returns false if we are not using virtual/manual ports
 *      and the event is an ALSA port-start, port-exit, or port-change event.
 *      It also returns false if there is no event to decode.  Otherwise, it
 *      returns true.
 */

bool
midi_alsa_info::api_get_midi_event (event * inev)
{
    snd_seq_event_t * ev;
    bool sysex = false;
    bool result = false;
    midibyte buffer[0x1000];                /* temporary buffer for MIDI data */
    int remcount = snd_seq_event_input(m_alsa_seq, &ev);
    if (remcount < 0 || is_nullptr(ev))
    {
        errprint("snd_seq_event_input() failure");
        return false;
    }
    if (! rc().manual_alsa_ports())
    {
        switch (ev->type)
        {
        case SND_SEQ_EVENT_PORT_START:
        {
            /*
             * TODO:  figure out how to best do this.  It has way too many
             * parameters now, and is currently meant to be called from
             * mastermidibus.  See mastermidibase::port_start().
             *
             * port_start(masterbus, ev->data.addr.client, ev->data.addr.port);
             * api_port_start (mastermidibus & masterbus, int bus, int port)
             */

            result = true;
            break;
        }
        case SND_SEQ_EVENT_PORT_EXIT:
        {
            /*
             * The port_exit() function is defined in mastermidibase and in
             * businfo.  They seem to cover this functionality.
             *
             * port_exit(masterbus, ev->data.addr.client, ev->data.addr.port);
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
    }
    if (result)
        return false;

    snd_midi_event_t * midi_ev;                     /* make ALSA MIDI parser */
    int rc = snd_midi_event_new(sizeof(buffer), &midi_ev);
    if (rc < 0 || is_nullptr(midi_ev))
    {
        errprint("snd_midi_event_new() failed");
        return false;
    }

    long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
    if (bytes <= 0)
    {
        /*
         * This happens even at startup, before anything is really happening.
         */

        snd_midi_event_free(midi_ev);
        return false;
    }

    inev->set_timestamp(ev->time.tick);
    inev->set_status_keep_channel(buffer[0]);

    /**
     *  We will only get EVENT_SYSEX on the first packet of MIDI data;
     *  the rest we have to poll for.  SysEx processing is currently
     *  disabled.
     */

#ifdef USE_SYSEX_PROCESSING                 /* currently disabled           */
    inev->set_sysex_size(bytes);            /* why here? why not in "if"?   */
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
         *  event::set_status_keep_channel() instead of using stazed's
         *  set_status function with the "record" parameter.  We do need to
         *  mask in the actual channel number!
         */

        inev->set_data(buffer[1], buffer[2]);
        if (inev->is_note_off_recorded())
        {
            midibyte channel = buffer[0] & EVENT_GET_CHAN_MASK;
            midibyte status = EVENT_NOTE_OFF | channel;
            inev->set_status_keep_channel(status);
        }
        sysex = false;

#ifdef USE_SYSEX_PROCESSING
    }
#endif

    while (sysex)       /* sysex messages might be more than one message */
    {
        snd_seq_event_input(m_alsa_seq, &ev);
        long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof(buffer), ev);
        if (bytes > 0)
            sysex = inev->append_sysex(buffer, bytes);
        else
            sysex = false;
    }
    snd_midi_event_free(midi_ev);
    return true;
}

}           // namespace seq64

/*
 * midi_alsa_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

