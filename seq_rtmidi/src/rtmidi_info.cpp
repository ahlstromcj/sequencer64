/**
 * \file          rtmidi_info.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2018-04-12
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  An abstract base class for realtime MIDI input/output.  This class
 *  implements some common functionality enumerating MIDI clients and ports.
 *
 */

#include "easy_macros.hpp"              /* C++ version of easy macros       */
#include "rtmidi_info.hpp"              /* seq64::rtmidi_info           */
#include "settings.hpp"                 /* seq64::rc().with_jack_...()  */
#include "seq64_rtmidi_features.h"      /* selects the usable APIs      */

#ifdef SEQ64_BUILD_LINUX_ALSA
#include "midi_alsa_info.hpp"
#endif

#ifdef SEQ64_BUILD_UNIX_JACK
#include "midi_jack_info.hpp"
#endif

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Holds the selected API code.
 */

rtmidi_api rtmidi_info::sm_selected_api = RTMIDI_API_UNSPECIFIED;

/**
 * \getter SEQ64_RTMIDI_VERSION
 *      This is a static function to replace the midi_api version.
 */

std::string
rtmidi_info::get_version ()
{
    return std::string(SEQ64_RTMIDI_VERSION);
}

/**
 *  Gets the list of APIs compiled into the application.  Note that we make
 *  ALSA versus JACK a runtime option as it is in the legacy Sequencer64
 *  application.
 *
 *  This is a static function to replace the midi_api version.
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
     * ALSA, and then to the dummy implementation.  We were checking
     * rc().with_jack_transport(), but the "rc" configuration file has not yet
     * been read by the time we get to here.  On the other hand, we can make
     * it default to "true" and see what happens.
     */

#ifdef SEQ64_BUILD_UNIX_JACK
     if (rc().with_jack_midi())
        apis.push_back(RTMIDI_API_UNIX_JACK);
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
        apis.push_back(RTMIDI_API_LINUX_ALSA);
#endif

    if (apis.empty())
    {
        std::string errortext = func_message("no compiled API support found");
        throw(rterror(errortext, rterror::UNSPECIFIED));
    }
}

/**
 *  Default constructor.  Code basically cut-and-paste from rtmidi_in or
 *  rtmidi_out. Common code!
 */

rtmidi_info::rtmidi_info
(
    rtmidi_api api,
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    m_info_api  (nullptr)
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        bool ok = openmidi_api(api, appname, ppqn, bpm);
        if (ok)
        {
            if (not_nullptr(get_api_info()))
            {
                if (get_api_info()->get_all_port_info() >= 0)
                {
                    selected_api(api);      /* log the API that worked      */
                    return;
                }
            }
        }
        errprintfunc("no system support for specified API");
    }

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        if (openmidi_api(apis[i], appname, ppqn, bpm)) // get_api_info()
        {
            /*
             * For JACK, or any other API, there may be no ports (from other
             * applications) yet in place.
             */

            if (not_nullptr(get_api_info()))
            {
                if (get_api_info()->get_all_port_info() >= 0)
                {
                    selected_api(apis[i]);  /* log first API that worked    */
                    break;
                }
            }
        }
        else
        {
            continue;
        }
    }
    if (is_nullptr(get_api_info()))
    {
        std::string errortext = func_message("no compiled API support found");
        throw(rterror(errortext, rterror::UNSPECIFIED));
    }
}

/**
 *  Destructor.  Gets rid of m_info_api and nullifies it.
 */

rtmidi_info::~rtmidi_info ()
{
    delete_api();
    // if (not_nullptr(m_info_api))
    //      delete m_info_api;
}

/**
 *  Opens the desired MIDI API.
 *
 *  If the JACK API is tried, and found missing, we turn off all of the other
 *  JACK flags found in the "rc" configuration file.  Also, the loop in the
 *  constructor will come back here to try the other compiled-in APIs
 *  (currently just ALSA).
 *
 * \param api
 *      The desired MIDI API.
 *
 * \param appname
 *      The name of the application, to be passed to the midi_info-derived
 *      constructor.
 *
 * \param ppqn
 *      The PPQN value to pass along to the midi_info_derived constructor.
 *
 * \param bpm
 *      The BPM (beats per minute) value to pass along to the
 *      midi_info_derived constructor.
 *
 * \return
 *      Returns true if a valid API is found.  A valid API is on that is both
 *      compiled into the application and is found existing on the host
 *      computer (system).
 */

bool
rtmidi_info::openmidi_api
(
    rtmidi_api api,
    const std::string & appname,
    int ppqn,
    midibpm bpm
)
{
    bool result = false;
    delete_api();

#ifdef SEQ64_BUILD_UNIX_JACK
    if (api == RTMIDI_API_UNIX_JACK)
    {
        if (rc().with_jack_midi())
        {
            result = set_api_info(new midi_jack_info(appname, ppqn, bpm));
            if (! result)
            {
                /**
                 * Disables the usage of JACK MIDI for the rest of the program
                 * run.  This includes JACK Transport, which also obviously
                 * needs JACK to work.
                 */

                rc().with_jack_transport(false);
                rc().with_jack_master(false);
                rc().with_jack_master_cond(false);
                rc().with_jack_midi(false);
            }
        }
    }
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == RTMIDI_API_LINUX_ALSA)
    {
        result = set_api_info(new midi_alsa_info(appname, ppqn, bpm));
    }
#endif

    return result;
}

}           // namespace seq64

/*
 * rtmidi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

