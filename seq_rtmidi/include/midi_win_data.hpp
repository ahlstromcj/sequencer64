#ifndef SEQ64_MIDI_WIN_DATA_HPP
#define SEQ64_MIDI_WIN_DATA_HPP

/**
 * \file          midi_win_data.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2017-08-20
 * \updates       2017-08-20
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  The Windows MM API is based on the use of a callback function for MIDI
 *  input.  We convert the system specific time stamps to delta time values.
 */

#include <windows.h>
#include <mmsystem.h>                   /* Windows MM MIDI header file  */

#define WIN_RT_SYSEX_BUFFER_SIZE    1024
#define WIN_RT_SYSEX_BUFFER_COUNT      4

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Contains the Windows MM MIDI API data as a kind of scratchpad for this
 *  object.  This guy needs a constructor taking parameters for an
 *  rtmidi_in_data pointer.
 *
 *  [Patrice] see https://groups.google.com/forum/#!topic/mididev/6OUjHutMpEo
 */

class midi_win_data
{

private:

    /**
     *  Unlike JACK and m_jack_client, the Windows MM API provides input and
     *  output handles.  This member is the handle to the MIDI input device.
     */

    HMIDIIN m_win_in_handle;                    // inHandle;

    /**
     *  This member is the handle to the MIDI output device.
     */

    HMIDIIN m_win_out_handle;                   // outHandle;

    /**
     *  The last time-stamp obtained.  Use for calculating the delta time, I
     *  would imagine.
     */

    DWORD m_win_lasttime;                       // lastTime

    /**
     * MidiInApi::MidiMessage message;
     *
     */

    midi_message m_win_message;

    /**
     * LPMIDIHDR sysexBuffer[RT_SYSEX_BUFFER_COUNT];
     */

    LPMIDIHDR m_sysex_buffer [WIN_RT_SYSEX_BUFFER_COUNT];

    /**
     *  Critical mutex.  Thar be dragons!
     */

    CRITICAL_SECTION m_win_mutex;               // _mutex;

    /**
     *  Holds special data peculiar to the client and its MIDI input
     *  processing.
     */

    rtmidi_in_data * m_win_rtmidiin;

    /**
     *  Easy flag for creation errors.
     */

    bool m_is_error;

    /**
     * \ctor midi_win_data
     */

    midi_win_data () :
        m_win_in_handle     (NULL),
        m_win_out_handle    (NULL),
        m_win_lasttime      (0),
        m_win_message       (),
        m_sysex_buffer      (),         // array
        m_win_mutex         (),
        m_win_rtmidiin      (nullptr),
        m_is_error          (false)
    {
        if (! InitializeCriticalSectionAndSpinCount(&m_win_mutex, 0x00000400)
        {
            // m_error_string = func_message("Win MM can't create mutex");
            // error(rterror::WARNING, m_error_string);

            m_is_error = true;
        }
    }

    /**
     *  This destructor deletes the critical section.
     */

    ~midi_win_data ()
    {
        DeleteCriticalSection(&m_win_mutex);
    }

    /**
     *  Tests that the buffers are good.
     */

    bool valid_buffer () const
    {
        return false;
    }

    /**
     * \getter m_is_error
     */

    bool is_error () const
    {
        return m_is_error;
    }

};          // class midi_win_data

}           // namespace seq64

#endif      // SEQ64_MIDI_WIN_DATA_HPP

/*
 * midi_win_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

