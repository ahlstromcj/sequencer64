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
 * \file          mainwnd.cpp
 *
 *  This module declares/defines the base class for the main window of the
 *  application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-17
 * \license       GNU GPLv2 or above
 *
 *  The main window holds the menu and the main controls of the application,
 *  and the mainwid that holds the patterns is nestled in the interior of the
 *  main window.
 *
 * \todo
 *      -   Figure out best way to select non-legacy PPQN behavior, probably,
 *          for now, a command-line option.
 *      -   Add a GUI element that shows the actual PPQN in force, maybe next
 *          to the maintime object, or in the title caption.
 */

#include <cctype>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <stdio.h>                      /* snprintf()                   */
#include <gdkmm/cursor.h>
#include <gtk/gtkversion.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menu.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/stock.h>
#include <gtkmm/tooltips.h>

#include "globals.h"
#include "gtk_helpers.h"
#include "keys_perform.hpp"             /* \new ca 2015-09-16           */
#include "keystroke.hpp"
#include "maintime.hpp"
#include "mainwid.hpp"
#include "mainwnd.hpp"
#include "options.hpp"
#include "perfedit.hpp"
#include "pixmaps/play2.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/learn.xpm"
#include "pixmaps/learn2.xpm"
#include "pixmaps/perfedit.xpm"
#include "pixmaps/seq24_32.xpm"
#include "pixmaps/sequencer64_square.xpm"
#include "pixmaps/sequencer64_legacy.xpm"

using namespace Gtk::Menu_Helpers;      /* MenuElem, etc.                */

namespace seq64
{

/**
 *  This static member provides a couple of pipes for signalling/messaging.
 */

int mainwnd::m_sigpipe[2];

/**
 *  The constructor the main window of the application.
 *  This constructor is way too large; it would be nicer to provide a
 *  number of well-named initialization functions.
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \todo
 *      Offload most of the work into an initialization function like
 *      options does; make the perform parameter a reference;
 *      valgrind flags m_tooltips as lost data, but if we try to manage it
 *      ourselves, many more leaks occur.
 */

mainwnd::mainwnd (perform & p)
 :
    gui_window_gtk2         (p),
    performcallback         (),
    m_tooltips              (manage(new Gtk::Tooltips())),  // valgrind complains!
    m_menubar               (manage(new Gtk::MenuBar())),
    m_menu_file             (manage(new Gtk::Menu())),
    m_menu_view             (manage(new Gtk::Menu())),
    m_menu_help             (manage(new Gtk::Menu())),
    m_main_wid              (manage(new mainwid(perf()))),  // p
    m_main_time             (manage(new maintime(perf()))), // p
    m_perf_edit             (new perfedit(perf())),         // p
    m_options               (nullptr),
    m_main_cursor           (),
    m_button_learn          (nullptr),
    m_button_stop           (nullptr),
    m_button_play           (nullptr),
    m_button_perfedit       (nullptr),
    m_spinbutton_bpm        (nullptr),
    m_adjust_bpm            (nullptr),
    m_spinbutton_ss         (nullptr),
    m_adjust_ss             (nullptr),
    m_spinbutton_load_offset(nullptr),
    m_adjust_load_offset    (nullptr),
    m_entry_notes           (nullptr),
    m_timeout_connect       (),                         // handler
    m_ppqn                  (SEQ64_USE_DEFAULT_PPQN)
{
    set_icon(Gdk::Pixbuf::create_from_xpm_data(seq24_32_xpm));

    /*
     * This request always leaves the bottom panel partly obscured.
     *
     *      set_size_request(794, 350);
     */

    set_resizable(false);
    perf().enregister(this);                        // register for notification
    update_window_title();                          // main window
    m_menubar->items().push_front(MenuElem("_File", *m_menu_file));
    m_menubar->items().push_back(MenuElem("_View", *m_menu_view));
    m_menubar->items().push_back(MenuElem("_Help", *m_menu_help));

    /**
     * File menu items, their accelerator keys, and their hot keys.
     */

    m_menu_file->items().push_back
    (
        MenuElem
        (
            "_New", Gtk::AccelKey("<control>N"),
            mem_fun(*this, &mainwnd::file_new)
        )
    );
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "_Open...", Gtk::AccelKey("<control>O"),
            mem_fun(*this, &mainwnd::file_open)
        )
    );
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "_Save", Gtk::AccelKey("<control>S"),
            mem_fun(*this, &mainwnd::file_save)
        )
    );
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "Save _as...", mem_fun(*this, &mainwnd::file_save_as)
        )
    );
    m_menu_file->items().push_back(SeparatorElem());
    m_menu_file->items().push_back
    (
        MenuElem("_Import...", mem_fun(*this, &mainwnd::file_import_dialog))
    );
    m_menu_file->items().push_back
    (
        MenuElem("O_ptions...", mem_fun(*this, &mainwnd::options_dialog))
    );
    m_menu_file->items().push_back(SeparatorElem());
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "E_xit", Gtk::AccelKey("<control>Q"),
            mem_fun(*this, &mainwnd::file_exit)
        )
    );

    /**
     * View menu items and their hot keys.
     */

    m_menu_view->items().push_back
    (
        MenuElem
        (
            "_Song Editor...", Gtk::AccelKey("<control>E"),
            mem_fun(*this, &mainwnd::open_performance_edit)
        )
    );

    /**
     * Help menu items
     */

    m_menu_help->items().push_back
    (
        MenuElem("_About...", mem_fun(*this, &mainwnd::about_dialog))
    );

    /**
     * Top panel items, including the logo (updated for the new version of
     * this application) and the "timeline" progress bar.
     */

    Gtk::HBox * tophbox = manage(new Gtk::HBox(false, 0));
    const char ** bitmap = g_rc_settings.legacy_format() ?
        sequencer64_legacy_xpm : sequencer64_square_xpm ;

    tophbox->pack_start
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(bitmap))),
        false, false
    );

    /* Adjust placement of the logo. */

    Gtk::VBox * vbox_b = manage(new Gtk::VBox());
    Gtk::HBox * hbox3 = manage(new Gtk::HBox(false, 0));
    vbox_b->pack_start(*hbox3, false, false);
    tophbox->pack_end(*vbox_b, false, false);
    hbox3->set_spacing(10);
    hbox3->pack_start(*m_main_time, false, false);  /* timeline          */
    m_button_learn = manage(new Gtk::Button());     /* group learn ("L") */
    m_button_learn->set_focus_on_click(false);
    m_button_learn->set_flags(m_button_learn->get_flags() & ~Gtk::CAN_FOCUS);
    m_button_learn->set_image
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(learn_xpm)))
    );
    m_button_learn->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::learn_toggle)
    );
    add_tooltip
    (
        m_button_learn,
        "Mute Group Learn: "
        "Click the 'L' button, then press a mute-group key to store "
        "the mute state of the sequences in that key. "
        "See File/Options/Keyboard for available mute-group keys "
        "and the corresponding hotkey for the 'L' button."
    );
    hbox3->pack_end(*m_button_learn, false, false);

    /*
     *  This seems to be a dirty hack to clear the focus, not to trigger L
     *  via keys.
     */

    Gtk::Button w;
    hbox3->set_focus_child(w);

    /*
     *  Bottom panel items.  The first ones are the start/stop buttons and
     *  the container that groups them.
     */

    Gtk::HBox * bottomhbox = manage(new Gtk::HBox(false, 10));   // bottom box
    Gtk::HBox * startstophbox = manage(new Gtk::HBox(false, 4)); // button box
    bottomhbox->pack_start(*startstophbox, Gtk::PACK_SHRINK);
    m_button_stop = manage(new Gtk::Button());                   // stop button
    m_button_stop->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(stop_xpm)))
    );
    m_button_stop->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::stop_playing)
    );
    add_tooltip(m_button_stop, "Stop playing the MIDI sequence");
    startstophbox->pack_start(*m_button_stop, Gtk::PACK_SHRINK);
    m_button_play = manage(new Gtk::Button());                  // play button
    m_button_play->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(play2_xpm)))
    );
    m_button_play->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::start_playing)
    );
    add_tooltip(m_button_play, "Play the MIDI sequence");
    startstophbox->pack_start(*m_button_play, Gtk::PACK_SHRINK);

    /*
     * BPM spin button with label.
     */

    Gtk::HBox * bpmhbox = manage(new Gtk::HBox(false, 4));
    bottomhbox->pack_start(*bpmhbox, Gtk::PACK_SHRINK);
    m_adjust_bpm = manage(new Gtk::Adjustment(perf().get_bpm(), 20, 500, 1));
    m_spinbutton_bpm = manage(new Gtk::SpinButton(*m_adjust_bpm));
    m_spinbutton_bpm->set_editable(false);
    m_adjust_bpm->signal_value_changed().connect
    (
        mem_fun(*this, &mainwnd::adj_callback_bpm)
    );
    add_tooltip(m_spinbutton_bpm, "Adjust beats per minute (BPM) value");
    Gtk::Label * bpmlabel = manage(new Gtk::Label("_BPM", true));
    bpmlabel->set_mnemonic_widget(*m_spinbutton_bpm);
    bpmhbox->pack_start(*bpmlabel, Gtk::PACK_SHRINK);
    bpmhbox->pack_start(*m_spinbutton_bpm, Gtk::PACK_SHRINK);

    /*
     * Screen set name edit line.
     */

    Gtk::HBox * notebox = manage(new Gtk::HBox(false, 4));
    bottomhbox->pack_start(*notebox, Gtk::PACK_EXPAND_WIDGET);
    m_entry_notes = manage(new Gtk::Entry());
    m_entry_notes->signal_changed().connect
    (
        mem_fun(*this, &mainwnd::edit_callback_notepad)
    );
    m_entry_notes->set_text(perf().current_screen_set_notepad());
    add_tooltip
    (
        m_entry_notes,
        "Enter screen-set name.  A screen-set is one page of "
        "up to 32 patterns that can be see and manipulated in "
        "the Patterns window."
    );

    Gtk::Label * notelabel = manage(new Gtk::Label("_Name", true));
    notelabel->set_mnemonic_widget(*m_entry_notes);
    notebox->pack_start(*notelabel, Gtk::PACK_SHRINK);
    notebox->pack_start(*m_entry_notes, Gtk::PACK_EXPAND_WIDGET);

    /*
     * Sequence screen-set spin button.
     */

    Gtk::HBox * sethbox = manage(new Gtk::HBox(false, 4));
    bottomhbox->pack_start(*sethbox, Gtk::PACK_SHRINK);
    m_adjust_ss = manage(new Gtk::Adjustment(0, 0, c_max_sets - 1, 1));
    m_spinbutton_ss = manage(new Gtk::SpinButton(*m_adjust_ss));
    m_spinbutton_ss->set_editable(false);
    m_spinbutton_ss->set_wrap(true);
    m_adjust_ss->signal_value_changed().connect
    (
        mem_fun(*this, &mainwnd::adj_callback_ss)
    );
    add_tooltip(m_spinbutton_ss, "Select screen-set from one of 32 sets.");
    Gtk::Label * setlabel = manage(new Gtk::Label("_Set", true));
    setlabel->set_mnemonic_widget(*m_spinbutton_ss);
    sethbox->pack_start(*setlabel, Gtk::PACK_SHRINK);
    sethbox->pack_start(*m_spinbutton_ss, Gtk::PACK_SHRINK);

    /*
     * Song editor button.
     */

    m_button_perfedit = manage(new Gtk::Button());
    m_button_perfedit->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm)))
    );
    m_button_perfedit->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::open_performance_edit)
    );
    add_tooltip(m_button_perfedit, "Show or hide song editor window");
    bottomhbox->pack_end(*m_button_perfedit, Gtk::PACK_SHRINK);

    /*
     * Vertical layout container for window content.  Letting Gtk manage it
     * does not improve leaks:
     *
     * Gtk::VBox * contentvbox = manage(new Gtk::VBox());
     */

    Gtk::VBox * contentvbox = new Gtk::VBox();
    contentvbox->set_spacing(10);
    contentvbox->set_border_width(10);
    contentvbox->pack_start(*tophbox, Gtk::PACK_SHRINK);
    contentvbox->pack_start(*m_main_wid, Gtk::PACK_SHRINK);
    contentvbox->pack_start(*bottomhbox, Gtk::PACK_SHRINK);

    /*
     * Main container for menu and window content.
     */

    Gtk::VBox * mainvbox = new Gtk::VBox();
    mainvbox->pack_start(*m_menubar, false, false);
    mainvbox->pack_start(*contentvbox);
    add(*mainvbox);                         // add main layout box (this->)
    show_all();                             // show everything
    add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
    m_timeout_connect = Glib::signal_timeout().connect
    (
        mem_fun(*this, &mainwnd::timer_callback), 25
    );
    m_sigpipe[0] = -1;                      // initialize static array
    m_sigpipe[1] = -1;
    install_signal_handlers();

    /*
     * Have to be careful here, otherwise changes are lost or we get
     * prompted even when nothing is done.
     *
     * m_perf_edit->is_modified(false);     // is it even set at this point?
     */
}

/**
 *  This destructor must explicitly delete some allocated resources.
 */

mainwnd::~mainwnd ()
{
    if (not_nullptr(m_perf_edit))
        delete m_perf_edit;

    if (not_nullptr(m_options))
        delete m_options;

    if (m_sigpipe[0] != -1)
        close(m_sigpipe[0]);

    if (m_sigpipe[1] != -1)
        close(m_sigpipe[1]);
}

/**
 *  This function is the GTK timer callback, used to draw our current time
 *  and BPM on_events (the main window).
 *
 * \note
 *      When Sequencer64 first starts up, and no MIDI tune is loaded, the call
 *      to mainwid::update_markers() leads to trying to do some work on
 *      sequences that don't yet exist.
 */

bool
mainwnd::timer_callback ()
{
    long ticks = perf().get_tick();
    m_main_time->idle_progress(ticks);
    m_main_wid->update_markers(ticks);          /* see note above */

    int bpm = perf().get_bpm();
    if (m_adjust_bpm->get_value() != bpm)
        m_adjust_bpm->set_value(bpm);

    int screenset = perf().get_screenset();
    if (m_adjust_ss->get_value() != screenset)
    {
        m_main_wid->set_screenset(screenset);
        m_adjust_ss->set_value(screenset);
        m_entry_notes->set_text(perf().current_screen_set_notepad());
    }
    return true;
}

/**
 *  Opens the Performance Editor (Song Editor).
 *
 * \todo
 *      Try to find a way to set m_modified only if the song editor
 *      actually changes something, instead of just because it was opened.
 */

void
mainwnd::open_performance_edit ()
{
    if (m_perf_edit->is_visible())
        m_perf_edit->hide();
    else
    {
        m_perf_edit->init_before_show();
        m_perf_edit->show_all();
        is_modified(true);              // STILL ENABLED
    }
}

/**
 *  Opens the File / Options dialog.
 */

void
mainwnd::options_dialog ()
{
    if (not_nullptr(m_options))
        delete m_options;

    m_options = new options(*this, perf());
    m_options->show_all();
}

/**
 *  This handler responds to a learn-mode change from perf().
 */

void
mainwnd::on_grouplearnchange (bool state)
{
    const char ** bitmap = state ? learn2_xpm : learn_xpm ;
    m_button_learn->set_image
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(bitmap)))
    );
}

/**
 *  A callback function for the File / New menu entry.
 */

void
mainwnd::file_new ()
{
    if (is_save())
        new_file();
}

/**
 *  Actually does the work of setting up for a new file.
 */

void
mainwnd::new_file ()
{
    perf().clear_all();
    m_main_wid->reset();
    m_entry_notes->set_text(perf().current_screen_set_notepad());
    g_rc_settings.filename("");
    update_window_title();
    is_modified(false);
}

/**
 *  A callback function for the File / Save menu entry.
 */

void
mainwnd::file_save ()
{
    save_file();
}

/**
 *  A callback function for the File / Save As menu entry.
 */

void
mainwnd::file_save_as ()
{
    Gtk::FileChooserDialog dialog("Save file as", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);
    dialog.set_current_folder(g_rc_settings.last_used_dir());
    int response = dialog.run();
    switch (response)
    {
        case Gtk::RESPONSE_OK:
        {
            std::string fname = dialog.get_filename();
            Gtk::FileFilter * current_filter = dialog.get_filter();
            if
            (
                (current_filter != NULL) &&
                (current_filter->get_name() == "MIDI files")
            )
            {
                /*
                 * Check for MIDI file extension; if missing, add ".midi".
                 */

                std::string suffix = fname.substr
                (
                    fname.find_last_of(".") + 1, std::string::npos
                );
                toLower(suffix);
                if ((suffix != "midi") && (suffix != "mid"))
                    fname += ".midi";
            }

            if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS))
            {
                Gtk::MessageDialog warning
                (
                    *this,
                   "File already exists!\nDo you want to overwrite it?",
                   false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true
                );
                response = warning.run();
                if (response == Gtk::RESPONSE_NO)
                    return;
            }
            g_rc_settings.filename(fname);
            update_window_title();
            save_file();
            break;
        }
        default:
            break;
    }
}

/**
 *  Opens and parses (reads) a MIDI file.
 *
 *  We leave the ppqn parameter set to the SEQ64_USE_DEFAULT for now, to
 *  preserve the legacy behavior of using c_ppqn, and scaling the running time
 *  against the PPQN read from the MIDI file.  Later, we can provide a value
 *  like 0, that will certainly be changed by reading the MIDI file.
 *
 *  We don't need to specify the "propformat" parameter of the midifile
 *  constructor when reading the MIDI file, since reading handles both the old
 *  and new formats.
 *
 * \param fn
 *      Provides the file-name for the MIDI file to be opened.
 */

void
mainwnd::open_file (const std::string & fn)
{
    bool result;
    midifile f(fn);                     /* create object to represent file  */
    perf().clear_all();
    result = f.parse(perf(), 0);        /* parsing handles old & new format */
    is_modified(! result);
    if (result)
    {
        ppqn(f.ppqn());                 /* get and save the actual PPQN     */
    }
    else
    {
        Gtk::MessageDialog errdialog
        (
            *this,
            "Error reading file: " + fn, false,
            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
        );
        errdialog.run();
        return;
    }

    g_rc_settings.last_used_dir(fn.substr(0, fn.rfind("/") + 1));
    g_rc_settings.filename(fn);
    update_window_title();
    m_main_wid->reset();
    m_entry_notes->set_text(perf().current_screen_set_notepad());
    m_adjust_bpm->set_value(perf().get_bpm());
}

/**
 *  A callback function for the File / Open menu entry.
 */

void
mainwnd::file_open ()
{
    if (is_save())
        choose_file();
}

/**
 *  Creates a file-chooser dialog.
 */

void
mainwnd::choose_file ()
{
    Gtk::FileChooserDialog dlg("Open MIDI file", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(*this);
    dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dlg.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dlg.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dlg.add_filter(filter_any);
    dlg.set_current_folder(g_rc_settings.last_used_dir());

    int result = dlg.run();
    switch (result)
    {
    case (Gtk::RESPONSE_OK):
        open_file(dlg.get_filename());
        break;

    default:
        break;
    }
}

/**
 *  Saves the current state in a MIDI file.
 *
 *  Here we specify the current value of m_ppqn, which was set when reading
 *  the MIDI file.
 */

bool
mainwnd::save_file ()
{
    bool result = false;
    if (g_rc_settings.filename().empty())
    {
        file_save_as();
        return true;
    }

    midifile f(g_rc_settings.filename(), ppqn(), ! g_rc_settings.legacy_format());
    result = f.write(perf());
    if (! result)
    {
        Gtk::MessageDialog errdialog
        (
            *this, "Error writing file.", false,
            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
        );
        errdialog.run();
    }
    is_modified(! result);
    return result;
}

/**
 *  Queries the user to save the changes made while the application was
 *  running.
 */

int
mainwnd::query_save_changes ()
{
    std::string query_str;
    if (g_rc_settings.filename().empty())
        query_str = "Unnamed file was changed.\nSave changes?";
    else
        query_str = "File '" + g_rc_settings.filename() +
            "' was changed.\nSave changes?";

    Gtk::MessageDialog dialog
    (
        *this, query_str, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true
    );
    dialog.add_button(Gtk::Stock::YES, Gtk::RESPONSE_YES);
    dialog.add_button(Gtk::Stock::NO, Gtk::RESPONSE_NO);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    return dialog.run();
}

/**
 *  If the data is modified, then the user is queried, and the file is
 *  save if okayed.
 */

bool
mainwnd::is_save ()
{
    bool result = false;
    if (is_modified() || m_perf_edit->is_modified() || m_main_wid->is_modified())
    {
        int choice = query_save_changes();
        switch (choice)
        {
        case Gtk::RESPONSE_YES:
            if (save_file())
                result = true;
            break;

        case Gtk::RESPONSE_NO:
            result = true;
            break;

        case Gtk::RESPONSE_CANCEL:
        default:
            break;
        }
    }
    else
        result = true;

    return result;
}

/**
 *  Converts a string to lower-case letters.
 */

void
mainwnd::toLower (std::string & s)
{
    for (std::string::iterator p = s.begin(); p != s.end(); p++)
        *p = tolower(*p);
}

/**
 *  Presents a file dialog to import a MIDI file.  Note that every track of
 *  the MIDI file will be imported, even if the track is only a label
 *  track (without any MIDI events), or a very long track.
 *
 *  The main difference between the Open operation and the Import operation
 *  seems to be that the latter can read MIDI files into a screen-set greater
 *  than screen-set 0.  No, that's not true, so far.  No matter what the
 *  current screen-set setting, the import is appended after the current data
 *  in screen-set 0.  Then, if it overflows that screen-set, the overflow goes
 *  into the next screen-set.
 *
 *  It might be nice to have the option of importing a MIDI file into a
 *  specific screen-set, for better organization.  Set versus append.
 *
 * \todo
 *      We need to look into the Import process and document it better.
 */

void
mainwnd::file_import_dialog ()
{
    Gtk::FileChooserDialog dlg("Import MIDI file", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(*this);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dlg.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dlg.add_filter(filter_any);
    dlg.set_current_folder(g_rc_settings.last_used_dir());

    Gtk::ButtonBox * btnbox = dlg.get_action_area();
    Gtk::HBox hbox(false, 2);
    m_adjust_load_offset = manage
    (
        new Gtk::Adjustment(0, -(c_max_sets - 1), c_max_sets - 1, 1)
    );
    m_spinbutton_load_offset = manage(new Gtk::SpinButton(*m_adjust_load_offset));
    m_spinbutton_load_offset->set_editable(false);
    m_spinbutton_load_offset->set_wrap(true);
    hbox.pack_end(*m_spinbutton_load_offset, false, false);
    hbox.pack_end
    (
        *(manage(new Gtk::Label("Screen Set Offset"))), false, false, 4
    );
    btnbox->pack_start(hbox, false, false);
    dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dlg.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
    dlg.show_all_children();

    int response = dlg.run();
    switch (response)                  // handle the response
    {
    case (Gtk::RESPONSE_OK):
    {
        try
        {
            midifile f(dlg.get_filename());
            f.parse(perf(), int(m_adjust_load_offset->get_value()));
        }
        catch (...)
        {
            Gtk::MessageDialog errdialog
            (
                *this, "Error reading file.", false,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
            );
            errdialog.run();
        }
        g_rc_settings.filename(std::string(dlg.get_filename()));
        update_window_title();
        is_modified(true);
        m_main_wid->reset();
        m_entry_notes->set_text(perf().current_screen_set_notepad());
        m_adjust_bpm->set_value(perf().get_bpm());
        break;
    }

    case (Gtk::RESPONSE_CANCEL):
        break;

    default:
        break;
    }
}

/**
 *  A callback function for the File / Exit menu entry.
 */

void
mainwnd::file_exit ()
{
    if (is_save())
    {
        if (g_rc_settings.is_pattern_playing())
            stop_playing();

        hide();
    }
}

/**
 *  Presents a Help / About dialog.  I (Chris) took the liberty of tacking
 *  my name at the end, and hope to eventually have done enough work to
 *  warrant having it there.
 */

void
mainwnd::about_dialog ()
{
    Gtk::AboutDialog dialog;
    dialog.set_transient_for(*this);
    dialog.set_name(SEQ64_PACKAGE_NAME);
    dialog.set_version(SEQ64_VERSION " " SEQ64_VERSION_DATE_SHORT);
    dialog.set_comments("Interactive MIDI Sequencer\n");
    dialog.set_copyright
    (
        "(C) 2002 - 2006 Rob C. Buse (seq24)\n"
        "(C) 2008 - 2010 Seq24team (seq24)\n"
        "(C) 2015 Chris Ahlstrom (sequencer64, sequencer24)"
    );
    dialog.set_website
    (
        "http://www.filter24.org/seq24\n"
        "http://edge.launchpad.net/seq24\n"
        "https://github.com/ahlstromcj/sequencer64.git"
    );

    std::list<std::string> list_authors;
    list_authors.push_back("Rob C. Buse <rcb@filter24.org>");
    list_authors.push_back("Ivan Hernandez <ihernandez@kiusys.com>");
    list_authors.push_back("Guido Scholz <guido.scholz@bayernline.de>");
    list_authors.push_back("Jaakko Sipari <jaakko.sipari@gmail.com>");
    list_authors.push_back("Peter Leigh <pete.leigh@gmail.com>");
    list_authors.push_back("Anthony Green <green@redhat.com>");
    list_authors.push_back("Daniel Ellis <mail@danellis.co.uk>");
    list_authors.push_back("Sebastien Alaiwan <sebastien.alaiwan@gmail.com>");
    list_authors.push_back("Kevin Meinert <kevin@subatomicglue.com>");
    list_authors.push_back("Andrea delle Canne <andreadellecanne@gmail.com>");
    list_authors.push_back("Chris Ahlstrom<ahlstromcj@gmail.com>");
    dialog.set_authors(list_authors);

    std::list<std::string> list_documenters;
    list_documenters.push_back("Dana Olson <seq24@ubuntustudio.com>");
    list_documenters.push_back("Chris Ahlstrom<ahlstromcj@gmail.com>");
    list_documenters.push_back
    (
        "See <https://github.com/ahlstromcj/seq24-doc.git>"
    );
    list_documenters.push_back
    (
        "and <https://github.com/ahlstromcj/sequencer24-doc.git>"
    );
    dialog.set_documenters(list_documenters);
    dialog.show_all_children();
    dialog.run();
}

/**
 *  This function is the callback for adjusting the screen-set value.
 *
 *  Sets the screen-set value in the Performance/Song window, the
 *  Patterns, and something about setting the text based on a screen-set
 *  notepad from the Performance/Song window.
 *
 *  Screen-set notepad?
 */

void
mainwnd::adj_callback_ss ()
{
    perf().set_screenset(int(m_adjust_ss->get_value()));
    m_main_wid->set_screenset(perf().get_screenset());
    m_entry_notes->set_text(perf().current_screen_set_notepad());
    is_modified(true);
}

/**
 *  This function is the callback for adjusting the BPM value.
 */

void
mainwnd::adj_callback_bpm ()
{
    perf().set_bpm(int(m_adjust_bpm->get_value()));
    is_modified(true);
}

/**
 *  A callback function for handling an edit to the screen-set notepad.
 */

void
mainwnd::edit_callback_notepad ()
{
    const std::string & text = m_entry_notes->get_text();
    perf().set_current_screen_set_notepad(text);
    is_modified(true);
}

/**
 *  This callback function handles a delete event from ...?
 *
 *  Any changed data is saved.  If the pattern is playing, then it is
 *  stopped.
 */

bool
mainwnd::on_delete_event (GdkEventAny * a_e)
{
    bool result = is_save();
    if (result && g_rc_settings.is_pattern_playing())
        stop_playing();

    return ! result;
}

/**
 *  Handles a key release event.  Is this worth turning into a switch
 *  statement?  Or offloading to a perform member function?  The latter.
 *
 * \todo
 *      Test this functionality in old and new application.
 *
 * \return
 *      Always returns false.
 */

bool
mainwnd::on_key_release_event (GdkEventKey * a_ev)
{
    keystroke k(a_ev->keyval, SEQ64_KEYSTROKE_RELEASE);
    (void) perf().mainwnd_key_event(k);
    return false;
}

/**
 *  Handles a key press event.  It also handles the control-key and
 *  modifier-key combinations matching the entries in its list of if
 *  statements.
 *
 * \todo
 *      Test this functionality in old and new application.
 */

bool
mainwnd::on_key_press_event (GdkEventKey * a_ev)
{
    Gtk::Window::on_key_press_event(a_ev);
    if (a_ev->type == GDK_KEY_PRESS)
    {
        if (g_rc_settings.print_keys())
        {
            printf("key_press[%d]\n", a_ev->keyval);
            fflush(stdout);
        }

        if (a_ev->keyval == PREFKEY(bpm_dn))
        {
            int newbpm = perf().decrement_bpm();
            m_adjust_bpm->set_value(newbpm);
        }
        else if (a_ev->keyval == PREFKEY(bpm_up))
        {
            int newbpm = perf().increment_bpm();
            m_adjust_bpm->set_value(newbpm);
        }

        keystroke k(a_ev->keyval, SEQ64_KEYSTROKE_PRESS);
        (void) perf().mainwnd_key_event(k);             // pass to perform

        if (a_ev->keyval == PREFKEY(screenset_dn))
        {
            int newss = perf().decrement_screenset();
            m_main_wid->set_screenset(newss);
            m_adjust_ss->set_value(newss);
            m_entry_notes->set_text(perf().current_screen_set_notepad());
        }
        else if (a_ev->keyval == PREFKEY(screenset_up))
        {
            int newss = perf().increment_screenset();
            m_main_wid->set_screenset(newss);
            m_adjust_ss->set_value(newss);
            m_entry_notes->set_text(perf().current_screen_set_notepad());
        }

        if (perf().get_key_groups().count(a_ev->keyval) != 0)
        {
            perf().select_and_mute_group        /* activate mute group key  */
            (
                perf().lookup_keygroup_group(a_ev->keyval)
            );
        }
        if                                      /* mute group learn         */
        (
            perf().is_learn_mode() && a_ev->keyval != PREFKEY(group_learn)
        )
        {
            if (perf().get_key_groups().count(a_ev->keyval) != 0)
            {
                std::ostringstream os;
                os << "Key \""
                   << gdk_keyval_name(a_ev->keyval)
                   << "\" (code = "
                   << a_ev->keyval
                   << ") successfully mapped."
                   ;

                Gtk::MessageDialog dialog
                (
                    *this, "MIDI mute group learn success", false,
                    Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true
                );
                dialog.set_secondary_text(os.str(), false);
                dialog.run();

                /*
                 * Missed the key-up message for group-learn, so force it
                 * to off
                 */

                perf().unset_mode_group_learn();
            }
            else
            {
                std::ostringstream os;
                os << "Key \""
                   << gdk_keyval_name(a_ev->keyval)
                   << "\" (code = "
                   << a_ev->keyval
                   << ") is not one of the configured mute-group keys.\n"
                   << "To change this see File/Options menu or the rc file."
                   ;

                Gtk::MessageDialog dialog
                (
                    *this, "MIDI mute group learn failed", false,
                    Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
                );
                dialog.set_secondary_text(os.str(), false);
                dialog.run();

                /*
                 * Missed the key-up message for group-learn, so force it
                 * to off.
                 */

                perf().unset_mode_group_learn();
            }
        }

        /*
         * The start/end key may be the same key (i.e. SPACE) to allow
         * toggling when the same key is mapped to both triggers (i.e.
         * SPACEBAR)
         */

        bool dont_toggle = PREFKEY(start) != PREFKEY(stop);
        if
        (
            a_ev->keyval == PREFKEY(start) &&
            (dont_toggle || ! g_rc_settings.is_pattern_playing())
        )
        {
            start_playing();
        }
        else if
        (
            a_ev->keyval == PREFKEY(stop) &&
            (dont_toggle || g_rc_settings.is_pattern_playing())
        )
        {
            stop_playing();
        }

        /*
         * Toggle the sequence mute/unmute setting using keyboard keys.
         *
         * \change ca 2015-08-12
         *      However, do not do this if the Ctrl key is being pressed.
         *      Ctrl-E, for example, brings up the Song Editor, and should
         *      not toggle the sequence controlled by the "e" key.  Will
         *      also see if the Alt key could/should be intercepted.
         */

        if (perf().get_key_events().count(a_ev->keyval) != 0)
        {
            guint modifiers = gtk_accelerator_get_default_mod_mask();
            bool ok = (a_ev->state & modifiers) != GDK_CONTROL_MASK;
            if (ok)
                sequence_key(perf().lookup_keyevent_seq(a_ev->keyval));
        }
    }
    return false;
}

/**
 *  Updates the title shown in the title bar of the window.  Note that the
 *  name of the application is obtained by the "(SEQ64_PACKAGE)"
 *  construction.
 *
 *  The format of the caption bar is the name of the package/application,
 *  followed by the file-specification (shortened if necessary so that the
 *  name of the file itself can be seen), ending with the PPQN value in
 *  parentheses.
 */

void
mainwnd::update_window_title ()
{
    std::string title = (SEQ64_PACKAGE) + std::string(" - [");
    std::string itemname = "unnamed";
    int ppqn = m_ppqn == SEQ64_USE_DEFAULT_PPQN ? global_ppqn : m_ppqn ;
    char temp[16];
    snprintf(temp, sizeof(temp), " (%d ppqn) ", ppqn);
    if (! g_rc_settings.filename().empty())
    {
        std::string name = shorten_file_spec(g_rc_settings.filename(), 56);
        itemname = Glib::filename_to_utf8(name);
    }
    title += itemname + std::string("]") + std::string(temp);
    set_title(title.c_str());
}

/**
 *  This function is the handler for system signals (SIGUSR1, SIGINT...)
 *  It writes a message to the pipe and leaves as soon as possible.
 */

void
mainwnd::handle_signal (int sig)
{
    if (write(m_sigpipe[1], &sig, sizeof(sig)) == -1)
        printf("signal write() failed: %s\n", std::strerror(errno));
}

/**
 *  Installs the signal handlers and pipe code.
 */

bool
mainwnd::install_signal_handlers ()
{
    // install pipe to forward received system signals

    if (pipe(m_sigpipe) < 0)
    {
        printf("pipe() failed: %s\n", std::strerror(errno));
        return false;
    }

    // install a notifier to handle pipe messages

    Glib::signal_io().connect
    (
        sigc::mem_fun(*this, &mainwnd::signal_action), m_sigpipe[0], Glib::IO_IN
    );

    // install signal handlers

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    if (sigaction(SIGUSR1, &action, NULL) == -1)
    {
        printf("sigaction() failed: %s\n", std::strerror(errno));
        return false;
    }
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        printf("sigaction() failed: %s\n", std::strerror(errno));
        return false;
    }
    return true;
}

/**
 *  Handles saving or exiting actions when signalled.
 *
 * \return
 *      Returns true if the signalling was able to be completed, even if
 *      it was an unexpected signal.
 */

bool
mainwnd::signal_action (Glib::IOCondition condition)
{
    bool result = true;
    if ((condition & Glib::IO_IN) == 0)
    {
        printf("Error: unexpected IO condition\n");
        result = false;
    }
    else
    {
        int message;
        if (read(m_sigpipe[0], &message, sizeof(message)) == -1)
        {
            printf("read() failed: %s\n", std::strerror(errno));
            result = false;
        }
        else
        {
            switch (message)
            {
            case SIGUSR1:
                save_file();
                break;

            case SIGINT:
                file_exit();
                break;

            default:
                printf("Unexpected signal received: %d\n", message);
                break;
            }
        }
    }
    return result;
}

}           // namespace seq64

/*
 * mainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
