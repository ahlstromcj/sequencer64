/**
 * \file          midi_win.cpp
 *
 *    A class for realtime MIDI input/output via the Window MM subsystem.
 *
 * \library       sequencer64 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2017-08-20
 * \updates       2017-06-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 * \deprecated
 *      We have decided to use the PortMidi re-implementation for Sequencer64
 *      for Windows.
 *
 *  Written primarily by Alexander Svetalkin, with updates for delta time by
 *  Gary Scavone, April 2011.
 *
 *  In this refactoring, we have to warp the RtMidi model, where ports are
 *  opened directly by the application, to the Sequencer64 midibus model,
 *  where the port object is created, but initialized after creation.  This
 *  proved very challenging -- it took a long time to get the midi_alsa
 *  implementation working, and still more time to get the midi_win
 *  implementation solid.  So to call this code "rtmidi" code is slightly
 *  misleading.
 *
 *  API information deciphered from:
 *
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/multimed/htm/_win32_midi_reference.asp
 *
 *  Thanks to Jean-Baptiste Berruchon for the sysex code.
 */

#error Internal RtMidi for Windows obsolete, use internal PortMidi instead.

#include <windows.h>
#include <mmsystem.h>
#include <sstream>

#include "calculations.hpp"             /* seq64::extract_port_name()       */
#include "event.hpp"                    /* seq64::event from main library   */
#include "midibus_rm.hpp"               /* seq64::midibus for rtmidi        */
#include "midi_win.hpp"                 /* seq64::midi_win                  */
#include "settings.hpp"                 /* seq64::rc() accessor function    */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * MIDI Windows MM base class
 */

/**
 *
 * \param parentbus
 *      Provides a reference to the midibus that represents this object.
 *
 * \param masterinfo
 *      Provides a reference to the midi_win_info object that may provide
 *      extra informatino that is needed by this port.  Too many entities!
 *
 * \param multiclient
 *      If true, use multiple Windows MM clients.
 */

midi_win::midi_win
(
    midibus & parentbus,
    midi_info & masterinfo,
    bool multiclient
) :
    midi_api            (parentbus, masterinfo),
    m_remote_port_name  ()
{
    client_handle(...);
}

/**
 *  This could be a rote empty destructor if we offload this destruction to the
 *  midi_win_data structure.  However, other than the initialization, that
 *  structure is currently "dumb".
 */

midi_win::~midi_win ()
{
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual ports" option is not in force.  This code is basically what was
 *  done by midi_out_win::open_port() in RtMidi.
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
 *      Returns true unless setting up Windows MM MIDI failed in some way.
 */

bool
midi_win::api_init_out ()
{
    bool result = true;
    std::string remoteportname = connect_name();    /* "bus:port"   */
    remote_port_name(remoteportname);
    if (result)
    {
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
 * \return
 *      Returns true if the function was successful, and sets the flag
 *      indicating that the port is open.
 */

bool
midi_win::api_init_in ()
{
    bool result = true;
    std::string remoteportname = connect_name();    /* "bus:port"       */
    remote_port_name(remoteportname);
    set_alt_name
    (
        rc().application_name(), rc().app_client_name(), remoteportname
    );
    parent_bus().set_alt_name           // TENTATIVE
    (
        rc().application_name(), rc().app_client_name(), remoteportname
    );







    if (result)
    {
        result = register_port(SEQ64_MIDI_INPUT_PORT, port_name());

        /*
         * Note that we cannot call connect_port() until activated, and cannot
         * be activated until all ports are properly set up.
         */
    }
    return result;
}

/**
 *  Assumes that the port has already been registered, and that Windows MM
 *  activation has already occurred.
 *
 * \return
 *      Returns true if all steps of the connection succeeded.
 */

bool
midi_win::api_connect ()
{
    std::string remotename = remote_port_name();
    std::string localname = connect_name();     /* modified!    */
    bool result;
    if (is_input_port())
        result = connect_port(SEQ64_MIDI_INPUT_PORT, remotename, localname);
    else
        result = connect_port(SEQ64_MIDI_OUTPUT_PORT, localname, remotename);

    if (result)
        set_port_open();

    return result;
}

/**
 *  Gets information directly from Windows MM.  The problem this function solves is
 *  that the midibus constructor for a virtual Windows MM port doesn't not have all
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
midi_win::set_virtual_name (int portid, const std::string & portname)
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
midi_win::api_init_out_sub ()
{
    master_midi_mode(SEQ64_MIDI_OUTPUT_PORT);    /* this is necessary */
    int portid = parent_bus().get_port_id();
    bool result = portid >= 0;
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
    return result;
}

/**
 *  Initializes a virtual/manual input port.
 *
 * \return
 *      Returns true if all steps of the initialization succeeded.
 */

bool
midi_win::api_init_in_sub ()
{
    master_midi_mode(SEQ64_MIDI_INPUT_PORT);
    int portid = parent_bus().get_port_id();
    bool result = portid >= 0;
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
    return result;
}

/**
 *  We could define these in the opposite order.
 */

bool
midi_win::api_deinit_in ()
{
    close_port();
    return true;
}

/**
 *  We could push the bytes of the event into a midibyte vector, as done in
 *  send_message().  The ALSA code (seq_alsamidi/src/midibus.cpp) sticks the
 *  event bytes in an array, which might be a little faster than using
 *  push_back(), but let's try the vector first.  The rtmidi code here is from
 *  midi_out_win::send_message().
 */

void
midi_win::api_play (event * e24, midibyte channel)
{
    midibyte status = e24->get_status() + (channel & 0x0F);
    midibyte d0, d1;
    e24->get_data(d0, d1);

    midi_message message;
    message.push(status);
    message.push(d0);
    if (e24->is_two_bytes())                    /* \change ca 2017-04-26 */
        message.push(d1);

#ifdef SEQ64_SHOW_API_CALLS_TMI
    printf("midi_win::play()\n");
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
            errprint("Windows MM api_play failed");
        }
    }
}

/**
 * \todo
 *      Flesh out this routine.
 */

void
midi_win::api_sysex (event * /* e24 */)
{
    // Will put this one off until later....
}

/**
 *  It seems like Windows MM doesn't have the concept of flushing event.
 */

void
midi_win::api_flush ()
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
midi_win::api_continue_from (midipulse tick, midipulse /*beats*/)
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
 *  Starts this Windows MM client and sends MIDI start.   Note that the
 *  jack_assistant code (which implements Windows MM transport) checks if
 *  Windows MM is
 *  running, but a check of the Windows MM client handle here should be enough.
 */

void
midi_win::api_start ()
{
    jack_transport_start(client_handle());
    send_byte(EVENT_MIDI_START);
    apiprint("jack_transport_start", "jack");
}

/**
 *  Stops this Windows MM client and sends MIDI stop.   Note that the jack_assistant
 *  code (which implements Windows MM transport) checks if Windows MM is running, but a
 *  check of the Windows MM client handle here should be enough.
 */

void
midi_win::api_stop ()
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
midi_win::api_clock (midipulse tick)
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
 *  and  Continue if the Windows MM transport state changed.
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
midi_win::send_byte (midibyte evbyte, midipulse tick)
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
            errprint("Windows MM send_byte() failed");
        }
    }
}

void
midi_win::api_set_ppqn (int /*ppqn*/)
{
    // No code needed yet
}

void
midi_win::api_set_beats_per_minute (midibpm /*bpm*/)
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
midi_win::api_get_port_name ()
{
    std::string result;
    if (not_nullptr(port_handle()))
        result = std::string(jack_port_name(port_handle()));

    return result;
}

/**
 *  Opens input or output Windows MM clients, sets up the input or output
 *  callback, and actives the Windows MM client.
 *
 * \param input
 *      True if an input connection is to be made, and false if an output
 *      connection is to be made.
 */

bool
midi_win::open_client_impl (bool input)
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
            // Not supported by Windows
        }
        set_multi_name(appname, clientname, rpname);
        parent_bus().set_multi_name(appname, clientname, rpname);

        const char * name = bus_name().c_str();                 // modified
        jack_client_t * clipointer = create_jack_client(name); // , uuid);

    rtmidi_in_data * data = (midi_in_api::rtmidi_in_data *) instance;
    midi_win_data * api_data = reinterpret_cast<midi_win_data *>
    (
        data->m_api_data
    );

        if (not_nullptr(clipointer))
        {
            client_handle(clipointer);          // midi_handle() too???
            if (input)
            {
                int rc = jack_set_process_callback
                (
                    clipointer, jack_process_rtmidi_input, &m_jack_data
                );
                if (rc != 0)
                {
                    m_error_string = func_message
                    (
                        "Windows MM error setting multi-client input callback"
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
                            "Windows MM error setting multi-client output callback"
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
 *  Closes the Windows MM client handle.
 */

void
midi_win::close_client ()
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
            m_error_string = func_message("Windows MM closing port #");
            m_error_string += std::to_string(index);
            m_error_string += " (id ";
            m_error_string += std::to_string(id);
            m_error_string += ")";
            error(rterror::DRIVER_ERROR, m_error_string);
        }
    }
}

/**
 *  Connects two named Windows MM ports.  First, we register the local port.  If
 *  this is nominally a local input port, it is really doing output, and this
 *  is the source-port name.  If this is nominally a local output port, it is
 *  really accepting input, and this is the destination-port name.
 *
 *  This code is disabled for now because the order of Windows MM setup calls that
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
 *      this should be the name associated with the Windows MM client handle; it is
 *      the port that gets registered.  For output, this should be the full
 *      name of the port that was enumerated at start-up.  The JackPortFlags
 *      of the source port must include JackPortIsOutput.
 *
 * \param destportname
 *      For input, this should be full name of port that was enumerated at
 *      start-up.  For output, this should be the name associated with the
 *      Windows MM client handle; it is the port that gets registered.  The
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
midi_win::connect_port
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
                    m_error_string = func_message("Windows MM error connecting port ");
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
 *  Registers a named Windows MM port.  We made this function to encapsulate
 *  some otherwise cut-and-paste functionality.
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
 *      input port (midi_in_win).  Confusing, but the same thing was implicit
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
midi_win::register_port (bool input, const std::string & portname)
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
            m_error_string = func_message("Windows MM error registering port");
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
midi_win::close_port ()
{
    if (not_nullptr(client_handle()) && not_nullptr(port_handle()))
    {
        jack_port_unregister(client_handle(), port_handle());
        port_handle(nullptr);
        apiprint("jack_port_unregister", "jack");
    }
}

/**
 *  Creates the Windows MM ring-buffers.
 */

bool
midi_win::create_ringbuffer (size_t rbsize)
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
            m_error_string = func_message("Windows MM ringbuffer error");
            error(rterror::WARNING, m_error_string);
        }
    }
    return result;
}

/*
 * MIDI Windows MM input class.
 */

/**
 *  Principal constructor.  For Sequencer64, we don't current need to create
 *  a midi_in_win object; all that is needed is created via the
 *  api_init_in*() functions.  Also, this constructor still needs to do
 *  something with queue size.
 *
 * \param parentbus
 *      Provides the buss object that determines buss-specific parameters of
 *      this class.
 *
 * \param masterinfo
 *      Provides information about the Windows MM system as found on this machine.
 */

midi_in_win::midi_in_win (midibus & parentbus, midi_info & masterinfo)
 :
    midi_win        (parentbus, masterinfo),
    m_client_name   ()
{
    // TODO
}

/**
 *  Checks the rtmidi_in_data queue for the number of items in the queue.
 *
 * \return
 *      Returns the value of rtindata->queue().count(), unless the caller is
 *      using an rtmidi callback function, in which case 0 is always returned.
 */

int
midi_in_win::api_poll_for_midi ()
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
 *  of the queue and converts it to a Sequencer64 event.
 *
 * \param inev
 *      Provides the destination for the MIDI event.
 *
 * \return
 *      Returns true if a MIDI event was obtained, indicating that the return
 *      parameter can be used.
 */

bool
midi_in_win::api_get_midi_event (event * inev)
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
             *  event::set_status_keep_channel() instead of using stazed's
             *  set_status function with the "record" parameter.  Also, we
             *  have to mask in the actual channel number.
             */

            if (inev->is_note_off_recorded())
            {
                midibyte channel = mm[0] & EVENT_GET_CHAN_MASK;
                midibyte status = EVENT_NOTE_OFF | channel;
                inev->set_status_keep_channel(status);
            }
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
 *  Destructor.  Currently the base class closes the port, closes the Windows
 *  MM
 *  client, and cleans up the API data structure.
 */

midi_in_win::~midi_in_win()
{
    // No code yet
}

/*
 * API: Windows MM Class Definitions: midi_out_win
 */

/**
 *  Principal constructor.  For Sequencer64, we don't current need to create
 *  a midi_out_win object; all that is needed is created via the
 *  api_init_out*() functions.
 *
 * \param parentbus
 *      Provides the buss object that determines buss-specific parameters of
 *      this class.
 *
 * \param masterinfo
 *      Provides information about the Windows MM system as found on this machine.
 */

midi_out_win::midi_out_win (midibus & parentbus, midi_info & masterinfo)
 :
    midi_win       (parentbus, masterinfo)
{
    // TODO
}

/**
 *  Destructor.  Currently the base class closes the port, closes the Windows
 *  MM
 *  client, and cleans up the API data structure.
 */

midi_out_win::~midi_out_win ()
{
    // No code yet
}

/**
 *  Sends a Windows MM MIDI output message.  It writes the full message size and
 *  the message itself to the Windows MM ring buffer.
 *
 * \param message
 *      Provides the MIDI message object, which contains the bytes to send.
 *
 * \return
 *      Returns true if the buffer message and buffer size seem to be written
 *      correctly.
 */

bool
midi_out_win::send_message (const midi_message & message)
{
    bool result = false;
    if (connected())
    {
        int nbytes = message.size();
        result = nbytes > 0;
        if (result)
        {
            int count = ....
            apiprint("send_message", "winmm");

            MMRESULT mmresult;
            midi_win_data * data = reinterpret_cast<midi_win_data *>
            (
                apidata ????
            );
            if (message.is_sysex())
            {
                char * buffer = malloc(nbytes);     /* for SysEx data   */
                if (is_nullptr(buffer)
                {
                    m_error_string = func_message("SysEx allocation failed");
                    error(rterror::MEMORY_ERROR, m_error_string);
                    return;
                }

                // Copy data to buffer.  Why?  Already an array accessor.

                for (unsigned i = 0; i < nbytes; ++i)
                    buffer[i] = message[i];

                // Create and prepare MIDIHDR structure.

                MIDIHDR sysex;
                sysex.lpData = (LPSTR) buffer;
                sysex.dwBufferLength = nbytes;
                sysex.dwFlags = 0;
                mmresult = midiOutPrepareHeader
                (
                    data->m_win_out_handle,  &sysex, sizeof MIDIHDR
                );
                if (mmresult != MMSYSERR_NOERROR)
                {
                    free(buffer);
                    m_error_string = func_message("SysEx header prep failed");
                    error(rterror::DRIVER_ERROR, m_error_string);
                    return;
                }

                mmresult = midiOutLongMsg   // Send the message.
                (
                    data->m_win_out_handle, &sysex, sizeof MIDIHDR
                );
                if (result != MMSYSERR_NOERROR)
                {
                    free(buffer);
                    m_error_string = func_message("SysEx send failed");
                    error(rterror::DRIVER_ERROR, m_error_string);
                    return;
                }

                // Unprepare the buffer and MIDIHDR.

                while
                (
                    MIDIERR_STILLPLAYING == midiOutUnprepareHeader
                    (
                        data->m_win_out_handle, &sysex, sizeof MIDIHDR
                    )
                )
                {
                    Sleep(1);
                }
                free(buffer);
            }
            else
            {
                // Channel or system message.
                // Make sure the message size isn't too big.

                if (nbytes > 3)
                {
                    m_error_string = func_message("Illegal MIDI message size");
                    error(rterror::DRIVER_ERROR, m_error_string);
                    return;
                }

                // Pack MIDI bytes into double word.
                // Then send the message immediately.

                DWORD packet;
                unsigned char * ptr = (unsigned char *) &packet;
                for (unsigned i = 0; i < nbytes; ++i)
                {
                    *ptr = message[i];
                    ++ptr;
                }

                mmresult = midiOutShortMsg(data->m_win_out_handle, packet);
                if (result != MMSYSERR_NOERROR)
                {
                    m_error_string = func_message("error sending MIDI message");
                    error(rterror::DRIVER_ERROR, m_error_string);
                }
            }
            // result = (count1 > 0) && (count2 > 0);
        }
        else
        {
            m_error_string = func_message("message argument is empty");
            error(rterror::WARNING, m_error_string);
        }
    }
    return result;
}

}           // namespace seq64

/*
 * midi_win.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

