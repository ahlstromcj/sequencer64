#ifndef SEQ64_MIDI_JACK_DATA_HPP
#define SEQ64_MIDI_JACK_DATA_HPP

/**
 * \file          midi_jack_data.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-01-02
 * \updates       2017-01-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 */

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include "midibyte.hpp"                 /* seq64::midibyte              */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Contains the JACK MIDI API data as a kind of scratchpad for this object.
 *  This guy needs a constructor taking parameters for an rtmidi_in_data
 *  pointer.
 */

struct midi_jack_data
{
    /**
     *  Holds the JACK sequencer client pointer so that it can be used
     *  by the midibus objects.  This is actually an opaque pointer; there is
     *  no way to get the actual fields in this structure; they can only be
     *  accessed through functions in the JACK API.  Note that it is also
     *  stored as a void pointer in midi_info::m_midi_handle.  This item
     *  can either be the single JACK client created by the midi_jack_info
     *  object, or a JACK client created by the midi_jack object in the
     *  "multi-client" mode (which is not yet complete or usable).
     */

    jack_client_t * m_jack_client;

    /**
     *  Holds the JACK port information of the JACK client.
     */

    jack_port_t * m_jack_port;

    /**
     *  Holds the size of data for communicating between the client
     *  ring-buffer and the JACK port's internal buffer.
     */

    jack_ringbuffer_t * m_jack_buffsize;

    /**
     *  Holds the data for communicating between the client ring-buffer and
     *  the JACK port's internal buffer.
     */

    jack_ringbuffer_t * m_jack_buffmessage;

    /**
     *  The last time-stamp obtained.  Use for calculating the delta time, I
     *  would imagine.
     */

    jack_time_t m_jack_lasttime;

    /**
     *  Holds special data peculiar to the client and its MIDI input
     *  processing.
     */

    rtmidi_in_data * m_jack_rtmidiin;

    /**
     * \ctor midi_jack_data
     */

    midi_jack_data () :
        m_jack_client       (nullptr),
        m_jack_port         (nullptr),
        m_jack_buffsize     (nullptr),
        m_jack_buffmessage  (nullptr),
        m_jack_lasttime     (0),
        m_jack_rtmidiin     (nullptr)
    {
        // Empty body
    }

    /**
     *  This destructor currently does nothing.  We rely on the enclosing class
     *  to close out the things that it created.
     */

    ~midi_jack_data ()
    {
        // Empty body
    }

    /**
     *  Tests that the buffer is good.
     */

    bool valid_buffer () const
    {
        return not_nullptr(m_jack_buffsize) && not_nullptr(m_jack_buffmessage);
    }

};          // class midi_jack_data

}           // namespace seq64

#endif      // SEQ64_MIDI_JACK_DATA_HPP

/*
 * midi_jack_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

