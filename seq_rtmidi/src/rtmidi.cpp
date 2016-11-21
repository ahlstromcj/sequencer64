/**
 * \file          rtmidi.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-20
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 *  This class implements some common functionality for the realtime
 *  MIDI input/output subclasses rtmidi_in and rtmidi_out.
 *
 *  RtMidi WWW site: http://music.mcgill.ca/~gary/rtmidi/
 *
 *  RtMidi:          realtime MIDI i/o C++ classes
 *
 *  Copyright (c) 2003-2016 Gary P. Scavone
 *
 *  Some projects using rtmidi:
 *
 *      -   Csound5 *
 *      -   SimpleSysexxer
 *      -   Canorus
 *      -   PianoBooster *
 *      -   stk *
 *      -   milkytracker *
 *
 */

#include "rtmidi.hpp"                   /* seq64::rtmidi, etc.          */

#ifdef SEQ64_BUILD_LINUX_ALSA
#include "midi_alsa.hpp"
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE
#include "midi_core.hpp"
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
#include "midi_dummy.hpp"
#endif

#ifdef SEQ64_BUILD_UNIX_JACK
#include "midi_jack.hpp"
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
#include "midi_winmm.hpp"
#endif

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 *  MIDI on the iPhone?  Whoa!
 */

#ifdef SEQ64_BUILD_MACOSX_CORE
#if TARGET_OS_IPHONE
#define AudioGetCurrentHostTime CAHostTimeBase::GetCurrentTime
#define AudioConvertHostTimeToNanos CAHostTimeBase::ConvertToNanos
#endif
#endif

/*
 * class rtmidi
 */

/**
 *  Default constructor.
 */

rtmidi::rtmidi ()
 :
   m_rtapi  (nullptr)
{
   // no code
}

/**
 *  Destructor.
 */

rtmidi::~rtmidi()
{
    if (m_rtapi)
        delete m_rtapi;

    m_rtapi = nullptr;
}

/**
 * \getter SEQ64_RTMIDI_VERSION
 */

std::string
rtmidi::get_version ()
{
    return std::string(SEQ64_RTMIDI_VERSION);
}

/**
 *  Gets the list of APIs compiled into the application.
 *
 * \param apis
 *      The API structure.
 */

void
rtmidi::get_compiled_api (std::vector<rtmidi_api> & apis)
{
    apis.clear();

    /*
     * The order here will control the order of rtmidi's API search in the
     * constructor.
     */

#ifdef SEQ64_BUILD_MACOSX_CORE
    apis.push_back(RTMIDI_API_MACOSX_CORE);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    apis.push_back(RTMIDI_API_LINUX_ALSA);
#endif

#ifdef SEQ64_BUILD_UNIX_JACK
    apis.push_back(RTMIDI_API_UNIX_JACK);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    apis.push_back(RTMIDI_API_WINDOWS_MM);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    apis.push_back(RTMIDI_API_DUMMY);
#endif
}

/*
 * class rtmidi_in
 */

void
rtmidi_in::openmidi_api
(
   rtmidi_api api,
   const std::string & clientname,
   unsigned queuesizelimit
)
{
    if (m_rtapi)
        delete m_rtapi;

    m_rtapi = nullptr;

#ifdef SEQ64_BUILD_UNIX_JACK
    if (api == RTMIDI_API_UNIX_JACK)
        m_rtapi = new midi_in_jack(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == RTMIDI_API_LINUX_ALSA)
        m_rtapi = new midi_in_alsa(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    if (api == RTMIDI_API_WINDOWS_MM)
        m_rtapi = new midi_in_winmm(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE
    if (api == RTMIDI_API_MACOSX_CORE)
        m_rtapi = new midi_in_core(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    if (api == RTMIDI_API_DUMMY)
        m_rtapi = new midi_in_dummy(clientname, queuesizelimit);
#endif

}

rtmidi_in::rtmidi_in
(
   rtmidi_api api,
   const std::string & clientname,
   unsigned queuesizelimit
)
 :
   rtmidi   ()
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        // Attempt to open the specified API.

        openmidi_api(api, clientname, queuesizelimit);
        if (m_rtapi) return;

        // No compiled support for specified API value.  Issue a warning
        // and continue as if no API was specified.

        std::cerr
            << func_message("no compiled support for specified API")
            << std::endl
            ;
    }

    /*
     * Iterate through the compiled APIs and return as soon as we find
     * one with at least one port or we reach the end of the list.
     */

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname, queuesizelimit);
        if (m_rtapi->get_port_count())
           break;
    }

    if (m_rtapi)
       return;

    /*
     * It should not be possible to get here because the preprocessor
     * definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
     * API-specific definitions are passed to the compiler. But just in
     * case something weird happens, we'll throw an error.
     */

    std::string errorText = func_message("no compiled API support found");
    throw(rterror(errorText, rterror::UNSPECIFIED));
}

rtmidi_in::~rtmidi_in()
{
   // no code
}

/**
 *  Principal constructor.
 *
 * \param api
 *      Provides the API to be constructed.
 *
 * \param clientname
 *      Provides the name of the MIDI output client, used by some of the APIs.
 */

rtmidi_out::rtmidi_out (rtmidi_api api, const std::string & clientname)
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        /*
         * Attempt to open the specified API.  If there is no compiled support
         * for specified API value, then issue a warning and continue as if no
         * API was specified.
         */

        openmidi_api(api, clientname);
        if (not_nullptr(m_rtapi))
           return;

        std::cerr
            << func_message("no compiled support for specified API argument")
            << std::endl
            ;
    }

    /*
     * Iterate through the compiled APIs and return as soon as we find one
     * with at least one port or we reach the end of the list.
     */

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname);
        if (m_rtapi->get_port_count())
           break;
    }

    if (not_nullptr(m_rtapi))
       return;

    /*
     * It should not be possible to get here because the preprocessor
     * definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
     * API-specific definitions are passed to the compiler. But just in case
     * something weird happens, we'll thrown an error.
     */

    std::string errorText = func_message("no compiled API support found");
    throw(rterror(errorText, rterror::UNSPECIFIED));
}

/*
 * class rtmidi_out
 */

rtmidi_out::~rtmidi_out()
{
   // no code
}

void
rtmidi_out::openmidi_api (rtmidi_api api, const std::string & clientname)
{
    if (m_rtapi)
        delete m_rtapi;

    m_rtapi = nullptr;

#ifdef SEQ64_BUILD_UNIX_JACK
    if (api == RTMIDI_API_UNIX_JACK)
        m_rtapi = new midi_out_jack(clientname);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == RTMIDI_API_LINUX_ALSA)
        m_rtapi = new midi_out_alsa(clientname);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    if (api == RTMIDI_API_WINDOWS_MM)
        m_rtapi = new midi_out_winmm(clientname);
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE
    if (api == RTMIDI_API_MACOSX_CORE)
        m_rtapi = new midi_out_core(clientname);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    if (api == RTMIDI_API_DUMMY)
        m_rtapi = new midi_out_dummy(clientname);
#endif

}

}           // namespace seq64

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

