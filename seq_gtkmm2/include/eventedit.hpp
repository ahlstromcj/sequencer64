#ifndef SEQ64_EVENTEDIT_HPP
#define SEQ64_EVENTEDIT_HPP

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
 * \file          eventedit.hpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2015-12-05
 * \license       GNU GPLv2 or above
 *
 */

#include <list>
#include <string>
#include <gtkmm/widget.h>       // somehow, can't forward-declare GdkEventAny
#include <gtkmm/window.h>       // ditto

#include "gui_window_gtk2.hpp"
#include "perform.hpp"

/*
 *  Since these items are pointers, we were able to move (most) of the
 *  included header files to the cpp file.   Except for the items that
 *  come from widget.h, perhaps because GdkEventAny was a typedef.
 */

namespace Gtk
{
    class Adjustment;
    class Button;
    class Entry;
    class HBox;
    class HScrollbar;
    class Menu;
    class Table;
    class ToggleButton;
    class Tooltips;
    class VScrollbar;
}

namespace seq64
{

class eventslots;

/**
 *  This class supports an Event Editor that is used to tweak the details of
 *  events and get a better idea of the mix of events in a sequence.
 */

class eventedit : public gui_window_gtk2
{

private:

    /**
     *  A whole horde of GUI elements.
     */

    Gtk::Table * m_table;
    Gtk::Adjustment * m_vadjust;
    Gtk::VScrollbar * m_vscroll;
    eventslots * m_eventslots;
#ifdef USE_PLAY_FUNCTION
    Gtk::Button * m_button_stop;
    Gtk::Button * m_button_play;
    Gtk::ToggleButton * m_button_loop;
#endif
#ifdef USE_COPY_FUNCTION
    Gtk::Button * m_button_copy;
#endif
    Gtk::Entry * m_entry_bpm;
    Gtk::Entry * m_entry_bw;
    Gtk::HBox * m_hbox;
    Gtk::HBox * m_hlbox;
    Gtk::Tooltips * m_tooltips;        // why not conditional on Gtk version?

    /**
     * Menus for time signature, beats per measure, beat width.
     */

    /**
     * Set snap-to in "pulses".
     */

    int m_snap;

    /**
     *  The current "beats per measure" value.  Do not confuse it with BPM
     *  (beats per minute). The numerator of the time signature.
     */

    int m_bpm;

    /**
     *  The current "beat width" value.  The denominator of the time
     *  signature.
     */

    int m_bw;

    /**
     *  The current "parts per quarter note" value.
     */

    int m_ppqn;

    /**
     *  The standard "beats per measure" of Sequencer64, which here matches
     *  the beats-per-measure displayed in the perfroll (piano roll).
     */

    int m_standard_bpm;

    /**
     *  Provides the timer period for the eventedit timer, used to determine
     *  the rate of redrawing.  This is hardwired to 40 ms in Linux, and 20 ms
     *  in Windows.
     */

    int m_redraw_ms;

public:

    eventedit
    (
        perform & p,
        bool second_eventedit    = false,
        int ppqn                = SEQ64_USE_DEFAULT_PPQN
    );
    ~eventedit ();

    void enqueue_draw (bool forward = true);

private:

    void expand ();
    void collapse ();
    void copy ();
    void undo ();
    void popup_menu (Gtk::Menu * menu);
    bool timeout ();

    /**
     *  Implement the playing.  JACK will be used if it is present and, in the
     *  application, enabled.  This call also sets
     *  rc().is_pattern_playing(true).
     */

    void start_playing ()
    {
        perf().start_playing(true);         // careful now, see perform!!!!
    }

    /**
     *  Stop the playing.  This call also sets rc().is_pattern_playing(true).
     */

    void stop_playing ()
    {
        perf().stop_playing();
    }

private:            // callbacks

    void on_realize ();
    bool on_key_press_event (GdkEventKey * ev);

    /**
     *  All this callback function does is return false.
     */

    bool on_delete_event (GdkEventAny * /*a_event*/ )
    {
        return false;
    }

};

}           // namespace seq64

#endif      // SEQ64_EVENTEDIT_HPP

/*
 * eventedit.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

