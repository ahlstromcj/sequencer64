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
 * \file          eventedit.cpp
 *
 *  This module declares/defines the base class for the Event Editor, a new
 *  feature.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2018-03-25
 * \license       GNU GPLv2 or above
 *
 * To consider:
 *
 *      -   Selecting multiple events?
 *      -   Looping over multiple events for play/stop?
 *      -   Undo?  Currently, one can only exit the dialog without saving the
 *          edits to the sequence.
 *
 * Current bugs to fix:
 *
 *      -   Implement End key.
 *      -   Implement Delete and Insert keys.  Could also implement
 *          Backspace.  However, the edit boxes currently some grab these
 *          keystrokes.  For the Delete key, using the asterisk is supported.
 *      -   Fix the effect of timestamp modification.
 *      -   Fix event modification (delete/insert).
 *      -   Fix event modification of event name.
 *      -   Improve labelling differentiation for the data of various channel
 *          events.
 *
 *      Actually, some of these bugs might already be fixed, but we don't
 *      remember which.  :-D
 */

#include <string>
#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/table.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/combo.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/separator.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/arrow.h>
#include <gtkmm/image.h>
#include <sigc++/bind.h>

#include "gdk_basic_keys.h"
#include "gtk_helpers.h"
#include "eventedit.hpp"
#include "eventslots.hpp"
#include "mainwid.hpp"
#include "perfedit.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()  */
#include "pixmaps/perfedit.xpm"         /* just a stock app icon        */

using namespace Gtk::Menu_Helpers;      /* MenuElem, etc.               */

/*
 * The namespace of all Sequencer64 library modules.
 */

namespace seq64
{

/**
 *  Principal constructor, has a reference to a perform object.
 *  We've reordered the pointer members and put them in the initializer
 *  list to make the constructor a bit cleaner.
 *
 *  Adjustment parameters:
 *
\verbatim
        value            initial value
        lower            minimum value
        upper            maximum value
        step_increment   step increment
        page_increment   page increment
        page_size        page size
\endverbatim
 *
 *  Table constructor parameters:
 *
\verbatim
        rows
        columns
        homogenous
\endverbatim
 *
 *  Table attach() parameters:
 *
\verbatim
        child           widget to add.
        left_attach     column number to attach left side of a child widget
        right_attach    column number to attach right side of a child widget
        top_attach      row number to attach the top of a child widget
        bottom_attach   row number to attach the bottom of a child widget
        xoptions        properties of the child widget when table resized
        yoptions        same as xoptions, except vertical.
        xpadding        padding on L and R of widget added to table
        ypadding        amount of padding above and below the child widget
\endverbatim
 *
 * Layout:
 *
\verbatim
          0                             1   2                         3   4
           ---------------------------------------------------------------   0
     htop |  (OLD LAYOUT)               :   :                             |
          |---------------------------------------- showbox --------------|  1
  e'slots |  1-120:0:192 Program Change | ^ | "Sequence name"         |   |
          |- - - - - - - - - - - - - - -|   | 4/4 PPQN 192            | r |  2
          |  2-120:1:0   Program Change | s | 9999 events             | i |  3
          |- - - - - - - - - - - - - - -| c |------ editbox ----------| g |  4
          | ...    ...          ...     | r | Channel Event: Ch. 5    | h |
          | ...    ...          ...     | o |- - - - - - - - - - - - -| t |  6
          | ...    ...          ...     | l | [Edit field: Note On  ] |   |
          | ...    ...          ...     | l |- - - - - - - - - - - - -| b |  7
          | ...    ...          ...     |   | [Edit field: Key #    ] | o |
          | ...    ...          ...     | b |- - - - - - - - - - - - -| x |  8
          | ...    ...          ...     | a | [Edit field: Vel #    ] |   |
          | ...    ...          ...     | r |- - - - - - - - - - - - -|   |  9
          | ...    ...          ...     |   | [Optional more data?  ] |   |
          | ...    ...          ...     |   |------ optsbox ----------|   | 10
          | ...    ...          ...     |   |  o Pulses               |   |
          | ...    ...          ...     |   |  o Measures             |   |
          | ...    ...          ...     | v |  o Time                 |   |
          |- - - - - - - - - - - - - - -|   |------ bottbox ----------|   | 13
          | 56-136:3:133 Program Change | v |  | Save |    |  Close | |   |
           ---------------------------------------------------------------  14
\endverbatim
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \param seq
 *      Refers to the sequence holding the event data to be edited.
 */

eventedit::eventedit (perform & p, sequence & seq)
 :
    gui_window_gtk2     (p, 700, 666),      /* make sure it is wide enough! */
    m_table             (manage(new Gtk::Table(14, 4, false))),
    m_vadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll           (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_eventslots        (manage(new eventslots(perf(), *this, seq, *m_vadjust))),
    m_htopbox           (manage(new Gtk::HBox(false, 2))),
    m_showbox           (manage(new Gtk::VBox(false, 2))),
    m_editbox           (manage(new Gtk::VBox(false, 2))),
    m_optsbox           (manage(new Gtk::VBox(true, 2))),
    m_bottbox           (manage(new Gtk::HBox(false, 2))),
    m_rightbox          (manage(new Gtk::VBox(true, 2))),
    m_button_del        (manage(new Gtk::Button())),
    m_button_ins        (manage(new Gtk::Button())),
    m_button_modify     (manage(new Gtk::Button())),
    m_button_save       (manage(new Gtk::Button())),
    m_button_cancel     (manage(new Gtk::Button())),
    m_label_seq_name    (manage(new Gtk::Label())),
    m_label_time_sig    (manage(new Gtk::Label())),
    m_label_ppqn        (manage(new Gtk::Label())),
    m_label_channel     (manage(new Gtk::Label())),
    m_label_ev_count    (manage(new Gtk::Label())),
    m_label_seq_length  (manage(new Gtk::Label())),
    m_label_spacer      (manage(new Gtk::Label())),
    m_label_modified    (manage(new Gtk::Label())),
    m_label_category    (manage(new Gtk::Label())),
    m_entry_ev_timestamp(manage(new Gtk::Entry())),
    m_entry_ev_name     (manage(new Gtk::Entry())),
    m_entry_ev_data_0   (manage(new Gtk::Entry())),
    m_entry_ev_data_1   (manage(new Gtk::Entry())),
    m_label_time_fmt    (manage(new Gtk::Label())),
    m_label_right       (manage(new Gtk::Label())),
    m_seq               (seq),
    m_have_focus        (false)
{
    std::string title = SEQ64_APP_NAME " #";        /* main window title    */
    title += m_seq.seq_number();
    title += " \"";
    title += m_seq.name();
    title += "\"";
    set_title(title);                                       /* caption bar  */
    set_icon(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm));

    /**
     *  The seqedit class indirectly sets the sequence dirty flags, and this
     *  allows the sequence's pattern slot to be updated, which, for example,
     *  allows the new optional in-edit-highlight feature to work.  To get
     *  the eventedit to also show the in-edit highlighting, we can make the
     *  sequence::set_dirty_mp() call.  This call does not cause a prompt for
     *  saving the file when exiting.
     */

    m_seq.set_editing(true);
    m_seq.set_dirty_mp();

    m_table->set_border_width(2);
    m_htopbox->set_border_width(4);
    m_showbox->set_border_width(4);
    m_editbox->set_border_width(4);
    m_optsbox->set_border_width(4);
    m_rightbox->set_border_width(1);
    m_table->attach(*m_eventslots, 0, 1,  0, 13,  Gtk::FILL, Gtk::FILL, 8, 8);
    m_table->attach
    (
        *m_vscroll, 1, 2, 0, 13, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND, 4, 4
    );
    m_table->attach(*m_showbox,  2, 3,  0,  3, Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_editbox,  2, 3,  3,  9, Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_optsbox,  2, 3,  9, 12, Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_bottbox,  2, 3, 12, 13, Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_rightbox, 3, 4,  0, 13, Gtk::SHRINK, Gtk::SHRINK, 8, 8);
    add_tooltip
    (
        m_eventslots,
        "Navigate using the scrollbar, arrow keys, Page keys, and Home/End keys. "
    );

    m_button_del->set_label("Delete Current Event (*)");
    m_button_del->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_delete)
    );
    add_tooltip
    (
        m_button_del,
        "Deletes the currently-selected event, even if event is not visible "
        "in the frame.  Can also use the asterisk key. The Delete key "
        "is reserved for the edit fields."
    );

    m_button_ins->set_label("Insert New Event");
    m_button_ins->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_insert)
    );
    add_tooltip
    (
        m_button_ins,
        "Insert a new event using the data in the edit fields. Its actual "
        "location is determined by the timestamp field, not the current "
        "event.  The Insert key is reserved for the edit fields, and only "
        "the Insert New Event button will work."
    );

    m_button_modify->set_label("Modify Current Event");
    m_button_modify->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_modify)
    );
    add_tooltip
    (
        m_button_modify,
        "Apply the changes in the edit fields to the currently-selected event, "
        "even if the event is not visible."
    );

    m_button_save->set_label("Save to Sequence");
    m_button_save->set_sensitive(false);
    m_button_save->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_save)
    );
    add_tooltip
    (
        m_button_save,
        "Save the edit.  Copies the edited events back to the sequence, "
        "making them permanent, but does not close the dialog."
    );

    m_button_cancel->set_label("Close");
    m_button_cancel->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_cancel)
    );
    add_tooltip
    (
        m_button_cancel,
        "Abort the edit and close the dialog.  Any changes made in this "
        "window are thrown away (without prompting), unless the Save button "
        "was pressed."
    );

    char temptext[40];
    snprintf
    (
        temptext, sizeof temptext, "%s: \"%s\"",
//      seq.seq_number().c_str(), seq.name().c_str()
        m_seq.seq_number().c_str(), m_seq.name().c_str()
    );
    m_label_seq_name->set_width_chars(32);
    m_label_seq_name->set_text(temptext);
    m_showbox->pack_start(*m_label_seq_name, false, false); /* expand and fill */

    snprintf
    (
        temptext, sizeof temptext, "Time Signature: %d/%d",
        seq.get_beats_per_bar(), seq.get_beat_width()
    );
    m_label_time_sig->set_text(temptext);
    m_showbox->pack_start(*m_label_time_sig, false, false);

    snprintf(temptext, sizeof temptext, "PPQN (Divisions): %d", seq.get_ppqn());
    m_label_ppqn->set_text(temptext);
    m_showbox->pack_start(*m_label_ppqn, false, false);

    snprintf
    (
        temptext, sizeof temptext, "Channel: %d [re 0]",
        seq.get_midi_channel()          /* + 1 to show MIDI channels 1-16 */
    );
    m_label_channel->set_text(temptext);
    m_showbox->pack_start(*m_label_channel, false, false);

    m_label_ev_count->set_width_chars(32);
    set_seq_count();
    m_showbox->pack_start(*m_label_ev_count, false, false);

    m_label_seq_length->set_width_chars(32);
    set_seq_length();
    m_showbox->pack_start(*m_label_seq_length, false, false);

    m_label_spacer->set_width_chars(1);
    m_showbox->pack_start(*m_label_spacer, false, false);
    m_label_spacer->set_text("");

    m_label_modified->set_width_chars(1);
    m_showbox->pack_start(*m_label_modified, false, false);
    m_label_modified->set_text("");

    m_label_category->set_width_chars(24);
    m_label_category->set_text("Channel Event: Ch. 5");
    m_editbox->pack_start(*m_label_category, false, false);

    m_entry_ev_timestamp->set_max_length(16);
    m_entry_ev_timestamp->set_editable(true);
    m_entry_ev_timestamp->set_width_chars(16);
    m_entry_ev_timestamp->set_text("001:1:000");

    /*
     * Let's see if we can get the key cursor to show in this entry box.
     * Hope it doesn't mess up edited-sequence highlighting!  None of this
     * works.  Why no edit cursor!?!?
     *
     *  m_entry_ev_timestamp->set_focus_on_click(true);
     *  m_entry_ev_timestamp->set_can_focus(true);
     *  m_entry_ev_timestamp->grab_focus();
     *  m_entry_ev_timestamp->set_flags
     *  (
     *      m_entry_ev_timestamp->get_flags() | Gtk::CAN_FOCUS
     *  );
     *  m_entry_ev_timestamp->set_has_frame(true);
     *
     *  m_editbox->set_focus_child(m_entry_ev_timestamp);
     */

    add_tooltip
    (
        m_entry_ev_timestamp,
        "Timestamp field.  Currently only the 'measures:beats:divisions' "
        "format is supported. Measure and beat numbers start at 1, not 0. "
        "As a shortcut, the divisions number can be a dollar sign ($), "
        "to indicate the PPQN value minus 1."
    );
    m_editbox->pack_start(*m_entry_ev_timestamp, false, false);

    m_entry_ev_name->set_max_length(32);
    m_entry_ev_name->set_editable(true);
    m_entry_ev_name->set_width_chars(18);
    m_entry_ev_name->set_text("Note On");
    add_tooltip
    (
        m_entry_ev_name,
        "Event name field.  Recognized events: Note On, Note Off, Aftertouch, "
        "Control Change, Program Change, Channel Pressure, Pitch Wheel, "
        "Time Sig, and Tempo."
    );
    m_editbox->pack_start(*m_entry_ev_name, false, false);

    m_entry_ev_data_0->set_max_length(32);
    m_entry_ev_data_0->set_editable(true);
    m_entry_ev_data_0->set_width_chars(32);
    m_entry_ev_data_0->set_text("Key 101");
    add_tooltip
    (
        m_entry_ev_data_0,
        "Type the numeric (hex or decimal) value of the first data byte here. "
        "Digits are converted until a non-digit is encountered."
        "The events that support only one value are Program Change, "
        "Channel Pressure, and Tempo."
    );
    m_editbox->pack_start(*m_entry_ev_data_0, false, false);

    m_entry_ev_data_1->set_max_length(32);
    m_entry_ev_data_1->set_editable(true);
    m_entry_ev_data_1->set_width_chars(32);
    m_entry_ev_data_1->set_text("Vel 64");
    add_tooltip
    (
        m_entry_ev_data_1,
        "Type the numeric (hex or decimal) value of the second data byte here. "
        "Digits are converted until a non-digit is encountered. "
        "The events that support two values are Note On, Note Off, Aftertouch, "
        "Control Change, and Pitch Wheel."
    );
    m_editbox->pack_start(*m_entry_ev_data_1, false, false);
    m_editbox->pack_start(*m_button_del, false, false);
    m_editbox->pack_start(*m_button_ins, false, false);
    m_editbox->pack_start(*m_button_modify, false, false);

    m_bottbox->pack_start(*m_button_save, true, false);     /* expand          */
    m_bottbox->pack_start(*m_button_cancel, true, true, 8); /* expand/fill/pad */

    m_label_time_fmt->set_width_chars(32);
    m_label_time_fmt->set_text("Sequencer64");

    m_optsbox->pack_end(*m_label_time_fmt, false, false);
    m_rightbox->pack_start(*m_label_right, false, false);

    add(*m_table);
    show_all();
}

/**
 *  This rote constructor does nothing.  We're going to have to run the
 *  application through valgrind to make sure that nothing is left behind.
 */

eventedit::~eventedit ()
{
    // Empty body
}

/**
 *  Sets m_label_seq_name to the title.
 *
 * \param title
 *      The name of the sequence.
 */

void
eventedit::set_seq_title (const std::string & title)
{
    m_label_seq_name->set_text(title);
}

/**
 *  Sets m_label_time_sig to the time-signature string.
 *
 * \param sig
 *      The time signature of the sequence.
 */

void
eventedit::set_seq_time_sig (const std::string & sig)
{
    m_label_time_sig->set_text(sig);
}

/**
 *  Sets m_label_ppqn to the parts-per-quarter-note string.
 *
 * \param p
 *      The parts-per-quarter-note string for the sequence.
 */

void
eventedit::set_seq_ppqn (const std::string & p)
{
    m_label_ppqn->set_text(p);
}

/**
 *  Sets m_label_ev_count to the number-of-events string.
 */

void
eventedit::set_seq_count ()
{
    char temptext[48];
    snprintf
    (
        temptext, sizeof temptext, "Count: %d events",
        m_eventslots->event_count()
    );
    m_label_ev_count->set_text(temptext);
}

/**
 *  Sets m_label_seq_length to the number-of-events string.
 *
 *      m_eventslots->seq().calculate_measures()
 */

void
eventedit::set_seq_length ()
{
    char temptext[48];
    snprintf
    (
        temptext, sizeof temptext, "Length: %d measures",
        m_eventslots->calculate_measures()
    );
    m_label_seq_length->set_text(temptext);
}

/**
 *  Sets m_label_category to the category string.
 *
 * \param c
 *      The category string for the current event.
 */

void
eventedit::set_event_category (const std::string & c)
{
    m_label_category->set_text(c);
}

/**
 *  Sets m_entry_ev_timestamp to the time-stamp string.
 *
 * \param ts
 *      The time-stamp string for the current event.
 */

void
eventedit::set_event_timestamp (const std::string & ts)
{
    m_entry_ev_timestamp->set_text(ts);
}

/**
 *  Sets m_entry_ev_name to the name-of-event string.
 *
 * \param n
 *      The name-of-event string for the current event.
 */

void
eventedit::set_event_name (const std::string & n)
{
    m_entry_ev_name->set_text(n);
}

/**
 *  Sets m_entry_ev_data_0 to the first data byte string.
 *
 * \param d
 *      The first data byte string for the current event.
 */

void
eventedit::set_event_data_0 (const std::string & d)
{
    m_entry_ev_data_0->set_text(d);
}

/**
 *  Sets m_entry_data_1 to the second data byte string.
 *
 * \param d
 *      The second data byte string for the current event.
 */

void
eventedit::set_event_data_1 (const std::string & d)
{
    m_entry_ev_data_1->set_text(d);
}

/**
 *  Sets the parameters for the vertical scroll-bar, using only the value
 *  parameter.  This function overload provides a common use case.
 *
 * \param value
 *      The new current value to be indicated by the scroll-bar.
 */

void
eventedit::v_adjustment (int value)
{
    v_adjustment(value, 0, m_eventslots->event_count());
}

/**
 *  Sets the parameters for the vertical scroll-bar that is associated with
 *  the eventslots event-list user-interface.  It keeps the frame scroll-bar
 *  in sync with the frame movement actions.  Some of the parameters are
 *  obtained from the eventslots object:
 *
 *      -   Page size comes from eventslots::line_maximum().
 *      -   Page increment is a little less than the page-size value.
 *
 * \param value
 *      The current value to be indicated by the scroll-bar.  It will lie
 *      between the lower and upper parameter.
 *
 * \param lower
 *      The lowest value to be indicated by the scroll-bar.
 *
 * \param upper
 *      The highest value to be indicated by the scroll-bar.
 */

void
eventedit::v_adjustment (int value, int lower, int upper)
{
    m_vadjust->set_lower(lower);
    m_vadjust->set_upper(upper);
    m_vadjust->set_page_size(m_eventslots->line_maximum());
    m_vadjust->set_step_increment(1);
    m_vadjust->set_page_increment(m_eventslots->line_increment());
    if (value >= lower && value <= upper)
        m_vadjust->set_value(value);
}

/**
 *  Helper wrapper for calling eventslots::queue_draw().
 */

void
eventedit::enqueue_draw ()
{
    m_eventslots->queue_draw();
}

/**
 *  Provides a way to mark the perform object as modified, when the modified
 *  sequence is saved.
 */

void
eventedit::perf_modify ()
{
    perf().modify();
    set_dirty(false);
}

/**
 *  Sets the "modified" status of the user-interface.  This includes changing
 *  a label, enabling/disabling the Save button, and modifying the event count
 *  and sequence length (in measures).
 *
 * \param flag
 *      If true, the modified status is indicated, otherwise it is cleared.
 */

void
eventedit::set_dirty (bool flag)
{
    if (flag)
    {
        m_label_modified->set_text("[ Modified ]");
        m_button_save->set_sensitive(true);
    }
    else
    {
        m_label_modified->set_text("[ Saved ]");
        m_button_save->set_sensitive(false);
    }
    set_seq_count();
    set_seq_length();
}

/**
 *  Initiates the deletion of the current editable event.
 */

void
eventedit::handle_delete ()
{
    bool isempty = ! m_eventslots->delete_current_event();
    set_seq_count();
    if (isempty)
    {
        m_button_del->set_sensitive(false);
        m_button_modify->set_sensitive(false);
    }
}

/**
 *  Initiates the insertion of a new editable event.  The event's location
 *  will be determined by the timestamp and existing events.  Note that we
 *  have to recalibrate the scroll-bar when we insert/delete events by calling
 *  v_adjustment().
 *
 *  As a new feature, we will allow events to extend the official length of
 *  the sequence.
 */

void
eventedit::handle_insert ()
{
    if (not_nullptr(m_eventslots))
    {
        std::string ts = m_entry_ev_timestamp->get_text();
        std::string name = m_entry_ev_name->get_text();
        std::string data0 = m_entry_ev_data_0->get_text();
        std::string data1 = m_entry_ev_data_1->get_text();
        bool has_events = m_eventslots->insert_event(ts, name, data0, data1);
        set_seq_count();
        if (has_events)
        {
            m_button_del->set_sensitive(true);
            m_button_modify->set_sensitive(true);
            v_adjustment(m_eventslots->pager_index());
        }
    }
}

/**
 *  Passes the edited fields to the current editable event in the eventslot.
 *  Note that there are two cases to worry about.  If the timestamp has not
 *  changed, then we can simply modify the existing current event in place.
 *  Otherwise, we need to delete the old event and insert the new one.
 *  But that is done for us by eventslots::modify_current_event().
 */

void
eventedit::handle_modify ()
{
    if (not_nullptr(m_eventslots))
    {
        std::string ts = m_entry_ev_timestamp->get_text();
        std::string name = m_entry_ev_name->get_text();
        std::string data0 = m_entry_ev_data_0->get_text();
        std::string data1 = m_entry_ev_data_1->get_text();
        (void) m_eventslots->modify_current_event(ts, name, data0, data1);
        set_seq_count();
    }
}

/**
 *  Handles saving the edited data back to the original sequence.
 *  The event list in the original sequence is cleared, and the editable
 *  events are converted to plain events, and added to the container, one by
 *  one.
 *
 * \todo
 *      Could also support writing the events to a new sequence, for added
 *      flexibility.
 */

void
eventedit::handle_save ()
{
    if (not_nullptr(m_eventslots))
    {
        bool ok = m_eventslots->save_events();
        if (ok)
            m_button_save->set_sensitive(false);
    }
}

/**
 *  Cancels the edits and closes the dialog box.  In order for removing the
 *  current-highlighting in the mainwd or perfedit windows, some of the work
 *  of handle_close() needs to be done here as well.
 */

void
eventedit::handle_cancel ()
{
    close_out();

#if GTK_MAJOR_VERSION >= 3
    Gtk::Window::close();
#else
    Gtk::Widget::hide();
    delete this;
#endif
}

/**
 *  Changes what perform and mainwid see as the "current sequence".  Similar
 *  to the same function in seqedit.
 *
 * \param set_it
 *      If true (the default value), indicates we want focus, otherwise we
 *      want to give up focus.
 */

void
eventedit::change_focus (bool set_it)
{
    if (set_it)
    {
        if (! m_have_focus)
        {
            perf().set_edit_sequence(m_seq.number());
            update_mainwid_sequences();
            update_perfedit_sequences();
            m_have_focus = true;
        }
    }
    else
    {
        if (m_have_focus)
        {
            perf().unset_edit_sequence(m_seq.number());
            update_mainwid_sequences();
            update_perfedit_sequences();
            m_have_focus = false;
        }
    }
}

/**
 *  Handles closing the sequence editor.  Simply calls close_out().
 */

void
eventedit::handle_close ()
{
    close_out();
}

/**
 *  Handles closing the sequence editor, common code for handle_cancel() and
 *  handle_close().
 */

void
eventedit::close_out ()
{
    /*
     * Added the next two lines to match seqedit::handle_close().
     */

    perf().master_bus().set_sequence_input(false, nullptr);
    m_seq.set_recording(false);
    m_seq.set_editing(false);
    change_focus(false);
}

/**
 *  This callback function calls the base-class on_realize() function.
 *  Then it sets the vertical adjustment to account for the number of events
 *  in the eventslot.
 */

void
eventedit::on_realize ()
{
    gui_window_gtk2::on_realize();
    v_adjustment(0, 0, m_eventslots->event_count());
}

/**
 *  On receiving focus, attempt to tell mainwid that this sequence is now the
 *  current sequence.  Only works in certain circumstances.
 *
 * \param focus
 *      The widget that has the focus.  Merely passed on to gui_window_gtk2's
 *      version of this function.
 */

void
eventedit::on_set_focus (Widget * focus)
{
    gui_window_gtk2::on_set_focus(focus);

    /*
     * This will cause a stack overflow :-D
     *
     * focus->grab_focus();
     */

    change_focus();
}

/**
 *  Implements the on-focus event handling.  It sets the focus flag and calls
 *  change_focus().
 */

bool
eventedit::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    change_focus();
    return false;
}

/**
 *  Implements the on-unfocus event handling.  It resets the focus flag and
 *  calls change_focus().
 */

bool
eventedit::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    change_focus(false);
    return false;
}

/**
 *  Handles an on-delete event.  It sets the sequence object's editing flag to
 *  false, and deletes "this".  This function is called if the "Close" ("X")
 *  button in the window's title bar is clicked.  That is a different action
 *  from clicking the Close button.
 *
 * \return
 *      Always returns false.
 */

bool
eventedit::on_delete_event (GdkEventAny *)
{
    handle_close();
    delete this;
    return false;
}

/**
 *  This function is the callback for a key-press event.  If the Up or Down
 *  arrow is pressed (later, k and j :-), then we tell the eventslots object to
 *  move the "current event" highlighting up or down.  In Gtkmm, these arrows
 *  also cause movement from one edit field to the next, so we disable
 *  that process if the event was handled here.
 *
 *  Note that the Delete key is needed for the edit fields.  For now, we
 *  replace it with the asterisk, which is easy to access from the numeric pad
 *  of a keyboard, and allows for rapid deletion.  The Insert key also causes
 *  confusing effects in the edit fields, so we replaced it by the slash, but
 *  that didn't work.  Note that the asterisk and slash should not be required
 *  in any of the edit fields.  HOWEVER, "/" still gets passed the edit fields
 *  (!), so you'll just have to click the button to insert an event.  Let's
 *  try the backslash!  No go there, either.
 *
 * \param ev
 *      The key event to process.
 *
 * \return
 *      Returns true if the event got handled somewhere along the line.
 */

bool
eventedit::on_key_press_event (GdkEventKey * ev)
{
    bool result = false;
    if (CAST_EQUIVALENT(ev->type, SEQ64_KEY_PRESS))
    {
        int key = int(ev->keyval);
        if (rc().print_keys())
        {
            printf
            (
                "key_press[%d] == %s\n",
                key, perf().key_name(key).c_str()   // keyval_name(key).c_str()
            );
        }
        if (key == SEQ64_Down)
        {
            result = true;
            m_eventslots->on_move_down();
        }
        else if (key == SEQ64_Up)
        {
            result = true;
            m_eventslots->on_move_up();
        }
        else if (key == SEQ64_Page_Down)
        {
            result = true;
            m_eventslots->on_frame_down();
            v_adjustment(m_eventslots->pager_index());
        }
        else if (key == SEQ64_Page_Up)
        {
            result = true;
            m_eventslots->on_frame_up();
            v_adjustment(m_eventslots->pager_index());
        }
        else if (key == SEQ64_Home)
        {
            result = true;
            m_eventslots->on_frame_home();
            v_adjustment(m_eventslots->pager_index());
        }
        else if (key == SEQ64_End)
        {
            result = true;
            m_eventslots->on_frame_end();
            v_adjustment(m_eventslots->pager_index());
        }
#ifdef CAN_USE_SLASH_PROPERLY                   /* we cannot    */
        else if (key == '/')                    /* SEQ64_Insert */
        {
            result = true;
            handle_insert ();
        }
        else if (key == '\\')                   /* SEQ64_Insert */
        {
            result = true;
            handle_insert ();
        }
#endif
        else if (key == '*')                    /* SEQ64_Delete */
        {
            result = true;
            handle_delete();
        }
        else if (key == SEQ64_KP_Multiply)
        {
            result = true;
            handle_delete();
        }
    }
    if (! result)
        result = Gtk::Window::on_key_press_event(ev);

    return result;
}

}           // namespace seq64

/*
 * eventedit.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

