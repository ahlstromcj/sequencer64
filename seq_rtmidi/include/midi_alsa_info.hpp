#ifndef SEQ64_MIDI_ALSA_INFO_HPP
#define SEQ64_MIDI_ALSA_INFO_HPP

/**
 * \file          midi_alsa_info.hpp
 *
 *    A class for holding the current status of the ALSA system on the host.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-04
 * \updates       2016-12-04
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    We need to have a way to get all of the ALSA information of
 *    the midi_alsa
 */

#include <string>
#include <vector>

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A class for holding port information.
 */

class midi_info
{

private:

    /**
     *  Hold the information for a single port.
     */

    typedef struct
    {
        int m_client_number;        /**< The major buss number of the port. */
        int m_port_number;          /**< The minor port number of the port. */
        std::string m_port_name;    /**< The system's name for the port.    */

    } port_info_t;

    /**
     *  Holds the number of ports counted.
     */

    int m_port_count;

    /**
     *
     */

    std::vector<port_info_t> m_port_container;

public:

    midi_info ();

    int port_count () const
    {
        return m_port_count;
    }

    int client_number (int index)
    {
        if (index >= 0 && index < port_count())
            return m_port_container[index].m_client_number;
        else
            return -1;
    }

    int port_number (int index)
    {
        if (index >= 0 && index < port_count())
            return m_port_container[index].m_port_number;
        else
            return -1;
    }

    const std::string & port_name (int index)
    {
        static std::string s_dummy;
        if (index >= 0 && index < port_count())
            return m_port_container[index].m_port_name;
        else
            return s_dummy;
    }

};          // class midi_info

/**
 *  Macros
 */

#define SEQ64_MIDI_OUTPUT       false
#define SEQ64_MIDI_INPUT        true

/**
 *  The class for handling ALSA MIDI input.
 */

class midi_alsa_info
{

private:

    /**
     *  Holds data on the ALSA inputs.
     */

    midi_info m_alsa_input;

    /**
     *  Holds data on the ALSA outputs.
     */

    midi_info m_alsa_output;

public:

    midi_alsa_info ();

    int port_count (bool input)
    {
        return input ?
            m_alsa_input.port_count() : m_alsa_output.port_count() ;
    }

    int client_number (int index, bool input)
    {
        return input ?
            m_alsa_input.client_number(index) :
            m_alsa_output.client_number(index) ;
    }

    int port_number (int index, bool input)
    {
        return input ?
            m_alsa_input.port_number(index) :
            m_alsa_output.port_number(index) ;
    }

    const std::string & port_name (int index);
    {
        return input ?
            m_alsa_input.port_name(index) :
            m_alsa_output.port_nname(index) ;
    }

private:

    /* virtual */ void initialize(const std::string & clientname);

    unsigned alsa_port_info
    (
        snd_seq_t * seq, snd_seq_port_info_t * pinfo,
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

