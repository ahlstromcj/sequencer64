#ifndef SEQ64_RTMIDI_INFO_HPP
#define SEQ64_RTMIDI_INFO_HPP

/**
 * \file          rtmidi_info.hpp
 *
 *  A base class for enumerating MIDI clients and ports.
 *
 * \author        refactoring by Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2016-12-12
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 * \license       GNU GPLv2 or above
 *
 *  This class is like the rtmidi_in and rtmidi_out classes, but cut down to
 *  the interface functions needed to enumerate clients and ports.  It is a
 *  wrapper/selector for the new midi_info class and its children.
 */

#include "midi_api.hpp"                     /* seq64::midi[_in][_out]_api   */
#include "rtmidi_base.hpp"                  /* seq64::rtmidi_base class     */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A class for enumerating MIDI clients and ports.  New, but ripe for
 *  refactoring nonetheless.
 */

class rtmidi_info : public rtmidi_base
{
    friend class midibus;

public:

    rtmidi_info (rtmidi_api api = RTMIDI_API_UNSPECIFIED);

    virtual ~rtmidi_info ();

    virtual void midi_mode (bool flag)
    {
        get_api()->midi_mode(flag);
    }

    /**
     *  Gets the buss/client ID for a MIDI interfaces.  This is the left-hand
     *  side of a X:Y pair (such as 128:0).
     *
     *  This function is a new part of the RtMidi interface.
     *
     * \param index
     *      The ordinal index of the desired interface to look up.
     *
     * \return
     *      Returns the buss/client value as provided by the selected API.
     */

    virtual unsigned get_client_id (unsigned index)
    {
        return get_api()->get_client_id(index);
    }

    virtual unsigned get_port_count ()
    {
        return get_api()->get_port_count();
    }

    virtual unsigned get_port_number (unsigned index)
    {
        return get_api()->get_port_number(index);
    }

    virtual std::string get_port_name (unsigned index)
    {
        return get_api()->get_port_name(index);
    }

    virtual unsigned get_all_port_info ()
    {
        return get_api()->get_all_port_info();
    }

protected:

    void openmidi_api
    (
       rtmidi_api api,
       const std::string & clientname,
       unsigned queuesizelimit
    );

};          // class rtmidi_info

}           // namespace seq64

#endif      // SEQ64_RTMIDI_INFO_HPP

/*
 * rtmidi_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

