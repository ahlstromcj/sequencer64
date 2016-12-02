/**
 * \file          midi_api.cpp
 *
 *    A class for a generic MIDI API.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-01
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  In this refactoring...
 *
 */

#include "midi_api.hpp"
#include "rtmidi.hpp"

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
        errprint(errorstring.c_str());
    }
    else if (type == rterror::DEBUG_WARNING)
    {
#ifdef SEQ64_USE_DEBUG_OUTPUT
        errprint(errorstring.c_str());
#endif
    }
    else
    {
        errprint(errorstring.c_str());
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
    m_input_data.queue.allocate(queuesize);
}

/**
 *  Destructor.
 *
 *  Deletes the MIDI input queue.
 */

midi_in_api::~midi_in_api ()
{
    m_input_data.queue.deallocate();
}

/**
 *  Wires in a MIDI input callback function.
 *
 * \param callback
 *      Provides the callback function.
 *
 * \param userdata
 *      Provides the user data needed by the callback function.
 */

void
midi_in_api::set_callback
(
    rtmidi_callback_t callback,
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
 *      A string of characters for the messages.
 *
 * \return
 *      Returns the delta-time (timestamp) of the incoming message.  If an
 *      error occurs, or if there is not message, then 0.0 is returned.
 */

double
midi_in_api::get_message (std::vector<midibyte> & message)
{
    message.clear();
    if (m_input_data.usingCallback)
    {
        m_error_string = func_message("user callback already set for this port");
        error(rterror::WARNING, m_error_string);
        return 0.0;
    }
    if (m_input_data.queue.empty())
        return 0.0;

    /*
     * Copy queued message to the vector reference argument and then "pop" it.
     */

    const std::vector<midibyte> & bytes = m_input_data.queue.front().bytes;
    message.assign(bytes.begin(), bytes.end());

    double stamp = m_input_data.queue.front().timeStamp;
    m_input_data.queue.pop();
    return stamp;
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

/**
 *
 */

bool
midi_out_api::poll_queue () const
{
    std::string errortext = func_message("not supported");
    throw(rterror(errortext, rterror::UNSPECIFIED));
    return false;
}


}           // namespace seq64

/*
 * midi_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

