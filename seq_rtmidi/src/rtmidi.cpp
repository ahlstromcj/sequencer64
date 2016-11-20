/**
 * \file          rtmidi.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-19
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 *  This class implements some common functionality for the realtime
 *  MIDI input/output subclasses rtmidi_in and rtmidi_out.
 *
 *  rtmidi WWW site: http://music.mcgill.ca/~gary/rtmidi/
 *
 *  rtmidi: realtime MIDI i/o C++ classes
 *
 *  Copyright (c) 2003-2016 Gary P. Scavone
 *
 */

#include "rtmidi.h"

#include <sstream>

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
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
rtmidi::get_compiled_api (std::vector<rtmidi::Api> & apis)
{
    apis.clear();

    /*
     * The order here will control the order of rtmidi's API search in the
     * constructor.
     */

#ifdef SEQ64_BUILD_MACOSX_CORE
    apis.push_back(MACOSX_CORE);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    apis.push_back(LINUX_ALSA);
#endif

#ifdef SEQ64_BUILD_UNIX_JACK
    apis.push_back(UNIX_JACK);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    apis.push_back(WINDOWS_MM);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    apis.push_back(RTMIDI_DUMMY);
#endif
}

/*
 * class rtmidi_in
 */

void
rtmidi_in::openmidi_api
(
   rtmidi::Api api,
   const std::string & clientname,
   unsigned queuesizelimit
)
{
    if (m_rtapi)
        delete m_rtapi;

    m_rtapi = nullptr;

#ifdef SEQ64_BUILD_UNIX_JACK
    if (api == UNIX_JACK)
        m_rtapi = new midi_in_jack(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == LINUX_ALSA)
        m_rtapi = new midi_in_alsa(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    if (api == WINDOWS_MM)
        m_rtapi = new midi_in_winmm(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE
    if (api == MACOSX_CORE)
        m_rtapi = new midi_in_core(clientname, queuesizelimit);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    if (api == RTMIDI_DUMMY)
        m_rtapi = new midi_in_dummy(clientname, queuesizelimit);
#endif

}

rtmidi_in::rtmidi_in
(
   rtmidi::Api api,
   const std::string & clientname,
   unsigned queuesizelimit
)
 :
   rtmidi   ()
{
    if (api != UNSPECIFIED)
    {
        // Attempt to open the specified API.

        openmidi_api(api, clientname, queuesizelimit);
        if (m_rtapi) return;

        // No compiled support for specified API value.  Issue a warning
        // and continue as if no API was specified.

        std::cerr
            << "\nrtmidi_in: no compiled support for specified API\n"
            << std::endl
            ;
    }

    // Iterate through the compiled APIs and return as soon as we find
    // one with at least one port or we reach the end of the list.

    std::vector<rtmidi::Api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname, queuesizelimit);
        if (m_rtapi->get_port_count())
           break;
    }

    if (m_rtapi)
       return;

    // It should not be possible to get here because the preprocessor
    // definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
    // API-specific definitions are passed to the compiler. But just in
    // case something weird happens, we'll throw an error.

    std::string errorText = "rtmidi_in: no compiled API support found";
    throw(rterror(errorText, rterror::UNSPECIFIED));
}

rtmidi_in::~rtmidi_in()
{
   // no code
}

/*
 * class rtmidi_out
 */

rtmidi_out::~rtmidi_out()
{
   // no code
}

void
rtmidi_out::openmidi_api (rtmidi::Api api, const std::string & clientname)
{
    if (m_rtapi)
        delete m_rtapi;

    m_rtapi = nullptr;

#ifdef SEQ64_BUILD_UNIX_JACK
    if (api == UNIX_JACK)
        m_rtapi = new midi_out_jack(clientname);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == LINUX_ALSA)
        m_rtapi = new midi_out_alsa(clientname);
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM
    if (api == WINDOWS_MM)
        m_rtapi = new midi_out_winmm(clientname);
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE
    if (api == MACOSX_CORE)
        m_rtapi = new midi_out_core(clientname);
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY
    if (api == RTMIDI_DUMMY)
        m_rtapi = new midi_out_dummy(clientname);
#endif

}

rtmidi_out::rtmidi_out (rtmidi::Api api, const std::string & clientname)
{
    if (api != UNSPECIFIED)
    {
        // Attempt to open the specified API.

        openmidi_api(api, clientname);
        if (m_rtapi)
           return;

        // No compiled support for specified API value.  Issue a warning
        // and continue as if no API was specified.

        std::cerr
            << "\nrtmidi_out: no compiled support for specified API argument!\n"
            << std::endl
            ;
    }

    // Iterate through the compiled APIs and return as soon as we find
    // one with at least one port or we reach the end of the list.

    std::vector<rtmidi::Api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname);
        if (m_rtapi->get_port_count())
           break;
    }

    if (m_rtapi)
       return;

    // It should not be possible to get here because the preprocessor
    // definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
    // API-specific definitions are passed to the compiler. But just in
    // case something weird happens, we'll thrown an error.

    std::string errorText = "rtmidi_out: no compiled API support found";
    throw(rterror(errorText, rterror::UNSPECIFIED));
}

}           // namespace seq64

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


