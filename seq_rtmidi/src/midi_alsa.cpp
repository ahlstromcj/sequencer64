/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          midi_alsa.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-12-18
 * \updates       2017-06-02
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Linux-only implementation of ALSA MIDI support.
 *  It is derived from the seq_alsamidi implementation in that midibus module.
 *  Note that we are changing the MIDI API somewhat from the original RtMidi
 *  midi_api class; that interface didn't really fit the sequencer64 model,
 *  and it was getting very painful to warp RtMidi to fit.
 *
 * Examples of subscription:
 *	Capture from keyboard:
 *
 *		Assume MIDI input port = 64:0, application port = 128:0, and queue for
 *		timestamp = 1 with real-time stamp. The application port must have
 *		capability SND_SEQ_PORT_CAP_WRITE.
 *
\verbatim
    void capture_keyboard (snd_seq_t * seq)
    {
        snd_seq_addr_t sender, dest;
        snd_seq_port_subscribe_t *subs;
        sender.client = 64;
        sender.port = 0;
        dest.client = 128;
        dest.port = 0;
        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_port_subscribe_set_queue(subs, 1);
        snd_seq_port_subscribe_set_time_update(subs, 1);
        snd_seq_port_subscribe_set_time_real(subs, 1);
        snd_seq_subscribe_port(seq, subs);
    }
\endverbatim
 *
 *  Output to MIDI device:
 *
 *      Assume MIDI output port = 65:1 and application port = 128:0. The
 *      application port must have capability SND_SEQ_PORT_CAP_READ.
 *
\verbatim
    void subscribe_output(snd_seq_t *seq)
    {
        snd_seq_addr_t sender, dest;
        snd_seq_port_subscribe_t *subs;
        sender.client = 128;
        sender.port = 0;
        dest.client = 65;
        dest.port = 1;
        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_subscribe_port(seq, subs);
    }
\endverbatim
 *
 *  See http://www.alsa-project.org/alsa-doc/alsa-lib/seq.html for a wealth of
 *  information on ALSA sequencing.
 */

#include "globals.h"
#include "calculations.hpp"             /* clock_ticks_from_ppqn()          */
#include "event.hpp"                    /* seq64::event (MIDI event)        */
#include "midibus_rm.hpp"               /* seq64::midibus for rtmidi        */
#include "midi_alsa.hpp"                /* seq64::midi_alsa for ALSA        */
#include "midi_info.hpp"                /* seq64::midi_info                 */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Provides a constructor with client number, port number, ALSA sequencer
 *  support, name of client, name of port, etc., mostly contained within an
 *  already-initialized midi_info object.
 *
 *  This constructor is the only one that is used for the MIDI input and
 *  output busses, whether the [manual-alsa-ports] option is in force or not.
 *  The actual setup of a normal or virtual port is done in the api_*_init_*()
 *  routines.
 *
 *  Also used for the announce buss, and in the mastermidi_alsa::port_start()
 *  function.  There's currently some overlap between local/dest client and
 *  port numbers and the buss and port numbers of the new midibase interface.
 *
 *  Also, note that the optionsfile module uses the master buss to get the
 *  buss names when it writes the file.
 *
 *  We get the actual user-client ID from ALSA, then rebuild the descriptive
 *  name for this port. Also have to do it for the parent midibus.  We'd like
 *  to use seq_client_name(), but it comes up unresolved by the damned GNU
 *  linker!  The obvious fixes don't work!
 *
 *  Another issue (2017-05-27):  ALSA returns "130" as the client ID.  That is
 *  our ALSA ID, not the ID of the client we are representing.  Thus, we
 *  should not set the buss ID and name of the parentbus, these have already
 *  been determined.
 *
 * \param parentbus
 *      Provides much of the infor about this ALSA buss.
 *
 * \param masterinfo
 *      Provides the information about the desired port, and more.
 */

midi_alsa::midi_alsa (midibus & parentbus, midi_info & masterinfo)
 :
    midi_api            (parentbus, masterinfo),
    m_seq
    (
        reinterpret_cast<snd_seq_t *>(masterinfo.midi_handle())
    ),
    m_dest_addr_client  (parentbus.get_bus_id()),
    m_dest_addr_port    (parentbus.get_port_id()),
    m_local_addr_client (snd_seq_client_id(m_seq)),     /* our client ID    */
    m_local_addr_port   (-1),
    m_input_port_name   (rc().app_client_name() + " in")
{
    set_bus_id(m_local_addr_client);
    set_name(SEQ64_CLIENT_NAME, bus_name(), port_name());
}

/**
 *  A rote empty virtual destructor.
 */

midi_alsa::~midi_alsa ()
{
    // empty body
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual ALSA ports" option is not in force.
 *
 *  This initialization is like the "open_port()" function of the RtMidi
 *  library, with the addition of the snd_seq_connect_to() call involving
 *  the local and destination ports.
 *
 * \tricky
 *      One important thing to note is that this output port is initialized
 *      with the SND_SEQ_PORT_CAP_READ flag, which means this is really an
 *      input port.  We connect this input port with a system output port that
 *      was discovered.  This is backwards of the way RtMidi does it.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midi_alsa::api_init_out ()
{
    std::string busname = parent_bus().bus_name();
    int result = snd_seq_create_simple_port         /* create ports     */
    (
        m_seq, busname.c_str(),
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    result = snd_seq_connect_to                     /* connect to port  */
    (
        m_seq, m_local_addr_port, m_dest_addr_client, m_dest_addr_port
    );
    if (result < 0)
    {
        fprintf
        (
            stderr, "snd_seq_connect_to(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    else
    {
        set_port_open();

#ifdef SEQ64_SHOW_API_CALLS
        printf
        (
            "READ/output port '%s' created:\n local port %d connected to %d:%d\n",
            busname.c_str(), m_local_addr_port,
            m_dest_addr_client, m_dest_addr_port
        );
#endif

    }
    return true;
}

/**
 *  Initialize the MIDI input port.
 *
 * Subscription handlers:
 *
 *  In ALSA library, subscription is done via snd_seq_subscribe_port()
 *  function. It takes the argument of snd_seq_port_subscribe_t record
 *  pointer. Suppose that you have a client which will receive data from a
 *  MIDI input device. The source and destination addresses are like the
 *  below:
 *
\verbatim
    snd_seq_addr_t sender, dest;
    sender.client = MIDI_input_client;
    sender.port = MIDI_input_port;
    dest.client = my_client;
    dest.port = my_port;
\endverbatim
 *
    To set these values as the connection call like this.
 *
\verbatim
    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    snd_seq_subscribe_port(handle, subs);
\endverbatim
 *
 * \tricky
 *      One important thing to note is that this input port is initialized
 *      with the SND_SEQ_PORT_CAP_WRITE flag, which means this is really an
 *      output port.  We connect this output port with a system input port
 *      that was discovered.  This is backwards of the way RtMidi does it.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midi_alsa::api_init_in ()
{
    std::string portname = parent_bus().port_name();
    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, portname.c_str(),
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(read) error");
        return false;
    }

    snd_seq_port_subscribe_t * subs;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_addr_t sender;
    sender.client = m_dest_addr_client;                 /* MIDI input client  */
    sender.port = m_dest_addr_port;                     /* MIDI input port    */
    snd_seq_port_subscribe_set_sender(subs, &sender);   /* destination        */

    snd_seq_addr_t dest;
    dest.client = m_local_addr_client;                  /* my client          */
    dest.port = m_local_addr_port;                      /* my port            */
    snd_seq_port_subscribe_set_dest(subs, &dest);       /* local              */

    /*
     * Use the master queue, and get ticks, then subscribe.
     */

    int queue = parent_bus().queue_number();
    snd_seq_port_subscribe_set_queue(subs, queue);
    snd_seq_port_subscribe_set_time_update(subs, 1);
    result = snd_seq_subscribe_port(m_seq, subs);
    if (result < 0)
    {
        fprintf
        (
            stderr, "snd_seq_connect_from(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    else
    {
        set_port_open();
#ifdef SEQ64_SHOW_API_CALLS
        printf
        (
            "WRITE/input port '%s' created; sender %d:%d, "
            "destination (local) %d:%d\n",
            m_input_port_name.c_str(),
            m_dest_addr_client, m_dest_addr_port,
            m_local_addr_client, m_local_addr_port
        );
#endif
    }
    return true;
}

/**
 *  Gets information directly from ALSA.  The problem this function solves is
 *  that the midibus constructor for a virtual ALSA port doesn't not have all
 *  of the information it needs at that point.  Here, we can get this
 *  information and get the actual data we need to rename the port to
 *  something accurate.  Same as the seq_alsamidi version of this function.
 *
 * \return
 *      Returns true if all of the information could be obtained.  If false is
 *      returned, then the caller should not use the side-effects.
 *
 * \sideeffect
 *      Passes back the values found.
 */

bool
midi_alsa::set_virtual_name (int portid, const std::string & portname)
{
    bool result = not_nullptr(m_seq);
    if (result)
    {
        snd_seq_client_info_t * cinfo;                  /* client info      */
        snd_seq_client_info_alloca(&cinfo);             /* will fill cinfo  */
        snd_seq_get_client_info(m_seq, cinfo);          /* filled!          */

        int cid = snd_seq_client_info_get_client(cinfo);
        const char * cname = snd_seq_client_info_get_name(cinfo);
        result = not_nullptr(cname);
        if (result)
        {
            std::string clientname = cname;
            std::string pname = portname;
            set_port_id(portid);
            pname += " ";
            pname += std::to_string(portid);
            port_name(pname);
            set_bus_id(cid);
            set_name(rc().application_name(), clientname, pname);
            parent_bus().set_name(rc().application_name(), clientname, pname);
        }
    }
    return result;
}

/**
 *  Initialize the output in a different way.  This version of initialization
 *  is used by mastermidi_alsa in the "manual ALSA ports" clause.  This code
 *  is also very similar to the same function in the
 *  midibus::api_init_out_sub() function of midibus::api_init_out_sub().
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midi_alsa::api_init_out_sub ()
{
    std::string portname = port_name();
    if (portname.empty())
        portname = rc().app_client_name() + " out";

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, portname.c_str(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    else
    {
        set_virtual_name(result, portname);
        set_port_open();

#ifdef SEQ64_SHOW_API_CALLS
        printf
        (
            "virtual READ/output port '%s' created, local port %d\n",
            portname.c_str(), result
        );
#endif

    }
    return true;
}

/**
 *  Initialize the output in a different way?
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midi_alsa::api_init_in_sub ()
{
    std::string portname = port_name();
    if (portname.empty())
        portname = rc().app_client_name() + " midi in";

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq,
        m_input_port_name.c_str(),                      // portname.c_str()
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        errprint("snd_seq_create_simple_port(write) error");
        return false;
    }
    else
    {
        set_virtual_name(result, portname);
        set_port_open();

#ifdef SEQ64_SHOW_API_CALLS
        printf("virtual WRITE/input port 'seq24 in' created; port %d\n", result);
#endif

    }
    return true;
}

/**
 *  Deinitialize the MIDI input.  Set the input and the output ports.
 *  The destination port is actually our local port.
 *
 * \return
 *      Returns true, unless an error occurs.
 */

bool
midi_alsa::api_deinit_in ()
{
    snd_seq_port_subscribe_t * subs;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_addr_t sender;                                  /* output       */
    sender.client = m_dest_addr_client;
    sender.port = m_dest_addr_port;
    snd_seq_port_subscribe_set_sender(subs, &sender);

    snd_seq_addr_t dest;                                    /* input        */
    dest.client = m_local_addr_client;
    dest.port = m_local_addr_port;
    snd_seq_port_subscribe_set_dest(subs, &dest);

    /*
     * This would seem to unsubscribe all ports.  True?  Danger?
     */

    int queue = parent_bus().queue_number();
    snd_seq_port_subscribe_set_queue(subs, queue);
    snd_seq_port_subscribe_set_time_update(subs, queue);    /* get ticks    */

    int result = snd_seq_unsubscribe_port(m_seq, subs);     /* unsubscribe  */
    if (result < 0)
    {
        fprintf
        (
            stderr, "snd_seq_unsubscribe_port(%d:%d) error\n",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }

#ifdef SEQ64_SHOW_API_CALLS
    printf
    (
        "WRITE/input port deinit'ed; sender %d:%d, destination (local) %d:%d\n",
        m_dest_addr_client, m_dest_addr_port,
        m_local_addr_client, m_local_addr_port
    );
#endif

    return true;
}

/*
 *  This function is supposed to poll for MIDI data, but the current
 *  ALSA implementation DOES NOT USE THIS FUNCTION.  Commented out.
 *  This kills startup: return master_info().api_poll_for_midi();
 *
 *  int
 *  midi_alsa::api_poll_for_midi ()
 *  {
 *      millisleep(1);
 *      return 0;
 *  }
 *
 */

/**
 *  Defines the size of the MIDI event buffer, which should be large enough to
 *  accomodate the largest MIDI message to be encoded.
 *  A local define for visibility.
 */

#define SEQ64_MIDI_EVENT_SIZE_MAX   10

/**
 *  This play() function takes a native event, encodes it to an ALSA MIDI
 *  sequencer event, sets the broadcasting to the subscribers, sets the
 *  direct-passing mode to send the event without queueing, and puts it in the
 *  queue.
 *
 * \threadsafe
 *
 * \param e24
 *      The event to be played on this bus.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel of the playback.
 */

void
midi_alsa::api_play (event * e24, midibyte channel)
{
    midibyte buffer[4];                             /* temp for MIDI data   */
    buffer[0] = e24->get_status();                  /* fill buffer          */
    buffer[0] += (channel & 0x0F);
    e24->get_data(buffer[1], buffer[2]);            /* set MIDI data        */

    snd_midi_event_t * midi_ev;                     /* ALSA MIDI parser     */
    snd_midi_event_new(SEQ64_MIDI_EVENT_SIZE_MAX, &midi_ev);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event          */
    snd_midi_event_encode(midi_ev, buffer, 3, &ev); /* encode 3 raw bytes   */
    snd_midi_event_free(midi_ev);                   /* free the parser      */
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source           */

#ifdef SEQ64_SHOW_API_CALLS_XXX                     /* Too Much Information */
    printf("midi_alsa::play() local port %d\n", m_local_addr_port);
#endif

    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it is immediate      */
    snd_seq_event_output(m_seq, &ev);               /* pump into the queue  */
}

/**
 *  min() for long values.
 *
 * \param a
 *      First operand.
 *
 * \param b
 *      Second operand.
 *
 * \return
 *      Returns the minimum value of a and b.
 */

inline long
min (long a, long b)
{
    return (a < b) ? a : b ;
}

/**
 *  Defines the value used for sleeping, in microseconds.  Defined locally
 *  simply for visibility.  Why 80000?
 */

#define SEQ64_USLEEP_US     80000

/**
 *  Takes a native SYSEX event, encodes it to an ALSA event, and then
 *  puts it in the queue.
 *
 * \param e24
 *      The event to be handled.
 */

void
midi_alsa::api_sysex (event * e24)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* clear event      */
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);      /* set source       */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */

    /*
     *  Replaced by a vector of midibytes:
     *
     *      midibyte * data = e24->get_sysex();
     *
     *  This is a bit tricky, and relies on the standard property of
     *  std::vector where all n elements of the vector are guaranteed to be
     *  stored contiguously (in order to be accessible via random-access
     *  iterators).
     *
     *  256 == c_midi_alsa_sysex_chunk
     */

    const int chunk = 256;
    event::SysexContainer & data = e24->get_sysex();
    int data_size = e24->get_sysex_size();
    for (int offset = 0; offset < data_size; offset += chunk)
    {
        int data_left = data_size - offset;
        snd_seq_ev_set_sysex(&ev, min(data_left, chunk), &data[offset]);
        snd_seq_event_output_direct(m_seq, &ev);        /* pump into queue  */
        usleep(SEQ64_USLEEP_US);
        api_flush();
    }
}

/**
 *  Flushes our local queue events out into ALSA.  This is also a midi_alsa_info
 *  function.
 */

void
midi_alsa::api_flush ()
{
    snd_seq_drain_output(m_seq);
}

/**
 *  Continue from the given tick.
 *
 *  Also defined in midi_alsa_info.
 *
 * \param tick
 *      The continuing tick, unused in the ALSA implementation here.
 *      The midibase::continue_from() function uses it.
 *
 * \param beats
 *      The beats value calculated by midibase::continue_from().
 */

#ifdef SEQ64_SHOW_API_CALLS
#define tick_parameter tick
#else
#define tick_parameter /* tick */
#endif

void
midi_alsa::api_continue_from (midipulse tick_parameter, midipulse beats)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event      */
    ev.type = SND_SEQ_EVENT_CONTINUE;

    snd_seq_event_t evc;
    snd_seq_ev_clear(&evc);                         /* clear event      */
    evc.type = SND_SEQ_EVENT_SONGPOS;
    evc.data.control.value = beats;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_fixed(&evc);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_priority(&evc, 1);
    snd_seq_ev_set_source(&evc, m_local_addr_port); /* set the source   */
    snd_seq_ev_set_subs(&evc);
    snd_seq_ev_set_source(&ev, m_local_addr_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
    snd_seq_ev_set_direct(&evc);
    snd_seq_event_output(m_seq, &evc);              /* pump into queue  */
    api_flush();
    snd_seq_event_output(m_seq, &ev);

#ifdef SEQ64_SHOW_API_CALLS
    if (tick > 0)
    {
        printf
        (
            "midi_alsa::continue_from(%ld) local port %d\n", tick, m_local_addr_port
        );
    }
#endif

}

/**
 *  This function gets the MIDI clock a-runnin', if the clock type is not
 *  e_clock_off.
 */

void
midi_alsa::api_start ()
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* memsets it to 0  */
    ev.type = SND_SEQ_EVENT_START;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source   */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
    snd_seq_event_output(m_seq, &ev);               /* pump into queue  */
}

/**
 *  Stop the MIDI buss.
 */

void
midi_alsa::api_stop ()
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* memsets it to 0      */
    ev.type = SND_SEQ_EVENT_STOP;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source       */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate       */
    snd_seq_event_output(m_seq, &ev);               /* pump into queue      */
}

/**
 *  Generates the MIDI clock, starting at the given tick value.
 *  Also sets the event tag to 127 so the sequences won't remove it.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the starting tick, unused in the ALSA implementation.
 */


void
midi_alsa::api_clock (midipulse tick)
{
    if (tick >= 0)
    {
#ifdef PLATFORM_DEBUG_TMI
        midibase::show_clock("ALSA", tick);
#endif
    }

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event          */
    ev.type = SND_SEQ_EVENT_CLOCK;
    ev.tag = 127;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source           */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate       */
    snd_seq_event_output(m_seq, &ev);               /* pump it into queue   */
}

/**
 * Currently, this code is implemented in the midi_alsa_info module, since
 * it is a mastermidibus function.  Note the implementation here, though.
 * Which actually gets used?
 */

void
midi_alsa::api_set_ppqn (int ppqn)
{
    int queue = parent_bus().queue_number();
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);             /* allocate tempo struct */
    snd_seq_get_queue_tempo(m_seq, queue, tempo);
    snd_seq_queue_tempo_set_ppq(tempo, ppqn);
    snd_seq_set_queue_tempo(m_seq, queue, tempo);
}

/**
 *  Set the BPM value (beats per minute).  This is done by creating
 *  an ALSA tempo structure, adding tempo information to it, and then
 *  setting the ALSA sequencer object with this information.
 *
 *  We fill the ALSA tempo structure (snd_seq_queue_tempo_t) with the current
 *  tempo information, set the BPM value, put it in the tempo structure, and
 *  give the tempo value to the ALSA queue.
 *
 * \note
 *      Consider using snd_seq_change_queue_tempo() here if the ALSA queue has
 *      already been started.  It's arguments would be m_alsa_seq, m_queue,
 *      tempo (microseconds), and null.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

void
midi_alsa::api_set_beats_per_minute (midibpm bpm)
{
    int queue = parent_bus().queue_number();
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);          /* allocate tempo struct */
    snd_seq_get_queue_tempo(m_seq, queue, tempo);
    snd_seq_queue_tempo_set_tempo(tempo, unsigned(tempo_us_from_bpm(bpm)));
    snd_seq_set_queue_tempo(m_seq, queue, tempo);
}

#ifdef REMOVE_QUEUED_ON_EVENTS_CODE

/**
 *  Deletes events in the queue.  This function is not used anywhere, and
 *  there was no comment about the intent/context of this function.
 */

void
midi_alsa::remove_queued_on_events (int tag)
{
    snd_seq_remove_events_t * remove_events;
    snd_seq_remove_events_malloc(&remove_events);
    snd_seq_remove_events_set_condition
    (
        remove_events, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH |
            SND_SEQ_REMOVE_IGNORE_OFF
    );
    snd_seq_remove_events_set_tag(remove_events, tag);
    snd_seq_remove_events(m_seq, remove_events);
    snd_seq_remove_events_free(remove_events);
}

#endif      // REMOVE_QUEUED_ON_EVENTS_CODE

/**
 *  ALSA MIDI input normal port or virtual port constructor.  The kind of port
 *  is determine by which port-initialization function the mastermidibus
 *  calls.
 */

midi_in_alsa::midi_in_alsa (midibus & parentbus, midi_info & masterinfo)
 :
    midi_alsa   (parentbus, masterinfo)
{
    // Empty body
}

/**
 *  ALSA MIDI output normal port or virtual port constructor.  The kind of
 *  port is determine by which port-initialization function the mastermidibus
 *  calls.
 */

midi_out_alsa::midi_out_alsa (midibus & parentbus, midi_info & masterinfo)
 :
    midi_alsa   (parentbus, masterinfo)
{
    // Empty body
}

}           // namespace seq64

/*
 * midi_alsa.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

