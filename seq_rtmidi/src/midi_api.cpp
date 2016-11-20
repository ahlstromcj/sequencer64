/**
 * \file          midi_api.cpp
 *
 *    A class for a generic MIDI API.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-17
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  In this refactoring...
 *
 */

#include "midi_api.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 *  midi_api section
 */

/**
 *  Default constructor.
 */

midi_api::midi_api ()
 :
    m_api_data                  (0),
    m_connected                 (false),
    m_error_class               ("midi_api"),
    m_error_string              (),
    m_error_callback            (0),
    m_first_error_occurred      (false),
    m_error_callback_user_data  (0)
{
    // no code
}

/**
 *  Destructor, needed because it is virtual.
 */

midi_api::~midi_api ()
{
    // no code
}

/**
 *  Provides an error handler.
 *
 * \throw
 *      If the error is not just a warning, then an rterror object is thrown.
 *
 * \param type
 *      The type of the error.
 *
 * \param errorstring
 *      The error message, which gets copied if this is the first error.
 */

void
midi_api::error (rterror::Type type, const std::string & errorstring)
{
    if (m_error_callback)
    {
        if (m_first_error_occurred)
            return;

        m_first_error_occurred = true;

        const std::string errorMessage = errorstring;
        m_error_callback(type, errorMessage, m_error_callback_user_data);
        m_first_error_occurred = false;
        return;
    }

    if (type == rterror::WARNING)
    {
        errprint(errorstring);
    }
    else if (type == rterror::DEBUG_WARNING)
    {
#ifdef SEQ64_USE_DEBUG_OUTPUT
        errprintf(errorstring);
#endif
    }
    else
    {
        errprintf(errorstring);
        throw rterror(errorstring, type);
    }
}

/*
 *  Common midi_in_api Definitions
 */

/**
 *  Constructor.
 *
 * \param queuesize
 *      Provides the ring-size of the MIDI input data queue.
 */

midi_in_api::midi_in_api (unsigned queuesize)
 :
    midi_api    ()
{
    m_input_data.queue.ringSize = queuesize; // allocate the MIDI queue
    if (queuesize > 0)
        m_input_data.queue.ring = new (std::nothrow) midi_message[queuesize];
}

/**
 *  Destructor.
 *
 *  Deletes the MIDI input queue.
 */

midi_in_api::~midi_in_api ()
{
    if (not_nullptr(m_input_data.queue.ring))  // m_input_data.queue.ringSize>0
        delete [] m_input_data.queue.ring;
}

/**
 *  Wires in a MIDI input callback function.
 *
 * \param callback
 *      Provides the callback function.
 *
 * \param
 *      Provides the user data needed by the callback function.
 */

void
midi_in_api::set_callback
(
    rtmidi_in::rtmidi_callback_t callback,
    void * userdata
)
{
    if (m_input_data.usingCallback)
    {
        m_error_string = func_message("callback function is already set");
        error(rterror::WARNING, m_error_string);
        return;
    }
    if (! callback)
    {
        m_error_string = func_message("callback function is invalid");
        error(rterror::WARNING, m_error_string);
        return;
    }
    m_input_data.userCallback = callback;
    m_input_data.userdata = userdata;
    m_input_data.usingCallback = true;
}

/**
 *  Removes the MIDI input callback and some items related to it.
 */

void
midi_in_api::cancel_callback ()
{
    if (m_input_data.usingCallback)
    {
        m_input_data.userCallback = 0;
        m_input_data.userdata = 0;
        m_input_data.usingCallback = false;
    }
    else
    {
        m_error_string = func_message("no callback function was set");
        error(rterror::WARNING, m_error_string);
    }
}

/**
 *  Sets m_input_data.ignoreFlag according to the given parameters.
 *
 * \param midisysex
 *      The MIDI SysEx flag.
 *
 * \param miditime
 *      The MIDI time flag.
 *
 * \param midisysex
 *      The MIDI sense flag.
 */

void
midi_in_api::ignore_types (bool midisysex, bool miditime, bool midisense)
{
    m_input_data.ignoreFlags = 0;
    if (midisysex)
        m_input_data.ignoreFlags = 0x01;

    if (miditime)
        m_input_data.ignoreFlags |= 0x02;

    if (midisense)
        m_input_data.ignoreFlags |= 0x04;
}

/**
 *  Gets a MIDI input message from the message queue.
 *
 * \param message
 *      A string of characters for the messages.  Not sure why it is a vector.
 */

double
midi_in_api::get_message (std::vector<midibyte> * message)
{
    message->clear();
    if (m_input_data.usingCallback)
    {
        m_error_string = func_message("user callback already set for this port");
        error(rterror::WARNING, m_error_string);
        return 0.0;
    }
    if (m_input_data.queue.size == 0)
        return 0.0;

    // Copy queued message to the vector pointer argument and then "pop" it.

    std::vector<midibyte> * bytes =
        &(m_input_data.queue.ring[m_input_data.queue.front].bytes);

    message->assign(bytes->begin(), bytes->end());
    double deltaTime =
        m_input_data.queue.ring[m_input_data.queue.front].timeStamp;

    m_input_data.queue.size--;
    m_input_data.queue.front++;
    if (m_input_data.queue.front == m_input_data.queue.ringSize)
        m_input_data.queue.front = 0;

    return deltaTime;
}

/*
 *  Common midi_out_api Definitions
 */

/**
 *  Default constructor.
 */

midi_out_api::midi_out_api ()
 :
    midi_api    ()
{
    // no code
}

/**
 *  Stock empty destructor.
 */

midi_out_api::~midi_out_api ()
{
    // no code
}

}           // namespace seq64

/*
 * midi_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

