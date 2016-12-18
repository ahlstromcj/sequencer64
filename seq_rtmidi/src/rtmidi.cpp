/**
 * \file          rtmidi.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-18
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

#include "rtmidi.hpp"                   /* seq64::rtmidi, etc.          */
#include "settings.hpp"                 /* seq64::rc().with_jack_...()  */

#ifdef SEQ64_BUILD_UNIX_JACK
#include "midi_jack.hpp"
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
 */

rtmidi::rtmidi ()
 :
    rtmidi_base     ()
{
   // No added code
}

/**
 *  Destructor.
 */

rtmidi::~rtmidi ()
{
    // No added code
}

/*
 * class rtmidi_in
 */

/**
 *  Constructs the desired MIDI API.
 *
 *  If no compiled support for specified API value is found, we issue a warning
 *  and continue as if no API was specified.  In this case, we iterate through
 *  the compiled APIs and return as soon as we find one with at least one port
 *  or we reach the end of the list.
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

rtmidi_in::rtmidi_in
(
   rtmidi_api api,
   const std::string & clientname,
   unsigned queuesizelimit
) :
    rtmidi   ()
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        openmidi_api(api, clientname, queuesizelimit);
        if (not_nullptr(get_api()))
        {
            selected_api(api);              /* log the API that worked  */
            return;
        }
        errprintfunc("no compiled support for specified API");
    }

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname, queuesizelimit);
        if (get_api()->get_port_count() > 0)
        {
            selected_api(apis[i]);          /* log the API that worked  */
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
 *      The desired MIDI API.
 *
 * \param clientname
 *      The name of the client.  This is the name used to access the client.
 *
 * \param queuesizelimit
 *      The maximum size of the MIDI message queue.
 */

void
rtmidi_in::openmidi_api
(
   rtmidi_api api,
   const std::string & clientname,
   unsigned queuesizelimit
)
{
    delete_api();

#ifdef SEQ64_BUILD_UNIX_JACK
    if (rc().with_jack_transport())
    {
        if (api == RTMIDI_API_UNIX_JACK)
            set_api(new midi_in_jack(clientname, queuesizelimit));
    }
#endif

}

/**
 *  Throws an error.
 */

void
rtmidi_in::send_message (const std::vector<midibyte> &)
{
    std::string errortext = func_message("not supported");
    throw(rterror(errortext, rterror::UNSPECIFIED));
}

/*
 * class rtmidi_out
 */

/**
 *  Principal constructor.  Attempt to open the specified API.  If there is no
 *  compiled support for specified API value, then issue a warning and
 *  continue as if no API was specified.  In that case, we Iterate through the
 *  compiled APIs and return as soon as we find one with at least one port or
 *  we reach the end of the list.
 *
 * \param api
 *      Provides the API to be constructed.
 *
 * \param clientname
 *      Provides the name of the MIDI output client, used by some of the APIs.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

rtmidi_out::rtmidi_out (rtmidi_api api, const std::string & clientname)
 :
    rtmidi   ()
{
    if (api != RTMIDI_API_UNSPECIFIED)
    {
        openmidi_api(api, clientname);
        if (not_nullptr(get_api()))
        {
            selected_api(api);              /* log the API that worked  */
            return;
        }
        errprintfunc("no compiled support for specified API argument");
    }

    std::vector<rtmidi_api> apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], clientname);
        if (get_api()->get_port_count() > 0)
        {
            selected_api(apis[i]);          /* log the API that worked  */
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
 * \param clientname
 *      Provides the name of the MIDI output client, used by some of the APIs.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

void
rtmidi_out::openmidi_api (rtmidi_api api, const std::string & clientname)
{
    delete_api();

#ifdef SEQ64_BUILD_UNIX_JACK
    if (rc().with_jack_transport())
    {
        if (api == RTMIDI_API_UNIX_JACK)
            set_api(new midi_out_jack(clientname));
    }
#endif
}

/**
 *  Throws an error.  No, for now just does nothing.
 */

void
rtmidi_out::ignore_types (bool, bool, bool)
{
    /*
     * std::string errortext = func_message("not supported");
     * throw(rterror(errortext, rterror::UNSPECIFIED));
     */
}

/**
 *  Throws an error.  No, for now just does nothing.
 */

double
rtmidi_out::get_message (std::vector<midibyte> &)
{
    /*
     * std::string errortext = func_message("not supported");
     * throw(rterror(errortext, rterror::UNSPECIFIED));
     */

    return 0.0;
}

}           // namespace seq64

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

