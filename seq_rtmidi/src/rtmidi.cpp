/**
 * \file          rtmidi.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \library       sequencer64 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2018-04-12
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
 */

#include "easy_macros.hpp"              /* C++ version of easy macros       */
#include "rtmidi.hpp"                   /* seq64::rtmidi, etc.          */
#include "rtmidi_info.hpp"              /* seq64::rtmidi_info, etc.     */
#include "settings.hpp"                 /* seq64::rc().with_jack_...()  */

#ifdef SEQ64_BUILD_UNIX_JACK
#include "midi_jack.hpp"
#endif

#ifdef SEQ64_BUILD_LINUX_ALSA
#include "midi_alsa.hpp"
#endif

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * class rtmidi
 */

/**
 *  Default constructor.
 *
 * \param parentbus
 *      This is the midibus that the rtmidi object is going to implement, by
 *      forwarding calls to the selected MIDI subsystem (e.g. ALSA or JACK).
 *
 * \param info
 *      This object provides the system's enumerated busses/ports as found by
 *      the selected MIDI subsystem.
 */

rtmidi::rtmidi (midibus & parentbus, rtmidi_info & info)
 :
    midi_api        (parentbus, *(info.get_api_info())),
    m_midi_info     (info),
    m_midi_api      (nullptr)
{
   // No added code
}

/**
 *  Destructor.
 */

rtmidi::~rtmidi ()
{
    delete_api();
}

/*
 * class rtmidi_in
 */

/**
 *  Constructs the desired MIDI API.
 *
 *  If no system support for the specified API value is found, we issue a
 *  warning and continue as if no API was specified.  In this case, we iterate
 *  through the compiled APIs and return as soon as we find one with at least
 *  one port or we reach the end of the list.
 *
 * \param parentbus
 *      This is the midibus that the rtmidi object is going to implement, by
 *      forwarding calls to the selected MIDI subsystem (e.g. ALSA or JACK).
 *
 * \param info
 *      Contains information about the existing ports and the selected MIDI
 *      API.  Actually, the selected MIDI API is a static value of the
 *      rtmidi_info class.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

rtmidi_in::rtmidi_in (midibus & parentbus, rtmidi_info & info)
 :
    rtmidi   (parentbus, info)
{
    if (rtmidi_info::selected_api() != RTMIDI_API_UNSPECIFIED)
    {
        openmidi_api(rtmidi_info::selected_api(), info);
        if (is_nullptr(get_api()))
        {
            errprintfunc("no system support for specified API");
        }
    }
    else
    {
        /*
         * Generally, we should have already selected the API at this point,
         * so this is fallback code.
         */

        std::vector<rtmidi_api> apis;
        rtmidi_info::get_compiled_api(apis);
        for (unsigned i = 0; i < apis.size(); ++i)
        {
            openmidi_api(apis[i], info);
            if (info.get_api_info()->get_port_count() > 0)
            {
                rtmidi_info::selected_api(apis[i]);  /* log API that worked */
                break;
            }
        }
        if (is_nullptr(get_api()))
        {
            /*
             * It should not be possible to get here because the preprocessor
             * definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
             * API-specific definitions are passed to the compiler. But just in
             * case something weird happens, we'll throw an error.
             */

            std::string errortext = func_message("no compiled API support found");
            throw(rterror(errortext, rterror::UNSPECIFIED));
        }
    }
}

/**
 *  A do-nothing virtual destructor.
 */

rtmidi_in::~rtmidi_in()
{
   // no code
}

/**
 *  Opens the desired MIDI API.
 *
 * \param api
 *      The enum value for the desired MIDI API.  If not specified, first JACK
 *      is tried, then ALSA.
 *
 * \param info
 *      Holds information about the API's setup (e.g. a list of the ALSA ports
 *      found on the system, the main ALSA "handle" pointer, plus the PPQN,
 *      BPM, and other information).
 */

void
rtmidi_in::openmidi_api (rtmidi_api api, rtmidi_info & info)
{
    bool got_an_api = false;
    if (not_nullptr(info.get_api_info()))
    {
        midi_info & midiinfo = *(info.get_api_info());
        delete_api();
        if (api == RTMIDI_API_UNSPECIFIED)
        {
            if (rc().with_jack_midi())
            {
                if (api == RTMIDI_API_UNIX_JACK)
                {
#ifdef SEQ64_BUILD_UNIX_JACK
                    set_api(new midi_in_jack(parent_bus(), midiinfo));
                    got_an_api = true;
#endif
                }
            }
            if (! got_an_api)
            {
                if (api == RTMIDI_API_LINUX_ALSA)
                {
#ifdef SEQ64_BUILD_LINUX_ALSA
                    set_api(new midi_in_alsa(parent_bus(), midiinfo));
#endif
                }
            }
        }
        else if (api == RTMIDI_API_UNIX_JACK)
        {
#ifdef SEQ64_BUILD_UNIX_JACK
            set_api(new midi_in_jack(parent_bus(), midiinfo));
#endif
        }
        else if (api == RTMIDI_API_LINUX_ALSA)
        {
#ifdef SEQ64_BUILD_LINUX_ALSA
            set_api(new midi_in_alsa(parent_bus(), midiinfo));
#endif
        }
    }
}

/*
 * class rtmidi_out
 */

/**
 *  Principal constructor.  Attempt to open the specified API.  If there is no
 *  system support for the specified API value, then issue a warning and
 *  continue as if no API was specified.  In that case, we iterate through the
 *  compiled APIs and return as soon as we find one with at least one port or
 *  we reach the end of the list.
 *
 * \param parentbus
 *      This is the midibus that the rtmidi object is going to implement, by
 *      forwarding calls to the selected MIDI subsystem (e.g. ALSA or JACK).
 *
 * \param info
 *      Contains information about the existing ports and the selected MIDI
 *      API.  Actually, the selected MIDI API is a static value of the
 *      rtmidi_info class.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

rtmidi_out::rtmidi_out (midibus & parentbus, rtmidi_info & info)
 :
    rtmidi   (parentbus, info)
{
    if (rtmidi_info::selected_api() != RTMIDI_API_UNSPECIFIED)
    {
        openmidi_api(rtmidi_info::selected_api(), info);
        if (not_nullptr(get_api()))
        {
            return;
        }
        else
        {
            errprintfunc("no system support for specified API argument");
        }
    }

    /*
     * Generally, we should have already selected the API at this point,
     * so this is fallback code.
     */

    std::vector<rtmidi_api> apis;
    rtmidi_info::get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], info);
        if (info.get_api_info()->get_port_count() > 0)
        {
            rtmidi_info::selected_api(apis[i]); /* log the API that worked  */
            break;
        }
    }

    if (is_nullptr(get_api()))
    {
        /*
         * It should not be possible to get here because the preprocessor
         * definition SEQ64_BUILD_RTMIDI_DUMMY is automatically defined if no
         * API-specific definitions are passed to the compiler. But just in
         * case something weird happens, we'll thrown an error.
         */

        std::string errorText = func_message("no compiled API support found");
        throw(rterror(errorText, rterror::UNSPECIFIED));
    }
}

/**
 *  A do-nothing virtual destructor.
 */

rtmidi_out::~rtmidi_out()
{
   // no code
}

/***
 *  Opens the desired MIDI API.
 *
 * \param api
 *      Provides the API to be constructed.
 *
 * \param info
 *      Provides information about the port.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

void
rtmidi_out::openmidi_api (rtmidi_api api, rtmidi_info & info)
{
    bool got_an_api = false;
    if (not_nullptr(info.get_api_info()))
    {
        midi_info & midiinfo = *(info.get_api_info());
        delete_api();
        if (api == RTMIDI_API_UNSPECIFIED)
        {
            if (rc().with_jack_midi())
            {
                if (api == RTMIDI_API_UNIX_JACK)
                {
#ifdef SEQ64_BUILD_UNIX_JACK
                    set_api(new midi_out_jack(parent_bus(), midiinfo));
                    got_an_api = true;
#endif
                }
            }
            if (! got_an_api)
            {
                if (api == RTMIDI_API_LINUX_ALSA)
                {
#ifdef SEQ64_BUILD_LINUX_ALSA
                    set_api(new midi_out_alsa(parent_bus(), midiinfo));
#endif
                }
            }
        }
        else if (api == RTMIDI_API_UNIX_JACK)
        {
#ifdef SEQ64_BUILD_UNIX_JACK
            set_api(new midi_out_jack(parent_bus(), midiinfo));
#endif
        }
        else if (api == RTMIDI_API_LINUX_ALSA)
        {
#ifdef SEQ64_BUILD_LINUX_ALSA
            set_api(new midi_out_alsa(parent_bus(), midiinfo));
#endif
        }
    }
}

}           // namespace seq64

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

