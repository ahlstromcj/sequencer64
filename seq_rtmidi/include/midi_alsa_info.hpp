#ifndef SEQ64_MIDI_ALSA_INFO_HPP
#define SEQ64_MIDI_ALSA_INFO_HPP

/**
 * \file          midi_alsa_info.hpp
 *
 *    A class for holding the current status of the ALSA system on the host.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-04
 * \updates       2016-12-06
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    We need to have a way to get all of the ALSA information of
 *    the midi_alsa
 */

#include "midi_info.hpp"                /* seq::midi_port_info etc.     */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The class for handling ALSA MIDI input.
 */

class midi_alsa_info
{

private:

    /**
     *  Flags that denote queries for input (read) ports.
     */

    static unsigned sm_input_caps;

    /**
     *  Flags that denote queries for output (write) ports.
     */

    static unsigned sm_output_caps;

    /**
     *  Holds a "handle" to the ALSA MIDI subsystem.
     */

    snd_seq_t m_alsa_seq;

    /**
     *  Holds data on the ALSA ports.
     */

    snd_seq_port_info_t m_alsa_port_info;

public:

    midi_alsa_info ();

private:

    /* virtual */ void initialize(const std::string & clientname);

    unsigned alsa_port_info
    (
        unsigned type, int portnumber
    );

};          // midi_alsa_info

}           // namespace seq64

#endif      // SEQ64_MIDI_ALSA_INFO_HPP

/*
 * midi_alsa_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

