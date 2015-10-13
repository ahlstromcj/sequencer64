#ifndef SEQ64_SEQEDIT_HPP
#define SEQ64_SEQEDIT_HPP

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
 * \file          seqedit.hpp
 *
 *  This module declares/defines the base class for editing a
 *  pattern/sequence.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-02
 * \license       GNU GPLv2 or above
 *
 */

#include <list>
#include <string>
#include <gtkmm/window.h>

#include "gui_window_gtk2.hpp"

namespace Gtk
{
    class Adjustment;
    class Entry;
    class HScrollbar;
    class Image;
    class Menu;
    class MenuBar;
    class Table;
    class RadionButton;
    class ToggleButton;
    class Tooltips;
    class VBox;
    class VScrollbar;
}

namespace seq64
{

class perform;
class seqroll;
class seqkeys;
class seqdata;
class seqtime;
class seqevent;
class sequence;

/**
 *  Implements the Pattern Editor, which has references to:
 *
 *      -   perform
 *      -   seqroll
 *      -   seqkeys
 *      -   seqdata
 *      -   seqtime
 *      -   seqevent
 *      -   sequence
 *
 *  This class has a metric ton of user-interface objects and other
 *  members.
 */

class seqedit : public gui_window_gtk2
{

private:

    static const int mc_min_zoom;
    static const int mc_max_zoom;
    static int m_initial_zoom;
    static int m_initial_snap;
    static int m_initial_note_length;
    static int m_initial_scale;
    static int m_initial_key;
    static int m_initial_sequence;

    /**
     *  Provides the zoom values: 0  1  2  3  4, and 1, 2, 4, 8, 16.
     */

    int m_zoom;

    /**
     *  Use in setting the snap-to in pulses, off = 1.
     */

    int m_snap;
    int m_note_length;

    /**
     *  Settings for the music scale and key.
     */

    int m_scale;
    int m_key;
    int m_sequence;
    long m_measures;
    const int m_ppqn;

    sequence & m_seq;
    Gtk::MenuBar * m_menubar;
    Gtk::Menu * m_menu_tools;
    Gtk::Menu * m_menu_zoom;
    Gtk::Menu * m_menu_snap;
    Gtk::Menu * m_menu_note_length;

    /**
     *  Provides the length in measures.
     */

    Gtk::Menu * m_menu_length;
    Gtk::Menu * m_menu_midich;
    Gtk::Menu * m_menu_midibus;
    Gtk::Menu * m_menu_data;
    Gtk::Menu * m_menu_key;
    Gtk::Menu * m_menu_scale;
    Gtk::Menu * m_menu_sequences;

    /**
     *  These member provife the time signature, beats per measure, and
     *  beat width menus.
     */

    Gtk::Menu * m_menu_bpm;
    Gtk::Menu * m_menu_bw;
    Gtk::Menu * m_menu_rec_vol;
    int m_pos;
    Gtk::Adjustment * m_vadjust;
    Gtk::Adjustment * m_hadjust;
    Gtk::VScrollbar * m_vscroll_new;
    Gtk::HScrollbar * m_hscroll_new;
    seqkeys * m_seqkeys_wid;
    seqtime * m_seqtime_wid;
    seqdata * m_seqdata_wid;
    seqevent * m_seqevent_wid;
    seqroll * m_seqroll_wid;
    Gtk::Table * m_table;
    Gtk::VBox * m_vbox;
    Gtk::HBox * m_hbox;
    Gtk::HBox * m_hbox2;
    Gtk::HBox * m_hbox3;
    Gtk::Button * m_button_undo;
    Gtk::Button * m_button_redo;
    Gtk::Button * m_button_quantize;
    Gtk::Button * m_button_tools;
    Gtk::Button * m_button_sequence;
    Gtk::Entry * m_entry_sequence;
    Gtk::Button * m_button_bus;
    Gtk::Entry * m_entry_bus;
    Gtk::Button * m_button_channel;
    Gtk::Entry * m_entry_channel;
    Gtk::Button * m_button_snap;
    Gtk::Entry * m_entry_snap;
    Gtk::Button * m_button_note_length;
    Gtk::Entry * m_entry_note_length;
    Gtk::Button * m_button_zoom;
    Gtk::Entry * m_entry_zoom;
    Gtk::Button * m_button_length;
    Gtk::Entry * m_entry_length;
    Gtk::Button * m_button_key;
    Gtk::Entry * m_entry_key;
    Gtk::Button * m_button_scale;
    Gtk::Entry * m_entry_scale;
    Gtk::Tooltips * m_tooltips;
    Gtk::Button * m_button_data;
    Gtk::Entry * m_entry_data;
    Gtk::Button * m_button_bpm;
    Gtk::Entry * m_entry_bpm;
    Gtk::Button * m_button_bw;
    Gtk::Entry * m_entry_bw;
    Gtk::Button * m_button_rec_vol;
    Gtk::ToggleButton * m_toggle_play;
    Gtk::ToggleButton * m_toggle_record;
    Gtk::ToggleButton * m_toggle_q_rec;
    Gtk::ToggleButton * m_toggle_thru;
    Gtk::RadioButton * m_radio_select;
    Gtk::RadioButton * m_radio_grow;
    Gtk::RadioButton * m_radio_draw;
    Gtk::Entry * m_entry_name;

    /**
     *  Indicates what is the data window currently editing?
     */

    unsigned char m_editing_status;
    unsigned char m_editing_cc;

public:

    seqedit (sequence & a_seq, perform & a_perf, int a_pos);
    ~seqedit ();

private:

    void set_zoom (int a_zoom);
    void set_snap (int a_snap);
    void set_note_length (int a_note_length);
    void set_bpm (int a_beats_per_measure);
    void set_bw (int a_beat_width);
    void set_rec_vol (int a_rec_vol);
    void set_measures (int a_length_measures);
    void apply_length (int a_bpm, int a_bw, int a_measures);
    long get_measures ();
    void set_midi_channel (int a_midichannel);
    void set_midi_bus (int a_midibus);
    void set_scale (int a_scale);
    void set_key (int a_note);
    void set_background_sequence (int a_seq);
    void name_change_callback ();
    void play_change_callback ();
    void record_change_callback ();
    void q_rec_change_callback ();
    void thru_change_callback ();
    void undo_callback ();
    void redo_callback ();
    void set_data_type (unsigned char a_status, unsigned char a_control = 0);
    void update_all_windows ();
    void fill_top_bar ();
    void create_menus ();

    /*
     * An unsed, empty function:    void menu_action_quantise ();
     */

    void popup_menu (Gtk::Menu * a_menu);
    void popup_event_menu ();
    void popup_midibus_menu ();
    void popup_sequence_menu ();
    void popup_tool_menu ();
    void popup_midich_menu ();
    Gtk::Image * create_menu_image (bool a_state = false);
    bool timeout ();
    void do_action (int a_action, int a_var);
    void mouse_action (mouse_action_e a_action);

private:        // callbacks

    void on_realize ();
    bool on_delete_event (GdkEventAny * a_event);
    bool on_scroll_event (GdkEventScroll * a_ev);
    bool on_key_press_event (GdkEventKey * a_ev);
};

}           // namespace seq64

#endif      // SEQ64_SEQEDIT_HPP

/*
 * seqedit.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
