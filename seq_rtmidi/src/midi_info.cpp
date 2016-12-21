/**
 * \file          midi_info.cpp
 *
 *    A class for obrtaining ALSA information
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2016-12-20
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
    const std::string & clientname,
    int portnumber,
    const std::string & portname,
    int queuenumber
)
{
    port_info_t temp;
    temp.m_client_number = clientnumber;
    temp.m_client_name = clientname;
    temp.m_port_number = portnumber;
    temp.m_port_name = portname;
    temp.m_queue_number = queuenumber;
    m_port_container.push_back(temp);
    m_port_count = unsigned(m_port_container.size());
}

/*
 * class midi_info
 */

/**
 *  Principal constructor.
 */

midi_info::midi_info (int queuenumber)
 :
    m_midi_mode_input   (true),
    m_input             (),             /* midi_port_info       */
    m_output            (),             /* midi_port_info       */
    m_queue             (queuenumber),  /* a la mastermidibase  */
    m_error_string      ()
{
    //
}

/**
 *  Provides an error handler.  Unlike the midi_api version, it cannot support
 *  an error callback.
 *
 * \throw
 *      If the error is not just a warning, then an rterror object is thrown.
 *
 * \param type
 *      The type of the error.
 *
 * \param errorstring
 *      The error message, which gets copied if this is the first error.
 */

void
midi_info::error (rterror::Type type, const std::string & errorstring)
{
    if (type == rterror::WARNING)
    {
        errprint(errorstring.c_str());
    }
    else if (type == rterror::DEBUG_WARNING)
    {
#ifdef SEQ64_USE_DEBUG_OUTPUT
        errprint(errorstring.c_str());
#endif
    }
    else
    {
        errprint(errorstring.c_str());
        throw rterror(errorstring, type);
    }
}

/**
 *
 */

std::string
midi_info::port_list () const
{
    int inportcount = m_input.get_port_count();
    int outportcount = m_output.get_port_count();
    std::ostringstream os;
    midi_info * nc_this = const_cast<midi_info *>(this);

    nc_this->midi_mode(SEQ64_MIDI_INPUT);
    os << "Input ports (" << inportcount << "):" << std::endl;
    for (int i = 0; i < inportcount; ++i)
    {
        os
            << "  [" << i << "] "
            << nc_this->get_bus_id(i) << ":" << nc_this->get_port_id(i)
            << " " << nc_this->get_bus_name(i) << ": "
            << nc_this->get_port_name(i)
            << std::endl
            ;
    }

    nc_this->midi_mode(SEQ64_MIDI_OUTPUT);
    os << "Output ports (" << outportcount << "):" << std::endl;
    for (int o = 0; o < outportcount; ++o)
    {
        os
            << "  [" << o << "] "
            << nc_this->get_bus_id(o) << ":" << nc_this->get_port_id(o)
            << "  " << nc_this->get_bus_name(o) << ":"
            << nc_this->get_port_name(o)
            << std::endl
            ;
    }
    return os.str();
}

}           // namespace seq64

/*
 * midi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

