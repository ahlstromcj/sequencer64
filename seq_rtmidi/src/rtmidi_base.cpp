/**
 * \file          rtmidi_base.cpp
 *
 *    A base class for managing various MIDI APIs.
 *
 * \author        Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2016-12-08
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  An abstract base class for realtime MIDI input/output and enumeration.
 *  This class implements some common functionality enumerating MIDI clients
 *  and ports.
 *
 */

#include "midi_api.hpp"                 /* seq64::midi_api base class       */
#include "rtmidi_base.hpp"              /* seq64::rtmidi_base class         */
#include "rterror.hpp"                  /* seq64::rterror class             */
#include "settings.hpp"                 /* seq64::rc().with_jack_...()      */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Default constructor.
 */

rtmidi_base::rtmidi_base ()
 :
   m_rtapi          (nullptr),
   m_selected_api   (RTMIDI_API_UNSPECIFIED)
{
   // no code
}

/**
 *  Destructor.
 */

rtmidi_base::~rtmidi_base ()
{
    if (not_nullptr(m_rtapi))
    {
        delete m_rtapi;
        m_rtapi = nullptr;
    }
}

/**
 * \getter SEQ64_RTMIDI_VERSION
 */

std::string
rtmidi_base::get_version ()
{
    return std::string(SEQ64_RTMIDI_VERSION);
}

/**
 *  Gets the list of APIs compiled into the application.
 *
 *  Note that we would also like to make ALSA versus JACK a runtime option as
 *  it is in the legacy Sequencer64 application.
 *
 * \param apis
 *      The API structure.
 */

void
rtmidi_base::get_compiled_api (std::vector<rtmidi_api> & apis)
{
    apis.clear();

    /*
     * The order here will control the order of rtmidi's API search in the
     * constructor.  For Linux, we will try JACK first, then fall back to
     * ALSA, and then to the dummy implementation.
     */

#ifdef SEQ64_BUILD_UNIX_JACK
    if (rc().with_jack_transport())
        apis.push_back(RTMIDI_API_UNIX_JACK);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    apis.push_back(RTMIDI_API_LINUX_ALSA);
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE
    apis.push_back(RTMIDI_API_MACOSX_CORE);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    apis.push_back(RTMIDI_API_WINDOWS_MM);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    apis.push_back(RTMIDI_API_DUMMY);
#endif

    /*
     * It should not be possible to get here because the preprocessor
     * definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
     * API-specific definitions are passed to the compiler. But just in
     * case something weird happens, we'll throw an error.
     */

    std::string errortext = func_message("no compiled API support found");
    throw(rterror(errortext, rterror::UNSPECIFIED));
}

}           // namespace seq64

/*
 * rtmidi_base.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

