/**
 * \file          rtmidi_info.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \author        Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2016-12-10
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
 *  Default constructor.  Code basically cut-and-paste from rtmidi_in or
 *  rtmidi_out. Common code!
 */

rtmidi_info::rtmidi_info (rtmidi_api api)
 :
     rtmidi_base    ()
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        openmidi_api(api, "", 0);
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
        openmidi_api(apis[i], "", 0);
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
   const std::string & clientname,          // probably unnecessary here
   unsigned queuesizelimit                  // probably unnecessary here
)
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
//      set_api(new midi_alsa_info(clientname, queuesizelimit));
#endif

#ifdef SEQ64_BUILD_MACOSX_CORE__NOT_READY
    if (api == RTMIDI_API_MACOSX_CORE)
        set_api(new midi_core_info(clientname, queuesizelimit));
#endif

#ifdef SEQ64_BUILD_WINDOWS_MM__NOT_READY
    if (api == RTMIDI_API_WINDOWS_MM)
        set_api(new midi_winmm_info(clientname, queuesizelimit));
#endif

#ifdef SEQ64_BUILD_RTMIDI_DUMMY__NOT_READY
    if (api == RTMIDI_API_DUMMY)
        set_api(new midi_dummy_info(clientname, queuesizelimit));
#endif

}

}           // namespace seq64

/*
 * rtmidi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

