#ifndef SEQ64_RTMIDI_BASE_HPP
#define SEQ64_RTMIDI_BASE_HPP

/**
 * \file          rtmidi_base.hpp
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
 *  wrapper/selector for the new midi_base class and its children.
 */

#include <string>

#include "easy_macros.h"                    /* platform macros for compiler */
#include "seq64_rtmidi_features.h"          /* SEQ64_BUILD_LINUX_ALSA etc.  */
#include "rtmidi_types.hpp"                 /* rtmidi helper entities       */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class midi_api;

/**
 *  A class for enumerating MIDI clients and ports.  New, but ripe for
 *  refactoring nonetheless.
 */

class rtmidi_base
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

    rtmidi_base ();
    virtual ~rtmidi_base ();

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

    virtual unsigned get_port_count () = 0;
    virtual unsigned get_client_id (unsigned portnumber) = 0;
    virtual unsigned get_port_number (unsigned portnumber) = 0;
    virtual std::string get_port_name (unsigned portnumber = 0) = 0;

    /**
     * \getter m_selected_api
     */

    rtmidi_api & selected_api ()
    {
        return m_selected_api;
    }


    /**
     * \getter m_rtapi
     */

    midi_api * api ()
    {
        return m_rtapi;
    }

protected:

    /**
     * \setter m_selected_api
     */

    void selected_api (const rtmidi_api & api)
    {
        m_selected_api = api;
    }

};          // class rtmidi_base

}           // namespace seq64

#endif      // SEQ64_RTMIDI_BASE_HPP

/*
 * rtmidi_base.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

