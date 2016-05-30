#ifndef SEQ64_OPTIONS_HPP
#define SEQ64_OPTIONS_HPP

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
 * \updates       2016-03-04
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/dialog.h>

namespace Gtk
{
    class Button;
    class CheckButton;
    class Label;
    class Notebook;
    class RadioButton;
    class Table;
    class Tooltips;
}

namespace seq64
{
    class perform;

/**
 *  This class supports a full tabbed options dialog.
 */

class options : public Gtk::Dialog
{

private:

    /**
     *  Defines buttons indices or IDs for some controls related to JACK.
     *  These values are handled in options::transport_callback().  Some of
     *  them set JACK-related values in the rc_settings object, while the
     *  others set up or tear down the JACK support of sequencer64.
     *
     *  The JACK Transport settings are a little messy.  They should be radio
     *  buttons, and control each other's settings.  Currently, if the user
     *  wants to set up for JACK Master, the JACK Transport button must also
     *  be checked.
     *
     * \var e_jack_transport
     *      Turns on the "with JACK Transport" option,
     *      rc_settings::with_jack_transport().
     *
     * \var e_jack_master
     *      Turns on the "with JACK Master" option,
     *      rc_settings::with_jack_master().  If another application is
     *      already JACK Master, this will fail.
     *
     * \var e_jack_master_cond
     *      Turns on the "with JACK Master" option
     *      rc_settings::with_jack_master_cond().  This option makes
     *      sequencer64 the JACK Master conditionally, that is, if no other
     *      application has claimed that role.
     *
     * \var e_jack_start_mode_live
     *      Doesn't directly do anything; the live mode versus song mode is
     *      set by the e_jack_start_mode_song value.
     *
     * \var e_jack_start_mode_song
     *      Sets the "JACK start mode" value to true, which means that
     *      sequencer64 is in song mode.  This value is obtained via
     *      rc_settings::jack_start_mode().
     *
     * \var e_jack_connect
     *      Causes the perform object's JACK initialization function,
     *      perform::init_jack(), to be called.
     *
     * \var e_jack_disconnect
     *      Causes the perform object's JACK deinitialization function,
     *      perform::deinit_jack(), to be called.
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

    Gtk::Tooltips * m_tooltips;

    /**
     *  The performance object to which some of these options apply.
     */

    perform & m_mainperf;

    /**
     *  The famous "OK" button's pointer.
     */

    Gtk::Button * m_button_ok;

    /**
     *  Main JACK transport selection.
     */

    Gtk::CheckButton * m_button_jack_transport;

    /**
     *  Main JACK transport master selection.
     */

    Gtk::CheckButton * m_button_jack_master;

    /**
     *  Main JACK transport master-conditional selection.
     */

    Gtk::CheckButton * m_button_jack_master_cond;

    /**
     *  JACK Connect button, which we need to enable/disable for clarity and
     *  some additional safety.
     */

    Gtk::Button * m_button_jack_connect;

    /**
     *  JACK Disonnect button, which we need to enable/disable for clarity and
     *  some additional safety.
     */

    Gtk::Button * m_button_jack_disconnect;

    /**
     *  Not sure yet what this notebook is for.  Must be a GTK thang.
     */

    Gtk::Notebook * m_notebook;

public:

    options (Gtk::Window & parent, perform & p);

private:

    /**
     * \getter m_mainperf
     */

    perform & perf ()
    {
        return m_mainperf;
    }

    void clock_callback_off (int bus, Gtk::RadioButton * button);
    void clock_callback_on (int bus, Gtk::RadioButton * button);
    void clock_callback_mod (int bus, Gtk::RadioButton * button);
    void clock_mod_callback (Gtk::Adjustment * adj);
    void input_callback (int bus, Gtk::Button * button);
    void transport_callback (button type, Gtk::Button * button);
    void mouse_seq24_callback (Gtk::RadioButton *);
    void mouse_fruity_callback (Gtk::RadioButton *);
    void mouse_mod4_callback (Gtk::CheckButton *);
    void lash_support_callback (Gtk::CheckButton *);

    /* Notebook pages (tabs) */

    void add_midi_clock_page ();
    void add_midi_input_page ();
    void add_keyboard_page ();
    void add_mouse_page ();
    void add_jack_sync_page ();

};          // class options

}           // namespace seq64

#endif      // SEQ64_OPTIONS_HPP

/*
 * options.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

