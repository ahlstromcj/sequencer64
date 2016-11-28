/**
 * \file          midi_probe.cpp
 *
 *  A function to check MIDI inputs and outputs, based on the RtMidi test
 *  program midiprobe.cpp.
 *
 * \author        Gary P. Scavone, 2003-2012; refactoring by Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2016-11-27
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
#include "rtmidi.hpp"

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
        s_api_map[RTMIDI_API_MACOSX_CORE] = "OS-X CoreMidi";
        s_api_map[RTMIDI_API_WINDOWS_MM]  = "Windows MultiMedia";
        s_api_map[RTMIDI_API_DUMMY]       = "rtmidi dummy";
        s_map_is_initialized = true;
    }

    std::string result = "Unknown MIDI API";
    if (i >= int(RTMIDI_API_UNSPECIFIED) && i <= int(RTMIDI_API_DUMMY))
        result = s_api_map[rtmidi_api(i)];

    return result;
}

/**
 *  Formerly the main program of the RtMidi test program midiprobe.
 *
 *  We will upgrade this function for some better testing eventually.
 *
 * \return
 *      Currently always returns 0.
 */

int
midi_probe ()
{
    std::vector<rtmidi_api> apis;
    rtmidi::get_compiled_api(apis);

    std::cout << "\nCompiled APIs:\n";
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        std::cout << "  " << midi_api_name(apis[i]) << std::endl;
    }

    try                         /* rtmidi constructors; exceptions possible */
    {
        rtmidi_in midiin;
        std::cout
            << "MIDI input API: "
            << midi_api_name(midiin.get_current_api())
            << std::endl
            ;

        unsigned nports = midiin.get_port_count();
        std::cout << nports << " MIDI input sources:" << std::endl;
        for (unsigned i = 0; i < nports; ++i)
        {
            std::string portname = midiin.get_port_name(i);
            std::cout << "  Input Port #" << i+1 << ": " << portname << std::endl;
        }

        rtmidi_out midiout;
        std::cout
            << "MIDI output API: "
            << midi_api_name(midiout.get_current_api())
            << std::endl
            ;

        nports = midiout.get_port_count();
        std::cout << nports << " MIDI output ports:" << std::endl;
        for (unsigned i = 0; i < nports; ++i)
        {
            std::string portname = midiout.get_port_name(i);
            std::cout << "  Output Port #" << i+1 << ": " << portname << std::endl;
        }
        std::cout << std::endl;
    }
    catch (const rterror & error)
    {
        error.print_message();
    }
    return 0;
}

}           // namespace seq64

/*
 * midi_probe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

