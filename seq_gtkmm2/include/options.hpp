#ifndef SEQ24_OPTIONS_HPP
#define SEQ24_OPTIONS_HPP

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
 * \file          options.hpp
 *
 *  This module declares/defines the base class for the File / Options
 *  dialog.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-10
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/dialog.h>

namespace Gtk
{
    class Button;
    class Label;
    class Notebook;
    class RadioButton;
    class Table;
}

class perform;

#if GTK_MINOR_VERSION < 12
    class Tooltips;
#endif

/**
 *  This class supports a full tabbed options dialog.
 */

class options : public Gtk::Dialog
{

private:

    /**
     *  Defines buttons indices or IDs for some controls related to JACK.
     */

    enum button
    {
        e_jack_transport,
        e_jack_master,
        e_jack_master_cond,
        e_jack_start_mode_live,
        e_jack_start_mode_song,
        e_jack_connect,
        e_jack_disconnect
    };

private:

#if GTK_MINOR_VERSION < 12
    Gtk::Tooltips * m_tooltips;
#endif

    /**
     *  The performance object to which some of these options apply.
     */

    perform * m_mainperf;

    /**
     *  The famous "OK" button's pointer.
     */

    Gtk::Button * m_button_ok;

    /**
     *  Not sure yet what this notebook is for.  Must be a GTK thang.
     */

    Gtk::Notebook * m_notebook;

public:

    options (Gtk::Window & parent, perform * a_p);

private:

    void clock_callback_off (int a_bus, Gtk::RadioButton * a_button);
    void clock_callback_on (int a_bus, Gtk::RadioButton * a_button);
    void clock_callback_mod (int a_bus, Gtk::RadioButton * a_button);
    void clock_mod_callback (Gtk::Adjustment * adj);
    void input_callback (int a_bus, Gtk::Button * a_button);
    void transport_callback (button a_type, Gtk::Button * a_button);
    void mouse_seq24_callback (Gtk::RadioButton *);
    void mouse_fruity_callback (Gtk::RadioButton *);
    void mouse_mod4_callback (Gtk::CheckButton *);

    /* notebook pages */

    void add_midi_clock_page ();
    void add_midi_input_page ();
    void add_keyboard_page ();
    void add_mouse_page ();
    void add_jack_sync_page ();
};

#endif   // SEQ24_OPTIONS_HPP

/*
 * options.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
