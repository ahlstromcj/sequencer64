/**
 * \file          midi_probe.cpp
 *
 *  A function to check MIDI inputs and outputs, based on the RtMidi test
 *  program midiprobe.cpp.
 *
 * \author        Gary P. Scavone, 2003-2012; refactoring by Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2016-11-20
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
    /*
     * Create an API map.
     */

    std::map<rtmidi_api, std::string> api_map;
    api_map[RTMIDI_API_MACOSX_CORE] = "OS-X CoreMidi";
    api_map[RTMIDI_API_WINDOWS_MM]  = "Windows MultiMedia";
    api_map[RTMIDI_API_UNIX_JACK]   = "Jack Client";
    api_map[RTMIDI_API_LINUX_ALSA]  = "Linux ALSA";
    api_map[RTMIDI_API_DUMMY]       = "rtmidi dummy";

    std::vector<rtmidi_api> apis;
    rtmidi::get_compiled_api(apis);

    std::cout << "\nCompiled APIs:\n";
    for (unsigned i = 0; i < apis.size(); i++)
        std::cout << "  " << api_map[apis[i]] << std::endl;

    rtmidi_in * midiin = nullptr;
    rtmidi_out * midiout = nullptr;
    try
    {
        // rtmidi_in constructor ... exception possible

        midiin = new rtmidi_in();
        std::cout
            << "Current input API: "
            << api_map[ midiin->get_current_api() ]
            << std::endl
            ;

        // Check inputs.

        unsigned nPorts = midiin->get_port_count();
        std::cout
            << "There are "
            << nPorts << " MIDI input sources available."
            << std::endl
            ;

        for (unsigned i = 0; i < nPorts; ++i)
        {
            std::string portname = midiin->get_port_name(i);
            std::cout
                << "  Input Port #"
                << i + 1 << ": " << portname
                << std::endl
                ;
        }

        // rtmidi_out constructor ... exception possible

        midiout = new rtmidi_out();
        std::cout
            << "Current output API: "
            << api_map[midiout->get_current_api()]
            << std::endl
            ;

        // Check outputs.

        nPorts = midiout->get_port_count();
        std::cout
            << "There are " << nPorts << " MIDI output ports available."
            << std::endl
            ;

        for (unsigned i = 0; i < nPorts; ++i)
        {
            std::string portname = midiout->get_port_name(i);
            std::cout
                << "  Output Port #" << i + 1 << ": " << portname
                << std::endl
                ;
        }
        std::cout << std::endl;
    }
    catch (rterror & error)
    {
        error.printMessage();
    }
    delete midiin;
    delete midiout;
    return 0;
}

}           // namespace seq64

/*
 * midi_probe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

