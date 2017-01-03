#ifndef SEQ64_MIDI_JACK_DATA_HPP
#define SEQ64_MIDI_JACK_DATA_HPP

/**
 * \file          midi_jack_data.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
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
    jack_client_t * m_jack_client;
    jack_port_t * m_jack_port;
    jack_ringbuffer_t * m_jack_buffsize;
    jack_ringbuffer_t * m_jack_buffmessage;
    jack_time_t m_jack_lasttime;
    rtmidi_in_data * m_jack_rtmidiin;

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

};          // class midi_jack_data

}           // namespace seq64

#endif      // SEQ64_MIDI_JACK_DATA_HPP

/*
 * midi_jack_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

