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
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include <list>
#include <string>
#include <gtkmm/widget.h>       // somehow, can't forward-declare GdkEventAny

#include "gui_window_gtk2.hpp"

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

class perfnames;
class perform;
class perfroll;
class perftime;

/*
 * ca 2015-07-24
 * Just a note:  The patches in the pld-linux/seq24 GitHub project had a
 * namespace sigc declaration here, which does not seem to be needed.
 * And a lot of the patches from that project were already applied to
 * seq24 v 0.9.2.
 */

/**
 *  This class supports a Performance Editor that is used to arrange the
 *  patterns/sequences defined in the patterns panel, I think.
 *
 *  It has a seqroll and piano roll?  No, it has a perform, a perfnames, a
 *  perfroll, and a perftime.
 */

class perfedit : public gui_window_gtk2
{

private:

//  perform * m_mainperf;
    Gtk::Table * m_table;
    Gtk::Adjustment * m_vadjust;
    Gtk::Adjustment * m_hadjust;
    Gtk::VScrollbar * m_vscroll;
    Gtk::HScrollbar * m_hscroll;
    perfnames * m_perfnames;
    perfroll * m_perfroll;
    perftime * m_perftime;
    Gtk::Menu * m_menu_snap;
    Gtk::Button * m_button_snap;
    Gtk::Entry * m_entry_snap;
    Gtk::Button * m_button_stop;
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
    int m_bpm;
    int m_bw;
    bool m_modified;

public:

    perfedit (perform & a_perf);
    ~perfedit ();

    void init_before_show ();

    /**
     * \setter m_modified
     */

    void is_modified (bool flag)
    {
        m_modified = flag;
    }

    /**
     * \getter m_modified
     */

    bool is_modified () const
    {
        return m_modified;
    }

private:

    void set_bpm (int a_beats_per_measure);
    void set_bw (int a_beat_width);
    void set_snap (int a_snap);
    void set_guides ();
    void grow ();
    void start_playing ();
    void stop_playing ();
    void set_looped ();
    void expand ();
    void collapse ();
    void copy ();
    void undo ();
    void popup_menu (Gtk::Menu * a_menu);
    bool timeout ();

    void on_realize ();
    bool on_delete_event (GdkEventAny * a_event);
    bool on_key_press_event (GdkEventKey * a_ev);
};

}           // namespace seq64

#endif      // SEQ64_PERFEDIT_HPP

/*
 * perfedit.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
