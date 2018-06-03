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
 * \updates       2018-06-02
 * \license       GNU GPLv2 or above
 *
 *  The seqedit is a kind of master class for holding aseqroll, seqkeys,
 *  seqdata, and seqevent object.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback, which is useful in working on a small part
 *  of a longer pattern.  Marked with the SEQ64_FOLLOW_PROGRESS_BAR macro.
 */

#include <list>
#include <string>
#include <gtkmm/window.h>

#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN           */
#include "gui_window_gtk2.hpp"          /* seq64::gui_window_gtk2 class     */
#include "midibyte.hpp"                 /* seq64::midibyte typedef          */
#include "sequence.hpp"                 /* seq64::sequence class            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace Gtk
{
    class Adjustment;
    class Button;
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

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class seqevent;
    class seqdata;
    class seqkeys;
    class seqmenu;
    class seqroll;
    class seqtime;

#ifdef SEQ64_STAZED_LFO_SUPPORT
    class lfownd;
#endif

/**
 *  Mouse actions, for the Pattern Editor.
 */

enum mouse_action_e
{
    e_action_select,    /*<< Indicates a selection of events.               */
    e_action_draw,      /*<< Indicates a drawing of events.                 */
    e_action_grow       /*<< Indicates a growing of a selection of events.  */
};

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
    friend seqmenu;                     /* new, to follow seqmenu setting   */

private:

    /*
     * Documented in the cpp file.
     */

    static int m_initial_snap;
    static int m_initial_note_length;

#ifdef SEQ64_STAZED_CHORD_GENERATOR
    static int m_initial_chord;
#endif

    /**
     *  Provides the initial zoom, used for restoring the original zoom using
     *  the 0 key.
     */

    const int m_initial_zoom;

    /**
     *  Provides the zoom values: 1  2  3  4, and 1, 2, 4, 8, 16.
     *  The value of zoom is the same as the number of pixels per tick on the
     *  piano roll.
     */

    int m_zoom;

    /**
     *  Used in setting the snap-to value in pulses, off = 1.
     */

    int m_snap;

    /**
     *  The default length of a note to be inserted by a right-left-click
     *  operation.
     */

    int m_note_length;

    /**
     *  Setting for the music scale, can now be saved with the sequence.
     */

    int m_scale;

#ifdef SEQ64_STAZED_CHORD_GENERATOR

    /**
     *  Setting for the current chord generation; not now saved with the
     *  sequence.
     */

    int m_chord;

#endif

    /**
     *  Setting for the music key, can now be saved with the sequence.
     */

    int m_key;

    /**
     *  Setting for the background sequence, can now be saved with the
     *  sequence.
     */

    int m_bgsequence;

    /**
     *  Provides the length of the sequence in measures.
     */

    long m_measures;

    /**
     *  Holds a copy of the current PPQN for the sequence (and the entire MIDI
     *  file).
     */

    int m_ppqn;

#ifdef USE_STAZED_ODD_EVEN_SELECTION
    int m_pp_whole;
    int m_pp_eighth;
    int m_pp_sixteenth;
#endif

    /**
     *  Holds a reference to the sequence that this window represents.
     */

    sequence & m_seq;

    /**
     *  A number of user-interface objects for common.  Many of these are menu
     *  items, and are associated with buttons that, when pressed, bring up
     *  the menu for display and selection of its entries.
     */

    Gtk::MenuBar * m_menubar;           /**< The top bar with menu buttons. */
    Gtk::Menu * m_menu_tools;           /**< The "hammer" tool button menu. */
    Gtk::Menu * m_menu_zoom;            /**< Magnifying glass zoom menu.    */
    Gtk::Menu * m_menu_snap;            /**< Two-arrows grid-snap menu.     */
    Gtk::Menu * m_menu_note_length;     /**< Notes menu for note length.    */
    Gtk::Menu * m_menu_length;          /**< Pattern-length "bars" menu.    */

#ifdef SEQ64_STAZED_TRANSPOSE
    Gtk::ToggleButton * m_toggle_transpose;  /**< Transpose toggle button.  */

    Gtk::Image * m_image_transpose;     /**< Image for transpose button.    */
#endif

    Gtk::Menu * m_menu_midich;          /**< MIDI channel DIN menu button.  */
    Gtk::Menu * m_menu_midibus;         /**< MIDI output buss menu button.  */
    Gtk::Menu * m_menu_data;            /**< "Event" button to select data. */
    Gtk::Menu * m_menu_minidata;        /**< Mini button for actual events. */
    Gtk::Menu * m_menu_key;             /**< "Music key" menu button.       */
    Gtk::Menu * m_menu_scale;           /**< "Music scale" menu button.     */

#ifdef SEQ64_STAZED_CHORD_GENERATOR
    Gtk::Menu * m_menu_chords;          /**< "Chords" menu button.          */
#endif

    Gtk::Menu * m_menu_sequences;       /**< "Background sequence" button.  */
    Gtk::Menu * m_menu_bpm;             /**< Beats/measure numerator menu.  */
    Gtk::Menu * m_menu_bw;              /**< Beat-width denominator menu.   */
    Gtk::Menu * m_menu_rec_vol;         /**< Recording level "Vol" button.  */
    Gtk::Menu * m_menu_rec_type;        /**< Recording type menu.           */

    /**
     *  Scrollbar and adjustment objects for horizontal and vertical panning.
     */

    Gtk::Adjustment * m_vadjust;        /**< Vertical position descriptor.  */
    Gtk::Adjustment * m_hadjust;        /**< Horizontal motion scratchpad.  */
    Gtk::VScrollbar * m_vscroll_new;    /**< Main vertical scroll-bar.      */
    Gtk::HScrollbar * m_hscroll_new;    /**< Main horizontal scroll-bar.    */

    /**
     *  Handles the piano-keys part of the pattern-editor user-interface.
     *  This item draws the piano-keys at the left of the seqedit window.
     */

    seqkeys * m_seqkeys_wid;

    /**
     *  Handles the time-line (bar or measures) part of the pattern-editor
     *  user-interface.  This is the location where the measure numbers and
     *  the END marker are shown.
     */

    seqtime * m_seqtime_wid;

    /**
     *  Handles the event-data part of the pattern-editor user-interface.
     *  This is the area at the bottom of the window that shows value lines
     *  for the selected kinds of events.
     */

    seqdata * m_seqdata_wid;

    /**
     *  Handles the small event part of the pattern-editor user-interface,
     *  where events can be moved and added.
     */

    seqevent * m_seqevent_wid;

    /**
     *  Handles the piano-roll part of the pattern-editor user-interface.
     */

    seqroll * m_seqroll_wid;

#ifdef SEQ64_STAZED_LFO_SUPPORT

    /**
     *  The LFO button in the pattern editor.  This item will always be an
     *  optional part of the build, enabled by defining
     *  SEQ64_STAZED_LFO_SUPPORT.
     */

    Gtk::Button * m_button_lfo;

    /**
     *  The LFO window object used by the pattern editor.  This item get the
     *  seqdata window hooked into it, and so must follow that item in the C++
     *  initializer list.
     */

    lfownd * m_lfo_wnd;

#endif

    /**
     *  More user-interface elements.  These items provide a number of buttons
     *  and text-entry fields, as well as their layout.
     */

    Gtk::Table * m_table;               /**< The layout table for editor.   */
    Gtk::VBox * m_vbox;                 /**< Layout box for 3 h-boxes.      */
    Gtk::HBox * m_hbox;                 /**< Topmost menu/text dialog row.  */
    Gtk::HBox * m_hbox2;                /**< Second row of buttons.         */
#if USE_THIRD_SEQEDIT_BUTTON_ROW
    Gtk::HBox * m_hbox3;                /**< Unused third row of buttons.   */
#endif
    Gtk::Button * m_button_undo;        /**< Undo-edit button.              */
    Gtk::Button * m_button_redo;        /**< Redo-edit button.              */
    Gtk::Button * m_button_quantize;    /**< Quantize-pattern button.       */
    Gtk::Button * m_button_tools;       /**< Button for the Tools menu.     */
    Gtk::Button * m_button_sequence;    /**< Button for Background pattern. */
    Gtk::Entry * m_entry_sequence;      /**< Text for background pattern.   */
    Gtk::Button * m_button_bus;         /**< Button for MIDI Buss menu.     */
    Gtk::Entry * m_entry_bus;           /**< Text showing MIDI Buss name.   */
    Gtk::Button * m_button_channel;     /**< Button for the MIDI Channel.   */
    Gtk::Entry * m_entry_channel;       /**< Text for the MIDI Channel.     */
    Gtk::Button * m_button_snap;        /**< Button for the Grid-snap menu. */
    Gtk::Entry * m_entry_snap;          /**< Text for selected Grid-snap.   */
    Gtk::Button * m_button_note_length; /**< Button for Note-length menu.   */
    Gtk::Entry * m_entry_note_length;   /**< Text showing the Note-length.  */
    Gtk::Button * m_button_zoom;        /**< Button for the Zoom menu.      */
    Gtk::Entry * m_entry_zoom;          /**< Text for the selected Zoom.    */
    Gtk::Button * m_button_length;      /**< Button for pattern-length.     */
    Gtk::Entry * m_entry_length;        /**< Text for the pattern-length.   */
    Gtk::Button * m_button_key;         /**< Button for the Music Key.      */
    Gtk::Entry * m_entry_key;           /**< Text for selected Music Key.   */
    Gtk::Button * m_button_scale;       /**< Button for the Music Scale.    */
    Gtk::Entry * m_entry_scale;         /**< Text for the Music Scale.      */
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    Gtk::Button * m_button_chord;       /**< Button for the current Chord.  */
    Gtk::Entry * m_entry_chord;         /**< Text for the current Chord.    */
#endif
    Gtk::Tooltips * m_tooltips;         /**< Tooltip collector for dialog.  */
    Gtk::Button * m_button_data;        /**< Button for Event (data) menu.  */
    Gtk::Button * m_button_minidata;    /**< Mini button for data menu.     */
    Gtk::Entry * m_entry_data;          /**< Text for the selected Event.   */
    Gtk::Button * m_button_bpm;         /**< Button for Beats/Measure menu. */
    Gtk::Entry * m_entry_bpm;           /**< Text for chosen Beats/Measure. */
    Gtk::Button * m_button_bw;          /**< Button for Beat-Width menu.    */
    Gtk::Entry * m_entry_bw;            /**< Text for chosen Beat-Width.    */
    Gtk::Button * m_button_rec_vol;     /**< Button for recording volume.   */
    Gtk::Button * m_button_rec_type;    /**< Button for recording type.     */

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    Gtk::ToggleButton * m_toggle_follow; /**< Follow progress bar button.   */
#endif

    Gtk::ToggleButton * m_toggle_play;  /**< Pattern-to-MIDI record button. */
    Gtk::ToggleButton * m_toggle_record; /**< MIDI-port-to-pattern button.  */
    Gtk::ToggleButton * m_toggle_q_rec; /**< Quantized-record MIDI button.  */
    Gtk::ToggleButton * m_toggle_thru;  /**< MIDI-to-pattern-MIDI button.   */
#if USE_THIRD_SEQEDIT_BUTTON_ROW
    Gtk::RadioButton * m_radio_select;  /**< Unused selection button.       */
    Gtk::RadioButton * m_radio_grow;    /**< Unused grow button.            */
    Gtk::RadioButton * m_radio_draw;    /**< Unused selection button.       */
#endif
    Gtk::Entry * m_entry_seqnumber;     /**< Number of the sequence.        */
    Gtk::Entry * m_entry_name;          /**< Name of the sequence.          */
    Gtk::Image * m_image_mousemode;     /**< Image for mouse mode button.   */

    /**
     *  Indicates what MIDI event/status the data window currently editing.
     */

    midibyte m_editing_status;

    /**
     *  Indicates what MIDI CC value the data window currently editing.
     */

    midibyte m_editing_cc;

    /**
     *  Indicates the first event found in the sequence while setting up the
     *  data menu via set_event_entry().  If no events exist, the value is
     *  0x00.
     */

    midibyte m_first_event;

    /**
     *  Provides the string describing the first event, or "(no events)".
     */

    std::string m_first_event_name;

    /**
     *
     */
    /*
     midibyte m_first_control;
     */

    /**
     *  Indicates that the focus has already been changed to this sequence.
     */

    bool m_have_focus;

public:

    seqedit
    (
        perform & perf,
        sequence & seq,
        int pos,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );
    virtual ~seqedit ();

private:

    void set_zoom (int zoom);
    void set_snap (int snap);
    void set_note_length (int note_length);
    void set_beats_per_bar (int bpm);
    void set_beats_per_bar_manual ();                       /* issue #77    */
    void set_beat_width (int bw);
#ifdef SEQ64_STAZED_TRANSPOSE
    void set_transpose_image (bool istransposable);
#endif
    void set_mousemode_image (bool isfruity);
    void set_rec_vol (int recvol);
    void set_rec_type (loop_record_t rectype);

    /**
     *  This function provides optimization for the on_scroll_event() function.
     *  A duplicate of the one in seqroll.
     *
     * \param step
     *      Provides the step value to use for adjusting the horizontal
     *      scrollbar.  See gui_drawingarea_gtk2::scroll_hadjust() for more
     *      information.
     */

    void horizontal_adjust (double step)
    {
        scroll_hadjust(*m_hadjust, step);
    }

    /**
     *  This function provides optimization for the on_scroll_event() function.
     *  A near-duplicate of the one in seqroll.
     *
     * \param step
     *      Provides the step value to use for adjusting the vertical
     *      scrollbar.  See gui_drawingarea_gtk2::scroll_vadjust() for more
     *      information.
     */

    void vertical_adjust (double step)
    {
        scroll_vadjust(*m_vadjust, step);
    }

    /**
     *  Sets the exact position of a horizontal scroll-bar.
     *
     * \param value
     *      The desired position.  Mostly this is either 0.0 or 9999999.0 (an
     *      "infinite" value to select the start or end position.
     */

    void horizontal_set (double value)
    {
        scroll_hset(*m_hadjust, value);
    }

    /**
     *  Sets the exact position of a vertical scroll-bar.
     *
     * \param value
     *      The desired position.  Mostly this is either 0.0 or 9999999.0 (an
     *      "infinite" value to select the start or end position.
     */

    void vertical_set (double value)
    {
        scroll_vset(*m_vadjust, value);
    }

    int get_measures ();
    void set_measures (int lim);
    void set_measures_manual ();                            /* issue #77    */
    void apply_length (int bpb, int bw, int measures);
    void set_midi_channel (int midichannel, bool user_change = false);
    void set_midi_bus (int midibus, bool user_change = false);
    void set_scale (int scale);

#ifdef SEQ64_STAZED_CHORD_GENERATOR
    void set_chord (int chord);
#endif

    void set_key (int note);
    void set_background_sequence (int seq);
#ifdef SEQ64_STAZED_TRANSPOSE
    void transpose_change_callback ();
#endif
    void name_change_callback ();
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    void follow_change_callback ();
#endif
    void play_change_callback ();
    void record_change_callback ();
    void q_rec_change_callback ();
    void thru_change_callback ();
    void undo_callback ();
    void redo_callback ();
    void update_all_windows ();
    void fill_top_bar ();
    void create_menus ();
    void set_data_type (midibyte status, midibyte control = 0);
    void set_event_entry
    (
        Gtk::Menu * menu, const std::string & text, bool present,
        midibyte status, midibyte control = 0
    );
    void popup_menu (Gtk::Menu * menu);
    void popup_event_menu ();
    void repopulate_event_menu (int buss, int channel);
    void popup_mini_event_menu ();
    void repopulate_mini_event_menu (int buss, int channel);
    void popup_record_menu ();
    void popup_midibus_menu ();
    void popup_sequence_menu ();
    void popup_tool_menu ();
    void popup_midich_menu ();
    void repopulate_midich_menu (int buss);
    Gtk::Image * create_menu_image (bool state = false);
    bool timeout ();
    void do_action (int action, int var);
    void mouse_action (mouse_action_e action);

#ifdef USE_STAZED_PLAYING_CONTROL
    void start_playing ();
    void stop_playing ();
#endif

    void change_focus (bool set_it = true);
    void handle_close ();

private:    // Gtkmm 2.4 callbacks

    void on_realize ();
    void on_set_focus (Widget * focus);
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);
    bool on_delete_event (GdkEventAny * event);
    bool on_scroll_event (GdkEventScroll * ev);
    bool on_key_press_event (GdkEventKey * ev);
};

}           // namespace seq64

#endif      // SEQ64_SEQEDIT_HPP

/*
 * seqedit.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

