/**
 * \file          rtmidi_info.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \author        Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2016-12-18
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
#include "seq64_rtmidi_features.h"      /* selects the usable APIs      */

#ifdef SEQ64_BUILD_LINUX_ALSA
#include "midi_alsa_info.hpp"
#endif

#ifdef SEQ64_BUILD_UNIX_JACK__NOT_READY
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
     * ALSA, and then to the dummy implementation.
     */

#ifdef SEQ64_BUILD_UNIX_JACK
    if (rc().with_jack_transport())
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

rtmidi_info::rtmidi_info (rtmidi_api api)   // : rtmidi_base    ()
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        openmidi_api(api);
        if (not_nullptr(get_api()))
        {
            selected_api(api);              /* log first API that worked    */
            return;
        }
        errprintfunc("no compiled support for specified API");
    }

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i]);
        if (not_nullptr(get_api()))
        {
            if (get_api()->get_all_port_info() > 0)
            {
                selected_api(apis[i]);      /* log first API that worked    */
                break;
            }
        }
        else
        {
            continue;
        }
    }
    if (is_nullptr(get_api()))
    {
        std::string errortext = func_message("no compiled API support found");
        throw(rterror(errortext, rterror::UNSPECIFIED));
    }
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
 */

void
rtmidi_info::openmidi_api (rtmidi_api api)
{
    delete_api();

#ifdef SEQ64_BUILD_UNIX_JACK__NOT_READY
    if (rc().with_jack_transport())
    {
        if (api == RTMIDI_API_UNIX_JACK)
            set_api(new midi_jack_info(clientname, queuesizelimit));
    }
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
    if (api == RTMIDI_API_LINUX_ALSA)
        set_api(new midi_alsa_info());
#endif

}

}           // namespace seq64

/*
 * rtmidi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

