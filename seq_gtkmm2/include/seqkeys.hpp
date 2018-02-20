#ifndef SEQ64_SEQKEYS_HPP
#define SEQ64_SEQKEYS_HPP

/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          seqkeys.hpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-08-11
 * \license       GNU GPLv2 or above
 *
 *      We've added the feature of a right-click toggling between showing the
 *      main octave values (e.g. "C1" or "C#1") versus the numerical MIDI
 *      values of the keys.
 */

#include <gtkmm/window.h>

#include "gui_drawingarea_gtk2.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class adjustment;
}

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;

/**
 *  This class implements the left side piano of the pattern/sequence
 *  editor.  Note the friends of this class, seqroll and FruitySeqRollInput.
 *  Where is Seq24SeqRollInput?  Gone.  It has been folded back into seqroll.
 */

class seqkeys : public gui_drawingarea_gtk2
{
    friend class seqroll;
    friend class FruitySeqRollInput;

private:

    /**
     *  The sequence object that the keys pane will be using.
     */

    sequence & m_seq;

    /**
     *  Provides the value of the current top key in the keys pane.
     *  Modified in change_vert().
     */

    int m_scroll_offset_key;

    /**
     *  Provides the value of the current top key in the keys pane in units of
     *  relative pixels.  Modified in change_vert().
     */

    int m_scroll_offset_y;

    /**
     *  Indicates if a piano key is set to indicate where on the pitch
     *  scale the mouse cursor is sitting.
     */

    bool m_hint_state;

    /**
     *  Indicates the current y-value of the mouse pointer in units of key
     *  value.
     */

    int m_hint_key;

    /**
     *  Set to true while the left mouse button is being pressed.  Used in
     *  playing the sound for each note as it is clicked in the seqkeys pane.
     */

    bool m_keying;

    /**
     *  The note to be played when selected in the seqkeys pane.
     */

    int m_keying_note;

    /**
     *  This member holds the scale value for the musical scale for the
     *  current edit of the sequence.
     */

    int m_scale;

    /**
     *  This member holds the key value for the musical key for the current
     *  edit of the sequence.
     */

    int m_key;

    /**
     *  The default value is to show the octave letters on the vertical
     *  virtual keyboard.  If false, then the MIDI key numbers are shown
     *  instead.  This is a new feature of Sequencer64.
     */

    bool m_show_octave_letters;

public:

    seqkeys (sequence & seq, perform & p, Gtk::Adjustment & vadjust);

    /**
     *  Let's provide a do-nothing virtual destructor.
     */

    virtual ~seqkeys ()
    {
        // I got nothin'
    }

    void set_scale (int scale);
    void set_key (int key);
    void set_hint_key (int key);        /* sets key to grey         */
    void set_hint_state (bool state);   /* true == on, false == off */

private:

    virtual void force_draw ();

private:

    /**
     *  Sneaky accessors for the seqroll friend.  From the stazed code.
     *
     * \param ev
     *      The event to be forwarded from the seqroll.
     */

    void set_listen_button_press (GdkEventButton * ev)
    {
        on_button_press_event(ev);
    }

    void set_listen_button_release (GdkEventButton * ev)
    {
        on_button_release_event(ev);
    }

    void set_listen_motion_notify (GdkEventMotion * ev)
    {
        on_motion_notify_event(ev);
    }

private:

    void draw_area ();
    void update_pixmap ();
    void convert_y (int y, int & note);
    void draw_key (int key, bool state);
    void change_vert ();
    void update_sizes ();
    void reset ();

    /**
     *  Detects a black key.
     *
     * \param key
     *      The key to analyze.
     *
     * \return
     *      Returns true if the key is black (value 1, 3, 6, 8, or 10).
     */

    bool is_black_key (int key) const
    {
        return key == 1 || key == 3 || key == 6 || key == 8 || key == 10;
    }

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * ev);
    bool on_button_press_event (GdkEventButton * ev);
    bool on_button_release_event (GdkEventButton * ev);
    bool on_motion_notify_event (GdkEventMotion * p0);
    bool on_enter_notify_event (GdkEventCrossing * p0);
    bool on_leave_notify_event (GdkEventCrossing * p0);
    bool on_scroll_event (GdkEventScroll * ev);
    void on_size_allocate (Gtk::Allocation &);

};          // class seqkeys

}           // namespace seq64

#endif      // SEQ64_SEQKEYS_HPP

/*
 * seqkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

