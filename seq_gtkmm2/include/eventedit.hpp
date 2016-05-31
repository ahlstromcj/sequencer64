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
 *  This module declares/defines the base class for the Event Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2016-05-29
 * \license       GNU GPLv2 or above
 *
 *  The Event Editor complements the Pattern (Sequence) Editor by allowing the
 *  composer to view all events, no matter what the type of event, and once,
 *  and to make detailed changes of events, or to add and delete individual
 *  events.  It is not a fully mature editor, but is useful enough for doing
 *  some fixups of MIDI events in a given pattern.
 */

#include <gtkmm/widget.h>               /* can't forward-declare GdkEventAny */
#include <gtkmm/window.h>               /* ditto for Window                  */

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
    class Label;
    class Menu;
    class Table;
    class ToggleButton;
    class Tooltips;
    class VBox;
    class VScrollbar;
}

namespace seq64
{
    class eventslots;
    class perform;
    class sequence;

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

    Gtk::Table * m_table;               /**< Provides the layout table for UI.  */
    Gtk::Adjustment * m_vadjust;        /**< Vertical paging for event list.    */
    Gtk::VScrollbar * m_vscroll;        /**< Vertical scroll for event list.    */
    eventslots * m_eventslots;          /**< Drawing area for events.           */
    Gtk::HBox * m_htopbox;              /**< Padding at the top of the dialog.  */
    Gtk::VBox * m_showbox;              /**< Area for sequence information.     */
    Gtk::VBox * m_editbox;              /**< Text-edits and buttons for data.   */
    Gtk::VBox * m_optsbox;              /**< Reserved for future options.       */
    Gtk::HBox * m_bottbox;              /**< Holds the Save and Close buttons.  */
    Gtk::VBox * m_rightbox;             /**< Used for padding on right side.    */

    Gtk::Button * m_button_del;         /**< "Delete Current Event (*)" button. */
    Gtk::Button * m_button_ins;         /**< "Insert New Event" button.         */
    Gtk::Button * m_button_modify;      /**< "Modify New Event" button.         */
    Gtk::Button * m_button_save;        /**< "Save to Sequence" button.         */
    Gtk::Button * m_button_cancel;      /**< "Close" button.                    */

    /**
     * Items for the inside of the m_showbox member.
     */

    Gtk::Label * m_label_seq_name;      /**< Shows the name of the pattern.     */
    Gtk::Label * m_label_time_sig;      /**< Shows time signature for pattern.  */
    Gtk::Label * m_label_ppqn;          /**< Shows the parts per quarter note.  */
    Gtk::Label * m_label_channel;       /**< Shows channel number of pattern.   */
    Gtk::Label * m_label_ev_count;      /**< Shows the count of pattern events. */
    Gtk::Label * m_label_spacer;        /**< Spacer for the showbox elements.   */
    Gtk::Label * m_label_modified;      /**< Shows "[Modified]" if edited.      */

    /**
     *  Items for the inside of the m_editbox member.
     */

    Gtk::Label * m_label_category;      /**< Shows the type of MIDI event.      */
    Gtk::Entry * m_entry_ev_timestamp;  /**< Text edit for event time-stamp.    */
    Gtk::Entry * m_entry_ev_name;       /**< Text edit for MIDI event name.     */
    Gtk::Entry * m_entry_ev_data_0;     /**< Text edit for first event datum.   */
    Gtk::Entry * m_entry_ev_data_1;     /**< Text edit for second event datum.  */
    Gtk::Label * m_label_time_fmt;      /**< Optsbox item, only "Sequencer64".  */
    Gtk::Label * m_label_right;         /**< Padding at the right of dialog.    */

    /**
     *  A reference to the sequence being edited, to control its editing flag.
     */

    sequence & m_seq;

    /**
     *  Indicates that the focus has already been changed to this sequence.
     *  This item is to modify the mainwid and perfedit "edit-sequence" value
     *  in order to highlight pattern slot of the pattern/event editor that
     *  currently has the user-input focus.
     */

    bool m_have_focus;

public:

    eventedit (perform & p, sequence & seq);
    virtual ~eventedit ();

    void enqueue_draw ();
    void set_seq_title (const std::string & title);
    void set_seq_time_sig (const std::string & sig);
    void set_seq_ppqn (const std::string & p);
    void set_seq_count ();
    void set_event_category (const std::string & c);
    void set_event_timestamp (const std::string & ts);
    void set_event_name (const std::string & n);
    void set_event_data_0 (const std::string & d);
    void set_event_data_1 (const std::string & d);
    void perf_modify ();
    void set_dirty (bool flag = true);
    void v_adjustment (int value);
    void v_adjustment (int value, int lower, int upper);
    void change_focus (bool set_it = true);
    void handle_close ();

private:

    /*
     * We don't need a timeout in this static editing window which doesn't
     * interact directly with other editing windows.
     *
     * bool timeout ();
     */

    void handle_delete ();
    void handle_insert ();
    void handle_modify ();
    void handle_save ();
    void handle_cancel ();

private:            // Gtkmm 2.4 callbacks

    void on_realize ();
    void on_set_focus (Widget * focus);
    bool on_focus_in_event (GdkEventFocus *);
    bool on_focus_out_event (GdkEventFocus *);
    bool on_key_press_event (GdkEventKey * ev);
    bool on_delete_event (GdkEventAny * event);

};          // class eventedit

}           // namespace seq64

#endif      // SEQ64_EVENTEDIT_HPP

/*
 * eventedit.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

