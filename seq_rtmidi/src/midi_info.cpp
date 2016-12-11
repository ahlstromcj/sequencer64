/**
 * \file          midi_info.cpp
 *
 *    A class for obrtaining ALSA information
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2016-12-10
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
    unsigned clientnumber,
    unsigned portnumber,
    const std::string & portname
)
{
    port_info_t temp;
    temp.m_client_number = clientnumber;
    temp.m_port_number = portnumber;
    temp.m_port_name = portname;
    m_port_container.push_back(temp);
    m_port_count = unsigned(m_port_container.size());
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
    unsigned inportcount = m_input.get_port_count();
    unsigned outportcount = m_output.get_port_count();
    std::ostringstream os;
    midi_info * nc_this = const_cast<midi_info *>(this);

    nc_this->midi_mode(SEQ64_MIDI_INPUT);
    os << "Input ports (" << inportcount << "):" << std::endl;
    for (unsigned i = 0; i < inportcount; ++i)
    {
        os << "  " << nc_this->get_port_name(i) << std::endl;
    }

    nc_this->midi_mode(SEQ64_MIDI_OUTPUT);
    os << "Output ports (" << outportcount << "):" << std::endl;
    for (unsigned o = 0; o < outportcount; ++o)
    {
        os << "  " << nc_this->get_port_name(o) << std::endl;
    }
    return os.str();
}

}           // namespace seq64

/*
 * midi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

