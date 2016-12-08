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

#ifdef SEQ64_BUILD_LINUX_ALSA__NOT_READY
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
     rtmidi_base    ()
{
   // no code
}

/**
 *  Destructor.
 */

rtmidi_info::~rtmidi_info ()
{
    // see base class
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

#ifdef SEQ64_BUILD_LINUX_ALSA__NOT_READY
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

