#ifndef SEQ64_RTMIDI_HPP
#define SEQ64_RTMIDI_HPP

/**
 * \file          rtmidi.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-28
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  The big difference between this class (seq64::rtmidi) and
 *  seq64::rtmidi_info is that it gets information via midi_api-derived
 *  functions, while the latter gets if via midi_api_info-derived functions.
 */

#include <string>

#include "seq64_rtmidi_features.h"
#include "midi_api.hpp"                     /* seq64::midi[_in][_out]_api   */
#include "easy_macros.h"                    /* platform macros for compiler */
#include "rterror.hpp"                      /* seq64::rterror               */
#include "rtmidi_types.hpp"                 /* seq64::rtmidi_api etc.       */
#include "rtmidi_info.hpp"                  /* seq64::rtmidi_info           */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The main class of the rtmidi API.  We moved the enum Api definition into
 *  the new rtmidi_types.hpp module to make refactoring the code easier.
 */

class rtmidi : public midi_api
{
    friend class midibus;

private:

    /**
     *  Holds a reference to the "global" midi_info wrapper object.
     *  Unlike the original RtMidi library, this library separates the
     *  port-enumeration code ("info") from the port-usage code ("api").
     *
     *  We might make it a static object at some point.
     */

    rtmidi_info & m_midi_info;

    /**
     *  Points to the API I/O object (e.g. midi_alsa or midi_jack) for which
     *  this class is a wrapper.
     */

    midi_api * m_midi_api;

protected:

    rtmidi (rtmidi_info & info, int index = SEQ64_NO_INDEX);
    virtual ~rtmidi ();

public:             // NEW APIS

    void api_play (event * e24, midibyte channel)
    {
        get_api()->api_play(e24, channel);
    }

    void api_continue_from (midipulse tick, midipulse beats)
    {
        get_api()->api_continue_from(tick, beats);
    }

    void api_start ()
    {
        get_api()->api_start();
    }

    void api_stop ()
    {
        get_api()->api_stop();
    }

    void api_clock (midipulse tick)
    {
        get_api()->api_clock(tick);
    }

    virtual bool api_init_out ()
    {
        return get_api()->api_init_out();
    }

    virtual bool api_init_out_sub ()
    {
        return get_api()->api_init_out_sub();
    }

    virtual bool api_init_in ()
    {
        return get_api()->api_init_in();
    }

    virtual bool api_init_in_sub ()
    {
        return get_api()->api_init_in_sub();
    }

    virtual bool api_deinit_in ()
    {
        return get_api()->api_deinit_in();
    }

    virtual void api_sysex (event * e24)
    {
        get_api()->api_sysex(e24);
    }

    virtual void api_flush ()
    {
        get_api()->api_flush();
    }

public:

    /**
     *  Returns true if a port is open and false if not.
     */

    virtual bool is_port_open () const
    {
       return get_api()->is_port_open();
    }

    /**
     *  Gets the buss/client ID for a MIDI interfaces.  This is the left-hand
     *  side of a X:Y pair (such as 128:0).
     *
     *  This function is a new part of the RtMidi interface.
     *
     * \param index
     *      The ordinal index of the desired interface to look up.
     *
     * \return
     *      Returns the buss/client value as provided by the selected API.
     */

    virtual int get_bus_id ()   // (int index)
    {
        return get_api()->get_bus_id();  // (index);
    }

    virtual std::string get_bus_name ()
    {
        return get_api()->bus_name();
    }

    virtual int get_port_id ()
    {
        return get_api()->get_port_id();
    }

    virtual std::string get_port_name ()
    {
        return get_api()->port_name();
    }

    int get_port_count ()
    {
        return m_midi_info.get_port_count();
    }

    /**
     * \getter m_midi_api const version
     */

    const midi_api * get_api () const
    {
        return m_midi_api;
    }

    /**
     * \getter m_midi_api non-const version
     */

    midi_api * get_api ()
    {
        return m_midi_api;
    }

protected:

    /**
     * \setter m_midi_api
     */

    void set_api (midi_api * ma)
    {
        if (not_nullptr(ma))
            m_midi_api = ma;
    }

    /**
     * \setter m_midi_api
     */

    void delete_api ()
    {
        if (not_nullptr(m_midi_api))
        {
            delete m_midi_api;
            m_midi_api = nullptr;
        }
    }

protected:

};          // class rtmidi

/**
 *  A realtime MIDI input class.
 *
 *  This class provides a common, platform-independent API for
 *  realtime MIDI input.  It allows access to a single MIDI input
 *  port.  Incoming MIDI messages are either saved to a queue for
 *  retrieval using the get_message() function or immediately passed to
 *  a user-specified callback function.  Create multiple instances of
 *  this class to connect to more than one MIDI device at the same
 *  time.  With the OS-X, Linux ALSA, and JACK MIDI APIs, it is also
 *  possible to open a virtual input port to which other MIDI software
 *  clients can connect.
 */

class rtmidi_in : public rtmidi
{

public:

    /**
     *  Default constructor that allows an optional api, client name and queue
     *  size.
     *
     *  An exception will be thrown if a MIDI system initialization
     *  error occurs.  The queue size defines the maximum number of
     *  messages that can be held in the MIDI queue (when not using a
     *  callback function).  If the queue size limit is reached,
     *  incoming messages will be ignored.
     *
     *  If no API argument is specified and multiple API support has been
     *  compiled, the default order of use is ALSA, JACK (Linux) and CORE,
     *  JACK (OS-X).
     *
     * \param api
     *      An optional API id can be specified.
     *
     * \param clientname
     *      An optional client name can be specified. This will be used to
     *      group the ports that are created by the application.
     *
     * \param queuesizelimit
     *      An optional size of the MIDI input queue can be specified.
     */

    rtmidi_in
    (
        rtmidi_info & info,
        int index = SEQ64_NO_INDEX
    );

    /**
     *  If a MIDI connection is still open, it will be closed by the
     *  destructor.
     */

    virtual ~rtmidi_in ();

protected:

    void openmidi_api
    (
        rtmidi_api api, rtmidi_info & info, int index = SEQ64_NO_INDEX
    );

};

/**
 *  A realtime MIDI output class.
 *
 *  This class provides a common, platform-independent API for MIDI
 *  output.  It allows one to probe available MIDI output ports, to
 *  connect to one such port, and to send MIDI bytes immediately over
 *  the connection.  Create multiple instances of this class to
 *  connect to more than one MIDI device at the same time.  With the
 *  OS-X, Linux ALSA and JACK MIDI APIs, it is also possible to open a
 *  virtual port to which other MIDI software clients can connect.
 */

class rtmidi_out : public rtmidi
{

public:

    /**
     *  Default constructor that allows an optional client name.
     *
     *  An exception will be thrown if a MIDI system initialization error occurs.
     *
     *  If no API argument is specified and multiple API support has been
     *  compiled, the default order of use is ALSA, JACK (Linux) and CORE,
     *  JACK (OS-X).
     */

    rtmidi_out
    (
        rtmidi_info & info,
        int index = SEQ64_NO_INDEX
    );

    /**
     *  The destructor closes any open MIDI connections.
     */

    virtual ~rtmidi_out ();

protected:

    void openmidi_api
    (
        rtmidi_api api, rtmidi_info & info, int index = SEQ64_NO_INDEX
    );

};

}           // namespace seq64

#endif      // SEQ64_RTMIDI_HPP

/*
 * rtmidi.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

