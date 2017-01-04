#ifndef SEQ64_MIDI_JACK_HPP
#define SEQ64_MIDI_JACK_HPP

/**
 * \file          midi_jack.hpp
 *
 *    A class for realtime MIDI input/output via JACK.
 *
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2017-01-04
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    In this refactoring...
 */

#include <string>

#include "midi_api.hpp"
#include "midi_jack_info.hpp"           /* seq64::midi_jack_info            */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This class implements with JACK version of the midi_alsa object.
 */

class midi_jack : public midi_api
{

private:

protected:

    void * m_api_data;  // TEMPORARY, just to get code to compile

public:

    midi_jack (midi_info & masterinfo, int index = SEQ64_NO_INDEX)
     :
        midi_api   (masterinfo, index),
        m_api_data (nullptr)
    {
    }

protected:

    virtual bool api_init_out ()
    {
        return true;
    }

    virtual bool api_init_in ()
    {
        return true;
    }

    virtual bool api_init_out_sub ()
    {
        return true;
    }

    virtual bool api_init_in_sub ()
    {
        return true;
    }

    virtual bool api_deinit_in ()
    {
        return true;
    }

    virtual void api_play (event * e24, midibyte channel)
    {
    }

    virtual void api_sysex (event * e24)
    {
    }

    virtual void api_flush ()
    {
    }

    virtual void api_continue_from (midipulse tick, midipulse beats)
    {
    }

    virtual void api_start ()
    {
    }

    virtual void api_stop ()
    {
    }

    virtual void api_clock (midipulse tick)
    {
    }

    virtual void api_set_ppqn (int ppqn)
    {
    }

    virtual void api_set_beats_per_minute (int bpm)
    {
    }

};

/**
 *  The class for handling JACK MIDI input.
 */

class midi_in_jack: public midi_jack
{

protected:

    std::string m_clientname;

public:

    midi_in_jack
    (
        midi_info & masterinfo, int index = SEQ64_NO_INDEX,
        const std::string & clientname = "", unsigned queuesize = 0
    );
    virtual ~midi_in_jack ();

    virtual void open_port (int portnumber, const std::string & portname);
    virtual void open_virtual_port (const std::string & portname);
    virtual void close_port ();
    virtual int get_port_count ();
    virtual std::string get_port_name (int portnumber);

private:

    /* virtual */ void initialize (const std::string & clientname);

    void connect ();

};          // midi_in_jack

/**
 *  The JACK MIDI output API class.
 */

class midi_out_jack: public midi_jack
{

protected:

    std::string m_clientname;

public:

    midi_out_jack
    (
        midi_info & masterinfo, int index = SEQ64_NO_INDEX,
        const std::string & clientname = ""
    );
    virtual ~midi_out_jack ();

    virtual void open_port (int portnumber, const std::string & portname);
    virtual void open_virtual_port (const std::string & portname);
    virtual void close_port ();
    virtual int get_port_count ();
    virtual std::string get_port_name (int portnumber);
    virtual void send_message (const std::vector<midibyte> & message);

private:

    /* virtual */ void initialize (const std::string & clientname);

    void connect ();

};          // midi_out_jack

}           // namespace seq64

#endif      // SEQ64_MIDI_JACK_HPP

/*
 * midi_jack.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

