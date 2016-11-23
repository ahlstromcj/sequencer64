#ifndef SEQ64_MIDI_WINMM_HPP
#define SEQ64_MIDI_WINMM_HPP

/**
 * \file          midi_winmm.hpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-16
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    In this refactoring...
 */

#include <string>

#include "midi_api.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The class for handling Windows MM API MIDI input.
 */

class midi_in_winmm: public midi_in_api
{

public:

    midi_in_winmm (const std::string & clientname, unsigned queuesizelimit);
    ~midi_in_winmm ();

    /**
     * \getter RTMIDI_API_WINDOWS_MM
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_WINDOWS_MM;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);

protected:

    void initialize (const std::string & clientname);

};          // class midi_in_winmm

/**
 *  The class for handling Windows MM API MIDI output.
 */

class midi_out_winmm: public midi_out_api
{

public:

    midi_out_winmm (const std::string & clientname);
    ~midi_out_winmm ();

    /**
     * \getter RTMIDI_API_WINDOWS_MM
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_WINDOWS_MM;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);
    void send_message (const std::vector<midibyte> & message);

protected:

    void initialize (const std::string & clientname);

};          // midi_out_winmm

}           // namespace seq64

#endif      // SEQ64_MIDI_WINMM_HPP

/*
 * midi_winmm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

