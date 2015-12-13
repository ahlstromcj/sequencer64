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
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2015-12-13
 * \license       GNU GPLv2 or above
 *
 *
 * To consider:
 *
 *      -   Selecting multiple events?
 *      -   Looping over multiple events for play/stp?
 *      -   Undo
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
#include "perform.hpp"

#if USE_BUTTON_PIXMAP                   /* not good enough to enable */
#include "pixmaps/del.xpm"
#include "pixmaps/ins.xpm"
#endif

#include "pixmaps/perfedit.xpm"

using namespace Gtk::Menu_Helpers;

namespace seq64
{

/**
 *  Principal constructor, has a reference to a perform object.
 *  We've reordered the pointer members and put them in the initializer
 *  list to make the constructor a bit cleaner.
 *
 *  Adjustment parameters:
 *
 *      value            initial value
 *      lower            minimum value
 *      upper            maximum value
 *      step_increment   step increment
 *      page_increment   page increment
 *      page_size        page size
 *
 *  Table constructor parameters:
 *
 *      rows
 *      columns
 *      homogenous
 *
 *  Table attach() parameters:
 *
 *      child           widget to add.
 *      left_attach     column number to attach left side of a child widget
 *      right_attach    column number to attach right side of a child widget
 *      top_attach      row number to attach the top of a child widget
 *      bottom_attach   row number to attach the bottom of a child widget
 *      xoptions        properties of the child widget when table resized
 *      yoptions        same as xoptions, except vertical.
 *      xpadding        padding on L and R of widget added to table
 *      ypadding        amount of padding above and below the child widget
 *
 * Layout:
 *
 *      We're going to change the layout.
 *
\verbatim
          0                             1   2                         3   4
           ---------------------------------------------------------------   0
     htop |                             :   :                             |
          |---------------------------------------- showbox --------------|  1
  e'slots |  1-120:0:192 Program Change | ^ | "Sequence name"         |   |
          |-----------------------------|   | 4/4 PPQN 192            | r |  2
          |  2-120:1:0   Program Change | s | 9999 events             | i |  3
          |-----------------------------| c |------ editbox ----------| g |  4
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
          |-----------------------------|   |  o Measures             |   |
          | 56-136:3:133 Program Change | v |  o Time                 |   |
          |---------------------------------------------------------------| 13
     bott |                             :   :                             |
           ---------------------------------------------------------------  14
\endverbatim
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \param seq
 *      Refers to the sequence holding the event data to be edited.
 */

eventedit::eventedit
(
    perform & p,
    sequence & seq
) :
    gui_window_gtk2     (p, 620, 700),
    m_table             (manage(new Gtk::Table(3, 3, false))),
    m_vadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll           (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_eventslots
    (
        manage(new eventslots(perf(), *this, seq, *m_vadjust))
    ),
    m_htopbox           (manage(new Gtk::HBox(false, 2))),
    m_showbox           (manage(new Gtk::VBox(false, 2))),
    m_editbox           (manage(new Gtk::VBox(false, 2))),
    m_optsbox           (manage(new Gtk::VBox(false, 2))),
    m_bottbox           (manage(new Gtk::HBox(false, 2))),
    m_rightbox          (manage(new Gtk::VBox(false, 2))),
    m_button_del        (manage(new Gtk::Button())),
    m_button_ins        (manage(new Gtk::Button())),
    m_button_apply      (manage(new Gtk::Button())),
    m_label_index       (manage(new Gtk::Label())),
    m_label_time        (manage(new Gtk::Label())),
    m_label_event       (manage(new Gtk::Label())),
    m_label_seq_name    (manage(new Gtk::Label())),
    m_label_time_sig    (manage(new Gtk::Label())),
    m_label_ppqn        (manage(new Gtk::Label())),
    m_label_ev_count    (manage(new Gtk::Label())),
    m_label_category    (manage(new Gtk::Label())),
    m_entry_ev_timestamp(manage(new Gtk::Entry())),
    m_entry_ev_name     (manage(new Gtk::Entry())),
    m_entry_ev_data_0   (manage(new Gtk::Entry())),
    m_entry_ev_data_1   (manage(new Gtk::Entry())),
    m_label_time_fmt    (manage(new Gtk::Label())),
    m_label_right       (manage(new Gtk::Label())),
    m_redraw_ms         (c_redraw_ms)                       /* 40 ms        */
{
    std::string title = "Sequencer64 - Event Editor";
    set_title(title);                                       /* caption bar  */
    set_icon(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm));
    m_table->set_border_width(2);
    m_htopbox->set_border_width(2);
    m_showbox->set_border_width(2);
    m_editbox->set_border_width(2);
    m_optsbox->set_border_width(2);
    m_rightbox->set_border_width(2);
    m_table->attach(*m_htopbox,    0, 4, 0, 1,   Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_eventslots, 0, 1, 1, 13,  Gtk::FILL, Gtk::FILL, 8, 8);
    m_table->attach
    (
        *m_vscroll, 1, 2, 1, 13, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND, 4, 4
    );
    m_table->attach(*m_showbox,    2, 3, 1, 4,   Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_editbox,    2, 3, 4, 10,  Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_optsbox,    2, 3, 10, 13, Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_bottbox,    0, 4, 13, 14, Gtk::FILL, Gtk::SHRINK, 8, 8);
    m_table->attach(*m_rightbox,   3, 4, 1, 13,  Gtk::FILL, Gtk::SHRINK, 2, 2);

    add(*m_table);

#if USE_BUTTON_PIXMAP
    m_button_del->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(del_xpm)))
    );
#else
    m_button_del->set_label("Delete");
#endif

    m_button_del->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_delete)
    );
    add_tooltip(m_button_del, "Delete the currently-selected event.");

#if USE_BUTTON_PIXMAP
    m_button_ins->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(ins_xpm)))
    );
#else
    m_button_ins->set_label("Insert");
#endif

    m_button_ins->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_insert)
    );
    add_tooltip
    (
        m_button_ins,
        "Insert event. Its location is determined by the timestamp."
    );

    m_button_apply->set_label("Apply");
    m_button_apply->signal_clicked().connect
    (
        sigc::mem_fun(*this, &eventedit::handle_apply)
    );
    add_tooltip(m_button_apply, "Apply changes to currently-selected event.");

    char temptext[36];

    snprintf(temptext, sizeof temptext, "\"%s\"", seq.get_name());
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

    m_label_ev_count->set_width_chars(32);
    set_seq_count();
    m_showbox->pack_start(*m_label_ev_count, false, false);

    m_label_category->set_width_chars(24);
    m_label_category->set_text("Channel Event: Ch. 5");
    m_editbox->pack_start(*m_label_category, false, false);

    m_entry_ev_timestamp->set_max_length(16);
    m_entry_ev_timestamp->set_editable(true);
    m_entry_ev_timestamp->set_width_chars(16);
    m_entry_ev_timestamp->set_text("000:0:000");
    m_editbox->pack_start(*m_entry_ev_timestamp, false, false);

    m_entry_ev_name->set_max_length(32);
    m_entry_ev_name->set_editable(true);
    m_entry_ev_name->set_width_chars(18);
    m_entry_ev_name->set_text("Note On");
    m_editbox->pack_start(*m_entry_ev_name, false, false);

    m_entry_ev_data_0->set_max_length(32);
    m_entry_ev_data_0->set_editable(true);
    m_entry_ev_data_0->set_width_chars(32);
    m_entry_ev_data_0->set_text("Key 101");
    m_editbox->pack_start(*m_entry_ev_data_0, false, false);

    m_entry_ev_data_1->set_max_length(32);
    m_entry_ev_data_1->set_editable(true);
    m_entry_ev_data_1->set_width_chars(32);
    m_entry_ev_data_1->set_text("Vel 64");
    m_editbox->pack_start(*m_entry_ev_data_1, false, false);

    m_editbox->pack_start(*m_button_del, false, false);
    m_editbox->pack_start(*m_button_ins, false, false);
    m_editbox->pack_start(*m_button_apply, false, false);

#ifdef USE_TIME_FORMAT_RADIOBUTTONS
    m_label_time_fmt->set_width_chars(24);
    m_label_time_fmt->set_text("\n\nTime Format (radio buttons)");
    m_optsbox->pack_end(*m_label_time_fmt, false, false);
#endif

    m_label_right->set_width_chars(2);
    m_label_right->set_text("--");
    m_rightbox->pack_start(*m_label_right, true, true);

    /*
     * Doesn't do anything:
     *
     * m_table->show();
     * show_all();
     */
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
 *
 * \param c
 *      The number-of-events string for the sequence.
 */

void
eventedit::set_seq_count ()
{
    char temptext[36];
    snprintf
    (
        temptext, sizeof temptext, "Sequence Count: %d events",
        m_eventslots->event_count()
    );
    m_label_ev_count->set_text(temptext);
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
 *  Sets the parameters for the vertical scroll-bar that is associated with
 *  the eventslots event-list user-interface.  Some of the parameters are
 *  obtained from the eventslots object:
 *
 *      -   Page size comes from eventslots::display_count().
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
    m_vadjust->set_page_size(m_eventslots->display_count());
    m_vadjust->set_step_increment(1);
    m_vadjust->set_page_increment(m_eventslots->display_count() - 8);
    if (value >= lower && value <= upper)
        m_vadjust->set_value(value);
}

/**
 *  Helper wrapper for calling perfroll::queue_draw() for one or both
 *  eventedits.
 *
 * \param forward
 *      If true (the default), pass the call to the peer.  When passing this
 *      call to the peer, this parameter is set to false to prevent an
 *      infinite loop and the resultant stack overflow.
 */

void
eventedit::enqueue_draw ()
{
    m_eventslots->queue_draw();
}

/**
 *  Initiates the deletion of the current editable event.
 */

void
eventedit::handle_delete ()
{
    (void) m_eventslots->delete_current_event();
    set_seq_count();
}

/**
 *  Initiates the insertion of a new editable event.  The event's location
 *  will be determined by the timestamp and existing events.
 */

void
eventedit::handle_insert ()
{
    // TODO
}

/**
 *  Passes the edited fields to the current editable event in the eventslot.
 *  Note that there are two cases to worry about.  If the timestamp has not
 *  changed, then we can simply modify the existing current event in place.
 *  Otherwise, we need to delete the old event and insert the new one.
 *  But that is done for us by eventslots::modify_current_event().
 */

void
eventedit::handle_apply ()
{
    if (not_nullptr(m_eventslots))
    {
        std::string ts = m_entry_ev_timestamp->get_text();
        std::string name = m_entry_ev_name->get_text();
        std::string data0 = m_entry_ev_data_0->get_text();
        std::string data1 = m_entry_ev_data_1->get_text();
        m_eventslots->modify_current_event(ts, name, data0, data1);
    }
}

/**
 *  Handles a drawing timeout.  It redraws "dirty" sequences in the
 *  perfroll and the eventslots objects, and shows draw progress on the
 *  perfroll.  This function is called frequently and continuously.
 */

bool
eventedit::timeout ()
{
    m_eventslots->redraw_dirty_events();
    return true;
}

/**
 *  This callback function calls the base-class on_realize() function, and
 *  then connects the eventedit::timeout() function to the Glib
 *  signal-timeout, with a redraw timeout of m_redraw_ms.
 */

void
eventedit::on_realize ()
{
    gui_window_gtk2::on_realize();
    Glib::signal_timeout().connect
    (
        mem_fun(*this, &eventedit::timeout), m_redraw_ms
    );
    v_adjustment(0, 0, m_eventslots->event_count());
}

/**
 *  This function is the callback for a key-press event.
 */

bool
eventedit::on_key_press_event (GdkEventKey * ev)
{
    // bool event_was_handled = false;
    if (CAST_EQUIVALENT(ev->type, SEQ64_KEY_PRESS))
    {
        if (rc().print_keys())
        {
            printf
            (
                "key_press[%d] == %s\n",
                ev->keyval, gdk_keyval_name(ev->keyval)
            );
        }
    }
    return Gtk::Window::on_key_press_event(ev);
}

}           // namespace seq64

/*
 * eventedit.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

