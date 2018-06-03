/**
 * \file          midi_api.cpp
 *
 *    A class for a generic MIDI API.
 *
 * \library       sequencer64 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2018-06-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  In this refactoring, we had to adapt the existing Sequencer64
 *  infrastructure to how the "RtMidi" library works.  We also had to
 *  refactor the RtMidi library significantly to fit it within the working
 *  mode of the Sequencer64 application and libraries.
 */

#include "easy_macros.hpp"              /* func_message() etc.              */
#include "event.hpp"
#include "midi_api.hpp"
#include "midi_info.hpp"
#include "midibus_rm.hpp"
#include "settings.hpp"                 /* seq64::rc_settings ...           */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 *  midi_api section
 */

/**
 *  Principle constructor.
 */

midi_api::midi_api (midibus & parentbus, midi_info & masterinfo)
 :
    midibase
    (
        rc().application_name(),
        parentbus.bus_name(),
        parentbus.port_name(),
        parentbus.get_bus_index(),
        parentbus.get_bus_id(),
        parentbus.get_port_id(),
        parentbus.get_bus_index(),
        parentbus.ppqn(),
        parentbus.bpm(),
        parentbus.is_virtual_port(),
        parentbus.is_input_port(),
        parentbus.is_system_port()
    ),
    m_master_info               (masterinfo),
    m_parent_bus                (parentbus),
    m_input_data                (),
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
 *  \return
 *      Returns true if the port is an input port.
 */

bool
midi_api::is_input_port () const
{
    return parent_bus().is_input_port();
}

/**
 *  A virtual port is what Seq24 called a "manual" port.  It is a MIDI port
 *  that an application can create as if it is a real ALSA port.
 *
 *  \return
 *      Returns true if the port is an input port.
 */

bool
midi_api::is_virtual_port () const
{
    return parent_bus().is_virtual_port();
}

/**
 *  A system port is one that is independent of the devices and applications
 *  that exist.  In the ALSA subsystem, the only system port is the "announce"
 *  port.
 *
 *  \return
 *      Returns true if the port is an system port.
 */

bool
midi_api::is_system_port () const
{
    return parent_bus().is_system_port();
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
#ifdef PLATFORM_DEBUG                       // SEQ64_USE_DEBUG_OUTPUT
        errprint(errorstring.c_str());
#endif
    }
    else
    {
        errprint(errorstring.c_str());

        /*
         * Not a big fan of throwing errors, especially since we currently log
         * errors in rtmidi to the console.  Might make this a build option.
         *
         * throw rterror(errorstring, type);
         */
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

/**
 *  Wires in a MIDI input callback function.
 *
 *  We moved it into the base class, trading convenience for the chance of
 *  confusion.
 *
 * \param callback
 *      Provides the callback function.
 *
 * \param userdata
 *      Provides the user data needed by the callback function.
 */

void
midi_api::user_callback (rtmidi_callback_t callback, void * userdata)
{
    if (m_input_data.using_callback())
    {
        m_error_string = func_message("callback function is already set");
        error(rterror::WARNING, m_error_string);
        return;
    }
    if (is_nullptr(callback))
    {
        m_error_string = func_message("callback function is null");
        error(rterror::WARNING, m_error_string);
        return;
    }
    m_input_data.user_callback(callback);
    m_input_data.user_data(userdata);
    m_input_data.using_callback(true);
}

/**
 *  Removes the MIDI input callback and some items related to it.
 *
 *  We moved it into the base class, trading convenience for the chance of
 *  confusion.
 */

void
midi_api::cancel_callback ()
{
    if (m_input_data.using_callback())
    {
        m_input_data.user_callback(nullptr);
        m_input_data.user_data(nullptr);
        m_input_data.using_callback(false);
    }
    else
    {
        m_error_string = func_message("no callback function was set");
        error(rterror::WARNING, m_error_string);
    }
}

/**
 *  We now provide a default version, since this usage is common and we don't
 *  like having so many overrides.
 *
 * \return
 *      Always returns 0, after a milliseconds's sleep.
 */

int
midi_api::api_poll_for_midi ()
{
    millisleep(1);
    return 0;
}

}           // namespace seq64

/*
 * midi_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

