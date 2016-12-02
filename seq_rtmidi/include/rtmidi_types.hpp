#ifndef SEQ64_RTMIDI_TYPES_HPP
#define SEQ64_RTMIDI_TYPES_HPP

/**
 * \file          rtmidi_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2016-11-20
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

struct rtmidi_in_data
{
    midi_queue queue;
    midi_message message;
    midibyte ignoreFlags;
    bool doInput;
    bool firstMessage;
    void * apiData;
    bool usingCallback;
    rtmidi_callback_t userCallback;
    void * userdata;
    bool continueSysex;

    rtmidi_in_data()
     :
        ignoreFlags(7),
        doInput(false),
        firstMessage(true),
        apiData(0),
        usingCallback(false),
        userCallback(0),
        userdata(0),
        continueSysex(false)
    {
        // no body
    }
};

}           // namespace seq64

#endif      // SEQ64_RTMIDI_TYPES_HPP

/*
 * rtmidi_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

