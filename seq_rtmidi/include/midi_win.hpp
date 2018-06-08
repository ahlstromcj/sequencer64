#ifndef SEQ64_MIDI_WIN_HPP
#define SEQ64_MIDI_WIN_HPP

/**
 * \file          midi_win.hpp
 *
 *    A class for realtime MIDI input/output via JACK.
 *
 * \library       sequencer64 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2017-08-20
 * \updates       2018-06-02
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 * \deprecated
 *      We have decided to use the PortMidi re-implementation for Sequencer64
 *      for Windows.
 *
 *    In this refactoring, we've stripped out most of the original RtMidi
 *    functionality, leaving only the method for selecting the API to use for
 *    MIDI.  The method that Sequencer64's mastermidibus uses to initialize
 *    port has been transplanted to this rtmidi library.  The name "rtmidi" is
 *    now somewhat misleading.
 */

#error Internal RtMidi for Windows obsolete, use internal PortMidi instead.

#include <string>

#include "midi_api.hpp"
#include "midi_win_info.hpp"            /* seq64::midi_win_info            */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class midibus;

/**
 *  This class implements with JACK version of the midi_alsa object.
 */

class midi_win : public midi_api
{

    friend class midi_win_info;

private:

    /**
     *  Preserves the original name of the remote port, so it can be used
     *  later for connection.
     */

    std::string m_remote_port_name;

protected:

    /**
     *  This reference is needed in order for this midi_win object to add
     *  itself to the main midi_win_info list when running in single-JACK
     *  client mode.
     */

    midi_win_info & m_win_info;

    /**
     *  Holds the data needed for JACK processing.  Please do not confuse this
     *  item with the m_midi_handle of the midi_api base class.  This object
     *  holds a JACK-client pointer and a JACK-port pointer.
     */

    midi_win_data m_win_data;

public:

    midi_win
    (
        midibus & parentbus,
        midi_info & masterinfo,
        bool multiclient = false
    );
    virtual ~midi_win ();

    /**
     * \getter m_jack_client
     *      This is the platform-specific version of midi_handle().

    jack_client_t * client_handle ()
    {
        return m_win_data.m_jack_client;
    }
     */

    /**
     * \getter m_win_data
     */

    midi_win_data & win_data()
    {
        return m_win_data;
    }

    /**
     * \getter m_remote_port_name
     */

    const std::string & remote_port_name () const
    {
        return m_remote_port_name;
    }

    /**
     * \setter m_remote_port_name
     */

    void remote_port_name (const std::string & s)
    {
        m_remote_port_name = s;
    }

    /**
     * \getter m_jack_port
     *      This is the platform-specific version of midi_handle().

    jack_port_t * port_handle ()
    {
        return m_win_data.m_jack_port;
    }
     */

protected:

    /**
     * \setter m_win_data.m_jack_client

    void client_handle (jack_client_t * handle)
    {
        m_win_data.m_jack_client = handle;
    }
     */

    /**
     * \setter m_win_data.m_jack_port

    void port_handle (jack_port_t * handle)
    {
        m_win_data.m_jack_port = handle;
    }
     */

    bool open_client_impl (bool input);     /* implements "connect()"   */
    void close_client ();
    void close_port ();
    bool create_ringbuffer (size_t rbsize);
    bool connect_port
    (
        bool input,
        const std::string & sourceportname,
        const std::string & destportname
    );
    bool register_port (bool input, const std::string & portname);

protected:

    virtual bool open_client () = 0;    // replaces "connect()"
    virtual bool api_connect ();
    virtual bool api_init_out ();       // still in progress
    virtual bool api_init_in ();
    virtual bool api_init_out_sub ();
    virtual bool api_init_in_sub ();
    virtual bool api_deinit_in ();

    /**
     * \return
     *      Returns false, since this is an input function that is implemented
     *      fully only by midi_in_win.
     */

    virtual bool api_get_midi_event (event *)
    {
        return false;
    }

    /**
     *  This is an input function that is implemented fully only by
     *  midi_in_win.
     *
     *  virtual int api_poll_for_midi ()
     *  {
     *      return 0;
     *  }
     */

    virtual void api_play (event * e24, midibyte channel);
    virtual void api_sysex (event * e24);
    virtual void api_flush ();
    virtual void api_continue_from (midipulse tick, midipulse beats);
    virtual void api_start ();
    virtual void api_stop ();
    virtual void api_clock (midipulse tick);
    virtual void api_set_ppqn (int ppqn);
    virtual void api_set_beats_per_minute (midibpm bpm);
    virtual std::string api_get_port_name ();

private:

    void send_byte (midibyte evbyte, midipulse tick = SEQ64_NULL_MIDIPULSE);
    bool set_virtual_name (int portid, const std::string & portname);

};          // class midi_win

/**
 *  The class for handling JACK MIDI input.
 */

class midi_in_win: public midi_win
{

protected:

    std::string m_client_name;

public:

    midi_in_win (midibus & parentbus, midi_info & masterinfo);
    virtual ~midi_in_win ();

    virtual int api_poll_for_midi ();
    virtual bool api_get_midi_event (event *);

private:

    /**
     *  This function is virtual, so we don't call it in the constructor,
     *  using open_client_impl() directly instead.  This function replaces the
     *  RtMidi function "connect()".
     */

    virtual bool open_client ()
    {
        return open_client_impl(SEQ64_MIDI_INPUT_PORT);
    }

};          // class midi_in_win

/**
 *  The JACK MIDI output API class.
 */

class midi_out_win: public midi_win
{

public:

    midi_out_win (midibus & parentbus, midi_info & masterinfo);
    virtual ~midi_out_win ();

    /*
     *  Note that midi_message::container is a vector<midibyte> object.
     */

    virtual bool send_message (const midi_message & message);

private:

    /**
     *  This function is virtual, so we don't call it in the constructor,
     *  using open_client_impl() directly instead.  This function replaces the
     *  RtMidi function "connect()".
     */

    virtual bool open_client ()
    {
        return open_client_impl(SEQ64_MIDI_OUTPUT_PORT);
    }

};          // class midi_out_win

}           // namespace seq64

#endif      // SEQ64_MIDI_WIN_HPP

/*
 * midi_win.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

