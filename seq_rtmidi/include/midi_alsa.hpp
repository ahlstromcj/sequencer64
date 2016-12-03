#ifndef SEQ64_MIDI_ALSA_HPP
#define SEQ64_MIDI_ALSA_HPP

/**
 * \file          midi_alsa.hpp
 *
 *    A class for realtime MIDI input/output via ALSA.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-03
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    In this refactoring...
 */

#include <string>

#include "midi_api.hpp"
#include "rtmidi_types.hpp"                 /* seq64::rtmidi_api type       */

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
    virtual ~midi_in_alsa ();

    /**
     * \getter RTMIDI_API_LINUX_ALSA
     */

    virtual rtmidi_api get_current_api () const
    {
        return RTMIDI_API_LINUX_ALSA;
    }

    virtual void open_port (unsigned portnumber, const std::string & portname);
    virtual void open_virtual_port (const std::string & portname);
    virtual void close_port ();
    virtual unsigned get_port_count ();
    virtual std::string get_port_name (unsigned portnumber);

protected:

    virtual void initialize(const std::string & clientname);

};          // midi_in_alsa

/**
 *  The class for handling ALSA MIDI output.
 */

class midi_out_alsa: public midi_out_api
{

public:

    midi_out_alsa (const std::string & clientname);
    virtual ~midi_out_alsa ();

    virtual rtmidi_api get_current_api () const
    {
        return RTMIDI_API_LINUX_ALSA;
    }

    virtual void open_port (unsigned portnumber, const std::string & portname);
    virtual void open_virtual_port (const std::string & portname);
    virtual void close_port ();
    virtual unsigned get_port_count ();
    virtual std::string get_port_name (unsigned portnumber);
    virtual void send_message (const std::vector<midibyte> & message);

protected:

    virtual void initialize (const std::string & clientname);

};          // midi_out_alsa

}           // namespace seq64

#endif      // SEQ64_MIDI_ALSA_HPP

/*
 * midi_alsa.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

