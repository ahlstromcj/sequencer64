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
 * \updates       2016-09-10
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <map>
#include <string>
#include <gdkmm/cursor.h>
#include <gtkmm/window.h>

#include "seq64_features.h"             /* feature macros for the app   */
#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN       */
#include "gui_window_gtk2.hpp"          /* seq64::qui_window_gtk2       */
#include "perform.hpp"                  // seq64::perform and callback  */

/*
 *  Easier access to Gtk-2 classes.
 */

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

#if defined SEQ64_STAZED_SONG_MODE_BUTTON || defined USE_STAZED_MENU_MODE_BUTTON
    class ToggleButton;
#endif
}

/*
 *  The main namespace for the Sequencer64 libraries.
 */

namespace seq64
{
    class maintime;
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
     *  This small array holds the "handles" for the pipes need to intercept
     *  the system signals SIGINT and SIGUSR1, so that the application shuts
     *  down gracefully when aborted.
     */

    static int m_sigpipe[2];

    /**
     *  A repository for tooltips.
     */

    Gtk::Tooltips * m_tooltips;

    /**
     *  Theses objects support the menu and its sub-menus.
     */

    Gtk::MenuBar * m_menubar;           /**< The whole menu bar.        */
    Gtk::Menu * m_menu_file;            /**< The File menu entry.       */
    Gtk::Menu * m_menu_edit;            /**< The (new) Edit menu entry. */
    Gtk::Menu * m_menu_view;            /**< The View menu entry.       */
    Gtk::Menu * m_menu_help;            /**< The Help menu entry.       */

    /**
     *  Saves the PPQN value obtained from the MIDI file (or the default
     *  value, the global ppqn, if SEQ64_USE_DEFAULT_PPQN was specified in
     *  reading the MIDI file.  We need it early here to be able to pass it
     *  along to child objects.
     */

    int m_ppqn;

    /**
     *  The biggest sub-components of mainwnd.  The first is the Patterns
     *  Panel, which the mainwid helps implement.  We end up sharing this
     *  object with perfedit, perfnames, and seqedit in order to allow the
     *  seqedit object to notify the mainwid (indirectly) of the
     *  currently-edited sequence.
     */

    mainwid * m_main_wid;

    /**
     *  Is this the bar at the top that shows moving squares, also known as
     *  "pills"?  Why yes, it is.
     */

    maintime * m_main_time;

    /**
     *  A pointer to the first song/performance editor.
     */

    perfedit * m_perf_edit;

    /**
     *  A pointer to an optional second song/performance editor.  The second
     *  makes it easy to line up two different patterns that cannot be seen
     *  together on one performance editor.
     */

    perfedit * m_perf_edit_2;

    /**
     *  A pointer to the program options.
     */

    options * m_options;

    /**
     *  Mouse cursor?
     */

    Gdk::Cursor m_main_cursor;

    /**
     *  Provides a pointer to hold the images for the pause/play button.
     */

    Gtk::Image * m_image_play;

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
     *  Implements the green triangle play button.  If configured to support
     *  pause, it also supports the pause pixmap and functionality.
     */

    Gtk::Button * m_button_play;

    /**
     *  The button for bringing up the Song Editor (Performance Editor).
     */

    Gtk::Button * m_button_perfedit;

#ifdef SEQ64_STAZED_SONG_MODE_BUTTON
    Gtk::ToggleButton * m_button_mode;
#endif

#ifdef USE_STAZED_MENU_MODE_BUTTON
    Gtk::ToggleButton * m_button_menu;
#endif

    /**
     *  The spin/adjustment controls for the BPM (beats-per-minute) value.
     */

    Gtk::Adjustment * m_adjust_bpm;         /**< BPM adjustment object.     */
    Gtk::SpinButton * m_spinbutton_bpm;     /**< BPM spin-button object.    */

    /**
     *  The spin/adjustment controls for the screenset value.
     */

    Gtk::Adjustment * m_adjust_ss;          /**< Screenset adjustment.      */
    Gtk::SpinButton * m_spinbutton_ss;      /**< Screenset adjustment.      */

    /**
     *  The spin/adjustment controls for the load offset value.
     *  These controls are used in the File / Import dialog to change where
     *  the imported file will be loaded in the sequences space, which ranges
     *  from 0 to 1024 in blocks of 32 patterns.
     */

    Gtk::Adjustment * m_adjust_load_offset;     /**< Load number for import.    */
    Gtk::SpinButton * m_spinbutton_load_offset; /**< Spin button for import.    */

    /**
     *  This item provides user-interface access to the screenset notepad
     *  editor.  This is just a long text-edit field that can be used to enter
     *  a long name or a short description of the current screenset.
     */

    Gtk::Entry * m_entry_notes;

#ifdef SEQ64_PAUSE_SUPPORT

    /**
     *  Holds the current status of running, for use in display the play
     *  versus pause icon.
     */

    bool m_is_running;

#endif

    /**
     *  Provides a timeout handler.
     */

    sigc::connection m_timeout_connect;

    /**
     *  Indicates if the menu bar is to be greyed out or not.  This is a
     *  "stazed" feature that might be generally useful.
     */

    bool m_menu_mode;

    /**
     *  Indicates that this object is in a mode where the usual mute/unmute
     *  keystroke will instead bring up the pattern slot for editing.
     *  Currently, the hard-wired key for this function is the equals key.
     */

    bool m_call_seq_edit;

    /**
     *  Indicates that this object is in a mode where the usual mute/unmute
     *  keystroke will instead bring up the pattern slot for event-editing.
     *  Currently, the hard-wired key for this function is the minus key.
     */

    bool m_call_seq_eventedit;

public:

    mainwnd
    (
        perform & p,
        bool allowperf2 = true,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );
    virtual ~mainwnd ();

    void open_file (const std::string & filename);

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
     *      m_ppqn = choose_ppqn(ppqn);
     */

    void ppqn (int ppqn)
    {
        m_ppqn = ppqn;
    }

private:

    static void handle_signal (int sig);

    void adj_callback_ss ();            // make 'em void at some point
    void adj_callback_bpm ();
    void edit_callback_notepad ();
    bool timer_callback ();
    void set_image (bool isrunning);
    void start_playing ();
    void pause_playing ();
    void stop_playing ();
    void toggle_playing ();

    /**
     *  Toggle the group-learn status.  Simply forwards the call to
     *  perform::learn_toggle().
     */

    void learn_toggle ()
    {
        perf().learn_toggle();
    }

    void open_performance_edit ();
    void open_performance_edit_2 ();
    void enregister_perfedits ();

    /**
     *  Use the sequence key to toggle the playing of an active pattern in
     *  the current screen-set.
     */

    void sequence_key (int seq)
    {
        perf().sequence_key(seq);
    }

#ifdef SEQ64_STAZED_TRANSPOSE
    void apply_song_transpose ();
#endif

#ifdef SEQ64_STAZED_SONG_MODE_BUTTON
    void set_song_mode ();
    void toggle_song_mode();
#endif

#ifdef USE_STAZED_MENU_MODE_BUTTON
    void set_menu_mode ();
    void toggle_menu_mode ();
#endif

    void update_window_title ();
    void toLower (std::string &);       // isn't this part of std::string?

    /**
     *  A callback function for the File / New menu entry.
     */

    void file_new ()
    {
        if (is_save())
            new_file();
    }

    /**
     *  A callback function for the File / Open menu entry.
     */

    void file_open ()
    {
        if (is_save())
            choose_file();
    }

    /**
     *  A callback function for the File / Save menu entry.
     */

    void file_save ()
    {
        save_file();
    }

    /**
     *  Sets the song-mute mode.
     */

    void set_song_mute (perform::mute_op_t op)
    {
        perf().set_song_mute(op);       // and modifies
    }

private:

    void file_import_dialog ();
    void options_dialog ();
    void about_dialog ();
    int query_save_changes ();
    void new_open_error_dialog ();

    void file_save_as (bool do_export = false);
    void file_exit ();
    void new_file ();
    bool save_file ();
    void choose_file ();
    bool is_save ();
    bool install_signal_handlers ();
    bool signal_action (Glib::IOCondition condition);

private:

    bool on_delete_event (GdkEventAny * ev);
    bool on_key_press_event (GdkEventKey * ev);
    bool on_key_release_event (GdkEventKey * ev);

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
