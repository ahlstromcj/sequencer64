/**
 * \file          rtmidi_info.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \author        Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2016-12-08
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 *  This class implements some common functionality enumerating MIDI clients
 *  and ports.
 *
 */

#include "rtmidi_info.hpp"              /* seq64::rtmidi_info           */
#include "settings.hpp"                 /* seq64::rc().with_jack_...()  */

#ifdef SEQ64_BUILD_LINUX_ALSA
#include "midi_alsa_info.hpp"
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE__NOT_READY
#include "midi_core_info.hpp"
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY__NOT_READY
#include "midi_dummy_info.hpp"
#endif

#ifdef SEQ64_BUILD_UNIX_JACK__NOT_READY
#include "midi_jack_info.hpp"
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM__NOT_READY
#include "midi_winmm_info.hpp"
#endif

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Default constructor.
 */

rtmidi_info::rtmidi_info ()
 :
   m_rtapi          (nullptr),
   m_selected_api   (RTMIDI_API_UNSPECIFIED)
{
   // no code
}

/**
 *  Destructor.
 */

rtmidi_info::~rtmidi_info ()
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
rtmidi_info::get_version ()
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
rtmidi_info::get_compiled_api (std::vector<rtmidi_api> & apis)
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
     * Iterate through the compiled APIs and return as soon as we find
     * one with at least one port or we reach the end of the list.
     */

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname, queuesizelimit);
        if (m_rtapi->get_port_count() > 0)
        {
            selected_api(apis[i]);          /* log the API that worked  */
            break;
        }
    }

    if (not_nullptr(m_rtapi))
       return;

    /*
     * It should not be possible to get here because the preprocessor
     * definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
     * API-specific definitions are passed to the compiler. But just in
     * case something weird happens, we'll throw an error.
     */

    std::string errortext = func_message("no compiled API support found");
    throw(rterror(errortext, rterror::UNSPECIFIED));
}

/**
 *  Opens the desired MIDI API.
 *
 * \param api
 *      The desired MIDI API.
 *
 * \param clientname
 *      The name of the client.  This is the name used to access the client.
 *
 * \param queuesizelimit
 *      The maximum size of the MIDI message queue.
 */

void
rtmidi_info::openmidi_api
(
   rtmidi_api api,
   const std::string & clientname,
   unsigned queuesizelimit
)
{
    if (not_nullptr(m_rtapi))
    {
        delete m_rtapi;
        m_rtapi = nullptr;
    }

#ifdef SEQ64_BUILD_UNIX_JACK__NOT_READY
    if (rc().with_jack_transport())
    {
        if (api == RTMIDI_API_UNIX_JACK)
            m_rtapi = new midi_jack_info(clientname, queuesizelimit);
    }
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == RTMIDI_API_LINUX_ALSA)
        m_rtapi = new midi_alsa_info(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE__NOT_READY
    if (api == RTMIDI_API_MACOSX_CORE)
        m_rtapi = new midi_core_info(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM__NOT_READY
    if (api == RTMIDI_API_WINDOWS_MM)
        m_rtapi = new midi_winmm_info(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY__NOT_READY
    if (api == RTMIDI_API_DUMMY)
        m_rtapi = new midi_dummy_info(clientname, queuesizelimit);
#endif

}

}           // namespace seq64

/*
 * rtmidi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

