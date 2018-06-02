#ifndef SEQ64_MIDI_WIN_INFO_HPP
#define SEQ64_MIDI_WIN_INFO_HPP

/**
 * \file          midi_win_info.hpp
 *
 *    A class for holding the current status of the JACK system on the host.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-08-20
 * \updates       2018-06-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 * \deprecated
 *      We have decided to use the PortMidi re-implementation for Sequencer64
 *      for Windows.
 *
 *    We need to have a way to get all of the JACK information of
 *    the midi_win
 */

#error Internal RtMidi for Windows obsolete, use internal PortMidi instead.

#include "midi_info.hpp"                    /* seq64::midi_port_info etc.   */
#include "midi_win_data.hpp"                /* seq64::midi_win_data         */
#include "mastermidibus_rm.hpp"
#include "midibus.hpp"                      /* seq64::midibus               */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class mastermidibus;
    class midi_win;

/**
 *  The class for handling JACK MIDI port enumeration.
 */

class midi_win_info : public midi_info
{
    friend class midi_win;

private:

    /**
     *  Holds data needed for enumerating and setting up Win MM MIDI input
     *  and output ports.
     */

    midi_win_data m_win_handles;

public:

    midi_win_info
    (
        const std::string & appname,
        int ppqn    = SEQ64_DEFAULT_PPQN,       /* 192    */
        midibpm bpm = SEQ64_DEFAULT_BPM         /* 120.0  */
    );
    virtual ~midi_win_info ();

    /**
     * \getter m_jack_client
     *      This is the platform-specific version of midi_handle().

    jack_client_t * client_handle ()
    {
        return m_jack_client;
    }
     */

    virtual bool api_get_midi_event (event * inev);
    virtual bool api_connect ();

    virtual int api_poll_for_midi ();

    virtual void api_set_ppqn (int p);
    virtual void api_set_beats_per_minute (midibpm b);
    virtual void api_port_start (mastermidibus & masterbus, int bus, int port);
    virtual void api_flush ();

private:

    virtual int get_all_port_info ();

    /**
     * \getter m_jack_client
     *      This is the platform-specific version of midi_handle().

    void client_handle (jack_client_t * j)
    {
        m_jack_client = j;
    }
     */

    // jack_client_t * connect ();
    void disconnect ();
    void extract_names
    (
        const std::string & fullname,
        std::string & clientname,
        std::string & portname
    );

private:

    /**
     *  Adds a pointer to a JACK port.
     */

    bool add (midi_win & mj)
    {
        // m_jack_ports.push_back(&mj);
        return true;
    }

};          // midi_win_info

}           // namespace seq64

#endif      // SEQ64_MIDI_WIN_INFO_HPP

/*
 * midi_win_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

