/**
 * \file          midi_probe.cpp
 *
 *  Functions to check MIDI inputs and outputs, based on the RtMidi test
 *  programs.
 *
 * \author        Gary P. Scavone, 2003-2012; refactoring by Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2017-01-11
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  We include this test code in our library, rather than in a separate
 *  application, because we want to include some diagnostic code in the
 *  application.
 */

#include <iostream>
#include <cstdlib>
#include <map>

#include "easy_macros.h"
#include "midi_probe.hpp"
#include "midibus_rm.hpp"
#include "rtmidi.hpp"                   /* rtmidi_in and rt_midi_out */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Function to get RtMidi API names in a reusable manner.
 *
 * \param i
 *      The integer value code for the desired API.  Must range from
 *      int(RTMIDI_API_UNSPECIFIED) to int(RTMIDI_API_DUMMY).
 *
 * \return
 *      Returns a human-readable name for the API.
 */

std::string
midi_api_name (int i)
{
    static std::map<rtmidi_api, std::string> s_api_map;
    static bool s_map_is_initialized = false;
    if (! s_map_is_initialized)
    {
        s_api_map[RTMIDI_API_UNSPECIFIED] = "Unspecified";
        s_api_map[RTMIDI_API_LINUX_ALSA]  = "Linux ALSA";
        s_api_map[RTMIDI_API_UNIX_JACK]   = "Jack Client";

#ifdef USE_RTMIDI_API_ALL

        /*
         * We're not supporting these until we get a simplified
         * sequencer64-friendly API worked out.
         */

        s_api_map[RTMIDI_API_MACOSX_CORE] = "OS-X CoreMidi";
        s_api_map[RTMIDI_API_WINDOWS_MM]  = "Windows MultiMedia";
        s_api_map[RTMIDI_API_DUMMY]       = "rtmidi dummy";
#endif

        s_map_is_initialized = true;
    }

    std::string result = "Unknown MIDI API";
    if (i >= int(RTMIDI_API_UNSPECIFIED) && i < int(RTMIDI_API_MAXIMUM))
        result = s_api_map[rtmidi_api(i)];

    return result;
}

/**
 *  Formerly the main program of the RtMidi test program midiprobe.
 *  We will upgrade this function for some better testing eventually.
 *  It uses the functionality of the midi_info/rtmidi_info objects, plus
 *  its own version of some of that functionality.
 *
 * \return
 *      Currently always returns 0.
 */

int
midi_probe ()
{
    static rtmidi_info s_rtmidi_info_dummy;
    static midibus s_midibus_dummy(s_rtmidi_info_dummy, "dummy", 0);
    std::vector<rtmidi_api> apis;
    rtmidi_info::get_compiled_api(apis);
    std::cout << "\nCompiled APIs:\n";
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        std::cout << "  " << midi_api_name(apis[i]) << std::endl;
    }

    try                         /* rtmidi constructors; exceptions possible */
    {
        rtmidi_info dummyinfo;
        rtmidi_in midiin(s_midibus_dummy, dummyinfo);
        std::cout
            << "MIDI Input/Output API: "
            << midi_api_name(rtmidi_info::selected_api())
            << std::endl
            ;

        int nports = midiin.get_port_count();
        std::cout << nports << " MIDI input sources:" << std::endl;
        for (int i = 0; i < nports; ++i)
        {
            std::string portname = midiin.get_port_name();
            std::cout
                << "  Input Port #" << i+1 << ": " << portname << std::endl
                ;
        }

        /*
         * We actually need to get this object in the loop!
         */

        rtmidi_out midiout(s_midibus_dummy, dummyinfo);
        std::cout << std::endl;

        nports = midiout.get_port_count();
        std::cout << nports << " MIDI output ports:" << std::endl;
        for (int i = 0; i < nports; ++i)
        {
            std::string portname = midiout.get_port_name();
            std::cout
                << "  Output Port #" << i+1 << ": " << portname << std::endl
                ;
        }
        std::cout << std::endl;
    }
    catch (const rterror & error)
    {
        error.print_message();
    }
    return 0;
}

/**
 *  Provides the callback for midi_input_test().
 */

static void
midi_input_callback (midi_message & message, void * /*userdata*/)
{
    if (! message.empty())
    {
        std::cout
            << "Message (" << message.count() << " bytes, "
            << "delta = " << message.timestamp() << "):"
            << std::endl
            ;
        for (int i = 0; i < message.count(); ++i)
        {
            std::cout << "  byte[" << i << "] = " << int(message[i]) << "; ";
        }
    }
}

/**
 *  Provides testing the MIDI input process for 10 seconds.
 */

bool
midi_input_test (rtmidi_info & info, int portindex)
{
    bool result = false;
    try
    {
        static midibus s_midibus_dummy(info, "dummy", portindex);
        rtmidi_in midiin(s_midibus_dummy, info);
        midiin.user_callback(midi_input_callback);
//      midiin.ignore_types(false, false, false); // sysex, timing, active sensing

        result = true;
        if (result)
        {
            std::cout << "You have 10 seconds to play some MIDI" << std::endl;
            millisleep(10000);
        }
    }
    catch (rterror & e)
    {
        e.print_message();
    }
    return result;
}

}           // namespace seq64

/*
 * midi_probe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

