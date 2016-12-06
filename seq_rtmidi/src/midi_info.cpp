/**
 * \file          midi_info.cpp
 *
 *    A class for obrtaining ALSA information
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2016-12-06
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  This class is meant to collect a whole bunch of ALSA information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating midibus objects and midi_api objects.
 */

#include <sstream>                      /* std::ostringstream               */

#include "midi_info.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * class midi_port_info
 */

/**
 *  Principal constructor.
 */

midi_port_info::midi_port_info
(
) :
    m_port_count        (),
    m_port_container    ()
{
    //
}

/**
 *
 */

void
midi_port_info::add
(
    int clientnumber,
    int portnumber,
    const std::string & portname
)
{
    port_info_t temp;
    temp.m_client_number = clientnumber;
    temp.m_port_number = portnumber;
    temp.m_port_name = portname;
    m_port_container.push_back(temp);
}

/*
 * class midi_info
 */

/**
 *  Principal constructor.
 */

midi_info::midi_info
(
) :
    m_input     (),
    m_output    ()
{
    //
}

/**
 *
 */

std::string
midi_info::port_list () const
{
    int in_portcount = m_input.port_count();
    int out_portcount = m_output.port_count();
    std::ostringstream os;

    os << "Input ports (" << inportcount << "):" << std::endl;
    for (int i = 0; i < inportcount; ++i)
    {
        os << "  " << port_name(SEQ64_MIDI_INPUT, i) << std::endl;
    }

    os << "Output ports (" << outportcount << "):" << std::endl;
    for (int o = 0; o < outportcount; ++o)
    {
        os << "  " << port_name(SEQ64_MIDI_OUTPUT, o) << std::endl;
    }
    return os.str();
}


}           // namespace seq64

/*
 * midi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

