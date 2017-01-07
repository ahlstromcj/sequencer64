/**
 * \file          midi_api.cpp
 *
 *    A class for a generic MIDI API.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2017-01-07
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  In this refactoring...
 *
 */

#include "event.hpp"
#include "midi_api.hpp"
#include "midi_info.hpp"

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

midi_api::midi_api (midi_info & masterinfo, int index)
 :
    midibase
    (
        SEQ64_APP_NAME,
        masterinfo.get_bus_name(index),
        masterinfo.get_port_name(index),
        index,
        masterinfo.get_bus_id(index),
        masterinfo.get_port_id(index),
        index,  // queue
        masterinfo.ppqn(),
        masterinfo.bpm(),
        false               /* non-virtual HMMMMMMMMMMM */
    ),
    m_master_info               (masterinfo),
    m_connected                 (false),
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
 *  Provides an error handler that can support an error callback.
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

/**
 * \getter m_master_info.midi_mode()
 *      This function makes it a bit simpler on the caller.
 */

void
midi_api::master_midi_mode (bool input)
{
    m_master_info.midi_mode(input);
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

midi_in_api::midi_in_api (midi_info & masterinfo, int index)
 :
    midi_api    (masterinfo, index)
{
    // any code?
}

/**
 *  Destructor.
 *
 *  Deletes the MIDI input queue.
 */

midi_in_api::~midi_in_api ()
{
    // any code?
}

/*
 *  Common midi_out_api Definitions
 */

/**
 *  Default constructor.
 */

midi_out_api::midi_out_api (midi_info & masterinfo, int index)
 :
    midi_api    (masterinfo, index)
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

