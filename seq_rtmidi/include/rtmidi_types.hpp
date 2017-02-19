#ifndef SEQ64_RTMIDI_TYPES_HPP
#define SEQ64_RTMIDI_TYPES_HPP

/**
 * \file          rtmidi_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2017-02-19
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition, and slightly easier to read.
 */

#include <string>                           /* std::string                  */
#include <vector>                           /* std::vector container        */

#include "midibyte.hpp"                     /* seq64::midibyte typedef      */

/**
 * This was the version of the RtMidi library from which this reimplementation
 * was forked.  However, the divergence from RtMidi by this library is now
 * very great... only the idea of selecting the MIDI API at runtime, and the
 * queuing and call-back mechanism  have been preserved.
 */

#define SEQ64_RTMIDI_VERSION "2.1.1"        /* revision at fork time        */

/**
 *  Macros for selecting input versus output ports in a more obvious way.
 *  These items are needed for the midi_mode() setter function.  Note that
 *  midi_mode() has no functionality in the midi_api base class, which has a
 *  number of such stub functions so that we can use the midi_info and midi_api
 *  derived classes.  Tested by the is_input_port() functions.
 */

#define SEQ64_MIDI_OUTPUT_PORT  false       /* the MIDI mode is not input   */
#define SEQ64_MIDI_INPUT_PORT   true        /* the MIDI mode is input       */

/**
 *  Macros for selecting virtual versus normal ports in a more obvious way.
 *  Used in the rtmidi midibus constructors.  Tested by the is_virtual_port()
 *  functions.  But note the overload usage of the SEQ64_MIDI_NORMAL_PORT
 *  macro.
 */

#define SEQ64_MIDI_NORMAL_PORT  false       /* the MIDI port is not virtual */
#define SEQ64_MIDI_VIRTUAL_PORT true        /* the MIDI port is virtual     */

/**
 *  Macros for indicating if the port is a built-in system port versus a port
 *  that exists because a MIDI device is plugged in or some application has
 *  set up a virtual port.  Tested by the is_system_port() functions.  But
 *  note the overload usage of the SEQ64_MIDI_NORMAL_PORT macro.
 */

#define SEQ64_MIDI_SYSTEM_PORT  true        /* API always exposes this port */

/**
 *  Like the SEQ64_NO_BUS and SEQ64_NO_PORT macros in
 *  libseq64/include/app_limits.h, this value indicates an unspecified or
 *  invalid index into the list of available ports.
 */

#define SEQ64_NO_INDEX          (-1)        /* good values start at 0       */

/**
 *  Default size of the MIDI queue.
 */

#define SEQ64_DEFAULT_QUEUE_SIZE    100

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A macro to prepend a fully qualified function name to a string.  Cannot
 *  get circular reference to message_concatenate() resolved!  In fact any
 *  true functions added to easy_macros are unresolved.  WTF!?
 *
 * #define func_message(x)         seq64::message_concatenate(__func__, x)

extern std::string message_concatenate (const char * m1, const char * m2);
extern bool info_message (const std::string & msg);
extern bool error_message (const std::string & msg);

 */

/**
 *    MIDI API specifier arguments.  These items used to be nested in
 *    the rtmidi class, but that only worked when RtMidi.cpp/h were
 *    large monolithic modules.
 */

enum rtmidi_api
{
    RTMIDI_API_UNSPECIFIED,     /**< Search for a working compiled API.     */
    RTMIDI_API_LINUX_ALSA,      /**< Advanced Linux Sound Architecture API. */
    RTMIDI_API_UNIX_JACK,       /**< JACK Low-Latency MIDI Server API.      */

#ifdef USE_RTMIDI_API_ALL

    /*
     * We're not supporting these until we get a simplified
     * sequencer64-friendly API worked out.
     */

    RTMIDI_API_MACOSX_CORE,     /**< Macintosh OS-X Core Midi API.          */
    RTMIDI_API_WINDOWS_MM,      /**< Microsoft Multimedia MIDI API.         */
    RTMIDI_API_DUMMY,           /**< A compilable but non-functional API.   */

#endif

    RTMIDI_API_MAXIMUM          /**< A count of APIs; an erroneous value.   */

};

/**
 *  Provides a handy capsule for a MIDI message, based on the
 *  std::vector<unsigned char> data type from the RtMidi project.
 *
 *  Please note that the ALSA module in sequencer64's rtmidi infrastructure
 *  uses the seq64::event rather than the seq64::midi_message object.
 *  For the moment, we will translate between them until we have the
 *  interactions between the old and new modules under control.
 */

class midi_message
{

public:

    /**
     *  Holds the data of the MIDI message.  Callers should use
     *  midi_message::container rather than using the vector directly.
     *  Bytes are added by the push() function, and are safely accessed
     *  (with bounds-checking) by operator [].
     */

    typedef std::vector<midibyte> container;

private:

    /**
     *  Holds the event status and data bytes.
     */

    container m_bytes;

    /**
     *  Holds the (optional) timestamp of the MIDI message.
     */

    double m_timestamp;

public:

    midi_message ();

    midibyte operator [] (int i) const
    {
        return (i >= 0 && i < int(m_bytes.size())) ? m_bytes[i] : 0 ;
    }

    midibyte & at (int i)
    {
        return m_bytes.at(i);       /* can throw an exception */
    }

    const midibyte & at (int i) const
    {
        return m_bytes.at(i);       /* can throw an exception */
    }

    const char * array () const
    {
        return reinterpret_cast<const char *>(&m_bytes[0]);
    }

    int count () const
    {
        return int(m_bytes.size());
    }

    bool empty () const
    {
        return m_bytes.empty();
    }

    void push (midibyte b)
    {
        m_bytes.push_back(b);
    }

    double timestamp () const
    {
        return m_timestamp;
    }

    void timestamp (double t)
    {
        m_timestamp = t;
    }

};          // class midi_message

/**
 *  MIDI caller callback function type definition.  Used to be nested in the
 *  rtmidi_in class.  The timestamp parameter has been folded into the
 *  midi_message class (a wrapper for std::vector<unsigned char>), and the
 *  pointer has been replaced by a reference.
 */

typedef void (* rtmidi_callback_t)
(
    midi_message & message,             /* includes the timestamp already */
    void * userdata
);

/**
 *  Provides a queue of midi_message structures.  This entity used to be a
 *  plain structure nested in the midi_in_api class.  We made it a class to
 *  encapsulate some common operations to save a burden on the callers.
 */

class midi_queue
{

private:

    unsigned m_front;
    unsigned m_back;
    unsigned m_size;
    unsigned m_ring_size;
    midi_message * m_ring;

public:

    midi_queue ();
    ~midi_queue ();

    /**
     * \getter m_size == 0
     */

    bool empty () const
    {
        return m_size == 0;
    }

    /**
     * \getter m_size == 0
     */

    int count () const
    {
        return int(m_size);
    }

    /**
     * \return
     *      Returns true if the queue size is at maximum.
     */

    bool full () const
    {
        return m_size == m_ring_size;
    }

    bool add (const midi_message & mmsg);
    void pop ();
    midi_message pop_front ();
    void allocate (unsigned queuesize = SEQ64_DEFAULT_QUEUE_SIZE);
    void deallocate ();

    /**
     * \getter m_ring[m_front]
     */

    const midi_message & front () const
    {
        return m_ring[m_front];
    }

};

/**
 *  The rtmidi_in_data structure is used to pass private class data to the
 *  MIDI input handling function or thread.  Used to be nested in the
 *  rtmidi_in class.
 */

class rtmidi_in_data
{

private:

    midi_queue m_queue;
    midi_message m_message;
    midibyte m_ignore_flags;
    bool m_do_input;
    bool m_first_message;
    void * m_api_data;
    bool m_using_callback;
    rtmidi_callback_t m_user_callback;
    void * m_user_data;
    bool m_continue_sysex;

public:

    rtmidi_in_data ();

    /**
     * \getter m_queue const
     */

    const midi_queue & queue () const
    {
        return m_queue;
    }

    /**
     * \getter m_queue non-const
     */

    midi_queue & queue ()
    {
        return m_queue;
    }

    const midi_message & message () const
    {
        return m_message;
    }

    midi_message & message ()
    {
        return m_message;
    }

    midibyte ignore_flags () const
    {
        return m_ignore_flags;
    }

    bool test_ignore_flags (midibyte testbits)
    {
        return bool(m_ignore_flags & testbits);
    }

    void ignore_flags (midibyte setbits)
    {
        m_ignore_flags = setbits;
    }

    bool do_input () const
    {
        return m_do_input;
    }

    void do_input (bool flag)
    {
        m_do_input = flag;
    }

    bool first_message () const
    {
        return m_first_message;
    }

    void first_message (bool flag)
    {
        m_first_message = flag;
    }

    bool continue_sysex () const
    {
        return m_continue_sysex;
    }

    void continue_sysex (bool flag)
    {
        m_continue_sysex = flag;
    }

    bool using_callback () const
    {
        return m_using_callback;
    }

    void using_callback (bool flag)
    {
        m_using_callback = flag;
    }

    /**
     * \getter m_api_data const
     */

    const void * api_data () const
    {
        return m_api_data;
    }

    /**
     * \getter m_api_data non-const
     */

    void * api_data ()
    {
        return m_api_data;
    }

    void api_data (void * dataptr)
    {
        m_api_data = dataptr;
    }

    /**
     * \getter m_user_data const
     */

    const void * user_data () const
    {
        return m_user_data;
    }

    /**
     * \getter m_user_data const
     */

    void * user_data ()
    {
        return m_user_data;
    }

    /**
     * \setter m_user_data
     */

    void user_data (void * dataptr)
    {
        m_user_data = dataptr;
    }

    /**
     * \getter m_user_callback
     */

    rtmidi_callback_t user_callback () const
    {
        return m_user_callback;
    }

    /**
     * \setter m_user_callback
     *      This should be done immediately after opening the port to avoid
     *      having incoming messages written to the queue instead of sent to
     *      the callback function.
     */

    void user_callback (rtmidi_callback_t cbptr)
    {
        m_user_callback = cbptr;
    }

};          // class rtmidi_in_data

}           // namespace seq64

#endif      // SEQ64_RTMIDI_TYPES_HPP

/*
 * rtmidi_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

