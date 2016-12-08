#ifndef SEQ64_MIDI_INFO_HPP
#define SEQ64_MIDI_INFO_HPP

/**
 * \file          midi_info.hpp
 *
 *    A class for holding the current status of the ALSA system on the host.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-05
 * \updates       2016-12-08
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    We need to have a way to get all of the ALSA information of
 *    the midi_alsa
 */

#include <string>
#include <vector>

#include "rtmidi_types.hpp"

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
        unsigned m_client_number;   /**< The major buss number of the port. */
        unsigned m_port_number;     /**< The minor port number of the port. */
        std::string m_port_name;    /**< The system's name for the port.    */

    } port_info_t;

    /**
     *  Holds the number of ports counted.
     */

    unsigned m_port_count;

    /**
     *  Holds information on all of the ports that were "scanned".
     */

    std::vector<port_info_t> m_port_container;

public:

    midi_port_info ();

    void add
    (
        unsigned clientnumber,
        unsigned portnumber,
        const std::string & portname
    );

    void clear ()
    {
        m_port_container.clear();
    }

    unsigned get_port_count () const
    {
        return m_port_count;
    }

    unsigned get_client_id (unsigned index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_client_number;
        else
            return SEQ64_BAD_PORT_ID;
    }

    unsigned get_port_number (unsigned index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_number;
        else
            return SEQ64_BAD_PORT_ID;
    }

    const std::string & get_port_name (unsigned index) const
    {
        static std::string s_dummy;
        if (index < get_port_count())
            return m_port_container[index].m_port_name;
        else
            return s_dummy;
    }

};          // class midi_port_info

/**
 *  Macros for selecting input versus output ports
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

    unsigned get_port_count (bool input) const
    {
        return input ?
            m_input.get_port_count() : m_output.get_port_count() ;
    }

    unsigned get_client_id (bool input, unsigned index) const
    {
        return input ?
            m_input.get_client_id(index) :
            m_output.get_client_id(index) ;
    }

    unsigned get_port_number (bool input, unsigned index) const
    {
        return input ?
            m_input.get_port_number(index) :
            m_output.get_port_number(index) ;
    }

    const std::string & get_port_name (bool input, unsigned index) const
    {
        return input ?
            m_input.get_port_name(index) :
            m_output.get_port_name(index) ;
    }

    std::string port_list () const;

};          // midi_info

}           // namespace seq64

#endif      // SEQ64_MIDI_INFO_HPP

/*
 * midi_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

