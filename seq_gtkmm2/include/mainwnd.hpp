#ifndef SEQ64_MAINWND_HPP
#define SEQ64_MAINWND_HPP

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
 * \file          mainwnd.hpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-15
 * \license       GNU GPLv2 or above
 *
 *  The main windows is known as the "Patterns window" or "Patterns
 *  panel".
 */

#include <map>
#include <string>
#include <gtkmm/window.h>

#include "gui_window_gtk2.hpp"
#include "midifile.hpp"
#include "perform.hpp"                 // perform and performcallback

namespace Gtk
{
    class Adjustment;
    class Button;
    class Cursor;
    class Entry;
    class MenuBar;
    class Menu;
    class SpinButton;
    class Tooltips;
}

namespace seq64
{

class maintime;
class mainwid;
class options;
class perfedit;

/**
 *  This class implements the functionality of the main window of the
 *  application, except for the Patterns Panel functionality, which is
 *  implemented in the mainwid class.
 */

class mainwnd : public gui_window_gtk2, public performcallback
{

private:

    /**
     *  Interesting; what is this used for.
     */

    static int m_sigpipe[2];

    Gtk::Tooltips * m_tooltips;

    /**
     *  Theses objects support the menu and its sub-menus.
     */

    Gtk::MenuBar * m_menubar;
    Gtk::Menu * m_menu_file;
    Gtk::Menu * m_menu_view;
    Gtk::Menu * m_menu_help;

    /**
     *  The biggest sub-components of mainwnd.  The first is the Patterns
     *  Panel.
     */

    mainwid * m_main_wid;

    /**
     *  Is this the bar at the top that shows moving squares?
     */

    maintime * m_main_time;

    /**
     *  A pointer to the song/performance editor.
     */

    perfedit * m_perf_edit;

    /**
     *  A pointer to the program options.
     */

    options * m_options;

    /**
     *  Mouse cursor?
     */

    Gdk::Cursor m_main_cursor;

    /**
     *  This button is the learn button, otherwise known as the "L"
     *  button.
     */

    Gtk::Button * m_button_learn;

    /**
     *  Implements the red square stop button.
     */

    Gtk::Button * m_button_stop;

    /**
     *  Implements the green triangle play button.
     */

    Gtk::Button * m_button_play;

    /**
     *  The button for bringing up the Song Editor (Performance Editor).
     */

    Gtk::Button * m_button_perfedit;

    /**
     *  The spin/adjustment controls for the BPM (beats-per-minute) value.
     */

    Gtk::SpinButton * m_spinbutton_bpm;
    Gtk::Adjustment * m_adjust_bpm;

    /**
     *  The spin/adjustment controls for the screen set value.
     */

    Gtk::SpinButton * m_spinbutton_ss;
    Gtk::Adjustment * m_adjust_ss;

    /**
     *  The spin/adjustment controls for the load offset value.
     *  However, where is this button located?  It is handled in the code,
     *  but I've never seen the button!
     */

    Gtk::SpinButton * m_spinbutton_load_offset;
    Gtk::Adjustment * m_adjust_load_offset;

    /**
     *  What is this?
     */

    Gtk::Entry * m_entry_notes;

    /**
     *  Provides a timeout handler.
     */

    sigc::connection m_timeout_connect;

    /**
     *  Saves the PPQN value obtained from the MIDI file (or the default
     *  value, c_ppqn, if SEQ64_USE_DEFAULT_PPQN was specified in reading the
     *  MIDI file.
     */

    int m_ppqn;

public:

    mainwnd (perform & a_p);
    ~mainwnd ();

    void open_file (const std::string &);

    /**
     * \getter m_ppqn
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \setter m_ppqn
     *      We can't set the PPQN value when the mainwnd is created, we have
     *      to do it later, using this function.
     *
     *      m_ppqn = (ppqn == SEQ64_USE_DEFAULT_PPQN) ? global_ppqn : ppqn ;
     */

    void ppqn (int ppqn)
    {
        m_ppqn = ppqn;
    }

private:

    static void handle_signal (int sig);

    void file_import_dialog ();
    void options_dialog ();
    void about_dialog ();

    void adj_callback_ss ();            // make 'em void at some point
    void adj_callback_bpm ();
    void edit_callback_notepad ();
    bool timer_callback ();

    void start_playing ()               // Play!
    {
        perf().start_playing();
    }

    void stop_playing ()                // Stop!
    {
        perf().stop_playing();
        m_main_wid->update_sequences_on_window();
    }

    /**
     *  Toggle the group-learn status.
     */

    void learn_toggle ()
    {
        perf().learn_toggle();
    }

    void open_performance_edit ();

    /**
     *  Use the sequence key to toggle the playing of an active pattern in
     *  the current screen-set.
     */

    void sequence_key (int seq)
    {
        perf().sequence_key(seq);
    }

    void update_window_title ();
    void toLower (std::string &);       // isn't this part of std::string?
    void file_new ();
    void file_open ();
    void file_save ();
    void file_save_as ();
    void file_exit ();
    void new_file ();
    bool save_file ();
    void choose_file ();
    int query_save_changes ();
    bool is_save ();
    bool install_signal_handlers ();
    bool signal_action (Glib::IOCondition condition);

private:

    bool on_delete_event (GdkEventAny * a_e);
    bool on_key_press_event (GdkEventKey * a_ev);
    bool on_key_release_event (GdkEventKey * a_ev);

private:

    /**
     *  Notification handler for learn mode toggle.
     */

    virtual void on_grouplearnchange (bool state);

};

}           // namespace seq64

#endif      // SEQ64_MAINWND_HPP

/*
 * mainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
