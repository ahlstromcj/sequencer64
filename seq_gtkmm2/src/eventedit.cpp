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
 * \updates       2015-12-05
 * \license       GNU GPLv2 or above
 *
 *
 * To consider:
 *
 *      -   Selecting multiple events?
 *      -   Looping over multiple events for play/stp?
 *      -   Undo
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
// #include <gtkmm/window.h>
// #include <gtkmm/accelgroup.h>
#include <gtkmm/box.h>
// #include <gtkmm/main.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
// #include <gtkmm/eventbox.h>
#include <gtkmm/table.h>
// #include <gtkmm/drawingarea.h>
// #include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>
// #include <gtkmm/viewport.h>
#include <gtkmm/combo.h>
#include <gtkmm/label.h>
// #include <gtkmm/toolbar.h>
// #include <gtkmm/optionmenu.h>
#include <gtkmm/togglebutton.h>
// #include <gtkmm/invisible.h>
#include <gtkmm/separator.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/arrow.h>
#include <gtkmm/image.h>
#include <sigc++/bind.h>

#include "gdk_basic_keys.h"
#include "gtk_helpers.h"
#include "eventedit.hpp"
#include "eventslots.hpp"
#include "perfroll.hpp"
#include "perftime.hpp"

#ifdef USE_PLAY_FUNCTION
#include "pixmaps/play2.xpm"
#include "pixmaps/stop.xpm"
#endif
#ifdef USE_COPY_FUNCTION
#include "pixmaps/copy.xpm"
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
 * \param p
 *      Refers to the main performance object.
 *
 * \todo
 *      Offload most of the work into an initialization function like
 *      options does.
 */

eventedit::eventedit
(
    perform & p,
    int ppqn
) :
    gui_window_gtk2     (p, 750, 500),      /* set_size_request(700, 400) */
    m_table             (manage(new Gtk::Table(6, 3, false))),
    m_vadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll           (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_eventslots        (manage(new eventslots(perf(), *this, *m_vadjust))),
    m_button_stop       (manage(new Gtk::Button())),
    m_button_play       (manage(new Gtk::Button())),
    m_button_loop       (manage(new Gtk::ToggleButton())),
    m_button_copy       (manage(new Gtk::Button())),
    m_hbox              (manage(new Gtk::HBox(false, 2))),
    m_hlbox             (manage(new Gtk::HBox(false, 2))),
    m_tooltips          (manage(new Gtk::Tooltips())),  // valgrind complains!
    m_snap              (SEQ64_DEFAULT_PERFEDIT_SNAP),
    m_bpm               (0),
    m_bw                (0),
    m_ppqn              (0),
    m_standard_bpm      (SEQ64_DEFAULT_LINES_PER_MEASURE),  /* 4            */
    m_redraw_ms         (c_redraw_ms)                       /* 40 ms        */
{
    std::string title = "Sequencer64 - Event Editor";
    m_ppqn = choose_ppqn(ppqn);
    set_icon(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm));
    set_title(title);                                       /* caption bar  */
    m_table->set_border_width(2);
    m_hlbox->set_border_width(2);

    /*
     * Fill the table
     */

    m_table->attach(*m_hlbox, 0, 3, 0, 1,  Gtk::FILL, Gtk::SHRINK, 2, 0);
    m_table->attach(*m_eventslots, 0, 1, 2, 3, Gtk::SHRINK, Gtk::FILL);
    m_table->attach(*m_vscroll, 2, 3, 2, 3, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND);
    m_table->attach(*m_hbox,  0, 1, 3, 4,  Gtk::FILL, Gtk::SHRINK, 0, 2);

#ifdef USE_COPY_FUNCTION
    m_button_copy->add                              /* expand & copy    */
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(copy_xpm)))
    );
    m_button_copy->signal_clicked().connect(mem_fun(*this, &eventedit::copy));
    add_tooltip(m_button_copy, "Expand and copy between the L and R markers.");
#endif
#ifdef USE_PLAY_FUNCTION
    m_button_loop->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(loop_xpm)))
    );
    m_button_loop->signal_toggled().connect
    (
        mem_fun(*this, &eventedit::set_looped)
    );
    add_tooltip(m_button_loop, "Playback looped between the L and R markers.");
    m_button_stop->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(stop_xpm)))
    );
    m_button_stop->signal_clicked().connect
    (
        mem_fun(*this, &eventedit::stop_playing)
    );
    add_tooltip(m_button_stop, "Stop playback.");
    m_button_play->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(play2_xpm)))
    );
    m_button_play->signal_clicked().connect
    (
        mem_fun(*this, &eventedit::start_playing)
    );
    add_tooltip(m_button_play, "Begin playback at the L marker.");
#endif  // USE_PLAY_FUNCTION
    m_hlbox->pack_end(*m_button_copy , false, false);
#ifdef USE_PLAY_FUNCTION
    m_hlbox->pack_start(*m_button_stop , false, false);
    m_hlbox->pack_start(*m_button_play , false, false);
    m_hlbox->pack_start(*m_button_loop , false, false);
    m_hlbox->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
#endif
    m_hlbox->pack_start(*m_entry_bpm , false, false);
    m_hlbox->pack_start(*(manage(new Gtk::Label("/"))), false, false, 4);
    m_hlbox->pack_start(*m_entry_bw , false, false);
    m_hlbox->pack_start(*(manage(new Gtk::Label("x"))), false, false, 4);
    m_hlbox->pack_start(*m_entry_snap , false, false);
    add(*m_table);
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
eventedit::enqueue_draw (bool forward)
{
    m_eventslots->queue_draw();
}

/**
 *  Implement the undo feature (Ctrl-Z).  We pop an Undo trigger, and then
 *  ask the perfroll to queue up a (re)drawing action.
 */

void
eventedit::undo ()
{
    perf().pop_trigger_undo();
    enqueue_draw();
}

/**
 *  Implement the collapse action.  This action removes all events between
 *  the L and R (left and right) markers.  This action is preceded by
 *  pushing an Undo operation in the perform object, not moving its
 *  triggers (they go away), and telling the perfroll to redraw.
 */

void
eventedit::collapse ()
{
    perf().collapse();
    enqueue_draw();
}

/**
 *  Implement the copy of an event (or range of events????)
 */

void
eventedit::copy ()
{
    // perf().copy();
    enqueue_draw();
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
 *  This callback function calls the base-class on_realize() fucntion, and
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
    bool event_was_handled = false;
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

        /*
         *  By default, the space-bar starts the playing, and the Escape
         *  key stops the playing.  The start/end key may be the same key
         *  (i.e. space-bar), allow toggling when the same key is mapped
         *  to both triggers.
         */

        bool notoggle = PREFKEY(start) != PREFKEY(stop);
        if (ev->keyval == PREFKEY(start) && (notoggle || ! perf().is_running()))
        {
            start_playing();
            return true;
        }
        if (ev->keyval == PREFKEY(stop) && (notoggle || perf().is_running()))
        {
            stop_playing();
            return true;
        }
        if (ev->keyval == PREFKEY(start) || ev->keyval == PREFKEY(stop))
            event_was_handled = true;
    }
    return Gtk::Window::on_key_press_event(ev);
}

}           // namespace seq64

/*
 * eventedit.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

