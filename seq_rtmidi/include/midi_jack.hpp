#ifndef SEQ64_MIDI_JACK_HPP
#define SEQ64_MIDI_JACK_HPP

/**
 * \file          midi_jack.hpp
 *
 *    A class for realtime MIDI input/output via JACK.
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
 *  The class for handling JACK MIDI input.
 */

class midi_in_jack: public midi_in_api
{

protected:

    std::string m_clientname;

public:

    midi_in_jack (const std::string & clientname, unsigned queuesize);
    ~midi_in_jack ();

    /**
     * \getter RTMIDI_API_UNIX_JACK
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_UNIX_JACK;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);

protected:

    void connect ();
    void initialize (const std::string & clientname);

};          // midi_in_jack

/**
 *  The JACK MIDI output API class.
 */

class midi_out_jack: public midi_out_api
{

protected:

    std::string m_clientname;

public:

    midi_out_jack (const std::string & clientname);
    ~midi_out_jack ();

    /**
     * \getter RTMIDI_API_UNIX_JACK
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_UNIX_JACK;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);
    void send_message (std::vector<midibyte> * message);

protected:

    void connect ();
    void initialize (const std::string & clientname);

};          // midi_out_jack

}           // namespace seq64

#endif      // SEQ64_MIDI_JACK_HPP

/*
 * midi_jack.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

