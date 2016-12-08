#ifndef SEQ64_RTMIDI_INFO_HPP
#define SEQ64_RTMIDI_INFO_HPP

/**
 * \file          rtmidi_info.hpp
 *
 *  A base class for enumerating MIDI clients and ports.
 *
 * \author        refactoring by Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2016-12-08
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 * \license       GNU GPLv2 or above
 *
 *  This class is like the rtmidi_in and rtmidi_out classes, but cut down to
 *  the interface functions needed to enumerate clients and ports.  It is a
 *  wrapper/selector for the new midi_info class and its children.
 */

#include <exception>
#include <iostream>
#include <string>

#include "easy_macros.h"                    /* platform macros for compiler */
#include "seq64_rtmidi_features.h"          /* SEQ64_BUILD_LINUX_ALSA etc.  */
// #include "midi_api.hpp"                     /* seq64::midi[_in][_out]_api   */
// #include "rterror.hpp"                      /* seq64::rterror               */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A class for enumerating MIDI clients and ports.  New, but ripe for
 *  refactoring nonetheless.
 */

class rtmidi_info
{

    friend class midibus;

protected:

    midi_api * m_rtapi;

    /**
     *  To save from repeated queries, we save this value.  Its default value
     *  is RTMIDI_API_UNSPECIFIED.
     */

    rtmidi_api m_selected_api;

public:

    rtmidi_info ();
    virtual ~rtmidi_info ();

    /*
     *  A static function to determine the current rtmidi version.
     */

    static std::string get_version ();

    /*
     *  A static function to determine the available compiled MIDI APIs.  The
     *  values returned in the std::vector can be compared against the
     *  enumerated list values.  Note that there can be more than one API
     *  compiled for certain operating systems.
     */

    static void get_compiled_api (std::vector<rtmidi_api> & apis);

    void rtmidi_info::openmidi_api
    (
       rtmidi_api api,
       const std::string & clientname,
       unsigned queuesizelimit
    );

    /**
     * \getter m_selected_api
     */

    rtmidi_api selected_api () const
    {
        return m_selected_api;
    }

    virtual unsigned get_port_count ()
    {
        return m_rtapi->port_count();
    }

    virtual std::string get_port_name (unsigned portnumber = 0)
    {
        return m_rtapi->port_name(portnumber);
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

    virtual int get_client_id (int index)
    {
        return m_rtapi->get_client_id(index);
    }

protected:

    /**
     * \setter m_selected_api
     */

    void selected_api (rtmidi_api api)
    {
        m_selected_api = api;
    }

};          // class rtmidi_info

}           // namespace seq64

#endif      // SEQ64_RTMIDI_INFO_HPP

/*
 * rtmidi_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

