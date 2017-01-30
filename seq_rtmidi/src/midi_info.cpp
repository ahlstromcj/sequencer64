/**
 * \file          midi_info.cpp
 *
 *    A class for obrtaining ALSA information
 *
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2017-01-28
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  This class is meant to collect a whole bunch of ALSA information
 *  about client/buss number, port numbers, and port names, and hold them
 *  for usage when creating midibus objects and midi_api objects.
 */

#include <sstream>                      /* std::ostringstream               */

#include "midibus.hpp"
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
    m_port_count        (0),
    m_port_container    ()
{
    // Empty body
}

/**
 *  Adds a set of port information to the port container.
 *
 * \param clientnumber
 *      Provides the client or buss number for the port.  This is a value like
 *
 * \param clientname
 *      Provides the system or user-supplied name for the client or buss.
 *
 * \param portnumber
 *      Provides the port number, usually re 0.
 *
 * \param portname
 *      Provides the system or user-supplied name for the port.
 *
 * \param makevirtual
 *      If the system currently has no input or output port available, then we
 *      want to create a virtual port so that the application has something to
 *      work with.
 *
 * \param queuenumber
 *      Provides the optional queue number, if applicable.  For example, the
 *      sequencer64 application grabs the client number (normally valued at 1)
 *      from the ALSA subsystem.
 *
 * \param makesystem
 *      In some systems, we need to create and activate a system port, such as
 *      a timer port or an ALSA announce port.  For all other ports, this
 *      value is false.
 */

void
midi_port_info::add
(
    int clientnumber,
    const std::string & clientname,
    int portnumber,
    const std::string & portname,
    bool makevirtual,
    int queuenumber,
    bool makesystem
)
{
    port_info_t temp;
    temp.m_client_number = clientnumber;
    temp.m_client_name = clientname;
    temp.m_port_number = portnumber;
    temp.m_port_name = portname;
    temp.m_is_virtual = makevirtual;
    temp.m_queue_number = queuenumber;
    temp.m_is_system = makesystem;
    m_port_container.push_back(temp);
    m_port_count = int(m_port_container.size());
}

/**
 *  Adds values from a midibus (actually a midibase class).
 *
 *  WHY ARE SOME OF THE FIELDS (e.g. port name) empty????
 */

void
midi_port_info::add (const midibus * m)
{
    add
    (
        m->get_bus_id(), m->bus_name(),
        m->get_port_id(), m->port_name(),
        m->is_virtual_port(), m->queue_number(),
        m->is_system_port()
    );
}

/*
 * class midi_info
 */

/**
 *  Principal constructor.
 */

midi_info::midi_info
(
    const std::string & appname,
    int ppqn,
    int bpm
) :
    m_midi_mode_input   (true),
    m_input             (),                 /* midi_port_info for inputs    */
    m_output            (),                 /* midi_port_info for outputs   */
    m_bus_container     (),
    m_global_queue      (SEQ64_NO_QUEUE),   /* a la mastermidibase; created */
    m_midi_handle       (nullptr),          /* usually looked up or created */
    m_app_name          (appname),
    m_ppqn              (ppqn),
    m_bpm               (bpm),
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
#ifdef PLATFORM_DEBUG                       // SEQ64_USE_DEBUG_OUTPUT
        errprint(errorstring.c_str());
#endif
    }
    else
    {
        errprint(errorstring.c_str());

        /*
         * Not a big fan of throwing errors, especially since we currently log
         * errors in rtmidi to the console.  Might make this a build option.
         *
         * throw rterror(errorstring, type);
         */
    }
}

/**
 *  Generates a string listing all of the ports present in the port container.
 *  Useful for debugging and probing.
 *
 * \return
 *      Returns a multi-line ASCII string enumerating all of the ports.
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
        std::string annotation;
        if (nc_this->get_virtual(i))
            annotation = "virtual";
        else if (nc_this->get_system(i))
            annotation = "system";

        os
            << "  [" << i << "] "
            << nc_this->get_bus_id(i) << ":" << nc_this->get_port_id(i) << " "
            << nc_this->get_bus_name(i) << ":" << nc_this->get_port_name(i)
            ;

        if (! annotation.empty())
            os << " (" << annotation << ")";

        os << std::endl;
    }

    nc_this->midi_mode(SEQ64_MIDI_OUTPUT);
    os << "Output ports (" << outportcount << "):" << std::endl;
    for (int o = 0; o < outportcount; ++o)
    {
        os
            << "  [" << o << "] "
            << nc_this->get_bus_id(o) << ":" << nc_this->get_port_id(o)
            << " " << nc_this->get_bus_name(o) << ":"
            << nc_this->get_port_name(o)
            << (nc_this->get_virtual(o) ? " (virtual)" : " ")
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

