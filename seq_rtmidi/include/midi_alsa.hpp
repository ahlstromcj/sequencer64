#ifndef SEQ64_MIDI_ALSA_HPP
#define SEQ64_MIDI_ALSA_HPP

/**
 * \file          midi_alsa.hpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-19
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
 *  The class for handling ALSA MIDI input.
 */

class midi_in_alsa : public midi_in_api
{

public:

    midi_in_alsa (const std::string & clientname, unsigned queuesizelimit);
    ~midi_in_alsa ();

    /**
     * \getter RTMIDI_API_LINUX_ALSA
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_LINUX_ALSA;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);

protected:

    void initialize(const std::string & clientname);

};          // midi_in_alsa

/**
 *  The class for handling ALSA MIDI output.
 */

class midi_out_alsa: public midi_out_api
{

public:

    midi_out_alsa (const std::string & clientname);
    ~midi_out_alsa ();

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_LINUX_ALSA;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);
    void send_message (std::vector<midibyte> * message);

protected:

    void initialize (const std::string & clientname);

};          // midi_out_alsa

}           // namespace seq64

#endif      // SEQ64_MIDI_ALSA_HPP

/*
 * midi_alsa.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

