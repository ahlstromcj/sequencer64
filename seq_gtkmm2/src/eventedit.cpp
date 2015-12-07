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
 * \updates       2015-12-06
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
 *          0    1           2                3   4                         5
 *           ---------------------------------------------------------------  0
 * Top bar  |    :           :                    :                         |
 *          |---------------------------------------------------------------| 1
 *          |  1 | 120:0:192 | Program Change | ^ | "Sequence name"         |
 *          |---------------------------------|   | 4/4 PPQN 192            |
 *          |  2 | 120:1:0   | Program Change | s | 9999 events             |
 *          |---------------------------------| c |-------------------------| 2
 *          | ...    ...          ...         | r | Channel Event: Ch. 5    |
 *          | ...    ...          ...         | o |- - - - - - - - - - - - -|
 *          | ...    ...          ...         | l | [Edit field: Note On  ] |
 *          | ...    ...          ...         | l |- - - - - - - - - - - - -|
 *          | ...    ...          ...         |   | [Edit field: Key #    ] |
 *          | ...    ...          ...         | b |- - - - - - - - - - - - -|
 *          | ...    ...          ...         | a | [Edit field: Vel #    ] |
 *          | ...    ...          ...         | r |- - - - - - - - - - - - -|
 *          | ...    ...          ...         |   | [Optional more data?  ] |
 *          | ...    ...          ...         |   |-------------------------| 3
 *          | ...    ...          ...         | v |  o Pulses               |
 *          |-------------------------------------|  o Measures             |
 *          | 56 | 136:3:133 | Program Change ... |  o Time                 |
 *           ------------------------------------- - - - - - - - - - - - - -  4
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
    gui_window_gtk2     (p, 500, 700),
    m_table             (manage(new Gtk::Table(3, 3, false))),
    m_vadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll           (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_eventslots
    (
        manage(new eventslots(perf(), *this, seq, *m_vadjust))
    ),
    m_htopbox           (manage(new Gtk::HBox(false, 2))),
    m_editbox           (manage(new Gtk::VBox(false, 2))),
    m_bottbox           (manage(new Gtk::HBox(false, 2))),
    m_label_seq_name    (manage(new Gtk::Label())),
    m_label_time_sig    (manage(new Gtk::Label())),
    m_label_ppqn        (manage(new Gtk::Label())),
    m_label_ev_count    (manage(new Gtk::Label())),
    m_label_category    (manage(new Gtk::Label())),
    m_label_ev_name     (manage(new Gtk::Label())),
    m_entry_ev_name     (manage(new Gtk::Entry())),
    m_redraw_ms         (c_redraw_ms)                       /* 40 ms        */
{
    std::string title = "Sequencer64 - Event Editor";
    set_title(title);                                       /* caption bar  */
    set_icon(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm));
    m_table->set_border_width(2);
    m_bottbox->set_border_width(2);
    m_table->attach(*m_htopbox, 0, 3, 0, 1,  Gtk::FILL, Gtk::SHRINK, 0, 2);
    m_table->attach(*m_eventslots, 0, 1, 1, 3, Gtk::FILL, Gtk::FILL);
    m_table->attach(*m_vscroll, 1, 2, 1, 3, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND);
    m_table->attach(*m_editbox, 2, 3, 1, 2,  Gtk::FILL, Gtk::SHRINK, 0, 2);
    m_table->attach(*m_bottbox, 2, 3, 2, 3,  Gtk::FILL, Gtk::SHRINK, 2, 0);
    add(*m_table);

//  m_label_seq_name->set_max_length(32);
//  m_label_seq_name->set_editable(false);

    m_label_seq_name->set_width_chars(32);
    m_label_seq_name->set_text("Untitled/Empty sequence");
    m_htopbox->pack_start(*m_label_seq_name, false, false); /* expand and fill */

    m_label_time_sig->set_width_chars(5);
    m_label_time_sig->set_text("4/4");
    m_htopbox->pack_start(*m_label_time_sig, false, false); /* expand and fill */

    m_label_ppqn->set_width_chars(5);
    m_label_ppqn->set_text("192");
    m_htopbox->pack_start(*m_label_ppqn, false, false); /* expand and fill */

    m_label_ev_count->set_width_chars(12);
    m_label_ev_count->set_text("9999 events");
    m_htopbox->pack_start(*m_label_ev_count, false, false); /* expand and fill */

    m_label_category->set_width_chars(24);
    m_label_category->set_text("Category:  Channel Event");
    m_editbox->pack_start(*m_label_category, false, false);

    m_label_ev_name->set_width_chars(24);
    m_label_ev_name->set_text("Event Type");
    m_editbox->pack_start(*m_label_ev_name, false, false);

    m_entry_ev_name->set_max_length(32);
    m_entry_ev_name->set_editable(true);
    m_entry_ev_name->set_width_chars(18);
    m_entry_ev_name->set_text("Unknown MIDI event");
    m_editbox->pack_start(*m_entry_ev_name, false, false);

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
 *  Opens the given popup menu.
 */

void
eventedit::popup_menu (Gtk::Menu * menu)
{
    menu->popup(0, 0);
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

