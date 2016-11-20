#ifndef SEQ64_MIDI_CORE_HPP
#define SEQ64_MIDI_CORE_HPP

/**
 * \file          midi_core.hpp
 *
 *    A class for realtime MIDI input/output via Mac OSX Core.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-19
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    In this refactoring, we are currently not concerned with making sure
 *    this class is airtight, because we have no Mac on which to test it.
 *    Help would be welcomed with open arms.
 */

#include <string>

#include "midi_in_api.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * midi_in_api and midi_out_api subclass prototypes.
 */

#if ! defined(SEQ64_BUILD_LINUX_ALSA) && ! defined(SEQ64_BUILD_UNIX_JACK) && \
 ! defined(SEQ64_BUILD_MACOSX_CORE) && ! defined(SEQ64_BUILD_WINDOWS_MM)

#define SEQ64_BUILD_RTMIDI_DUMMY

#endif

/**
 *  The class for handling Mac OSX Core MIDI input.
 */

class midi_in_core: public midi_in_api
{

public:

    midi_in_core (const std::string & clientname, unsigned queuesizelimit);
    ~midi_in_core ();

    /**
     * \getter RTMIDI_API_MACOSX_CORE
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_MACOSX_CORE;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);

protected:

    void initialize (const std::string & clientname);

};

/**
 *  The class for handling Mac OSX Core MIDI output.
 */

class midi_out_core: public midi_out_api
{

public:

    midi_out_core (const std::string & clientname);
    ~midi_out_core ();

    /**
     * \getter RTMIDI_API_MACOSX_CORE
     */

    rtmidi_api get_current_api () const
    {
        return RTMIDI_API_MACOSX_CORE;
    }

    void open_port (unsigned portnumber, const std::string & portname);
    void open_virtual_port (const std::string & portname);
    void close_port ();
    unsigned get_port_count ();
    std::string get_port_name (unsigned portnumber);
    void send_message (std::vector<midibyte> * message);

protected:

    void initialize (const std::string & clientname);

};          // midi_out_core

}           // namespace seq64

#endif      // SEQ64_MIDI_CORE_HPP

/*
 * midi_core.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

