/**
 * \file          midi_jack.cpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2017-04-25
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  Written primarily by Alexander Svetalkin, with updates for delta time by
 *  Gary Scavone, April 2011.
 *
 *  In this refactoring, we have to warp the RtMidi model, where ports are
 *  opened directly by the application, to the Sequencer64 midibus model,
 *  where the port object is created, but initialized after creation.  This
 *  proved very challenging -- it took a long time to get the midi_alsa
 *  implementation working, and still more time to get the midi_jack
 *  implementation solid.  So to call this code "rtmidi" code is slightly
 *  misleading.
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
 * Random JACK notes:
 *
 *      jack_activate() tells the JACK server that the program is ready to
 *      process JACK events.  jack_deactivate() removes the JACK client from
 *      the process graph and disconnects all ports belonging to it.
 *
 * Callbacks:
 *
 *      The input JACK callback can call an rtmidi input callback of the form
 *
 *          void callback (midi_message & message, void * userdata)
 *
 *      This callback is wired in by calling rtmidi_in_data ::
 *      user_callback().  Unlike RtMidi, the delta time is stored as part of
 *      the message.
 *
 * JackPortFlags:
 *
\verbatim
        JackPortIsInput     = 0x01
        JackPortIsOutput    = 0x02
        JackPortIsPhysical  = 0x04
        JackPortCanMonitor  = 0x08
        JackPortIsTerminal  = 0x10
\endverbatim
 *
 * I/O Issues:
 *
 *      The nomenclature for JACK input/output ports seems to be backwards of
 *      that for ALSA.  Note the confusing (but necessary) orientation of the
 *      driver backend ports: playback ports are "input" to the backend, and
 *      capture ports are "output" from the backend.  Here are the properties
 *      we have gleaned for JACK I/O ports:
 *
 *      -#  JACK Input Port.
 *          -#  A writable client.
 *          -#  Accepts input from an application or device.
 *          -#  We create an "output" port and connect it to this input port,
 *              so that we can send data to the port.
 *          -#  We set up an "output" JACK process callback that hands off the
 *              data to JACK, so that it can be played.
 *      -#  JACK Output Port.
 *          -#  A readable client.
 *          -#  Provides output to an application or device.
 *          -#  We create an "input" port and connect it to this output port,
 *              so that we can receive data from the port.
 *          -#  We set up an "input" JACK process callback that provides us
 *              with the data collected by JACK, so that we can record the
 *              data.
 *
 *  jack_port_get_buffer() returns a pointer to the memory area associated with
 *  the specified port. For an output port, it will be a memory area that can be
 *  written to; for an input port, it will be an area containing the data from
 *  the port's connection(s), or zero-filled. if there are multiple inbound
 *  connections, the data will be mixed appropriately.  Do not cache the
 *  returned address across process() callbacks. Port buffers have to be
 *  retrieved in each callback for proper functionning.
 *
 *  jack_midi_clear_buffer() clears the buffer, which must be an output buffer.
 *  It must be called before calling jack_midi_event_reserve() or
 *  jack_midi_event_write().
 *
 *  jack_midi_event_reserve() allocates space for an event to be written to an
 *  event port buffer.  Clients must write the event data to the pointer
 *  returned by this function. Clients must not write more than data_size
 *  bytes into this buffer. Clients must write normalised MIDI data to the
 *  port - no running status and no (1-byte) realtime messages interspersed
 *  with other messages (realtime messages are fine when they occur on their
 *  own, like other messages).  Events must be written in order, sorted by
 *  their sample offsets. JACK will not sort the events for you, and will
 *  refuse to store out-of-order events.
 *
 *  jack_ringbuffer_read() reads data from the ring-buffer and advances the data
 *  pointer.  The first parameter is the pointer to the ring-buffer.  The second
 *  parameter is the destination for the data that is read from the ring-buffer.
 *  The third parameter is the number of bytes to read.  It returns the number
 *  of bytes actually read.
 *
 *  jack_midi_event_get() gets a MIDI event from an event port buffer.
 *  JACK MIDI is normalized, the MIDI event returned by this function is
 *  guaranteed to be a complete MIDI event (the status byte will always be
 *  present, and no realtime events will interspered with the event).
 *  It returns 0 on success, or ENODATA if buffer is empty.
 *
 * MIDI clock:
 *
 *      We need to study the source code to the jack_midi_clock application to
 *      make sure we're doing this correctly.
 */

#include <sstream>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include "calculations.hpp"             /* seq64::extract_port_name()       */
#include "event.hpp"                    /* seq64::event from main library   */
#include "jack_assistant.hpp"           /* seq64::jack_status_pair_t        */
#include "midibus_rm.hpp"               /* seq64::midibus for rtmidi        */
#include "midi_jack.hpp"                /* seq64::midi_jack                 */
#include "settings.hpp"                 /* seq64::rc() accessor function    */

/**
 *  Delimits the size of the JACK ringbuffer.
 */

#define JACK_RINGBUFFER_SIZE 16384      /* default size for ringbuffer  */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Defines the JACK input process callback.  It is the JACK process callback
 *  for a MIDI output port (e.g. "system:midi_capture_1", which gives us the
 *  output of the Korg nanoKEY2 MIDI controller), also known as a "Readable
 *  Client" by qjackctl. It does the following:
 *
 *  This callback receives data from JACK and gives it to our application's
 *  input port.
 *
 *  This function does the following:
 *
 *      -#  Get the JACK port buffer and the MIDI event-count into this
 *          buffer.
 *      -#  For each MIDI event, get the event from JACK and push it into a
 *          local midi_message object.
 *      -#  Get the event time, converting it to a delta time if possible.
 *      -#  If it is not a SysEx continuation, then:
 *          -#  If we're using a callback, pass the data to that callback.  Do
 *              we need this callback to interface with the midibus-based
 *              code?
 *          -#  Otherwise, add the midi_message container to the rtmidi input
 *              queue.  One can then grab this data in a midibase ::
 *              poll_for_midi() call.  We still ought to check the add
 *              success.
 *
 *  The ALSA code polls for events, and that model is also available here.
 *  We're still working exactly how it will work best.
 *
 *  This function used to be static, but now we make if available to
 *  midi_jack_info.  Also note the s_null_detected flag.  It is used only to
 *  have the apiprint() debug messages appear only once, for better
 *  trouble-shooting.
 *
 * \param nframes
 *    The frame number to be processed.
 *
 * \param arg
 *    A pointer to the midi_jack_data structure to be processed.
 *
 * \return
 *    Returns 0.
 */

int
jack_process_rtmidi_input (jack_nframes_t nframes, void * arg)
{
    static bool s_null_detected = false;
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    rtmidi_in_data * rtindata = jackdata->m_jack_rtmidiin;
    if (is_nullptr(jackdata->m_jack_port))     /* is port created?        */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_input", "null jack port");
        }
        return 0;
    }
    if (is_nullptr(rtindata))
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_input", "null rtmidi_in_data");
        }
        return 0;
    }
    s_null_detected = false;

    /*
     * Since this is an input port, buff is the area that contains data from
     * the "remote" (i.e. outside our application) port.
     */

    void * buff = jack_port_get_buffer(jackdata->m_jack_port, nframes);
    if (not_nullptr(buff))
    {
        jack_midi_event_t jmevent;
        jack_time_t jtime;
        int evcount = jack_midi_get_event_count(buff);
        for (int j = 0; j < evcount; ++j)
        {
            midi_message message;
            int rc = jack_midi_event_get(&jmevent, buff, j);
            if (rc == 0)
            {
                int eventsize = int(jmevent.size);
                for (int i = 0; i < eventsize; ++i)
                    message.push(jmevent.buffer[i]);

                jtime = jack_get_time();            /* compute delta time   */
                if (rtindata->first_message())
                {
                    rtindata->first_message(false);
                }
                else
                {
                    message.timestamp
                    (
                        (jtime - jackdata->m_jack_lasttime) * 0.000001
                    );
                }
                jackdata->m_jack_lasttime = jtime;
                if (! rtindata->continue_sysex())
                {
                    if (rtindata->using_callback())
                    {
                        rtmidi_callback_t callback = rtindata->user_callback();
                        callback(message, rtindata->user_data());
                    }
                    else
                    {
                        (void) rtindata->queue().add(message);
                    }
                }
            }
            else
            {
                if (rc == ENODATA)
                {
                    errprintf("jack_process_rtmidi_input() ENODATA = %x", rc);
                }
                else
                {
                    errprintf("jack_process_rtmidi_input() ERROR = %x", rc);
                }
            }
        }
    }
    return 0;
}

/**
 *  Defines the JACK process input callback.  It is the JACK process callback
 *  for a MIDI input port (a midi_in_jack object associated with, for example,
 *  "system:midi_playback_1", representing, for example, a Korg nanoKEY2 to
 *  which we can send information), also known as a "Writable Client" by
 *  qjackctl.  Here's how it works:
 *
 *      -#  Get the JACK port buffer, for our local jack port.  Clear it.
 *      -#  Loop while the number of bytes available for reading [via
 *          jack_ringbuffer_read_space()] is non-zero.  Note that the second
 *          parameter is where the data is copied.
 *      -#  Get the size of each event, and allocate space for an event to be
 *          written to an event port buffer (the JACK "reserve" function).
 *      -#  Read the data from the ringbuffer into this port buffer.  JACK
 *          should then send it to the remote port.
 *
 *  Since this is an output port, "buff" is the area to which we can write
 *  data, to send it to the "remote" (i.e. outside our application) port.  The
 *  data is written to the ringbuffer in api_init_out(), and here we read the
 *  ring buffer and pass it to the output buffer.
 *
 *  We were wondering if, like the JACK midiseq example program, we need to
 *  wrap out process in a for-loop over the number of frames.  In our tests,
 *  we are getting 1024 frames, and the code seems to work without that loop.
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

int
jack_process_rtmidi_output (jack_nframes_t nframes, void * arg)
{
    static bool s_null_detected = false;
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    if (is_nullptr(jackdata->m_jack_port))          /* is port created?     */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_output", "null jack port");
        }
        return 0;
    }
    if (is_nullptr(jackdata->m_jack_buffsize))      /* port set up?        */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_output", "null jack buffer");
        }
        return 0;
    }

    static size_t soffset = 0;
    void * buf = jack_port_get_buffer(jackdata->m_jack_port, nframes);

#ifdef SEQ64_SHOW_API_CALLS_TMI
    printf
    (
        "%d frames for jack port %lx\n",
        int(nframes), (unsigned long)(jackdata->m_jack_port)
    );
#endif

    jack_midi_clear_buffer(buf);

    /*
     * A for-loop over the number of nframes?  See discussion above.
     */

    while (jack_ringbuffer_read_space(jackdata->m_jack_buffsize) > 0)
    {
        int space;
        (void) jack_ringbuffer_read
        (
            jackdata->m_jack_buffsize, (char *) &space, sizeof space
        );
        jack_midi_data_t * md = jack_midi_event_reserve(buf, soffset, space);
        if (not_nullptr(md))
        {
            char * mididata = reinterpret_cast<char *>(md);
            (void) jack_ringbuffer_read         /* copy into mididata */
            (
                jackdata->m_jack_buffmessage, mididata, size_t(space)
            );

#ifdef SEQ64_SHOW_API_CALLS_TMI
            printf("%d bytes read: ", space);
            for (int i = 0; i < space; ++i)
                printf("%x ", (unsigned char)(mididata[i]));

            printf("\n");
#endif
        }
        else
        {
            errprint("jack_midi_event_reserve() returned a null pointer");
        }
    }
    return 0;
}

/*
 * MIDI JACK base class
 */

/**
 *  We still need to figure out if we want a "master" client handle, or a
 *  handle to each port.  Same for the JACK data item.  Currently, we provide
 *  the m_multi_client member so that we can experiment, but "multi-client"
 *  mode is currently incompletely implemented.
 *
 *  Note that this constructor also adds its object to the midi_jack_info port
 *  list, so that the JACK callback functions can iterate through all of the
 *  JACK ports in use by this application, performing work on them.
 *
 * \param parentbus
 *      Provides a reference to the midibus that represents this object.
 *
 * \param masterinfo
 *      Provides a reference to the midi_jack_info object that may provide
 *      extra informatino that is needed by this port.  Too many entities!
 */

midi_jack::midi_jack
(
    midibus & parentbus,
    midi_info & masterinfo
) :
    midi_api            (parentbus, masterinfo),
    m_multi_client      (SEQ64_RTMIDI_MULTICLIENT),
    m_remote_port_name  (),
    m_jack_info         (dynamic_cast<midi_jack_info &>(masterinfo)),
    m_jack_data         ()
{
    if (multi_client())
    {
        // No code needed, yet
    }
    else
    {
        client_handle
        (
            reinterpret_cast<jack_client_t *>(masterinfo.midi_handle())
        );
        (void) m_jack_info.add(*this);
    }
}

/**
 *  This could be a rote empty destructor if we offload this destruction to the
 *  midi_jack_data structure.  However, other than the initialization, that
 *  structure is currently "dumb".
 */

midi_jack::~midi_jack ()
{
    if (multi_client())
    {
        close_port();
        close_client();
    }
    if (not_nullptr(m_jack_data.m_jack_buffsize))
        jack_ringbuffer_free(m_jack_data.m_jack_buffsize);

    if (not_nullptr(m_jack_data.m_jack_buffmessage))
        jack_ringbuffer_free(m_jack_data.m_jack_buffmessage);

    apiprint("~midi_jack", "jack");
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual ports" option is not in force.  This code is basically what was
 *  done by midi_out_jack::open_port() in RtMidi.
 *
 *  For jack_connect(), the first port-name is the source port, and the source
 *  port must include the JackPortIsOutput flag.  The second port-name is the
 *  destination port, and the destination port must include the
 *  JackPortIsInput flag.  For this function, the source/output port is this
 *  port, and the destination/input is....
 *
 *  Note that connect_port() [which calls jack_connect()] cannot usefully be
 *  called until jack_activate() has been called.
 *
 * \return
 *      Returns true unless setting up JACK MIDI failed in some way.
 */

bool
midi_jack::api_init_out ()
{
    bool result = true;
    std::string remoteportname = connect_name();    /* "bus:port"   */
    remote_port_name(remoteportname);
    if (multi_client())
        result = open_client_impl(SEQ64_MIDI_OUTPUT_PORT);

    if (result)
    {
        result = create_ringbuffer(JACK_RINGBUFFER_SIZE);
        if (result)
        {
            set_alt_name
            (
                rc().application_name(), rc().app_client_name(), remoteportname
            );
            parent_bus().set_alt_name           // TENTATIVE
            (
                rc().application_name(), rc().app_client_name(), remoteportname
            );
        }
    }
    if (result)
    {
        result = register_port(SEQ64_MIDI_OUTPUT_PORT, port_name());

        /*
         * Note that we cannot connect ports until we are activated, and we
         * cannot be activated until all ports are properly set up.
         * Otherwise, we'd call:
         *
         *  std::string localname = connect_name();
         *  result = connect_port(SEQ64_MIDI_OUTPUT, localname, remoteportname);
         *  if (result) set_port_open();
         */
    }
    return result;
}

/**
 *  This function is called when we are processing the list of system input
 *  ports. We want to create an output port of a similar name, but with the
 *  application as client, and connect it to this sytem input port.  We want to
 *  follow the model we got from seq24, rather than the RtMidi model, so that we
 *  do not have to rework (and probably break) the seq24 model.
 *
 *  We can't use the API port name here at this time, because it comes up
 *  empty.  It comes up empty because we haven't yet registered the ports,
 *  including the source ports.  So we register it first; connect_port() will
 *  not try to register it again.
 *
 *  Based on the comments in the jack.txt note file, here is what we need to do:
 *
 *      -#  open_client_impl(INPUT).
 *          -#  Get port name via master_info().connect_name().
 *          -#  Call jack_client_open() with or without a UUID.
 *          -#  Call jack_set_process_callback() for input/output.
 *          -#  Call jack_activate().  Premature?
 *      -#  register_port(INPUT...).  The flag is JackPortIsInput.
 *      -#  connect_port(INPUT...).  Call jack_connect(srcport, destport).
 *
 *  Unlike the corresponding virtual port, this input port is actually an
 *  output port.
 *
 * \return
 *      Returns true if the function was successful, and sets the flag
 *      indicating that the port is open.
 */

bool
midi_jack::api_init_in ()
{
    bool result = true;
    std::string remoteportname = connect_name();    /* "bus:port"       */
    remote_port_name(remoteportname);
    if (multi_client())
    {
        result = open_client_impl(SEQ64_MIDI_INPUT_PORT);
    }
    else
    {
        set_alt_name
        (
            rc().application_name(), rc().app_client_name(), remoteportname
        );
        parent_bus().set_alt_name           // TENTATIVE
        (
            rc().application_name(), rc().app_client_name(), remoteportname
        );
    }
    if (result)
    {
        result = register_port(SEQ64_MIDI_INPUT_PORT, port_name());

        /*
         * Note that we cannot connect ports until we are activated, and we
         * cannot be activated until all ports are properly set up.
         * Otherwise, we'd call:
         *
         *  std::string localname = connect_name();
         *  result = connect_port(SEQ64_MIDI_INPUT, localname, remoteportname);
         *  if (result) set_port_open();
         *
         * We also need to fill in the m_jack_data member here.
         */
    }
    return result;
}

/**
 *  Assumes that the port has already been registered, and that JACK
 *  activation has already occurred.
 *
 * \return
 *      Returns true if all steps of the connection succeeded.
 */

bool
midi_jack::api_connect ()
{
    std::string remotename = remote_port_name();
    std::string localname = connect_name();     /* modified!    */
    bool result;
    if (multi_client())
        jack_activate(client_handle());

    if (is_input_port())
        result = connect_port(SEQ64_MIDI_INPUT_PORT, remotename, localname);
    else
        result = connect_port(SEQ64_MIDI_OUTPUT_PORT, localname, remotename);

    if (result)
        set_port_open();

    return result;
}

/**
 *  Gets information directly from JACK.  The problem this function solves is
 *  that the midibus constructor for a virtual JACK port doesn't not have all
 *  of the information it needs at that point.  Here, we can get this
 *  information and get the actual data we need to rename the port to
 *  something accurate.
 *
 *      const char * pname = jack_port_name(const jack_port_t *);
 *
 * \return
 *      Returns true if all of the information could be obtained.  If false is
 *      returned, then the caller should not use the side-effects.
 *
 * \sideeffect
 *      Passes back the values found.
 */

bool
midi_jack::set_virtual_name (int portid, const std::string & portname)
{
    bool result = not_nullptr(client_handle());
    if (result)
    {
        char * cname = jack_get_client_name(client_handle());
        result = not_nullptr(cname);
        if (result)
        {
            std::string clientname = cname;
            set_port_id(portid);
            port_name(portname);
            set_name(rc().application_name(), clientname, portname);
            parent_bus().set_name(rc().application_name(), clientname, portname);
        }
    }
    return result;
}

/*
 *  This initialization is like the "open_virtual_port()" function of the
 *  RtMidi library.  However, unlike the ALSA case... to be determined.
 *
 * \return
 *      Returns true if all steps of the initialization succeeded.
 */

bool
midi_jack::api_init_out_sub ()
{
    bool result = true;
    master_midi_mode(SEQ64_MIDI_OUTPUT_PORT);    /* this is necessary */
    if (multi_client())
        result = open_client_impl(SEQ64_MIDI_OUTPUT_PORT);

    if (result)
    {
        int portid = parent_bus().get_port_id();
        result = portid >= 0;
        if (! result)
        {
            portid = get_bus_index();
            result = portid >= 0;
        }
        if (result)
            result = create_ringbuffer(JACK_RINGBUFFER_SIZE);

        if (result)
        {
            std::string portname = parent_bus().port_name();
            if (portname.empty())
            {
                portname = rc().app_client_name() + " midi out ";
                portname += std::to_string(portid);
            }
            result = register_port(SEQ64_MIDI_OUTPUT_PORT, portname);
            if (result)
            {
                set_virtual_name(portid, portname);
                set_port_open();
            }
        }
    }
    return result;
}

/**
 *  Initializes a virtual/manual input port.
 *
 * \return
 *      Returns true if all steps of the initialization succeeded.
 */

bool
midi_jack::api_init_in_sub ()
{
    bool result = true;
    master_midi_mode(SEQ64_MIDI_INPUT_PORT);
    if (multi_client())
        result = open_client_impl(SEQ64_MIDI_INPUT_PORT);

    if (result)
    {
        int portid = parent_bus().get_port_id();
        result = portid >= 0;
        if (! result)
        {
            portid = get_bus_index();
            result = portid >= 0;
        }
        if (result)
        {
            std::string portname = master_info().get_port_name(get_bus_index());
            std::string portname2 = parent_bus().port_name();
            if (portname.empty())
            {
                portname = rc().app_client_name() + " midi in ";
                portname += std::to_string(portid);
            }
            result = register_port(SEQ64_MIDI_INPUT_PORT, portname);
            if (result)
            {
                set_virtual_name(portid, portname);
                set_port_open();
            }
        }
    }
    return result;
}

/**
 *  We could define these in the opposite order.
 */

bool
midi_jack::api_deinit_in ()
{
    close_port();
    return true;
}

/**
 *  We could push the bytes of the event into a midibyte vector, as done in
 *  send_message().  The ALSA code (seq_alsamidi/src/midibus.cpp) sticks the
 *  event bytes in an array, which might be a little faster than using
 *  push_back(), but let's try the vector first.  The rtmidi code here is from
 *  midi_out_jack::send_message().
 */

void
midi_jack::api_play (event * e24, midibyte channel)
{
    midibyte status = e24->get_status() + (channel & 0x0F);
    midibyte d0, d1;
    e24->get_data(d0, d1);

    midi_message message;
    message.push(status);
    message.push(d0);
    message.push(d1);

#ifdef SEQ64_SHOW_API_CALLS_TMI
    printf("midi_jack::play()\n");
#endif

    int nbytes = message.count();               /* send_message(message) */
    if (nbytes > 0 && m_jack_data.valid_buffer())
    {
        int count1 = jack_ringbuffer_write
        (
            m_jack_data.m_jack_buffmessage,
            message.array(),                    /* (const char *) &m[0] */
            message.count()
        );
        int count2 = jack_ringbuffer_write
        (
            m_jack_data.m_jack_buffsize, (char *) &nbytes, sizeof nbytes
        );
        if ((count1 <= 0) || (count2 <= 0))
        {
            errprint("JACK api_play failed");
        }
    }
}

/**
 * \todo
 *      Flesh out this routine.
 */

void
midi_jack::api_sysex (event * /* e24 */)
{
    // Will put this one off until later....
}

/**
 *  It seems like JACK doesn't have the concept of flushing event.
 */

void
midi_jack::api_flush ()
{
    // No code needed
}

/**
 *  jack_transport_locate(), jack_transport_reposition(), or something else?
 *  What is used by jack_assistant?
 *
 * \param tick
 *      Provides the tick value to continue from.
 *
 *  The param beats is currently unused.
 */

void
midi_jack::api_continue_from (midipulse tick, midipulse /*beats*/)
{
    int beat_width = 4;                                 // no m_beat_width !!!
    int ticks_per_beat = ppqn() * 10;
    midibpm beats_per_minute = bpm();
    uint64_t tick_rate =
    (
        uint64_t(jack_get_sample_rate(client_handle())) * tick * 60.0
    );
    long tpb_bpm = long(ticks_per_beat * beats_per_minute * 4.0 / beat_width);
    uint64_t jack_frame = tick_rate / tpb_bpm;
    if (jack_transport_locate(client_handle(), jack_frame) != 0)
        (void) info_message("jack api_continue_from() failed");

    /*
     * New code to work like the ALSA version, needs testing.  Related to
     * issue #67.
     */

    send_byte(EVENT_MIDI_SONG_POS);
    api_flush();
    send_byte(EVENT_MIDI_CONTINUE);
    apiprint("api_continue_from", "jack");
}

/**
 *  Starts this JACK client and sends MIDI start.   Note that the
 *  jack_assistant code (which implements JACK transport) checks if JACK is
 *  running, but a check of the JACK client handle here should be enough.
 */

void
midi_jack::api_start ()
{
    jack_transport_start(client_handle());
    send_byte(EVENT_MIDI_START);
    apiprint("jack_transport_start", "jack");
}

/**
 *  Stops this JACK client and sends MIDI stop.   Note that the jack_assistant
 *  code (which implements JACK transport) checks if JACK is running, but a
 *  check of the JACK client handle here should be enough.
 */

void
midi_jack::api_stop ()
{
    jack_transport_stop(client_handle());
    send_byte(EVENT_MIDI_STOP);
    apiprint("jack_transport_stop", "jack");
}

/**
 *  Sends a MIDI clock event.
 *
 * \param tick
 *      The value of the tick to use, but currently unused.
 */

void
midi_jack::api_clock (midipulse tick)
{
    send_byte(EVENT_MIDI_CLOCK, tick);
}

/**
 *  An internal helper function for sending MIDI clock bytes.
 *
 *  Based on the GitHub project "jack_midi_clock", we could try to bypass the
 *  ringbuffer used here and convert the ticks to jack_nframes_t, and use the
 *  following code:
 *
 *      uint8_t * buffer = jack_midi_event_reserve(port_buf, time, 1);
 *      if (buffer)
 *          buffer[0] = rt_msg;
 *
 *  We generally need to send the (realtime) MIDI clock messages Start, Stop,
 *  and  Continue if the JACK transport state changed.
 *
 * \param evbyte
 *      Provides one of the following values (though any byte can be sent):
 *
 *          -   EVENT_MIDI_SONG_POS
 *          -   EVENT_MIDI_CLOCK. The tick value is needed if...
 *          -   EVENT_MIDI_START.  The tick value is not needed.
 *          -   EVENT_MIDI_CONTINUE.  The tick value is needed if...
 *          -   EVENT_MIDI_STOP.  The tick value is not needed.
 *
 * \param tick
 *      Provides the tick value, if applicable.  We will eventually implement
 *      this, along with an "impossible" default value that means "ignore the
 *      tick".
 */

void
midi_jack::send_byte (midibyte evbyte, midipulse tick)
{
    midi_message message;
    message.push(evbyte);
    int nbytes = 1;

    if (is_null_midipulse(tick))
    {
        // TODO
    }

    if (m_jack_data.valid_buffer())
    {
        int count1 = jack_ringbuffer_write
        (
            m_jack_data.m_jack_buffmessage, message.array(), 1
        );
        int count2 = jack_ringbuffer_write
        (
            m_jack_data.m_jack_buffsize, (char *) &nbytes, sizeof nbytes
        );
        if ((count1 <= 0) || (count2 <= 0))
        {
            errprint("JACK send_byte() failed");
        }
    }
}

void
midi_jack::api_set_ppqn (int /*ppqn*/)
{
    // No code needed yet
}

void
midi_jack::api_set_beats_per_minute (midibpm /*bpm*/)
{
    // No code needed yet
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
    if (not_nullptr(port_handle()))
        result = std::string(jack_port_name(port_handle()));

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
 *      -   jack_set_process_callback(), to set jack_process_rtmidi_input() or
 *          jack_process_rtmidi_output().
 *
 *  Note that jack_activate() is no longer called here.
 *
 *  For output, connects the MIDI output port.  The following calls are made:
 *
 *      -   jack_ringbuffer_create(), called twice, to initialize the
 *          output ringbuffers
 *      -   jack_client_open(), to initialize JACK client
 *      -   jack_set_process_callback(), to set jack_process_inpu()
 *
 *  Note that jack_activate() is no longer called here.
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
        std::string appname = rc().application_name();
        std::string clientname = rc().app_client_name();
        std::string rpname = remote_port_name();
        if (is_virtual_port())
        {
            set_alt_name(appname, clientname, rpname);
            parent_bus().set_alt_name(appname, clientname, rpname);
        }
        else
        {
            set_multi_name(appname, clientname, rpname);
            parent_bus().set_multi_name(appname, clientname, rpname);
        }

        const char * name = bus_name().c_str();                 // modified
        jack_client_t * clipointer = create_jack_client(name); // , uuid);
        if (not_nullptr(clipointer))
        {
            client_handle(clipointer);          // midi_handle() too???
            if (input)
            {
                /*
                 * TODO:  MAKE SURE THE m_jack_data PORT VALUE IS NOT NULL!!!
                 */

                int rc = jack_set_process_callback
                (
                    clipointer, jack_process_rtmidi_input, &m_jack_data
                );
                if (rc != 0)
                {
                    m_error_string = func_message
                    (
                        "JACK error setting multi-client "
                        "process-input callback"
                    );
                    error(rterror::WARNING, m_error_string);
                }
            }
            else
            {
                bool ok = create_ringbuffer(JACK_RINGBUFFER_SIZE);
                if (ok)
                {
                    int rc = jack_set_process_callback
                    (
                        clipointer, jack_process_rtmidi_output, &m_jack_data
                    );
                    if (rc != 0)
                    {
                        m_error_string = func_message
                        (
                            "JACK error setting multi-client "
                            "process-output callback"
                        );
                        error(rterror::WARNING, m_error_string);
                    }
                }
            }
        }
    }
    apiprint("open_client_impl", "jack");
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
        apiprint("jack_client_close", "jack");
        client_handle(nullptr);
        if (rc != 0)
        {
            int index = get_bus_index();
            int id = parent_bus().get_port_id();
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
 *  Connects two named JACK ports.  First, we register the local port.  If
 *  this is nominally a local input port, it is really doing output, and this
 *  is the source-port name.  If this is nominally a local output port, it is
 *  really accepting input, and this is the destination-port name.
 *
 *  This code is disabled for now because the order of JACK setup calls that
 *  works is
 *
 *      -   jack_port_register()
 *      -   jack_activate()
 *      -   jack_connect()
 *
 *  So we have to break this up.
 *
 * \param input
 *      Indicates true if the port to register and connect is an input port,
 *      and false if the port is an output port.  Useful macros for readability:
 *      SEQ64_MIDI_INPUT_PORT and SEQ64_MIDI_OUTPUT_PORT.
 *
 * \param srcportname
 *      Provides the destination port-name for the connection.  For input,
 *      this should be the name associated with the JACK client handle; it is
 *      the port that gets registered.  For output, this should be the full
 *      name of the port that was enumerated at start-up.  The JackPortFlags
 *      of the source port must include JackPortIsOutput.
 *
 * \param destportname
 *      For input, this should be full name of port that was enumerated at
 *      start-up.  For output, this should be the name associated with the
 *      JACK client handle; it is the port that gets registered.  The
 *      JackPortFlags of the destination port must include JackPortIsInput.
 *      Now, if this name is empty, that basically means that there is no such
 *      port, and we create a virtual port in this case.  So jack_connect() is
 *      not called in this case.  We do not treat this case as an error.
 *
 * \return
 *      If the jack_connect() call succeeds, true is returned.  If the port is
 *      a virtual (manual) port, then it is not connected, and true is
 *      returned without any action.
 */

bool
midi_jack::connect_port
(
    bool input,
    const std::string & srcportname,
    const std::string & destportname
)
{
    bool result = true;
    if (! is_virtual_port())
    {
        result = ! srcportname.empty() && ! destportname.empty();
        if (result)
        {
            int rc = jack_connect
            (
                client_handle(), srcportname.c_str(), destportname.c_str()
            );
#ifdef SEQ64_SHOW_API_CALLS_TMI
            printf("Parent bus:\n");
            parent_bus().show_bus_values();
            printf
            (
                "jack_connect(src = '%s', dest = '%s')\n",
                srcportname.c_str(), destportname.c_str()
            );
#endif
            result = rc == 0;
            if (! result)
            {
                if (rc == EEXIST)
                {
                    /*
                     * Probably not worth emitting a warning to the console.
                     */
                }
                else
                {
                    m_error_string = func_message("JACK error connecting port ");
                    m_error_string += input ? "input '" : "output '";
                    m_error_string += srcportname;
                    m_error_string += "' to '";
                    m_error_string += destportname;
                    m_error_string += "'";
                    error(rterror::DRIVER_ERROR, m_error_string);
                }
            }
        }
    }
    return result;
}

/**
 *  Registers a named JACK port.  We made this function to encapsulate some
 *  otherwise cut-and-paste functionality.
 *
 *  For jack_port_register(), the port-name must be the actual port-name, not
 *  the full-port name ("busname:portname").
 *
 *  Note that the buffer size of non-built-in port type is 0, and so it is
 *  ignored.
 *
 * \tricky
 *      If we are registering an input port, this means that we got the input
 *      port from the system.  In order to connect to that port, we have
 *      to register as an output port, even though the application calls it in
 *      input port (midi_in_jack).  Confusing, but the same thing was implicit
 *      in the ALSA implementation, and so we have to apply that same setup
 *      here.
 *
 * \param input
 *      Indicates true if the port to register input port, and false if the
 *      port is an output port.  Two macros can be used for this purpose:
 *      SEQ64_MIDI_INPUT_PORT and SEQ64_MIDI_OUTPUT_PORT.
 *
 * \param portname
 *      Provides the local name of the port.  This is the full name
 *      ("clientname:portname"), but the "portname" part is extracted to fit
 *      the requirements of the jack_port_register() function.  There is an
 *      issue here when a2jmidid is running.  We may see a client name of
 *      "seq64", but a port name of "a2j Midi Through [1] capture: Midi
 *      Through Port-0", which as a colon in it.  What to do?  Just not
 *      extract the port name from the portname parameter.  If we have an
 *      issue here, we'll ahve to fix it in the caller.
 */

bool
midi_jack::register_port (bool input, const std::string & portname)
{
    bool result = not_nullptr(port_handle());
    if (! result)
    {
        /*
         * See description of the portname parameter above.
         *
         * std::string shortname = extract_port_name(portname);
         */

        std::string shortname = portname;
        unsigned long flag = input ? JackPortIsInput : JackPortIsOutput;
        unsigned long buffsize = 0;
        jack_port_t * p = jack_port_register
        (
            client_handle(), shortname.c_str(), JACK_DEFAULT_MIDI_TYPE,
            flag, buffsize
        );
#ifdef SEQ64_SHOW_API_CALLS_TMI
        std::string flagname = input ? "JackPortIsOutput" : "JackPortIsInput" ;
        printf("Parent bus:\n");
        parent_bus().show_bus_values();
        printf
        (
            "%lx = jack_port_register(name = '%s', flag(%s) = '%s')\n",
            (unsigned long)(p), shortname.c_str(), input ? "input" : "output",
            flagname.c_str()
        );
        printf("  Full port name: '%s'\n", portname.c_str());
#endif
        if (not_nullptr(p))
        {
            port_handle(p);
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
    if (not_nullptr(client_handle()) && not_nullptr(port_handle()))
    {
        jack_port_unregister(client_handle(), port_handle());
        port_handle(nullptr);
        apiprint("jack_port_unregister", "jack");
    }
}

/**
 *  Creates the JACK ring-buffers.
 */

bool
midi_jack::create_ringbuffer (size_t rbsize)
{
    bool result = rbsize > 0;
    if (result)
    {
        jack_ringbuffer_t * rb = jack_ringbuffer_create(rbsize);
        if (not_nullptr(rb))
        {
            m_jack_data.m_jack_buffsize = rb;
            rb = jack_ringbuffer_create(rbsize);
            if (not_nullptr(rb))
                m_jack_data.m_jack_buffmessage = rb;
            else
                result = false;
        }
        else
            result = false;

        if (! result)
        {
            m_error_string = func_message("JACK ringbuffer error");
            error(rterror::WARNING, m_error_string);
        }
    }
    return result;
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
 * \param parentbus
 *      Provides the buss object that determines buss-specific parameters of
 *      this class.
 *
 * \param masterinfo
 *      Provides information about the JACK system as found on this machine.
 */

midi_in_jack::midi_in_jack (midibus & parentbus, midi_info & masterinfo)
 :
    midi_jack       (parentbus, masterinfo),
    m_client_name   ()
{
    /*
     * Currently, we cannot initialize here because the clientname is empty.
     * It is retrieved in api_init_in().
     *
     * if (multi_client())
     *     (void) initialize(clientname);
     *
     * Hook in the input data.  The JACK port pointer will get set in
     * api_init_in() or api_init_out() when the port is registered.
     */

    m_jack_data.m_jack_rtmidiin = input_data();
}

/**
 *  Checks the rtmidi_in_data queue for the number of items in the queue.
 *
 *  WE MAY NEED LOCKING.
 *
 * \return
 *      Returns the value of rtindata->queue().count(), unless the caller is
 *      using an rtmidi callback function, in which case 0 is always returned.
 */

int
midi_in_jack::api_poll_for_midi ()
{
    rtmidi_in_data * rtindata = m_jack_data.m_jack_rtmidiin;
    if (rtindata->using_callback())
    {
        millisleep(1);
        return 0;
    }
    else
    {
        millisleep(1);
        return rtindata->queue().count();
    }
}

/**
 *  Gets a MIDI event.  This implementation gets a midi_message off the front
 *  of the queue and converts it to an sequence64 event.
 *
 * \param inev
 *      Provides the destination for the MIDI event.
 *
 * \return
 *      Returns true if a MIDI event was obtained, indicating that the return
 *      parameter can be used.
 */

bool
midi_in_jack::api_get_midi_event (event * inev)
{
    rtmidi_in_data * rtindata = m_jack_data.m_jack_rtmidiin;
    bool result = ! rtindata->queue().empty();
    if (result)
    {
        midi_message mm = rtindata->queue().pop_front();
        inev->set_timestamp(mm.timestamp());
        if (mm.count() == 3)
        {
            inev->set_status_keep_channel(mm[0]);
            inev->set_data(mm[1], mm[2]);

            /*
             *  Some keyboards send Note On with velocity 0 for Note Off, so
             *  we take care of that situation here by creating a Note Off
             *  event, with the channel nybble preserved. Note that we call
             *  event :: set_status_keep_channel() instead of using stazed's
             *  set_status function with the "record" parameter.  A little
             *  more confusing, but faster. BACKPORTED BUG FIX.
             */

            if (inev->is_note_off_recorded())
                inev->set_status_keep_channel(EVENT_NOTE_OFF);
        }
        else if (mm.count() == 2)
        {
            inev->set_status_keep_channel(mm[0]);
            inev->set_data(mm[1]);
        }
        else
        {
            infoprint("SysEx information encountered?");

#ifdef USE_SYSEX_PROCESSING                 /* currently disabled           */

            /**
             *  We will only get EVENT_SYSEX on the first packet of MIDI data;
             *  the rest we have to poll for.  SysEx processing is currently
             *  disabled.  The code that follows has a big bug!
             */

            midibyte buffer[0x1000];        /* temporary buffer for Sysex   */
            inev->set_sysex_size(bytes);
            if (buffer[0] == EVENT_MIDI_SYSEX)
            {
                inev->restart_sysex();      /* set up for sysex if needed   */
                sysex = inev->append_sysex(buffer, bytes);
            }
#endif
        }
    }
    return result;
}

/**
 *  Destructor.  Currently the base class closes the port, closes the JACK
 *  client, and cleans up the API data structure.
 */

midi_in_jack::~midi_in_jack()
{
    // No code yet
}

/*
 * API: JACK Class Definitions: midi_out_jack
 */

/**
 *  Principal constructor.  For Sequencer64, we don't current need to create
 *  a midi_out_jack object; all that is needed is created via the
 *  api_init_out*() functions.
 *
 * \param parentbus
 *      Provides the buss object that determines buss-specific parameters of
 *      this class.
 *
 * \param masterinfo
 *      Provides information about the JACK system as found on this machine.
 */

midi_out_jack::midi_out_jack (midibus & parentbus, midi_info & masterinfo)
 :
    midi_jack       (parentbus, masterinfo)
{
    /*
     * Currently, we cannot initialize here because the clientname is empty.
     * It is retrieved in api_init_out().
     *
     * if (multi_client())
     *     (void) initialize(clientname);
     */
}

/**
 *  Destructor.  Currently the base class closes the port, closes the JACK
 *  client, and cleans up the API data structure.
 */

midi_out_jack::~midi_out_jack ()
{
    // No code yet
}

/**
 *  Sends a JACK MIDI output message.  It writes the full message size and
 *  the message itself to the JACK ring buffer.
 *
 * \param message
 *      Provides the vector of message bytes to send.
 *
 * \return
 *      Returns true if the buffer message and buffer size seem to be written
 *      correctly.
 */

bool
midi_out_jack::send_message (const midi_message::container & message)
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
    apiprint("send_message", "jack");
    return (count1 > 0) && (count2 > 0);
}

}           // namespace seq64

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

