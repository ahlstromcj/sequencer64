#ifndef SEQ64_MIDI_INFO_HPP
#define SEQ64_MIDI_INFO_HPP

/**
 * \file          midi_info.hpp
 *
 *    A class for holding the current status of the ALSA system on the host.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-05
 * \updates       2016-12-06
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

class midi_port_info
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
     *  Holds information on all of the ports that were "scanned".
     */

    std::vector<port_info_t> m_port_container;

public:

    midi_port_info ();

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

};          // class midi_port_info

/**
 *  Macros
 */

#define SEQ64_MIDI_OUTPUT       false
#define SEQ64_MIDI_INPUT        true

/**
 *  The class for handling ALSA MIDI input.
 */

class midi_info
{

private:

    /**
     *  Holds data on the ALSA/JACK/Core/WinMM inputs.
     */

    midi_port_info m_input;

    /**
     *  Holds data on the ALSA/JACK/Core/WinMM outputs.
     */

    midi_port_info m_output;

public:

    midi_info ();

    /**
     * \getter m_input
     */

    midi_port_info & input_ports ()
    {
        return m_input;
    }

    /**
     * \getter m_output
     */

    midi_port_info & output_ports ()
    {
        return m_output;
    }

    /**
     *
     */

    int port_count (bool input) const
    {
        return input ?
            m_input.port_count() : m_output.port_count() ;
    }

    int client_number (int index, bool input) const
    {
        return input ?
            m_input.client_number(index) :
            m_output.client_number(index) ;
    }

    int port_number (int index, bool input) const
    {
        return input ?
            m_input.port_number(index) :
            m_output.port_number(index) ;
    }

    const std::string & port_name (int index) const
    {
        return input ?
            m_input.port_name(index) :
            m_output.port_nname(index) ;
    }

private:

    void initialize(const std::string & clientname);

    virtual unsigned get_all_port_info ();

};          // midi_info

}           // namespace seq64

#endif      // SEQ64_MIDI_INFO_HPP

/*
 * midi_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

