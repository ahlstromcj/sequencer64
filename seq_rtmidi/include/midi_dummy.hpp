#ifndef SEQ64_MIDI_DUMMY_HPP
#define SEQ64_MIDI_DUMMY_HPP

/**
 * \file          midi_dummy.hpp
 *
 *    A class for realtime MIDI input/output via DUMMY.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    In this refactoring...
 */

#include <string>

#include "midi_api.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The class for handling dummy MIDI input.
 */

class midi_in_dummy : public midi_in_api
{

public:

    midi_in_dummy (const std::string & /*clientname*/, unsigned queuesizelimit)
     :
         midi_in_api(queuesizelimit)
    {
        m_error_string = func_message("class provides no functionality");
        error(rterror::WARNING, m_error_string);
    }

    virtual ~midi_in_dummy ()
    {
        // Empty body
    }

    virtual rtmidi_api get_current_api () const
    {
        return RTMIDI_API_DUMMY;
    }

    virtual void open_port (unsigned /*number*/, const std::string & /*name*/)
    {
        // no code
    }

    virtual void open_virtual_port (const std::string & /*portname*/)
    {
        // no code
    }

    virtual void close_port ()
    {
        // no code
    }

    virtual unsigned get_port_count ()
    {
        return 0;
    }

    virtual std::string get_port_name (unsigned /*portnumber*/)
    {
        static std::string s_dummy;
        return s_dummy;
    }

private:

    /* virtual */ void initialize (const std::string & /*clientname*/)
    {
        // no code
    }

};          // midi_in_dummy

/**
 *  The class for handling dummy MIDI outut.
 */

class midi_out_dummy : public midi_out_api
{

public:

    midi_out_dummy (const std::string & /*clientname*/)
    {
        m_error_string = func_message("class provides no functionality");
        error(rterror::WARNING, m_error_string);
    }

    virtual ~midi_out_dummy ()
    {
        // Empty body
    }

    /**
     * \getter RTMIDI_API_DUMMY
     */

    virtual rtmidi_api get_current_api () const
    {
        return RTMIDI_API_DUMMY;
    }

    virtual void open_port (unsigned /*number*/, const std::string & /*name*/)
    {
        // no code
    }

    virtual void open_virtual_port (const std::string & /*portname*/)
    {
        // no code
    }

    virtual void close_port ()
    {
        // no code
    }

    virtual unsigned get_port_count ()
    {
        return 0;
    }

    virtual std::string get_port_name (unsigned /*portnumber*/)
    {
        return std::string("");
    }

    virtual void send_message (const std::vector<midibyte> & /*message*/)
    {
        // no code
    }

private:

    /* virtual */ void initialize (const std::string & /*clientname*/)
    {
        // no code
    }

};

}           // namespace seq64

#endif      // SEQ64_MIDI_DUMMY_HPP

/*
 * midi_dummy.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

