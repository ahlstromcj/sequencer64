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
 * \updates       2017-08-06
 * \license       GNU GPLv2 or above
 *
 *  The main window holds the menu and the main controls of the application,
 *  and the mainwid that holds the patterns is nestled in the interior of the
 *  main window.
 *
 *  This object now has a GUI element that shows the actual PPQN in force,
 *  in the title caption.
 *
 *  It can also create and bring up a second perfedit object, as a way
 *  to deal better with large sets of sequences.
 *
 * Pause:
 *
 *      We have basically finished this feature, with some minor follow-on
 *      work in making some of it a user-configurable option.  It works as
 *      desired if JACK transport is selected, in that, upon pause and
 *      restart, the progress bars pick up where they left off, as opposed to
 *      live mode, where the progress bars go back to 0.  The progress bars
 *      now behave when the song is paused.  (Heavy testing still needed.)
 *
 *      The progress pills, however, stay in place when paused.  They pick up
 *      where they left off when played back in JACK, but restart at the
 *      beginning in "ALSA" mode.
 *
 * Current sequence:
 *
 *      We've elaborated on the concept of the currently-edited sequence so
 *      that the seqedit window can tell the mainwid when to change the
 *      display of the current sequence.  This makes the code more complex,
 *      because, after creating the mainwid (derived from seqmenu), we have to
 *      get a reference to the mainwid object into the seqedit object by
 *      passing down through the following constructors:  mainwnd, mainwid,
 *      perfedit, perfnames (derived from seqmenu), seqmenu, and seqedit.  We
 *      thought it was  preferable to making a global object, because at least
 *      the provenance of the mainwid reference is now clearcut, but then we
 *      realized we'd have to do the same for the perfedit/perfnames class,
 *      and that's just too much.  We now add a global/free function to the
 *      mainwid module to access the update function we need.
 *
 * Toggle playing:
 *
 *      This feature was keyed by SEQ64_TOGGLE_PLAYING, but is now permanent.
 *      See the INSTALL file for more information.
 *
 *  User jean-emmanuel made the main window resizable if his scroll-bar
 *  feature is enable, pull #84.  We might eventually make resizability
 *  enabled only if the larger screen-sets or multi-mainwid features are
 *  enabled, just to preserve expected "legacy" behavior under "legacy" usage.
 */

#include <cctype>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <stdio.h>                      /* snprintf()                   */
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
#include "gui_key_tests.hpp"            /* is_ctrl_key(), etc.          */
#include "keys_perform.hpp"
#include "keystroke.hpp"
#include "maintime.hpp"
#include "mainwid.hpp"
#include "mainwnd.hpp"
#include "midifile.hpp"
#include "options.hpp"
#include "perfedit.hpp"
#include "cmdlineopts.hpp"              /* for build info function          */

#ifdef USE_SHOW_TICK_TIME
#include "calculations.hpp"             /* pulse_to_measurestring()         */
#endif

#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS
#include <gtkmm/layout.h>
#include <gtkmm/scrollbar.h>
#endif

#if defined SEQ64_MULTI_MAINWID
#include <gtkmm/frame.h>
#include <gtkmm/separator.h>
#include <gtkmm/table.h>                /* <gtkmm-2.4/grid.h> doesn't exist */
#endif

#include "pixmaps/pause.xpm"
#include "pixmaps/play2.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/learn.xpm"
#include "pixmaps/learn2.xpm"
#include "pixmaps/perfedit.xpm"
#include "pixmaps/seq64.xpm"

#ifdef SEQ64_RTMIDI_SUPPORT
#include "pixmaps/seq64_logo.xpm"
#include "pixmaps/seq64_logo_legacy.xpm"
#else
#include "pixmaps/sequencer64_square_small.xpm"
#include "pixmaps/sequencer64_legacy.xpm"
#endif

#ifdef SEQ64_STAZED_MENU_BUTTONS
#include "pixmaps/live_mode.xpm"
#include "pixmaps/menu.xpm"
#include "pixmaps/muting.xpm"
#include "pixmaps/song_mode.xpm"
#endif

#ifdef USE_RECORD_TEMPO_MENU                // too clumsy
#include "pixmaps/tempo_autorecord.xpm"
#include "pixmaps/tempo_record.xpm"
#else
#include "pixmaps/tempo_log.xpm"
#include "pixmaps/tempo_rec_off.xpm"
#include "pixmaps/tempo_rec_on.xpm"
#endif

/**
 *  Provides the value of padding, in pixels, to use for the top horizontal
 *  box containing the Sequencer64 label and some extra buttons.
 */

#define HBOX_PADDING                  10

/**
 *  The amount of time to wait for inaction before clearing the tap-button
 *  values, in milliseconds.
 */

#ifdef SEQ64_MAINWND_TAP_BUTTON
#define SEQ64_TAP_BUTTON_TIMEOUT    5000L
#endif

/*
 * Access some menu elements more easily.
 */

using namespace Gtk::Menu_Helpers;      /* MenuElem, etc.                */

/*
 *  All library code for this project is in the "seq64" namespace.  Do not
 *  attempt to document this namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This static member provides a couple of pipes for signalling/messaging.
 */

int mainwnd::sm_sigpipe[2];

/**
 *  The constructor the main window of the application.
 *  This constructor is way too large; it would be nicer to provide a
 *  number of well-named initialization functions.
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \param allowperf2
 *      Indicates if a second perfedit window should be created.
 *      This is currently a run-time option, selectable in the "user"
 *      configuration file.
 *
 * \param ppqn
 *      An optional PPQN value to use in the song.
 *
 * \param mainwid_rows
 *      The number of rows of mainwids to create vertically.  The default
 *      value is one.  Used only if SEQ64_MULTI_MAINWID is defined.
 *
 * \param mainwid_cols
 *      The number of columns of mainwids to create horizontally.  The default
 *      value is one.  Used only if SEQ64_MULTI_MAINWID is defined.
 *
 * \param mainwid_indep
 *      If true, indicates that the mainwids in multi-wid mode can have their
 *      set-numbers controller independently.
 *
 * \todo
 *      Offload most of the work into an initialization function like
 *      options does; make the perform parameter a reference;
 *      valgrind flags m_tooltips as lost data, but if we try to manage it
 *      ourselves, many more leaks occur.
 */

mainwnd::mainwnd
(
    perform & p,
    bool allowperf2,
    int ppqn
#if defined SEQ64_MULTI_MAINWID
    ,
    int mainwid_rows,
    int mainwid_cols,
    bool mainwid_indep
#endif
) :
    gui_window_gtk2         (p),
    performcallback         (),
    m_tooltips              (manage(new Gtk::Tooltips())),  /* valgrind bitches */
    m_menubar               (manage(new Gtk::MenuBar())),
    m_menu_file             (manage(new Gtk::Menu())),
    m_menu_edit             (manage(new Gtk::Menu())),
    m_menu_view             (manage(new Gtk::Menu())),
    m_menu_help             (manage(new Gtk::Menu())),
    m_status_label          (manage(new Gtk::Label(" ", true))),
    m_ppqn                  (choose_ppqn(ppqn)),
#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS
    m_hadjust               (manage(new Gtk::Adjustment(0, 0, 1, 10, 100, 0))),
    m_vadjust               (manage(new Gtk::Adjustment(0, 0, 1, 10, 100, 0))),
    m_hscroll               (manage(new Gtk::HScrollbar(*m_hadjust))),
    m_vscroll               (manage(new Gtk::VScrollbar(*m_vadjust))),
#endif
#if defined SEQ64_MULTI_MAINWID
    m_mainwid_grid          (nullptr),
    m_mainwid_frames        (),                         /* a 2 x 3 array    */
    m_mainwid_adjustors     (),                         /* a 2 x 3 array    */
    m_mainwid_spinners      (),                         /* a 2 x 3 array    */
    m_mainwid_blocks        (),                         /* a 2 x 3 array    */
    m_mainwid_rows          (mainwid_rows),             /* assumed valid    */
    m_mainwid_columns       (mainwid_cols),             /* assumed valid    */
    m_mainwid_count         (mainwid_rows * mainwid_cols),
    m_mainwid_independent   (mainwid_indep),
    m_main_wid              (nullptr),
    m_adjust_ss             (nullptr),
    m_spinbutton_ss         (nullptr),
#else
    m_main_wid              (manage(new mainwid(p))),
    m_adjust_ss             (manage(new Gtk::Adjustment(0, 0, c_max_sets-1, 1))),
    m_spinbutton_ss         (manage(new Gtk::SpinButton(*m_adjust_ss))),
#endif
    m_main_time             (manage(new maintime(p, ppqn))),
    m_perf_edit             (new perfedit(p, false /*allowperf2*/, ppqn)),
    m_perf_edit_2           (allowperf2 ? new perfedit(p, true, ppqn) : nullptr),
    m_options               (nullptr),
    m_main_cursor           (),
    m_image_play            (),
    m_button_learn          (manage(new Gtk::Button())),    /* group learn (L) */
    m_button_stop           (manage(new Gtk::Button())),
    m_button_play           (manage(new Gtk::Button())),    /* also for pause  */
    m_button_tempo_log      (manage(new Gtk::Button())),
    m_button_tempo_record   (manage(new Gtk::ToggleButton())),
    m_is_tempo_recording    (false),
    m_button_perfedit       (manage(new Gtk::Button())),
#ifdef SEQ64_STAZED_MENU_BUTTONS
    m_image_songlive        (),
    m_button_mode
    (
        usr().use_more_icons() ?
            manage(new Gtk::ToggleButton()) :
            manage(new Gtk::ToggleButton(" Live "))
    ),
    m_button_mute
    (
        usr().use_more_icons() ?
            manage(new Gtk::ToggleButton()) :
            manage(new Gtk::ToggleButton("Mute"))
    ),
    m_button_menu
    (
        usr().use_more_icons() ?
            manage(new Gtk::ToggleButton()) :
            manage(new Gtk::ToggleButton("Menu"))
    ),
#endif
#ifdef SEQ64_SHOW_JACK_STATUS
    m_button_jack           (manage(new Gtk::Button("ALSA"))),
#endif
#ifdef USE_SHOW_TICK_TIME
    m_tick_time             (manage(new Gtk::Label("**:**:****"))),
#endif
    m_adjust_bpm
    (
        manage
        (
            new Gtk::Adjustment
            (
                perf().get_beats_per_minute(),
                SEQ64_MINIMUM_BPM, SEQ64_MAXIMUM_BPM, 1
            )
        )
    ),
    m_spinbutton_bpm        (manage(new Gtk::SpinButton(*m_adjust_bpm))),
#ifdef SEQ64_MAINWND_TAP_BUTTON
    m_button_tap            (manage(new Gtk::Button("0"))),
#endif
    m_button_queue          (manage(new Gtk::ToggleButton("Q"))),

    /*
     * \change ca 2016-05-15
     *      We were allocating these items here, but doing so causes a
     *      segfault the second time file_import_dialog() was called.  We
     *      moved the allocation to that function, as it was in seq24.
     */

    m_adjust_load_offset    (nullptr),  /* created in file_import_dialog()  */
    m_spinbutton_load_offset(nullptr),  /* created in file_import_dialog()  */
    m_entry_notes           (manage(new Gtk::Entry())),
    m_is_running            (false),
    m_timeout_connect       (),                     /* handler              */
#ifdef SEQ64_MAINWND_TAP_BUTTON
    m_current_beats         (0),
    m_base_time_ms          (0),
    m_last_time_ms          (0),
#endif
    m_menu_mode             (true),                 /* stazed 2016-07-30    */
    m_call_seq_edit         (false),                /* new ca 2016-05-15    */
    m_call_seq_shift        (0),                    /* new ca 2017-06-17    */
    m_call_seq_eventedit    (false)                 /* new ca 2016-05-19    */
{
#if defined SEQ64_MULTI_MAINWID
    if (! multi_wid())
        m_mainwid_independent = true;
#endif

#ifdef PLATFORM_DEBUG

    /*
     * Trying to debug a way out of a weird GTK freeze-up that occurs if
     * Alt-F (or other menu hotkeys) is pressed before clicking in the GUI.
     */

    GLogLevelFlags f = GLogLevelFlags(G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING);
    g_log_set_always_fatal(f);

#endif

    /*
     * This provides the application icon, seen in the title bar of the
     * window decoration.
     */

    set_icon(Gdk::Pixbuf::create_from_xpm_data(seq64_xpm));

    /*
     * Setting this to true allows the main window to resize to its contents.
     * Needed to display multiple mainwids easily.
     */

#if defined SEQ64_MULTI_MAINWID
    set_resizable(multi_wid());
#elif defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS
    set_resizable(! usr().is_default_mainwid_size());
#else
    set_resizable(false);
#endif

    perf().enregister(this);                        /* register for notify  */
    update_window_title();                          /* main window          */

    m_menubar->items().push_front(MenuElem("_File", *m_menu_file));
    populate_menu_file();
    m_menubar->items().push_back(MenuElem("_Edit", *m_menu_edit));
    populate_menu_edit();
    m_menubar->items().push_back(MenuElem("_View", *m_menu_view));
    populate_menu_view();
    m_menubar->items().push_back(MenuElem("_Help", *m_menu_help));
    populate_menu_help();
    m_menubar->set_sensitive(m_menu_mode);

    /**
     * Top panel items, including the logo (updated for the new version of
     * this application) and the "timeline" progress bar.
     */

    Gtk::HBox * tophbox = manage(new Gtk::HBox(false, 0));

#ifdef SEQ64_RTMIDI_SUPPORT
    const char ** bitmap = rc().legacy_format() ?
        seq64_logo_legacy_xpm : seq64_logo_xpm ;
#else
    const char ** bitmap = rc().legacy_format() ?
        sequencer64_legacy_xpm : sequencer64_square_small_xpm ;
#endif

    tophbox->pack_start
    (
        *manage(new PIXBUF_IMAGE(bitmap)), false, false, HBOX_PADDING
    );

#ifdef SEQ64_STAZED_MENU_BUTTONS            /* also enables muting button */

    if (usr().use_more_icons())
        m_button_mode->add(*manage(new PIXBUF_IMAGE(live_mode_xpm)));

    m_button_mode->set_focus_on_click(false);
    m_button_mode->signal_toggled().connect
    (
        sigc::mem_fun(*this, &mainwnd::set_song_mode)
    );

    std::string modetext =
        "Toggle Song mode vs Live mode.  If the button is active, the Song "
        "mode is active.  The label also shows which mode is active."
        ;

    add_tooltip(m_button_mode, modetext);
    m_button_mode->set_active(perf().song_start_mode());
    tophbox->pack_start(*m_button_mode, false, false, HBOX_PADDING/2);

    /*
     * We bind the muting button to the mainwid's toggle_all_tracks()
     * function.  A little tricky.  Testing calling a new function,
     * toggle_playing_tracks(), now.
     */

    if (usr().use_more_icons())
        m_button_mute->add(*manage(new PIXBUF_IMAGE(muting_xpm)));

    m_button_mute->set_can_focus(false);

#if defined SEQ64_MULTI_MAINWID

    for (int block = 0; block < SEQ64_MAINWIDS_MAX; ++block)
    {
        m_mainwid_adjustors[block] = nullptr;
        m_mainwid_spinners[block] = nullptr;
        m_mainwid_frames[block] = nullptr;
        m_mainwid_blocks[block] = nullptr;
    }

    /**
     *  For multiple mainwids, the numbering of the sets is like that of the
     *  patterns...  increasing downward and increasing to the right.
     *  We're trying to add a frame for each set.  However, although attaching
     *  a Frame by itself works, when the mainwid is added to the frame,
     *  attaching the frame to the table causes a segfault!
     */

    for (int block = 0; block < m_mainwid_count; ++block)
    {
        std::string label = "   Set ";
        label += std::to_string(block);
        m_mainwid_blocks[block] = manage(new mainwid(p, block, multi_wid()));
        if (independent() || block == 0)
        {
            m_mainwid_adjustors[block] = manage
            (
                new Gtk::Adjustment(block, 0, c_max_sets-1, 1)
            );
            m_mainwid_spinners[block] = manage
            (
                new Gtk::SpinButton(*m_mainwid_adjustors[block])
            );
            m_mainwid_spinners[block]->set_sensitive(true);
            m_mainwid_spinners[block]->set_editable(true);
            m_mainwid_spinners[block]->set_wrap(false);
            m_mainwid_spinners[block]->set_width_chars(3);
        }
        if (multi_wid())
        {
            m_mainwid_frames[block] = manage(new Gtk::Frame(label));
            m_mainwid_frames[block]->set_border_width(4);
            m_mainwid_frames[block]->set_shadow_type(Gtk::SHADOW_NONE);

            /*
             * We get more control if we add the frame and mainwid
             * separately to the table.  Try adding it to table instead.
             *
             *  m_mainwid_frames[block]->add(*m_mainwid_blocks[block]);
             *  m_mainwid_frames[block]->add(*m_mainwid_spinners[block]);
             */
        }
    }
    m_main_wid = m_mainwid_blocks[0];
    m_adjust_ss = m_mainwid_adjustors[0];
    m_spinbutton_ss = m_mainwid_spinners[0];
    if (multi_wid())
    {
        m_mainwid_frames[0]->set_shadow_type(Gtk::SHADOW_OUT);
        m_mainwid_frames[0]->set_label("   Set 0 [active]");
    }

#else

    /*
     * m_main_wid, m_adjust_ss, and m_spinbutton_ss are created in the
     * initializer list if not compiling for multi-wid support.
     */

#endif  // SEQ64_MULTI_MAINWID

    /*
     * The main window is always the active window, no matter which set it is
     * displaying.
     */

    m_button_mute->signal_clicked().connect
    (
        sigc::mem_fun(*m_main_wid, &seqmenu::toggle_playing_tracks)
    );
    add_tooltip
    (
        m_button_mute,
        "Toggle the mute status of playing tracks. Effective only in Live "
        "mode.  Affects only tracks that are currently armed. Muted tracks "
        "are remembered even if the mode is toggled to Song and back to Live. "
    );

    tophbox->pack_start(*m_button_mute, false, false);

    if (usr().use_more_icons())
        m_button_menu->add(*manage(new PIXBUF_IMAGE(menu_xpm)));

    add_tooltip
    (
        m_button_menu,
        "Toggle to disable/enable the menu when sequencer is not running. "
        "The menu is automatically disabled when the sequencer is running."
    );
    m_button_menu->set_sensitive(true);
    m_button_menu->set_can_focus(false);
    m_button_menu->set_active(true);
    m_button_menu->signal_toggled().connect
    (
        sigc::mem_fun(*this, &mainwnd::set_menu_mode)
    );
    tophbox->pack_start(*m_button_menu, false, false, HBOX_PADDING/2);

#ifdef SEQ64_SHOW_JACK_STATUS
    add_tooltip
    (
        m_button_jack,
        "Shows the current transport mode:  JACK, Master, or ALSA. Click "
        "this button to bring up the JACK connection options page. Ctrl-P "
        "also brings up this page."
    );
    m_button_jack->set_focus_on_click(false);
    m_button_jack->signal_clicked().connect
    (
        sigc::mem_fun(*this, &mainwnd::jack_dialog)
    );
    tophbox->pack_start(*m_button_jack, false, false);
#endif

    tophbox->pack_start(*(manage(new Gtk::HSeparator())), false, false, 4);
    tophbox->pack_start(*m_status_label, false, false);  /* new */

#endif  // SEQ64_STAZED_MENU_BUTTONS

    /* Adjust placement of the logo. */

    Gtk::VBox * vbox_b = manage(new Gtk::VBox(true,  0));
    Gtk::HBox * hbox3 = manage(new Gtk::HBox(false, 0));
    vbox_b->pack_start(*hbox3, false, false);
#ifdef USE_SHOW_TICK_TIME
    Gtk::HBox * hbox4 = manage(new Gtk::HBox(false, 0));
    Gtk::Label * timedummy = manage(new Gtk::Label(" "));
    m_tick_time->set_justify(Gtk::JUSTIFY_LEFT);
    hbox4->pack_start(*m_tick_time, false, false, 0);
    hbox4->pack_start(*timedummy, false, false, 0);
    vbox_b->pack_start(*hbox4, false, false, 0);
    tophbox->pack_end(*vbox_b, false, false);
    hbox3->pack_start(*m_main_time, false, false, 8);  /* pill timeline    */
#else
    tophbox->pack_end(*vbox_b, false, false);
    hbox3->pack_start(*m_main_time, false, false);  /* pill timeline    */
#endif

    m_button_learn->set_focus_on_click(false);
    m_button_learn->set_flags(m_button_learn->get_flags() & ~Gtk::CAN_FOCUS);
    m_button_learn->set_image(*manage(new PIXBUF_IMAGE(learn_xpm)));
    m_button_learn->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::learn_toggle)
    );
    add_tooltip
    (
        m_button_learn,
        "Mute Group Learn. "
        "Click the 'L' button, then press a mute-group key to store "
        "the mute state of the sequences in that key. "
        "See File / Options / Keyboard for available mute-group keys "
        "and the hotkey for the 'L' button. Ctrl-L also causes this action."
    );
    hbox3->pack_end(*m_button_learn, false, false);

    /*
     *  This seems to be a dirty hack to clear the focus, not to trigger L
     *  via keys.  We had to add some set_focus_on_click(false) calls anyway.
     */

    Gtk::Button w;
    hbox3->set_focus_child(w);

    /*
     *  Bottom panel items.  The first ones are the start/stop buttons and
     *  the container that groups them.
     */

    Gtk::HBox * bottomhbox = manage(new Gtk::HBox(false, 10));   /* bottom */
    Gtk::HBox * startstophbox = manage(new Gtk::HBox(false, 4)); /* button */
    bottomhbox->pack_start(*startstophbox, Gtk::PACK_SHRINK);

    /*
     * If we don't call this function, then clicking the stop button makes
     * it steal focus, and be "clicked" when the space bar is hit, which is
     * very confusing.
     */

    m_button_stop->set_focus_on_click(false);
    m_button_stop->add(*manage(new PIXBUF_IMAGE(stop_xpm)));
    m_button_stop->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::stop_playing)
    );
    add_tooltip(m_button_stop, "Stop playing the MIDI sequences.");
    startstophbox->pack_start(*m_button_stop, Gtk::PACK_SHRINK);
    m_button_stop->set_sensitive(true);

    /*
     * If we don't call this function, then clicking the stop button makes
     * it steal focus, and be "clicked" when the space bar is hit, which is
     * very confusing.
     */

    m_button_play->set_focus_on_click(false);
    m_button_play->add(*manage(new PIXBUF_IMAGE(play2_xpm)));
    m_button_play->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::start_playing)
    );
    add_tooltip(m_button_play, "Start playback from the current location.");
    startstophbox->pack_start(*m_button_play, Gtk::PACK_SHRINK);
    m_button_play->set_sensitive(true);

    /*
     * BPM spin button with label.  Be sure to document that right-clicking on
     * the up or down button brings it to the very end of its range.  We now
     * set the editable status of the spin to be true.  This still also
     * affects the muting of slots that have a digit for a shortcut (hot) key.
     */

    Gtk::HBox * bpmhbox = manage(new Gtk::HBox(false, 4));
    bottomhbox->pack_start(*bpmhbox, Gtk::PACK_SHRINK);
    m_spinbutton_bpm->set_sensitive(true);
    m_spinbutton_bpm->set_editable(true);
    m_spinbutton_bpm->set_digits(usr().bpm_precision());

#ifdef SEQ64_MAINWND_TAP_BUTTON

    m_button_tap->signal_clicked().connect(mem_fun(*this, &mainwnd::tap));
    add_tooltip
    (
        m_button_tap,
        "Tap in time to set the beats per minute (BPM) value. "
        "After 5 seconds of no taps, the tap-counter will reset to 0. "
        "Also see the File / Options / Ext Keys / Tap BPM key assignment."
    );

#endif

    m_button_tempo_log->set_focus_on_click(false);
    m_button_tempo_log->add(*manage(new PIXBUF_IMAGE(tempo_log_xpm)));
    m_button_tempo_log->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::tempo_log)
    );
    add_tooltip
    (
        m_button_tempo_log,
        "Click this button to add the current tempo as an event at the current "
        "time.  Recording a tempo event can extend the length of the tempo "
        "track, which is always pattern 0."
    );

    m_button_tempo_record->set_focus_on_click(false);
    m_button_tempo_record->add(*manage(new PIXBUF_IMAGE(tempo_rec_off_xpm)));
    m_button_tempo_record->signal_toggled().connect
    (
        mem_fun(*this, &mainwnd::toggle_tempo_record)
    );
    add_tooltip
    (
        m_button_tempo_record,
        "Click this button to toggle the recording of live changes to the "
        "tempo. Recording tempo events can extend the length of the tempo "
        "track, which is always pattern 0."
    );

    m_button_queue->signal_clicked().connect(mem_fun(*this, &mainwnd::queue_it));
    add_tooltip(m_button_queue, "Shows and toggles the keep-queue status.");

    m_adjust_bpm->signal_value_changed().connect
    (
        mem_fun(*this, &mainwnd::adj_callback_bpm)
    );
    m_adjust_bpm->set_step_increment(usr().bpm_step_increment());
    m_adjust_bpm->set_page_increment(usr().bpm_page_increment());

    add_tooltip
    (
        m_spinbutton_bpm,
        "Adjusts the beats per minute (BPM) value. Once it has focus, "
        "the Up/Down arrows adjust by the step size, and the Page-Up/Page-Down "
        "keys adjust by the page size, as configured in the 'usr' file."
    );
    Gtk::Label * bpmlabel = manage(new Gtk::Label("_BPM", true));
    bpmlabel->set_mnemonic_widget(*m_spinbutton_bpm);
    bpmhbox->pack_start(*bpmlabel, Gtk::PACK_SHRINK);
    bpmhbox->pack_start(*m_spinbutton_bpm, Gtk::PACK_SHRINK);

#ifdef SEQ64_MAINWND_TAP_BUTTON
    bpmhbox->pack_start(*m_button_tap, Gtk::PACK_SHRINK);
#endif

    bpmhbox->pack_start(*m_button_tempo_log, Gtk::PACK_SHRINK);
    bpmhbox->pack_start(*m_button_tempo_record, Gtk::PACK_SHRINK);
    bpmhbox->pack_start(*m_button_queue, Gtk::PACK_SHRINK);

    /*
     * Screen set name edit line.
     */

    Gtk::HBox * notebox = manage(new Gtk::HBox(false, 4));
    bottomhbox->pack_start(*notebox, Gtk::PACK_EXPAND_WIDGET);
    m_entry_notes->signal_changed().connect
    (
        mem_fun(*this, &mainwnd::edit_callback_notepad)
    );
    m_entry_notes->set_text(perf().current_screen_set_notepad());
    add_tooltip
    (
        m_entry_notes,
        "Enter the screen-set notes.  A screen-set is one page of "
        "up to 32 patterns that can be seen and manipulated in "
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
    if (! multi_wid())
    {
        m_spinbutton_ss->set_sensitive(true);
        m_spinbutton_ss->set_editable(true);
        m_spinbutton_ss->set_wrap(false);
        m_spinbutton_ss->set_width_chars(3);
        m_spinbutton_ss->set_wrap(false);

#if ! defined SEQ64_MULTI_MAINWID

        /*
         * If built for multi-wid, this control is connected to
         * adj_callback_wid() instead.
         */

        m_adjust_ss->signal_value_changed().connect
        (
            mem_fun(*this, &mainwnd::adj_callback_ss)
        );
#endif

        add_tooltip(m_spinbutton_ss, "Select screen-set from one of 32 sets.");
        Gtk::Label * setlabel = manage(new Gtk::Label("_Set", true));
        setlabel->set_mnemonic_widget(*m_spinbutton_ss);
        sethbox->pack_start(*setlabel, Gtk::PACK_SHRINK);
        sethbox->pack_start(*m_spinbutton_ss, Gtk::PACK_SHRINK);
    }

#if defined SEQ64_MULTI_MAINWID

    if (multi_wid())
    {
        int block = 0;
        for (int col = 0; col < m_mainwid_columns; ++col)
        {
            sethbox->pack_start
            (
                *(manage(new Gtk::VSeparator())), false, false, 4
            );
            for (int row = 0; row < m_mainwid_rows; ++row, ++block)
            {
                if (need_set_spinner(block))
                {
                    Gtk::Adjustment * mwap = m_mainwid_adjustors[block];
                    Gtk::SpinButton * sbp = m_mainwid_spinners[block];
                    std::string tip = independent() ?
                        "Change screen-set of corresponding set window." :
                        "Change screen-set of all set windows in sync." ;

                    tip += " Top-left mainwid is always the active screen-set.";
                    add_tooltip(sbp, tip);
                    mwap->signal_value_changed().connect
                    (
                        sigc::bind      /* bind parameter to function   */
                        (
                            mem_fun(*this, &mainwnd::adj_callback_wid), block
                        )
                    );
                    sethbox->pack_start(*sbp, Gtk::PACK_SHRINK);
                }
            }
        }
    }

#endif  // SEQ64_MULTI_MAINWID

    /*
     * Song editor button.  Although there can be two of them brought
     * on-screen, only one has a button devoted to it.  Also, we prevent this
     * button from grabbing focus, as it interacts "badly" with the space bar.
     */

    m_button_perfedit->set_focus_on_click(false);
    m_button_perfedit->add(*manage(new PIXBUF_IMAGE(perfedit_xpm)));
    m_button_perfedit->signal_clicked().connect
    (
        mem_fun(*this, &mainwnd::open_performance_edit)
    );
    add_tooltip
    (
        m_button_perfedit,
        "Show or hide the main song editor window. Ctrl-E also brings up "
        "the editor."
    );
    bottomhbox->pack_end(*m_button_perfedit, Gtk::PACK_SHRINK);

#if ! defined SEQ64_MULTI_MAINWID
#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS

    /*
     * Pattern panel scrollable wrapper.
     *
     * TODO:    Note that, if we have SEQ64_MULTI_MAINWND enabled, we could
     *          repurpose these scrollbars for layouts larger than the main
     *          window size.  Right now, these two features are not
     *          compatible, and so the patterns-panel scrollbars are disabled
     *          at configure time by default.
     */

    Gtk::VBox * mainwid_hscroll_wrapper = nullptr;
    if (! usr().is_default_mainwid_size())
    {
        Gtk::Layout * mainwid_wrapper = new Gtk::Layout(*m_hadjust, *m_vadjust);
        mainwid_wrapper->add(*m_main_wid);
        mainwid_wrapper->set_size(m_main_wid->m_mainwid_x, m_main_wid->m_mainwid_y);

        Gtk::HBox * mainwid_vscroll_wrapper = new Gtk::HBox();
        mainwid_vscroll_wrapper->set_spacing(5);
        mainwid_vscroll_wrapper->pack_start
        (
            *mainwid_wrapper,
            Gtk::PACK_EXPAND_WIDGET
        );
        mainwid_vscroll_wrapper->pack_start(*m_vscroll, false, false);

        Gtk::VBox * mainwid_hscroll_wrapper = new Gtk::VBox();
        mainwid_hscroll_wrapper->set_spacing(5);
        mainwid_hscroll_wrapper->pack_start
        (
            *mainwid_vscroll_wrapper, Gtk::PACK_EXPAND_WIDGET
        );
        mainwid_hscroll_wrapper->pack_start(*m_hscroll, false, false);

        m_main_wid->signal_scroll_event().connect
        (
            mem_fun(*this, &mainwnd::on_scroll_event)
        );
        m_hadjust->signal_changed().connect
        (
            mem_fun(*this, &mainwnd::on_scrollbar_resize)
        );
        m_vadjust->signal_changed().connect
        (
            mem_fun(*this, &mainwnd::on_scrollbar_resize)
        );
    }

#endif  // SEQ64_JE_PATTERN_PANEL_SCROLLBARS
#endif  // ! SEQ64_MULTI_WID

    /*
     * Vertical layout container for window content.  Letting Gtk manage it
     * does not improve leaks:
     *
     * Gtk::VBox * contentvbox = manage(new Gtk::VBox());
     */

    Gtk::VBox * contentvbox = manage(new Gtk::VBox());
    contentvbox->set_spacing(10);
    contentvbox->set_border_width(10);
    contentvbox->pack_start(*tophbox, Gtk::PACK_SHRINK);

#if ! defined SEQ64_MULTI_MAINWID
#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS

    // contentvbox->pack_start(*mainwid_hscroll_wrapper, true, true);
    // contentvbox->pack_start(*bottomhbox, false, false);

    /*
     * Make sure this is correct after the merge.
     */

    if (! usr().is_default_mainwid_size())
    {
        contentvbox->pack_start(*mainwid_hscroll_wrapper, true, true);
        contentvbox->pack_start(*bottomhbox, false, false);
    }
    else
    {
        contentvbox->pack_start(*m_main_wid, Gtk::PACK_SHRINK);
        contentvbox->pack_start(*bottomhbox, Gtk::PACK_SHRINK);
    }

    // contentvbox->pack_start(*m_main_wid, Gtk::PACK_SHRINK);

    m_main_wid->set_can_focus();            /* from stazed */
    m_main_wid->grab_focus();

#endif
#endif

#if defined SEQ64_MULTI_MAINWID

    /*
     * This could perhaps be replaced with jean-emmanuel's scrollbar code that
     * uses a Gtk::Layout object.
     */

    int wpadding = 20;
    int hpadding = 20;
    if (multi_wid())
    {
        m_mainwid_grid = manage
        (
            new Gtk::Table(guint(m_mainwid_rows), guint(m_mainwid_columns), true)
        );
        int block = 0;
        for (int col = 0; col < m_mainwid_columns; ++col)
        {
            for (int row = 0; row < m_mainwid_rows; ++row, ++block)
            {
                Gtk::Frame * frp = m_mainwid_frames[block];
                mainwid * mwp = m_mainwid_blocks[block];
                m_mainwid_grid->attach(*frp, col, col+1, row, row+1);
                m_mainwid_grid->attach
                (
                    *mwp, col, col+1, row, row+1,
                    Gtk::FILL | Gtk::EXPAND,
                    Gtk::FILL | Gtk::EXPAND,
                    guint(wpadding), guint(hpadding)
                );
            }
        }
        contentvbox->pack_start(*m_mainwid_grid, Gtk::PACK_SHRINK);
    }

    m_main_wid->set_can_focus(true);            /* from stazed */
    m_main_wid->grab_focus();

    /*
     * Setting this allows other hot-keys to work, but then the user cannot
     * edit the spinbutton numeric value!
     *
     *  m_spinbutton_ss->set_can_focus(false);
     *
     */

#endif  // SEQ64_MULTI_WID

    if (! multi_wid())
        contentvbox->pack_start(*m_main_wid, Gtk::PACK_SHRINK);

    contentvbox->pack_start(*bottomhbox, Gtk::PACK_SHRINK);
    m_main_wid->set_can_focus();            /* from stazed */
    m_main_wid->grab_focus();

    /*
     * Main container for menu and window content.
     */

    Gtk::VBox * mainvbox = new Gtk::VBox();
    mainvbox->pack_start(*m_menubar, false, false);
    mainvbox->pack_start(*contentvbox);
    add(*mainvbox);                         /* add main layout box (this->) */

#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS
    add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::SCROLL_MASK);
#else
    add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
#endif

    m_timeout_connect = Glib::signal_timeout().connect
    (
        mem_fun(*this, &mainwnd::timer_callback), redraw_period_ms()
    );
    show_all();                             /* works here as well           */

#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS

    /*
     * Set window initial size: mainwid doesn't require any space because it's
     * wrapped in a Layout object, so we must add the window's children sizes
     * and spacings
     */

    if (! usr().is_default_mainwid_size())
    {
        resize
        (
            m_main_wid->m_mainwid_x + 20,
            tophbox->get_allocation().get_height() +
                bottomhbox->get_allocation().get_height() +
                m_menubar->get_allocation().get_height() +
                m_main_wid->m_mainwid_y + 40
        );
    }

#else

    int width = m_main_wid->nominal_width();
    int height = m_main_wid->nominal_height();
    int menuheight = 22;
    int bottomheight = 52;  // 48;
    int topheight = 64;     // 52;
    if (multi_wid())
    {
        topheight = 100;
    }
    else
    {
        width += 24;
    }
    height += menuheight + topheight + bottomheight;

#endif  // SEQ64_JE_PATTERN_PANEL_SCROLLBARS

#if defined SEQ64_MULTI_MAINWID
    height += hpadding * (m_mainwid_rows - 1);
    width += wpadding * (m_mainwid_columns - 1);
#endif

    set_size_request(width, height);
    install_signal_handlers();
}

/**
 *  This destructor must explicitly delete some allocated resources.
 */

mainwnd::~mainwnd ()
{
    if (not_nullptr(m_perf_edit_2))
        delete m_perf_edit_2;

    if (not_nullptr(m_perf_edit))
        delete m_perf_edit;

    if (not_nullptr(m_options))
        delete m_options;

    /*
     * delete m_tooltips;
     */

    if (sm_sigpipe[0] != -1)
        close(sm_sigpipe[0]);

    if (sm_sigpipe[1] != -1)
        close(sm_sigpipe[1]);
}

/**
 *  Check if one of the edit fields (BPM spinbutton, screenset spinbutton, or
 *  the Name field) has focus.
 *
 * \return
 *      Returns true if one of the three editable/modifiable fields has the
 *      keyboard focus.
 */

bool
mainwnd::edit_field_has_focus () const
{
    bool result =
    (
        m_spinbutton_bpm->has_focus() ||
        m_spinbutton_ss->has_focus()  ||
        m_entry_notes->has_focus()
    );
    return result;
}

#ifdef SEQ64_STAZED_MENU_BUTTONS

/**
 *  Sets the song mode, which is actually the JACK start mode.  If true, we
 *  are in playback/song mode.  If false, we are in live mode.  This
 *  function must be in the cpp module, where the button header file is
 *  included.
 */

void
mainwnd::set_song_mode ()
{
    bool is_active = m_button_mode->get_active();
    if (usr().use_more_icons())
    {
        set_songlive_image(is_active);
    }
    else
    {
        std::string label = is_active ? "Song" : " Live ";
        Gtk::Label * lblptr(dynamic_cast<Gtk::Label *>
        (
            m_button_mode->get_child())
        );
        if (not_nullptr(lblptr))
            lblptr->set_text(label);
    }
    perf().song_start_mode(is_active);
}

/**
 *  Toggles the song mode.  Note that calling this function will trigger the
 *  button signal callback, set_song_mode().  It only operates if the patterns
 *  are not playing.  This function must be in the cpp module, where the
 *  button header file is included.
 */

void
mainwnd::toggle_song_mode()
{
    if (! perf().is_pattern_playing())
        m_button_mode->set_active(! m_button_mode->get_active());
}

#endif

#ifdef SEQ64_STAZED_MENU_BUTTONS

/**
 *  This function must be in the cpp module, where the button header file
 *  is included.
 */

void
mainwnd::set_menu_mode ()
{
    m_menu_mode = m_button_menu->get_active();
}

/**
 *  Toggles the menu mode.  Note that calling this function will trigger the
 *  button signal callback.
 */

void
mainwnd::toggle_menu_mode ()
{
    m_button_menu->set_active(! m_button_menu->get_active());
}

#endif  // SEQ64_STAZED_MENU_BUTTONS

/**
 *  This function is the GTK timer callback, used to draw our current time
 *  and BPM on_events (the main window).  It also supports the ALSA pause
 *  functionality.
 *
 * \note
 *      When Sequencer64 first starts up, and no MIDI tune is loaded, the call
 *      to mainwid::update_markers() leads to trying to do some work on
 *      sequences that don't yet exist.  Also, if a sequence is changed by the
 *      event editor, we get a crash; need to find out how seqedit gets away
 *      with the changes.
 *
 * \return
 *      Always returns true.
 */

bool
mainwnd::timer_callback ()
{
    midipulse tick = perf().get_tick();         /* use no get_start_tick()! */
    midibpm bpm = perf().get_beats_per_minute();
    update_markers(tick);
    if (m_button_queue->get_active() != perf().is_keep_queue())
        m_button_queue->set_active(perf().is_keep_queue());

#ifdef USE_SHOW_TICK_TIME
    midi_timing mt
    (
        perf().get_beats_per_minute(),
        perf().get_beats_per_bar(),
        perf().get_beat_width(),
        perf().ppqn()
    );
    std::string t = pulses_to_measurestring(tick, mt);
    m_tick_time->set_text(t);
#endif

#ifdef SEQ64_USE_DEBUG_OUTPUT_XXX               /* TMI */
    static midibpm s_bpm = 0.0;
    if (bpm != s_bpm)
    {
        s_bpm = bpm;
        printf("BPM = %g\n", bpm);
    }
#endif

    if (m_adjust_bpm->get_value() != bpm)
        m_adjust_bpm->set_value(bpm);

    int screenset = perf().screenset();
    int newset = m_adjust_ss->get_value();
    if (newset != screenset)
    {

#if defined SEQ64_MULTI_MAINWID
        screenset = set_screenset(newset, true);    // handles wrap-around
        m_adjust_ss->set_value(screenset);
#else
        (void) set_screenset(screenset);            // newset ????
        m_adjust_ss->set_value(screenset);          // newset ????
#endif

        m_entry_notes->set_text(perf().current_screen_set_notepad());
    }

#ifdef SEQ64_STAZED_MENU_BUTTONS

    m_button_mute->set_sensitive(! perf().song_start_mode());
    if (m_button_mode->get_active() != perf().song_start_mode())
        m_button_mode->set_active(perf().song_start_mode());

    if (perf().is_pattern_playing())
    {
        if (m_button_mode->get_sensitive())
            m_button_mode->set_sensitive(false);
    }
    else
    {
        if (! m_button_mode->get_sensitive())
            m_button_mode->set_sensitive(true);
    }
    m_menubar->set_sensitive(m_menu_mode);

#endif

#ifdef SEQ64_SHOW_JACK_STATUS

    /*
     * Show JACK status in the main window, in a button that can also bring up
     * the JACK connection page from the Options dialog.
     */

    std::string label("ALSA");
    if (perf().is_jack_running())
    {
        if (rc().with_jack_master())
            label = "Master";
        else if (rc().with_jack_transport())
            label = "JACK";
    }

#ifdef SEQ64_RTMIDI_SUPPORT
    if (rc().with_jack_midi())
        label = "Native";
#endif

    /*
     * We should make this a member and optimize the change as well.
     */

    Gtk::Label * lblptr(dynamic_cast<Gtk::Label *>(m_button_jack->get_child()));
    if (not_nullptr(lblptr))
        lblptr->set_text(label);

    if (perf().is_pattern_playing())
    {
        if (m_button_jack->get_sensitive())
            m_button_jack->set_sensitive(false);
    }
    else
    {
        if (! m_button_jack->get_sensitive())
            m_button_jack->set_sensitive(true);
    }

#endif  // SEQ64_SHOW_JACK_STATUS

    /*
     * For seqroll keybinding, this is needed here instead of perfedit
     * timeout(), since perfedit may not be open all the time.
     */

    if (m_perf_edit->get_toggle_jack() != perf().get_toggle_jack())
        m_perf_edit->toggle_jack();

    if (perf().is_running() != m_is_running)
    {
        m_is_running = perf().is_running();
#ifdef SEQ64_PAUSE_SUPPORT
        set_play_image(m_is_running);
#endif
    }

#ifdef SEQ64_MAINWND_TAP_BUTTON

    if (m_current_beats > 0 && m_last_time_ms > 0)
    {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        long ms = long(spec.tv_sec) * 1000;         /* seconds to ms        */
        ms += round(spec.tv_nsec * 1.0e-6);         /* nanoseconds to ms    */
        long difference = ms - m_last_time_ms;
        if (difference > SEQ64_TAP_BUTTON_TIMEOUT)
        {
            m_current_beats = m_base_time_ms = m_last_time_ms = 0;
            set_tap_button(0);
        }
    }

#endif

    return true;
}

/**
 *  New function to consolidate screen-set handling.  Sets the active
 *  screenset to the given value.  This is use by the main Set spin-button.
 *
 * \param screenset
 *      The new prospective screen-set value.  This will become the active
 *      screen-set.
 *
 * \param setperf
 *      If true, the perform object's screen-set is modified as well.
 *      The default value is false.
 */

int
mainwnd::set_screenset (int screenset, bool setperf)
{
    return m_main_wid->set_screenset(screenset, setperf);
}

/**
 *  Updates the markers on one (or more, if multi-mainwid is enabled) mainwid
 *  objects.
 *
 * \param tick
 *      The current tick number for playback, etc.
 */

void
mainwnd::update_markers (midipulse tick)
{
#if defined SEQ64_MULTI_MAINWID
    if (multi_wid())
    {
        mainwid ** widptr = &m_mainwid_blocks[0];
        for (int wid = 0; wid < m_mainwid_count; ++wid, ++widptr)
            (*widptr)->update_markers(tick);
    }
    else
        m_main_wid->update_markers(tick);       /* tick ignored for pause   */
#else
    m_main_wid->update_markers(tick);           /* tick ignored for pause   */
#endif

    /*
     * If this is null, all bets are off.  Let's save some time.  We can
     * re-enable this check if we make the pill bar optional.
     *
     * if (not_nullptr(m_main_time))
     */

    m_main_time->idle_progress(tick);
}

/**
 *  Resets one (or more, if multi-mainwid is enabled) mainwid objects.  The
 *  reset function draws the patterns on the pixmap, and then draws the pixmap
 *  on the window.
 */

void
mainwnd::reset ()
{

#if defined SEQ64_MULTI_MAINWID
    mainwid ** widptr = &m_mainwid_blocks[0];
    for (int wid = 0; wid < m_mainwid_count; ++wid, ++widptr)
    {
        if (not_nullptr(*widptr))
            (*widptr)->reset();
    }
#else
    m_main_wid->reset();
#endif

}

/**
 *  Opens the Performance Editor (Song Editor).
 *
 *  We will let perform keep track of modifications, and not just set an
 *  is-modified flag just because we opened the song editor.  We're going to
 *  centralize the modification flag in the perform object, and see if it can
 *  work.
 */

void
mainwnd::open_performance_edit ()
{
    if (not_nullptr(m_perf_edit))
    {
        if (m_perf_edit->is_visible())
        {
            m_perf_edit->hide();
        }
        else
        {
            m_perf_edit->init_before_show();
            m_perf_edit->show_all();
        }
    }
}

/**
 *  Opens the second Performance Editor (Song Editor).  Experiment: open a
 *  second one and see what happens.  It works, but one needs to tell the
 *  other to redraw if a change is made.
 */

void
mainwnd::open_performance_edit_2 ()
{
    if (not_nullptr(m_perf_edit_2))
    {
        if (m_perf_edit_2->is_visible())
        {
            m_perf_edit_2->hide();
        }
        else
        {
            m_perf_edit_2->init_before_show();
            m_perf_edit_2->show_all();
        }
    }
}

/**
 *  This function brings together the two perfedit objects, so that they
 *  can tell each other when to queue up a draw operation.
 */

void
mainwnd::enregister_perfedits ()
{
    if (not_nullptr(m_perf_edit) && not_nullptr(m_perf_edit_2))
    {
        m_perf_edit->enregister_peer(m_perf_edit_2);
        m_perf_edit_2->enregister_peer(m_perf_edit);
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
 *  Opens the File / Options dialog to show only the JACK page.
 */

void
mainwnd::jack_dialog ()
{
    if (not_nullptr(m_options))
        delete m_options;

    m_options = new options(*this, perf(), true);
    m_options->show_all();
}

/**
 *  We are trying to work around an apparent Gtk+ bug (which occurs on my
 *  64-bit Debian Sid laptop, but not on my 32-bit Debian Jessie laptop) that
 *  causes Sequencer64 to freeze, emitting Gtk errors, if one tries to access
 *  the main menu via Alt-F, Alt-E, etc. without first moving the mouse to
 *  the main window.  Weird with a beard!
 */

void
mainwnd::on_realize ()
{
    gui_window_gtk2::on_realize();

    /*
     * None of these lines fix the Alt-F issue.
     *
     *  test_widget_click(m_button_tap->get_child()->gobj());
     *  m_main_wid->grab_focus();
     *  grab_focus();
     *  set_focus(*this);
     *  present();
     *  m_timeout_connect = Glib::signal_timeout().connect
     *  (
     *      mem_fun(*this, &mainwnd::timer_callback), redraw_period_ms()
     *  );
     *
     * set_screenset(0);           // causes a segfault
     */
}

/**
 *  This handler responds to a learn-mode change from perf().
 */

void
mainwnd::on_grouplearnchange (bool state)
{
    const char ** bitmap = state ? learn2_xpm : learn_xpm ;
    m_button_learn->set_image(*manage(new PIXBUF_IMAGE(bitmap)));
}

/**
 *  Actually does the work of setting up for a new file.  Not sure that we
 *  need to clear the modified flag here, especially since it is now
 *  centralizeed in the perform object.  Let perf().clear_all() handle it now.
 */

void
mainwnd::new_file ()
{
    if (perf().clear_all())
    {
        /*
         * TODO
         *
         *  m_perf_edit->set_bp_measure(4);
         *  m_perf_edit->set_bw(4);
         *  m_perf_edit->set_transpose(0);
         *
         * Also not sure if the reset() call is really necessary.  Commented
         * out for now.
         *
         * reset();                                // m_main_wid->reset();
         */

        m_entry_notes->set_text(perf().current_screen_set_notepad());
        rc().filename("");
        update_window_title();
    }
    else
        new_open_error_dialog();
}

/**
 *  Tells the user to close all the edit windows first.
 */

void
mainwnd::new_open_error_dialog ()
{
    std::string prompt =
        "All sequence edit windows must be closed\n"
        "before opening a new file." ;

    Gtk::MessageDialog errdialog
    (
        *this, prompt, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
    );
    errdialog.run();
}

/**
 *  Tells the user that the "rc" file is erroneous.  We can't yet display the
 *  specific error, except in a terminal window.
 *
 * \param message
 *      Provides the error message returned by the configuration file.
 */

void
mainwnd::rc_error_dialog (const std::string & message)
{
    std::string prompt;
    if (message.empty())
    {
        prompt =
            "Error found in 'rc' configuration file.  Run within\n"
            "a terminal window to see the error message." ;
    }
    else
    {
        prompt = "Error found in 'rc' configuration file:\n";
        prompt += message;
    }

    Gtk::MessageDialog errdialog
    (
        *this, prompt, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
    );
    errdialog.run();
}

/**
 *  A callback function for the File / Save As menu entry.  Please note that
 *  Sequencer64 will not adopt the "c_seq32_midi" type of file, because
 *  it already saves its files in a format that other sequencers should be
 *  able to read.
 *
 * Stazed on the intent of the export functionality:
 *
 *      The original intent was to be able to play an exported song in
 *      something like TiMIDIty. After I completed things I realized that
 *      there could be an editing benefit as well. I like to record from my
 *      MIDI keyboard, improvised to a drum beat, on a long sequence (64
 *      measures). Some is junk, but there are usually parts that I can use.
 *      In original seq24, to cut out the good or bad stuff, you would have to
 *      search the sequence by listening, then cut and move or copy and paste
 *      to a new sequence. It could be done but was always tedious.  The paste
 *      box for the sequence sometimes made it difficult to find the correct
 *      note location, measure, and beat. Also, on a long sequence, you need
 *      to zoom out to see the copy location as it played, but zoom in for the
 *      precise paste location. In addition if you wanted to change the
 *      measure of the notes, it became a trial and error of copy/paste,
 *      listen, move, listen, move....

 *      With the added Song editor feature of split trigger to mouse and copy
 *      paste trigger to mouse, you can now do all the editing from the song
 *      editor. Listen to the sequence, cut out the good or bad parts and
 *      reassemble. Move or copy all good trigger parts to the left start and
 *      delete all the bad stuff. Now you can use the song export to create
 *      the new sequence. Just mute all other tracks and export. Re-import and
 *      the new cleaned sequence is already done. Also I use it for importing
 *      drum beats from a single '32/'42 file that contains dozens of
 *      different styles with intros and endings. I like to sync two instances
 *      of '32 or '42 together with jack, then play/experiment with the
 *      different beats. If I find something I like, create the song trigger
 *      for the part I like in the drum file, export and import.
 *
 *      I actually do not use the song export for anything but editing.
 *
 *  Note that the split trigger variant of Stazed, where it doesn't just split
 *  the section in half, is not yet implemented (2016-08-05).
 *
 * \param do_export
 *      If true, then just write out the file and don't change the name of
 *      the current file based on the file-name the user selected.  The
 *      default value of this parameter is false.
 */

void
mainwnd::file_save_as (bool do_export)
{
    const char * const prompt = do_export ? "Export file as" : "Save file as" ;
    Gtk::FileChooserDialog dialog(prompt, Gtk::FILE_CHOOSER_ACTION_SAVE);
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
    dialog.set_current_folder(rc().last_used_dir());
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
            if (do_export)
            {
                midifile f(fname, ppqn());
                bool result = f.write_song(perf());     // f.write(perf());
                if (! result)
                {
                    std::string errmsg = f.error_message();
                    Gtk::MessageDialog errdialog
                    (
                        *this, errmsg, false, Gtk::MESSAGE_ERROR,
                        Gtk::BUTTONS_OK, true
                    );
                    errdialog.run();
                }
            }
            else
            {
                rc().filename(fname);
                update_window_title();
                save_file();
            }
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
 *  preserve the legacy behavior of using the global ppqn, and scaling the
 *  running time against the PPQN read from the MIDI file.  Later, we can
 *  provide a value like 0, that will certainly be changed by reading the MIDI
 *  file.
 *
 *  We don't need to specify the "oldformat" or "global sequence" parameters
 *  of the midifile constructor when reading the MIDI file, since reading
 *  handles both the old and new formats, dealing with new constructs only if
 *  they are present in the file.
 *
 * \param fn
 *      Provides the file-name for the MIDI file to be opened.
 */

void
mainwnd::open_file (const std::string & fn)
{
    midifile f(fn);                     /* create object to represent file  */
    perf().clear_all();
    bool result = f.parse(perf());      /* parsing handles old & new format */
    if (! result)
    {
        std::string errmsg = f.error_message();
        Gtk::MessageDialog errdialog
        (
            *this, errmsg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
        );
        errdialog.run();
        if (f.error_is_fatal())
            return;
    }

    ppqn(f.ppqn());                     /* get and save the actual PPQN     */
    rc().last_used_dir(fn.substr(0, fn.rfind("/") + 1));
    rc().filename(fn);
    update_window_title();

    /*
     * Also not sure if the reset() call is really necessary.
     *
     * reset();                                // m_main_wid->reset();
     */

    m_entry_notes->set_text(perf().current_screen_set_notepad());
    m_adjust_bpm->set_value(perf().get_beats_per_minute());
}

/**
 *  Creates a file-chooser dialog.
 *
 * \change layk 2016-10-11
 *      Issue #43 Added filters for upper-case MIDI-file extensions.
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
    filter_midi.add_pattern("*.MIDI");
    filter_midi.add_pattern("*.mid");
    filter_midi.add_pattern("*.MID");
    dlg.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dlg.add_filter(filter_any);
    dlg.set_current_folder(rc().last_used_dir());

    int result = dlg.run();
    if (result == Gtk::RESPONSE_OK)
        open_file(dlg.get_filename());
}

/**
 *  Saves the current state in a MIDI file.  Here we specify the current value
 *  of m_ppqn, which was set when reading the MIDI file.  We also let midifile
 *  tell the perform that saving worked, so that the "is modified" flag can be
 *  cleared.  The midifile class is already a friend of perform.
 */

bool
mainwnd::save_file ()
{
    bool result = false;
    if (rc().filename().empty())
    {
        file_save_as();
        return true;
    }

    midifile f
    (
        rc().filename(), ppqn(),
        rc().legacy_format(), usr().global_seq_feature()
    );
    result = f.write(perf());
    if (! result)
    {
        std::string errmsg = f.error_message();
        Gtk::MessageDialog errdialog
        (
            *this, errmsg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
        );
        errdialog.run();
    }
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
    if (rc().filename().empty())
        query_str = "Unnamed MIDI file was changed.\nSave changes?";
    else
        query_str = "MIDI file '" + rc().filename() +
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
    if (perf().is_modified())
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
 *  specific screen-set, for better organization, as well as being able to
 *  offset the sequence number.
 *
 *  Also, it is important to note that perf().clear_all() is not called by
 *  this routine, as we are merely adding to what might already be there.
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
    dlg.set_current_folder(rc().last_used_dir());

    Gtk::ButtonBox * btnbox = dlg.get_action_area();
    Gtk::HBox hbox(false, 2);
    m_adjust_load_offset = manage(new Gtk::Adjustment(0, 0, c_max_sets-1, 1));
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
    switch (response)                  /* handle the response */
    {
    case (Gtk::RESPONSE_OK):
    {
        std::string fn = dlg.get_filename();
        try
        {
            midifile f(fn);
            f.parse(perf(), int(m_adjust_load_offset->get_value()));
        }
        catch (...)
        {
            Gtk::MessageDialog errdialog
            (
                *this, "Error importing file: " + fn, false,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
            );
            errdialog.run();
        }
        rc().filename(std::string(dlg.get_filename()));
        update_window_title();

        /*
         * Doesn't seem to be necessary.
         *
         * reset();                                // m_main_wid->reset();
         */

        m_entry_notes->set_text(perf().current_screen_set_notepad());
        m_adjust_bpm->set_value(perf().get_beats_per_minute());
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
        if (perf().is_running())        /* \change ca 2016-03-19    */
            stop_playing();

        hide();
    }
}

/**
 *  Presents a Help / About dialog.  I (Chris) took the liberty of tacking
 *  my name at the end, and hope to have done eventually enough work to
 *  warrant having it there.  Hmmmmm....
 */

void
mainwnd::about_dialog ()
{
    std::string comment("Interactive MIDI Sequencer\n");
    Gtk::AboutDialog dialog;
    dialog.set_transient_for(*this);

    /*
     * For some reason, using the new functions causes them to be found as
     * unresolved.  Weird.
     */

#if USE_PROBLEMATICLY_LINKED_FUNCTION
    std::string apptag = seq_app_name();
    apptag += " ";
    apptag += seq_version();
#else
    std::string apptag = SEQ64_APP_NAME " " SEQ64_VERSION; // "\n";
#endif

    dialog.set_name(apptag);
    dialog.set_version
    (
        "\n" SEQ64_VERSION_DATE_SHORT "\n" SEQ64_GIT_VERSION "\n"
    );
    if (rc().legacy_format())
        comment += "Using original seq24 format (legacy mode)\n";
    else
        comment += "Derived from seq24\n";

    dialog.set_comments(comment);
    dialog.set_copyright
    (
        "(C) 2002-2006 Rob C. Buse (seq24)\n"
        "(C) 2008-2016 Seq24team (seq24)\n"
        "(C) 2015-2017 Chris Ahlstrom (sequencer64/seq64)"
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
    list_authors.push_back("Stan Preston <stazed10@gmail.com>");
    list_authors.push_back("Chris Ahlstrom <ahlstromcj@gmail.com>");
    dialog.set_authors(list_authors);

    std::list<std::string> list_documenters;
    list_documenters.push_back("Dana Olson <seq24@ubuntustudio.com>");
    list_documenters.push_back("Chris Ahlstrom <ahlstromcj@gmail.com>:");
    list_documenters.push_back("<https://github.com/ahlstromcj/seq24-doc.git>");
    list_documenters.push_back
    (
        "<https://github.com/ahlstromcj/sequencer64-doc.git>"
    );
    dialog.set_documenters(list_documenters);
    dialog.show_all_children();
    dialog.run();
}

/**
 *  Presents a Help / Build Info dialog.  It is similar to the "--version"
 *  option on the command line.  The AboutDialog doesn't seem to have a way to
 *  left-align the text, so we're trying the MessageDialog.
 */

void
mainwnd::build_info_dialog ()
{
#ifdef USE_ABOUT_DIALOG
    std::string comment("\n");
    comment += build_details();
    Gtk::AboutDialog dialog;
    dialog.set_transient_for(*this);
    dialog.set_name(SEQ64_PACKAGE_NAME " " SEQ64_VERSION "\n");
    dialog.set_comments(comment);
    dialog.show_all_children();
#else
    std::string caption(SEQ64_PACKAGE_NAME " " SEQ64_VERSION);
    std::string comment = build_details();
    std::string junk("JUNK");
    Gtk::MessageDialog dialog
    (
        *this, junk, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true
    );
    dialog.set_title("Sequencer64 Build Info");
    dialog.set_message(caption);
    dialog.set_secondary_text(comment);
#endif
    dialog.run();
}

/**
 *  This function is the callback for adjusting the active screen-set value.
 *  Its sets the active screen-set value in the Performance/Song window, the
 *  Patterns, and something about setting the text based on a screen-set
 *  notepad from the Performance/Song window.  We let the perform object keep
 *  track of modifications.
 *
 * Multi-mainwid mode:
 *
 *  This status is flagged by the multi_wid() function.  If true, the we want
 *  the Set spin-button to affect the active sequence and all sequences.
 *  Let's start Seq64 in 2 columns x 3 rows mode.  The mainwid slots are set
 *  up so that, like the screen-sets, the row index varies fastest.  Here is
 *  the layout:
 *
\verbatim
        Column          0          1
        Row     0     [0,0]      [1,0]
        Row     1     [0,1]      [1,1]
        Row     2     [0,2]      [1,2]
\endverbatim
 *
 *  This two-dimensional array can be access by stepping along a
 *  one-dimensional array, with the one-dimensional array having an index we
 *  will call "slot number" for this description:
 *
\verbatim
      1-D index       2-D indices
        [0]             [0,0]
        [1]             [0,1]
        [2]             [0,2]
        [3]             [1,0]
        [4]             [1,1]
        [5]             [1,2]
\endverbatim
 *
 *  When the application starts, the "slot numbers" in the left column
 *  correspond exactly to set numbers.  The 0th set is the "active" set.
 *  When the Set spinner is incremented, the 1st set should become the active
 *  one.  As it is incremented, the active slot will move from top-to-bottom
 *  and then left-to-right, until the number of visible mainwids is reached.
 *  In that case, we want to then increment the set number for all of the
 *  mainwids in the matrix, so that they move down one row, and slot 0 now
 *  holds the active set.  Or, for a direct edit, move that set directly to
 *  slot 0.
 *
 *  For now, let's just start from slot 0 and set the values of all slots, and
 *  worry about preserving relative set-numbers later.
 */

void
mainwnd::adj_callback_ss ()
{
    if (multi_wid())
    {
        int newset = int(m_adjust_ss->get_value());
        if (newset <= spinner_max())
        {
            perf().set_screenset(newset);   /* set active screen-set    */
#if defined SEQ64_MULTI_MAINWID
            for (int block = 0; block < m_mainwid_count; ++block)
                m_mainwid_blocks[block]->set_screenset(newset + block);
#endif
        }
    }
    else
    {
        set_screenset(int(m_adjust_ss->get_value()), true);
        m_entry_notes->set_text(perf().current_screen_set_notepad());
    }
    m_main_wid->grab_focus();               /* allows hot-keys to work  */
}

/**
 *  This function is the callback for adjusting the BPM value.
 *  Let the perform object keep track of modifications.
 */

void
mainwnd::adj_callback_bpm ()
{
    perf().set_beats_per_minute(midibpm(m_adjust_bpm->get_value()));
    if (m_is_tempo_recording)
        (void) perf().log_current_tempo();
}

#if defined SEQ64_MULTI_MAINWID

/**
 *  This function is the callback for adjusting the screen-set value of a
 *  particular mainwid.  Note that we have to actually set the perform
 *  object's screen-set, to get its screen-set offsets in place, before
 *  logging the changes to the mainwid.
 *
 * \param widblock
 *      This parameter ranges from 0 to 5, depending on how many mainwids are
 *      created.  This function operates only when multiple mainwids are
 *      active.  Just to be clear, index 0 is slot [0, 0], 1 is slot [0, 1],
 *      etc.  The row index varies fastest, just like the seq-number does in a
 *      set.
 */

void
mainwnd::adj_callback_wid (int widblock)
{
    if (widblock < m_mainwid_count)
    {
        if (independent())
        {
            Gtk::Frame * fslot = m_mainwid_frames[widblock];
            int newset = m_mainwid_adjustors[widblock]->get_value();
            std::string label = "   Set ";
            label += std::to_string(newset);
            if (widblock == 0)
                perf().set_screenset(newset);                   /* first    */

            m_mainwid_blocks[widblock]->log_screenset(newset);  /* second   */
            if (newset == perf().screenset())
            {
                fslot->set_shadow_type(Gtk::SHADOW_OUT);
                label += " [active]";
            }
            fslot->set_label(label);
            m_main_wid->grab_focus();           /* allows hot-keys to work  */
        }
        else
            adj_callback_ss();
    }
}

#endif  // SEQ64_MULTI_MAINWID

/**
 *  A callback function for handling an edit to the screen-set notepad.
 *  Let the perform object keep track of modifications.
 */

void
mainwnd::edit_callback_notepad ()
{
    const std::string & text = m_entry_notes->get_text();
    perf().set_screen_set_notepad(text);
}

/**
 *  Changes the image used for the pause/play button.  Is this a memory leak?
 *
 * \param isrunning
 *      If true, set the image to the "Pause" icon, since playback is running.
 *      Otherwise, set it to the "Play" button, since playback is not running.
 */

void
mainwnd::set_play_image (bool isrunning)
{
    delete m_image_play;
    if (isrunning)
    {
        m_image_play = manage(new PIXBUF_IMAGE(pause_xpm));
        add_tooltip(m_button_play, "Pause playback at the current location.");
    }
    else
    {
        m_image_play = manage(new PIXBUF_IMAGE(play2_xpm));
        add_tooltip(m_button_play, "Resume playback from the current location.");
    }
    m_button_play->set_image(*m_image_play);
}

/**
 *  Changes the image used for the song/live mode button
 *
 * \param issong
 *      If true, set the image to the "Song" icon.
 *      Otherwise, set it to the "Live" button.
 */

void
mainwnd::set_songlive_image (bool issong)
{
    delete m_image_songlive;
    if (issong)
    {
        m_image_songlive = manage(new PIXBUF_IMAGE(song_mode_xpm));
        add_tooltip
        (
            m_button_mode,
            "The Song playback mode is active, and will apply no matter what "
            "window (song, pattern, and main) is used to start the playback."
        );
    }
    else
    {
        m_image_songlive = manage(new PIXBUF_IMAGE(live_mode_xpm));
        add_tooltip
        (
            m_button_mode,
            "The Live playback mode is active. If playback is started from "
            "the Song Editor, this setting is ignored, to preserve legacy "
            "behavior."
        );
    }
    m_button_mode->set_image(*m_image_songlive);
}

#ifdef SEQ64_STAZED_TRANSPOSE

/**
 *  Apply full song transposition, if enabled.  Then reset the perfedit
 *  transpose setting to 0.
 */

void
mainwnd::apply_song_transpose ()
{
    if (perf().get_transpose() != 0)
    {
        perf().apply_song_transpose();
        m_perf_edit->set_transpose(0);
    }
}

#endif

/**
 *  Reload all mute-group settings from the "rc" file.
 */

void
mainwnd::reload_mute_groups ()
{
    std::string errmessage;
    bool result = perf().reload_mute_groups(errmessage);
    if (! result)
    {
        // display the error-message
    }
}

/**
 *  Clear all mute-group settings.  Sets all values to false/zero.  Also,
 *  since the intent might be to clean up the MIDI file, the user is prompted
 *  to save.
 */

void
mainwnd::clear_mute_groups ()
{
    if (perf().clear_mute_groups())     /* did any mute statuses change?    */
    {
        if (is_save())
        {
            if (perf().is_running())    /* \change ca 2016-03-19            */
                stop_playing();
        }
    }
}

/**
 *  Starts playing of the song.  An accessor to perform::start_playing().
 *  This function is actually a callback for the pause/play button.
 *  Now very similar to perfedit::start_playing(), except that the implicit
 *  songmode == false parameter is used here.
 *
 *  We still need to see if pause_key() is workable with Stazed JACK support
 *  in force.  Doesn't pause at present.
 */

void
mainwnd::start_playing ()                           /* Play!                */
{
    perf().pause_key();                             /* not start_key()      */
}

/**
 *  Pauses the playing of the song, leaving the progress bar where it stopped.
 *  Currently, it is just the same as stop_playing(), but we will get it to
 *  work.
 */

void
mainwnd::pause_playing ()                           /* Stop in place!       */
{
    perf().pause_key();
}

/**
 *  Stops the playing of the song.  An accessor to perform's stop_playing()
 *  function.  Also calls the mainwid::update_sequences_on_window() function.
 *  Not sure that we need this call, since the slots seem to update anyway.
 *  But we've noticed that, with this call in place, hitting the Stop button
 *  causes a subtle change in the appearance of the first non-empty pattern of
 *  the "allofarow.mid" file.
 *
 *  After the Stop button is pushed (in ALSA mode), then the Space key
 *  ("start") doesn't work properly.  The song starts, then quickly stops.  It
 *  doesn't matter if update_sequences_on_window() is called or not.  This
 *  happens even in seq24!  This bug has proven incredibly difficult to track
 *  down, still working on it.
 */

void
mainwnd::stop_playing ()                        /* Stop!                    */
{
    perf().stop_key();                          /* make sure it's seq32able */

#if defined SEQ64_MULTI_MAINWID
    mainwid ** widptr = &m_mainwid_blocks[0];
    for (int wid = 0; wid < m_mainwid_count; ++wid, ++widptr)
    {
        (*widptr)->update_sequences_on_window();
    }
#else
    m_main_wid->update_sequences_on_window();   /* update_mainwid_sequences() */
#endif
}

/**
 *  Reverses the state of playback.  Meant only to be called when the "Play"
 *  button is pressed, if the pause feature has been compiled into the
 *  application.
 */

void
mainwnd::toggle_playing ()
{
    if (perf().is_running())
    {
        perf().stop_key();
    }
    else
    {
        perf().start_key();
    }
}

#ifdef SEQ64_MAINWND_TAP_BUTTON

/**
 *  Implements the Tap button or Tap keystroke (currently hard-wired as F9).
 */

void
mainwnd::tap ()
{
    midibpm bpm = update_bpm();
    set_tap_button(m_current_beats);
    if (m_current_beats > 1)                    /* first one is useless */
        m_adjust_bpm->set_value(double(bpm));
}

/**
 *  Sets the label in the Tap button to the given number of taps.
 *
 * \param beats
 *      The current number of times the user has clicked the Tap button/key.
 */

void
mainwnd::set_tap_button (int beats)
{
    Gtk::Label * tapptr(dynamic_cast<Gtk::Label *>(m_button_tap->get_child()));
    if (not_nullptr(tapptr))
    {
        char temp[8];
        snprintf(temp, sizeof temp, "%d", beats);
        tapptr->set_text(temp);
    }
}

/**
 *  Calculates the running BPM value from the user's sequences of taps.
 *
 * \return
 *      Returns the current BPM value.
 */

midibpm
mainwnd::update_bpm ()
{
    midibpm bpm = 0.0;
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long ms = long(spec.tv_sec) * 1000;     /* seconds to milliseconds      */
    ms += round(spec.tv_nsec * 1.0e-6);     /* nanoseconds to milliseconds  */
    if (m_current_beats == 0)
    {
        m_base_time_ms = ms;
        m_last_time_ms = 0;
    }
    else if (m_current_beats >= 1)
    {
        int diffms = ms - m_base_time_ms;
        bpm = m_current_beats * 60000.0 / diffms;
        m_last_time_ms = ms;
    }
    ++m_current_beats;
    return bpm;
}

#endif      // SEQ64_MAINWND_TAP_BUTTON

/**
 *  Logs the current tempo/tick value as a Set Tempo event.
 */

void
mainwnd::tempo_log ()
{
    (void) perf().log_current_tempo();  // TODO:  check the return value
}

/**
 *  Toggles the recording of the tempo.
 */

void
mainwnd::toggle_tempo_record ()
{
    m_is_tempo_recording = ! m_is_tempo_recording;
    if (m_is_tempo_recording)
    {
        Gtk::Image * image_tempo = manage(new PIXBUF_IMAGE(tempo_rec_on_xpm));
        m_button_tempo_record->set_image(*image_tempo);
    }
    else
    {
        Gtk::Image * image_tempo = manage(new PIXBUF_IMAGE(tempo_rec_off_xpm));
        m_button_tempo_record->set_image(*image_tempo);
    }
}

/**
 *  Implements the keep-queue button.
 */

void
mainwnd::queue_it ()
{
    bool is_active = m_button_queue->get_active();
    perf().set_keep_queue(is_active);
}

/**
 *  Overwrites the text in the set-notes field, for debugging purposes only.
 *
 * \param tag
 *      Human-readable text to show the context of the message.  Keep it well
 *      under 80 characters.
 *
 * \param value
 *      An integer value to be shown.
 */

void
mainwnd::debug_text (const std::string & tag, int value)
{
    char temp[80];
    snprintf(temp, sizeof temp, "%s: %d", tag.c_str(), value);
    m_entry_notes->set_text(temp);
}

/**
 *  This callback function handles a delete event from ...?
 *
 *  Any changed data is saved.  If the pattern is playing, then it is
 *  stopped.  We now use perform::is_pattern_playing().
 */

bool
mainwnd::on_delete_event (GdkEventAny * /*ev*/)
{
    bool result = is_save();
    if (result && perf().is_pattern_playing())
        stop_playing();

    return ! result;
}

/**
 *  Handles a key release event.  Is this worth turning into a switch
 *  statement?  Or offloading to a perform member function?  The latter.
 *  Also, we now effectively press the CAPS LOCK key for the user if in
 *  group-learn mode.  The function that does this is keystroke::shift_lock().
 *
 * \todo
 *      Test this functionality in old and new application.
 *
 * \return
 *      Always returns false.  This matches seq24 behavior.
 */

bool
mainwnd::on_key_release_event (GdkEventKey * ev)
{
    keystroke k(ev->keyval, SEQ64_KEYSTROKE_RELEASE);
    if (perf().is_group_learning())
        k.shift_lock();

    (void) perf().mainwnd_key_event(k);
    return false;
}

/**
 *  Handles a key press event.  It also handles the control-key and
 *  modifier-key combinations matching the entries in its list of if
 *  statements.
 *
 *  Also, we now effectively press the CAPS LOCK key for the user if in
 *  group-learn mode, via the keystroke::shift_lock() function.
 */

bool
mainwnd::on_key_press_event (GdkEventKey * ev)
{
    bool result = false;

    /*
     * We need to do this near the end of this function:
     * Gtk::Window::on_key_press_event(ev);
     */

    if (CAST_EQUIVALENT(ev->type, SEQ64_KEY_PRESS))
    {
        keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS);
        if (perf().is_group_learning())
            k.shift_lock();

        if (rc().print_keys())
        {
            printf("key_press[%d]\n", k.key());
            fflush(stdout);
        }

        /*
         * TODO:  Why is this called on both press and release???
         */

        if (! perf().mainwnd_key_event(k))
        {
            if (k.key() == PREFKEY(bpm_dn))
            {
                midibpm newbpm = perf().decrement_beats_per_minute();
                m_adjust_bpm->set_value(double(newbpm));
            }
            else if (k.key() == PREFKEY(bpm_up))
            {
                midibpm newbpm = perf().increment_beats_per_minute();
                m_adjust_bpm->set_value(double(newbpm));
            }

            /*
             * Replace, Queue, Snapshot 1, Snapshot 2, Set-Playing-Screenset,
             * Group On, Group Off, Group Learn key handling moved to a
             * perform function.
             */

            if (k.key() == PREFKEY(screenset_dn) || k.key() == SEQ64_Page_Down)
            {
                int newss = perf().decrement_screenset();
                set_screenset(newss);
                m_adjust_ss->set_value(newss);
                m_entry_notes->set_text(perf().current_screen_set_notepad());
            }
            else if (k.key() == PREFKEY(screenset_up) || k.key() == SEQ64_Page_Up)
            {
                int newss = perf().increment_screenset();
                set_screenset(newss);
                m_adjust_ss->set_value(newss);
                m_entry_notes->set_text(perf().current_screen_set_notepad());
            }
#ifdef SEQ64_MAINWND_TAP_BUTTON
            else if (k.key() == PREFKEY(tap_bpm))
            {
                tap();
            }
#endif
#ifdef SEQ64_STAZED_MENU_BUTTONS
            else if (k.key() == PREFKEY(toggle_mutes))
            {
                /*
                 * TODO: SEQ64_MULTI_MAINWND
                 */

                m_main_wid->toggle_playing_tracks();
            }
            else if (k.key() == PREFKEY(song_mode))
            {
                toggle_song_mode();
            }
            else if (k.key() == PREFKEY(menu_mode))
            {
                toggle_menu_mode();
            }
#endif
        }

        /*
         * Need to change ev->keyvalue to k.key() in all of these.
         */

        int count = perf().get_key_groups().count(k.key());
        if (count != 0)
        {
            /*
             * activate mute group key
             */

            int group = perf().lookup_keygroup_group(k.key());
            perf().select_and_mute_group(group);
        }

        bool mgl = perf().is_group_learning() && k.key() != PREFKEY(group_learn);
        if (mgl)                                /* mute group learn         */
        {
            if (perf().get_key_groups().count(k.key()) != 0)
            {
                std::ostringstream os;
                os
                    << "Key '" << keyval_name(k.key())   // gdk_
                    << "' (code = " << k.key() << ") successfully mapped."
                   ;

                Gtk::MessageDialog dialog
                (
                    *this, "MIDI mute group learn success", false,
                    Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true
                );
                dialog.set_secondary_text(os.str(), false);
                dialog.run();

                /*
                 * Missed the key-up group-learn message, so force it to off.
                 */

                perf().unset_mode_group_learn();
            }
            else
            {
                std::ostringstream os;
                os
                    << "Key '" << keyval_name(k.key())
                    << "' (code = " << k.key()
                    << ") is not one of the configured mute-group keys. "
                    << "To change it, see File/Options menu or the rc file."
                   ;

                Gtk::MessageDialog dialog
                (
                    *this, "MIDI mute group learn failed", false,
                    Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
                );
                dialog.set_secondary_text(os.str(), false);
                dialog.run();

                /*
                 * Missed the key-up message for group-learn, so force it off.
                 */

                perf().unset_mode_group_learn();
            }
        }
        if (! perf().playback_key_event(k))
        {
            /*
             *  Toggle the sequence mute/unmute setting using keyboard keys.
             *  However, do not do this if the Ctrl key is being pressed.
             *  Ctrl-E, for example, brings up the Song Editor, and should not
             *  toggle the sequence controlled by the "e" key.  Will also see
             *  if the Alt key could/should be intercepted.
             *
             *  Also, try to avoid using the hotkeys if an editable field has
             *  focus.
             *
             *  Also, if the pattern-edit keys (will be minus and equals by
             *  default) are enabled and have been pressed, then bring up one
             *  of the editors (pattern or event) when the slot shortcut key
             *  is pressed.
             *
             *  Finally, as a new feature, if the pattern-shift key (the
             *  forward slash by default) is pressed, increment the flag that
             *  indicates an extended sequence (value + 32 [c_seqs_in_set] or
             *  value + 64) will be toggled, instead of the normal sequence.
             */

            if (perf().get_key_events().count(k.key()) != 0)
            {
                bool ok = perf().is_control_status();
                if (! ok)
                    ok = ! is_ctrl_key(ev);

                if (ok)
                    ok = ! edit_field_has_focus();

                if (ok)
                {
                    int seqnum = perf().lookup_keyevent_seq(k.key());
                    if (m_call_seq_edit)
                    {
                        m_call_seq_edit = false;
                        m_main_wid->seq_set_and_edit(seqnum);
                        result = true;
                    }
                    else if (m_call_seq_eventedit)
                    {
                        m_call_seq_eventedit = false;
                        m_main_wid->seq_set_and_eventedit(seqnum);
                        result = true;
                    }
                    else if (m_call_seq_shift > 0)      /* variset support  */
                    {
                        int keynum = seqnum + m_call_seq_shift * c_seqs_in_set;
                        sequence_key(keynum);
                        result = true;
                    }
                    else
                    {
                        sequence_key(seqnum);           /* toggle sequence  */
                        result = true;
                    }
                }
            }
            else
            {
                if (k.key() == PREFKEY(pattern_edit))
                {
                    m_call_seq_edit = ! m_call_seq_edit;
                }
                else if (k.key() == PREFKEY(pattern_shift))
                {
                    ++m_call_seq_shift;
                    if (m_call_seq_shift == 3)
                        m_call_seq_shift = 0;

                    std::string temp = "";
                    for (int i = 0; i < m_call_seq_shift; ++i)
                        temp += '/';

                    set_status_text(temp);
                }
                else if (k.key() == PREFKEY(event_edit))
                {
                    m_call_seq_eventedit = ! m_call_seq_eventedit;
                }
                else
                {
                    m_call_seq_edit = m_call_seq_eventedit = false;

                    /*
                     * Ctrl-L to toggle the group-learn mode.  Similar to
                     * Ctrl-E to bring up the Song Editor.  Ctrl-J to bring up
                     * the new JACK connection page.
                     */

                    bool ok = is_ctrl_key(ev);
                    if (ok)
                    {
                        if (k.key() == SEQ64_l)
                            perf().learn_toggle();
                        else if (k.key() == SEQ64_p)
                            jack_dialog();
                    }
                }
            }
        }
    }
    (void) Gtk::Window::on_key_press_event(ev);
    return result;
}

#if defined SEQ64_JE_PATTERN_PANEL_SCROLLBARS

/**
 *  Handles left, right, up, and down scroll events.
 *
 * \param ev
 *      Provides the event to process.
 *
 * \return
 *      Always returns true.
 */

bool
mainwnd::on_scroll_event (GdkEventScroll * ev)
{
    if
    (
        ev->direction == GDK_SCROLL_LEFT  ||
        ev->direction == GDK_SCROLL_RIGHT || is_shift_key(ev)
    )
    {
        double v =
            ev->direction == GDK_SCROLL_LEFT || ev->direction == GDK_SCROLL_UP ?
                m_hadjust->get_value() - m_hadjust->get_step_increment() :
                m_hadjust->get_value() + m_hadjust->get_step_increment() ;

        m_hadjust->clamp_page(v, v + m_hadjust->get_page_size());
    }
    else if (ev->direction == GDK_SCROLL_UP || ev->direction == GDK_SCROLL_DOWN)
    {
        double v =
            ev->direction == GDK_SCROLL_UP ?
                m_vadjust->get_value() - m_vadjust->get_step_increment() :
                m_vadjust->get_value() + m_vadjust->get_step_increment() ;

        m_vadjust->clamp_page(v, v + m_vadjust->get_page_size());
    }
    return true;
}

/**
 *  Handles the resizing of the scroll-bars when the main window is resized.
 *
 * \param ev
 *      Provides the event to process.
 */

void
mainwnd::on_scrollbar_resize ()
{
    int bar = m_vscroll->get_allocation().get_width() + 5;
    bool h_visible = (m_vscroll->get_visible() ? bar : 0) <
        m_hadjust->get_upper() - m_hadjust->get_page_size();

    bool v_visible = (m_hscroll->get_visible() ? bar : 0) <
        m_vadjust->get_upper() - m_vadjust->get_page_size();

    if (m_hscroll->get_visible() != h_visible)
        m_hscroll->set_visible(h_visible);

    if (m_vscroll->get_visible() != v_visible)
        m_vscroll->set_visible(v_visible);
}

#endif  // SEQ64_JE_PATTERN_PANEL_SCROLLBARS

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
    int ppqn = choose_ppqn(m_ppqn);
    char temp[16];
    snprintf(temp, sizeof temp, " (%d ppqn) ", ppqn);
    if (! rc().filename().empty())
    {
        std::string name = shorten_file_spec(rc().filename(), 56);
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
    if (write(sm_sigpipe[1], &sig, sizeof(sig)) == -1)
        printf("signal write() failed: %s\n", std::strerror(errno));
}

/**
 *  Installs the signal handlers and pipe code.
 */

bool
mainwnd::install_signal_handlers ()
{
    sm_sigpipe[0] = -1;         /* initialize this static array             */
    sm_sigpipe[1] = -1;
    if (pipe(sm_sigpipe) < 0)   /* pipe to forward received system signals  */
    {
        printf("pipe() failed: %s\n", std::strerror(errno));
        return false;
    }
    Glib::signal_io().connect   /* notifier to handle pipe messages         */
    (
        sigc::mem_fun(*this, &mainwnd::signal_action), sm_sigpipe[0], Glib::IO_IN
    );

    struct sigaction action;    /* install signal handlers                  */
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
        if (read(sm_sigpipe[0], &message, sizeof(message)) == -1)
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

/**
 *  Populates the File menu: File menu items; their accelerator keys; and their
 *  hot keys.  Provided to make the constructor more readable and manageable.
 */

void
mainwnd::populate_menu_file ()
{
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "_New", Gtk::AccelKey("<control>N"),    /* Ctrl-N does nothing! */
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
            "Save _as...", Gtk::AccelKey("<control><shift>S"),
            sigc::bind(mem_fun(*this, &mainwnd::file_save_as), false)
        )
    );
    m_menu_file->items().push_back(SeparatorElem());
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "_Import MIDI...", Gtk::AccelKey("<control>I"),
            mem_fun(*this, &mainwnd::file_import_dialog)
        )
    );

    /*
     * Export means to write out the song as a standard MIDI file based on the
     * triggers shown in the performance window.
     */

    m_menu_file->items().push_back
    (
        MenuElem
        (
            "E_xport song as MIDI...", Gtk::AccelKey("<control><shift>I"),
            sigc::bind(mem_fun(*this, &mainwnd::file_save_as), true)
        )
    );
    m_menu_file->items().push_back(SeparatorElem());
    m_menu_file->items().push_back
    (
        MenuElem
        (
            "O_ptions...", Gtk::AccelKey("<control>B"),
            mem_fun(*this, &mainwnd::options_dialog)
        )
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
}

/**
 *  Populates the Edit menu: Edit menu items; their accelerator keys; and their
 *  hot keys.  Provided to make the constructor more readable and manageable.
 */

void
mainwnd::populate_menu_edit ()
{
    m_menu_edit->items().push_back
    (
        MenuElem
        (
            "_Song Editor...", Gtk::AccelKey("<control>E"),
            mem_fun(*this, &mainwnd::open_performance_edit)
        )
    );

#ifdef SEQ64_STAZED_TRANSPOSE

    m_menu_edit->items().push_back
    (
        MenuElem
        (
            "_Apply song transpose",
            mem_fun(*this, &mainwnd::apply_song_transpose)
        )
    );

#endif

    m_menu_edit->items().push_back
    (
        MenuElem
        (
            "_Clear mute groups",
            mem_fun(*this, &mainwnd::clear_mute_groups)
        )
    );

    m_menu_edit->items().push_back
    (
        MenuElem
        (
            "_Reload mute groups",
            mem_fun(*this, &mainwnd::reload_mute_groups)
        )
    );

    m_menu_edit->items().push_back(SeparatorElem());

    m_menu_edit->items().push_back
    (
        MenuElem("_Mute all tracks",
        sigc::bind(mem_fun(*this, &mainwnd::set_song_mute), perform::MUTE_ON))
    );
    m_menu_edit->items().push_back
    (
        MenuElem("_Unmute all tracks",
        sigc::bind(mem_fun(*this, &mainwnd::set_song_mute), perform::MUTE_OFF))
    );
    m_menu_edit->items().push_back
    (
        MenuElem("_Toggle mute all tracks",
        sigc::bind(mem_fun(*this, &mainwnd::set_song_mute), perform::MUTE_TOGGLE))
    );
}

/**
 *  Populates the View menu: View menu items and their hot keys.  It repeats
 *  the song editor edit command, just to help those whose muscle memory is
 *  already seq32-oriented.  Provided to make the constructor more readable and
 *  manageable.
 */

void
mainwnd::populate_menu_view ()
{
    m_menu_view->items().push_back
    (
        MenuElem
        (
            "_Song Editor toggle...", Gtk::AccelKey("<control>E"),
            mem_fun(*this, &mainwnd::open_performance_edit)
        )
    );

    /**
     * View menu items and their hot keys.
     */

    if (not_nullptr(m_perf_edit_2))
    {
        m_menu_view->items().push_back
        (
            MenuElem
            (
                "Song Editor _2 toggle...", // Gtk::AccelKey("<control>F"),
                mem_fun(*this, &mainwnd::open_performance_edit_2)
            )
        );
        enregister_perfedits();
    }
}

/**
 *  Populates the Help menu.  Provided to make the constructor more readable
 *  and manageable.
 */

void
mainwnd::populate_menu_help ()
{
    m_menu_help->items().push_back
    (
        MenuElem("_About...", mem_fun(*this, &mainwnd::about_dialog))
    );

    m_menu_help->items().push_back
    (
        MenuElem
        (
            "_Build Info...", mem_fun(*this, &mainwnd::build_info_dialog)
        )
    );
}

/**
 *  Use the sequence key to toggle the playing of an active pattern in
 *  the current screen-set.
 */

void
mainwnd::sequence_key (int seq)
{
    m_call_seq_shift = 0;               /* flag now done, if in force   */
    set_status_text(std::string(""));
    perf().sequence_key(seq);
}

/**
 *  Sets the text on the new status label.
 *
 * \param text
 *      Provides the short (6 characters in the default state) string to 
 *      set the label.
 */

void
mainwnd::set_status_text (const std::string & text)
{
    m_status_label->set_text(text);
}

}           /* namespace seq64 */

/*
 * mainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

