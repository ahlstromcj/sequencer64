#ifndef SEQ64_MIDI_ALSA_INFO_HPP
#define SEQ64_MIDI_ALSA_INFO_HPP

/**
 * \file          midi_alsa_info.hpp
 *
 *    A class for holding the current status of the ALSA system on the host.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-12-04
 * \updates       2017-03-21
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    We need to have a way to get all of the ALSA information of
 *    the midi_alsa
 */

#include <alsa/asoundlib.h>

#include "midi_info.hpp"                /* seq64::midi_port_info etc.   */
#include "mastermidibus_rm.hpp"
#include "midibus.hpp"                  /* seq64::midibus               */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class mastermidibus;

/**
 *  The class for handling ALSA MIDI input.
 */

class midi_alsa_info : public midi_info
{

private:

    /**
     *  Flags that denote queries for input (read) ports.
     */

    static unsigned sm_input_caps;

    /**
     *  Flags that denote queries for output (write) ports.
     */

    static unsigned sm_output_caps;

    /**
     *  Holds the ALSA sequencer client pointer so that it can be used
     *  by the midibus objects.  This is actually an opaque pointer; there is
     *  no way to get the actual fields in this structure; they can only be
     *  accessed through functions in the ALSA API.
     */

    snd_seq_t * m_alsa_seq;

    /**
     *  The number of descriptors for polling.
     */

    int m_num_poll_descriptors;

    /**
     *  Points to the list of descriptors for polling.
     */

    struct pollfd * m_poll_descriptors;

public:

    midi_alsa_info
    (
        const std::string & appname,
        int ppqn    = SEQ64_DEFAULT_PPQN,       /* 192    */
        midibpm bpm = SEQ64_DEFAULT_BPM         /* 120.0  */
    );
    virtual ~midi_alsa_info ();

    /**
     * \getter m_alsa_seq
     *      This is the platform-specific version of midi_handle().
     */

    snd_seq_t * seq ()
    {
        return m_alsa_seq;
    }

    virtual bool api_get_midi_event (event * inev);
    virtual int api_poll_for_midi ();
    virtual void api_set_ppqn (int p);
    virtual void api_set_beats_per_minute (midibpm b);
    virtual void api_port_start (mastermidibus & masterbus, int bus, int port);
    virtual void api_flush ();

private:

    virtual int get_all_port_info ();

};          // class midi_alsa_info

}           // namespace seq64

#endif      // SEQ64_MIDI_ALSA_INFO_HPP

/*
 * midi_alsa_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

