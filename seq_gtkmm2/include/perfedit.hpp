#ifndef SEQ64_PERFEDIT_HPP
#define SEQ64_PERFEDIT_HPP

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
 * \file          perfedit.hpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-05-12
 * \license       GNU GPLv2 or above
 *
 *  Note that, as of version 0.9.11, the z and Z keys, when focus is on the
 *  perfroll (piano roll), will zoom the view horizontally.
 */

#include <list>
#include <string>
#include <gtkmm/widget.h>       // somehow, can't forward-declare GdkEventAny
#include <gtkmm/window.h>       // ditto

#include "gui_window_gtk2.hpp"
#include "perform.hpp"

#define DEFAULT_PERFEDIT_SNAP   8

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
    class Image;
    class Menu;
    class Table;
    class ToggleButton;
    class Tooltips;
    class VScrollbar;
}

namespace seq64
{

class perfnames;
class perfroll;
class perftime;

/*
 * ca 2015-07-24
 * Just a note:  The patches in the pld-linux/seq24 GitHub project had a
 * namespace sigc declaration here, which does not seem to be needed.
 * And a lot of the patches from that project were already applied to
 * seq24 v 0.9.2.  They are now all applied.
 */

/**
 *  This class supports a Performance Editor that is used to arrange the
 *  patterns/sequences defined in the patterns panel.  It has a seqroll and
 *  piano roll?  No, it has a perform, a perfnames, a perfroll, and a
 *  perftime.
 */

class perfedit : public gui_window_gtk2
{

    friend void update_perfedit_sequences ();

private:

    /**
     *  The partner instance of perfedit.
     */

    perfedit * m_peer_perfedit;

    /**
     *  A whole horde of GUI elements.
     */

    Gtk::Table * m_table;
    Gtk::Adjustment * m_vadjust;
    Gtk::Adjustment * m_hadjust;
    Gtk::VScrollbar * m_vscroll;
    Gtk::HScrollbar * m_hscroll;
    perfnames * m_perfnames;
    perfroll * m_perfroll;
    perftime * m_perftime;
    Gtk::Menu * m_menu_snap;
    Gtk::Image * m_image_play;
    Gtk::Button * m_button_snap;
    Gtk::Entry * m_entry_snap;
    Gtk::Button * m_button_stop;

    /**
     *  Implements the yellow two-bar pause button.
     */

    Gtk::Button * m_button_play;
    Gtk::ToggleButton * m_button_loop;
    Gtk::Button * m_button_expand;
    Gtk::Button * m_button_collapse;
    Gtk::Button * m_button_copy;
    Gtk::Button * m_button_grow;
    Gtk::Button * m_button_undo;
    Gtk::Button * m_button_bpm;
    Gtk::Entry * m_entry_bpm;
    Gtk::Button * m_button_bw;
    Gtk::Entry * m_entry_bw;
    Gtk::HBox * m_hbox;
    Gtk::HBox * m_hlbox;
    Gtk::Tooltips * m_tooltips;        // why not conditional on Gtk version?

    /**
     * Menus for time signature, beats per measure, beat width.
     */

    Gtk::Menu * m_menu_bpm;
    Gtk::Menu * m_menu_bw;

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

#ifdef SEQ64_PAUSE_SUPPORT

    /**
     *  Holds the current status of running, for use in display the play
     *  versus pause icon.
     */

    bool m_is_running;

#endif

    /**
     *  The standard "beats per measure" of Sequencer64, which here matches
     *  the beats-per-measure displayed in the perfroll (piano roll).
     */

    int m_standard_bpm;

public:

    perfedit
    (
        perform & p,
        bool second_perfedit    = false,
        int ppqn                = SEQ64_USE_DEFAULT_PPQN
    );
    ~perfedit ();

    void init_before_show ();
    void enqueue_draw (bool forward = true);
    void set_zoom (int z);

    /**
     *  Checks zoom values for the z/Z keystrokes used in perfroll and
     *  perftime.  It has to range from greater than 1 (the highest zoom-in
     *  causes an unexplained drawing artifact at this time), and not greater
     *  than four times the c_perf_scale_x value, at which point we have
     *  zoomed out so far that the measure numbers are almost completely
     *  obscured.
     */

    static bool zoom_check (int z)
    {
        return z > 1 && z <= (4 * c_perf_scale_x);
    }

    /**
     *  Register the peer perfedit object.  This function is meant to be
     *  called by mainwnd, which creates the perfedits and then makes sure
     *  they get along.
     */

    void enregister_peer (perfedit * peer)
    {
        if (not_nullptr(peer) && is_nullptr(m_peer_perfedit))
            m_peer_perfedit = peer;
    }

private:

    void set_beats_per_bar (int bpm);
    void set_beat_width (int bw);
    void set_snap (int snap);
    void set_guides ();
    void grow ();
    void set_looped ();
    void expand ();
    void collapse ();
    void copy ();
    void undo ();
    void popup_menu (Gtk::Menu * menu);
    void draw_sequences ();
    bool timeout ();
    void set_image (bool isrunning);
    void start_playing ();
    void pause_playing ();
    void stop_playing ();

    /**
     *  Reverses the state of playback.  Meant only to be called when the
     *  "Play" button is pressed.  Currently, the GUI does not change.
     *  This function will ultimately act like a Pause/Play button, but
     *  currently the pause functionality on works (partially) for JACK
     *  transport.  Currently not used.
     */

    void toggle_playing ()
    {
        if (perf().is_running())
            stop_playing();
        else
            start_playing();
    }

private:        // Gtkmm 2.4 callbacks

    void on_realize ();
    bool on_key_press_event (GdkEventKey * ev);

    /**
     *  All this callback function does is return false.
     */

    bool on_delete_event (GdkEventAny * /*a_event*/ )
    {
        return false;
    }

};              // class perfedit

/*
 * Free functions and values is the seq64 namespace.
 */

extern void update_perfedit_sequences ();

}               // namespace seq64

#endif          // SEQ64_PERFEDIT_HPP

/*
 * perfedit.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

