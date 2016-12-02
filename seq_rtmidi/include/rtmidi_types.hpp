#ifndef SEQ64_RTMIDI_TYPES_HPP
#define SEQ64_RTMIDI_TYPES_HPP

/**
 * \file          rtmidi_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2016-12-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition, and slightly easier to read.
 */

#include <vector>                           /* std::vector container        */

#include "midibyte.hpp"

/*
 * This was the version of the RtMidi library from which this reimplementation
 * was forked.
 */

#define SEQ64_RTMIDI_VERSION "2.1.1"        /* revision at fork time        */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *    MIDI API specifier arguments.  These items used to be nested in
 *    the rtmidi class, but that only worked when RtMidi.cpp/h were
 *    large monolithic modules.
 */

enum rtmidi_api
{
    RTMIDI_API_UNSPECIFIED,     /**< Search for a working compiled API.      */
    RTMIDI_API_LINUX_ALSA,      /**< Advanced Linux Sound Architecture API.  */
    RTMIDI_API_UNIX_JACK,       /**< JACK Low-Latency MIDI Server API.       */
    RTMIDI_API_MACOSX_CORE,     /**< Macintosh OS-X Core Midi API.           */
    RTMIDI_API_WINDOWS_MM,      /**< Microsoft Multimedia MIDI API.          */
    RTMIDI_API_DUMMY            /**< A compilable but non-functional API.    */
};

/**
 *  MIDI caller callback function type definition.  Used to be nested in the
 *  rtmidi_in class.
 */

typedef void (* rtmidi_callback_t)
(
    double timeStamp,
    std::vector<midibyte> & message,
    void * userdata
);

/**
 *  A MIDI structure used internally by the class to store incoming messages.
 *  Each message represents one and only one MIDI message.  This entity used
 *  to be nested in the midi_in_api class.
 */

struct midi_message
{
    friend class midi_queue;

    std::vector<midibyte> bytes;
    double timeStamp;

    midi_message () : bytes(), timeStamp(0.0)
    {
        // no body
    }
};

/**
 *  Provides a queue of midi_message structures.  This entity used to be
 *  nested in the midi_in_api class.
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

    bool empty () const
    {
        return m_size == 0;
    }

    bool full () const
    {
        return m_size == m_ring_size;
    }

    bool add (const midi_message & mmsg);
    void pop ();
    void allocate (unsigned queuesize);
    void deallocate ();

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

    midi_queue & queue ()
    {
        return m_queue;
    }

    const midi_message & message () const
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
        m_first_message = false;
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

    void * api_data ()
    {
        return m_api_data;
    }

    void * user_data ()
    {
        return m_user_data;
    }

    void user_data (void * dataptr)
    {
        m_user_data = dataptr;
    }

    rtmidi_callback_t user_callback ()
    {
        return m_user_callback;
    }

    void user_callback (rtmidi_callback_t cbptr)
    {
        m_user_callback = cbptr;
    }

};

}           // namespace seq64

#endif      // SEQ64_RTMIDI_TYPES_HPP

/*
 * rtmidi_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

