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
 * \file          options.cpp
 *
 *  This module declares/defines the base class for the File / Options
 *  dialog.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-05-19
 * \license       GNU GPLv2 or above
 *
 *  Here is a list of the global variables used/stored/modified by this
 *  module:
 *
 *      -   c_max_sequence
 *      -   e_fruity_interaction and e_seq24_interation
 *      -   e_clock_off, e_clock_pos, e_clock_mod, and e_clock_disabled
 *      -   e_keylabelsonsequence and e_keylabelsonsequence
 *      -   e_jack_transport, e_jack_master,
 *          e_jack_master_cond, e_jack_master_connect,
 *          e_jack_master_disconnect, e_jack_master_song_mode
 */

#include <sstream>

#include <gtkmm/checkbutton.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>

#if GTK_MINOR_VERSION < 12
#include <gtkmm/tooltips.h>
#endif

#include <sigc++/bind.h>

#include "globals.h"
#include "gtk_helpers.h"
#include "keybindentry.hpp"
#include "options.hpp"
#include "perform.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()  */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The principal constructor for the options class creates a number of
 *  dialog elements.
 *
 * \param parent
 *      The parent window of the options dialog.
 *
 * \param p
 *      The parent performance object.
 *
 * \param showjack
 *      If true, show only the JACK page, for quick access.  The default value
 *      is false.
 */

options::options
(
    Gtk::Window & parent,
    perform & p,
    bool showjack
) :
    Gtk::Dialog                     ("Options", parent, true, true),
#if GTK_MINOR_VERSION < 12
    m_tooltips                      (manage(new Gtk::Tooltips()),
#endif
    m_mainperf                      (p),            /* accessed via perf() */
    m_button_ok                     (manage(new Gtk::Button(Gtk::Stock::OK))),
    m_button_jack_transport
    (
        manage(new Gtk::CheckButton("JACK _Transport", true))
    ),
    m_button_jack_master
    (
        manage(new Gtk::CheckButton("Trans_port Master", true))
    ),
    m_button_jack_master_cond
    (
        manage(new Gtk::CheckButton("Master C_onditional", true))
    ),
#ifdef SEQ64_RTMIDI_SUPPORT
    m_button_jack_midi
    (
        manage
        (
            new Gtk::CheckButton("Native JACK _MIDI (requires a restart)", true)
        )
    ),
#endif
    m_button_jack_connect
    (
        manage(new Gtk::Button("JACK Transport Co_nnect", true))
    ),
    m_button_jack_disconnect
    (
        manage(new Gtk::Button("JACK Transport _Disconnect", true))
    ),
    m_notebook                      (manage(new Gtk::Notebook()))
{
    Gtk::HBox * hbox = manage(new Gtk::HBox());
    get_vbox()->pack_start(*hbox, false, false);
    get_action_area()->set_border_width(2);
    hbox->set_border_width(6);
    get_action_area()->pack_end(*m_button_ok, false, false);
    m_button_ok->signal_clicked().connect(mem_fun(*this, &options::hide));
    hbox->pack_start(*m_notebook);
    if (showjack)
    {
        add_jack_sync_page();
    }
    else
    {
        add_midi_clock_page();
        add_midi_input_page();
        add_keyboard_page();
        if (! rc().legacy_format())
            add_extended_keys_page();

        add_mouse_page();
        add_jack_sync_page();
    }
}

/**
 *  Adds the MIDI Clock page (tab) to the Options dialog.
 *  It counts the MIDI Output clock busses, among other things.
 */

void
options::add_midi_clock_page ()
{
    Gtk::VBox * vbox = manage(new Gtk::VBox());
    vbox->set_border_width(6);
    m_notebook->append_page(*vbox, "MIDI _Clock", true);

    Gtk::Frame * inputframe = manage(new Gtk::Frame("Clocks"));
    inputframe->set_border_width(4);
    vbox->pack_start(*inputframe, Gtk::PACK_SHRINK);

    Gtk::VBox * inputbox = manage(new Gtk::VBox());
    inputbox->set_border_width(4);
    inputframe->add(*inputbox);

#if GTK_MINOR_VERSION < 12
    manage(new Gtk::Tooltips());
#endif

    int buses = perf().master_bus().get_num_out_buses();
    for (int bus = 0; bus < buses; ++bus)
    {
        Gtk::HBox * hbox2 = manage(new Gtk::HBox());

#ifdef USE_MIDI_CLOCK_CONNECT_BUTTON        // NOT YET READY

        Gtk::CheckButton * check = manage
        (
            new Gtk::CheckButton
            (
                perf().master_bus().get_midi_out_bus_name(bus), 0
            )
        );
        add_tooltip
        (
            check,
            "Select (click/space-bar) to connect/disconnect this MIDI output."
        );
        check->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::output_callback), bus, check)
        );
        check->set_active(perf().get_output(bus));      // ???
        check->set_sensitive(false);                    // FOR NOW

#else

        std::string txt = perf().master_bus().get_midi_out_bus_name(bus);
        Gtk::Label * label = manage(new Gtk::Label(txt, 0));
        hbox2->pack_start(*label, false, false);

#endif  // USE_MIDI_CLOCK_CONNECT_BUTTON

        Gtk::RadioButton * rb_off = manage(new Gtk::RadioButton("Off"));
        add_tooltip
        (
            rb_off,
            "MIDI Clock will be disabled. Used for conventional playback."
        );

        Gtk::RadioButton * rb_on = manage(new Gtk::RadioButton("On (Pos)"));
        add_tooltip
        (
            rb_on,
            "MIDI Clock will be sent. MIDI Song Position and MIDI Continue "
            "will be sent if starting after tick 0 in song mode; otherwise "
            "MIDI Start is sent."
        );

        Gtk::RadioButton * rb_mod = manage(new Gtk::RadioButton("On (Mod)"));
        add_tooltip
        (
            rb_mod,
            "MIDI Clock will be sent.  MIDI Start will be sent and clocking "
            "will begin once the song position has reached the modulo of "
            "the specified Size. Use for gear that doesn't respond to Song "
            "Position."
        );

        Gtk::RadioButton * rb_disabled = manage
        (
            new Gtk::RadioButton("Port Disabled")
        );
        add_tooltip
        (
            rb_disabled,
            "This setting disables the usage of this output port, completely.  "
            "It is needed in some cases for devices that are detected, but "
            "cannot be used (e.g. devices locked by another application)."
        );

        Gtk::RadioButton::Group group = rb_off->get_group();
        rb_on->set_group(group);
        rb_mod->set_group(group);
        rb_disabled->set_group(group);

        /*
         * Wire in some clock callbacks.
         */

        rb_off->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::clock_callback_off), bus, rb_off)
        );
        rb_on->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::clock_callback_on),  bus, rb_on)
        );
        rb_mod->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::clock_callback_mod), bus, rb_mod)
        );
        rb_disabled->signal_toggled().connect
        (
            sigc::bind
            (
                mem_fun(*this, &options::clock_callback_disable), bus,
                rb_disabled
            )
        );
        hbox2->pack_end(*rb_mod, false, false);
        hbox2->pack_end(*rb_on, false, false);
        hbox2->pack_end(*rb_off, false, false);
        hbox2->pack_end(*rb_disabled, false, false);
        inputbox->pack_start(*hbox2, false, false);
        switch (perf().master_bus().get_clock(bus))
        {
        case e_clock_off:
            rb_off->set_active(1);
            break;

        case e_clock_pos:
            rb_on->set_active(1);
            break;

        case e_clock_mod:
            rb_mod->set_active(1);
            break;

        case e_clock_disabled:
            rb_disabled->set_active(1);
            break;
        }
    }

    Gtk::Adjustment * clock_mod_adj = new Gtk::Adjustment
    (
        midibus::get_clock_mod(), 1, 16 << 10, 1
    );
    Gtk::SpinButton * clock_mod_spin = new Gtk::SpinButton(*clock_mod_adj);
    Gtk::HBox * hbox2 = manage(new Gtk::HBox());
    hbox2->pack_start
    (
        *(manage(new Gtk::Label("Clock Start Modulo (1/16 Notes)"))),
        false, false, 4
    );
    hbox2->pack_start(*clock_mod_spin, false, false);
    inputbox->pack_start(*hbox2, false, false);
    clock_mod_adj->signal_value_changed().connect
    (
        sigc::bind(mem_fun(*this, &options::clock_mod_callback), clock_mod_adj)
    );

    Gtk::Frame * midimetaframe = manage(new Gtk::Frame("Meta Events"));
    midimetaframe->set_border_width(4);
    vbox->pack_start(*midimetaframe, Gtk::PACK_SHRINK);
    Gtk::VBox * vboxmeta = manage(new Gtk::VBox());
    Gtk::HBox * hboxmeta = manage(new Gtk::HBox());
    Gtk::Entry * entry = manage(new Gtk::Entry());
    Gtk::Label * label = manage
    (
        new Gtk::Label
        (
            " Pattern number for tempo track, from 0 to 1023 (0 recommended)",
            Gtk::ALIGN_LEFT
        )
    );
    entry->set_width_chars(4);
    entry->signal_changed().connect
    (
        sigc::bind(mem_fun(*this, &options::edit_tempo_track_number), entry)
    );
    entry->set_text(std::to_string(rc().tempo_track_number()));
    add_tooltip
    (
        entry,
        "Sets the number of the tempo track, and it is saved to the 'rc' file. "

        /*
         * Too much:
         *
         * "A very interactive control; play with it to understand how it works."
         */
    );
    hboxmeta->pack_start(*entry, Gtk::PACK_SHRINK, 4);
    hboxmeta->pack_start(*label, Gtk::PACK_SHRINK, 4);
    vboxmeta->pack_start(*hboxmeta, Gtk::PACK_SHRINK, 4);
    midimetaframe->add(*vboxmeta);

#define LOG_LABEL "Set as Song Tempo Track"

    Gtk::Button * log_to_song = manage(new Gtk::Button(LOG_LABEL));
    hboxmeta->pack_start(*log_to_song, Gtk::PACK_EXPAND_WIDGET, 8);
    log_to_song->signal_clicked().connect
    (
        mem_fun(*this, &options::log_tempo_track_number)
    );
    add_tooltip
    (
        log_to_song,
        "Saves the current tempo track number as a song parameter, saved "
        "to the MIDI file, as opposed to a global Sequencer64 value. "
        "However, remember that the value will be saved to the 'rc' "
        "file when exiting."
    );
}

/**
 *  Adds the MIDI Input page (tab) to the Options dialog.  We've added a frame
 *  for the MIDI input, and a frame for additional MIDI input options.
 *
 *  Here is the sequence of calls made when enabling a MIDI input in this
 *  dialog:
 *
\verbatim
options::input_callback()
   perform::set_input_bus(1, true)
      mastermidibus::set_input(1, true)
         busarray::set_input(1, true)
            businfo::active() [true]
            midibase::set_input(true)           [m_inputing = true, init_in()]
            businfo::init_input(true)           [m_init_input = true]
               midibase::set_input_status(true) [m_inputing = true (again)]
\endverbatim
 */

void
options::add_midi_input_page ()
{
    Gtk::VBox * vbox = manage(new Gtk::VBox());
    m_notebook->append_page(*vbox, "MIDI _Input", true);

    Gtk::Frame * inputframe = manage(new Gtk::Frame("Input Buses"));
    inputframe->set_border_width(4);
    vbox->pack_start(*inputframe, Gtk::PACK_SHRINK);

    Gtk::VBox * inputbox = manage(new Gtk::VBox());
    inputbox->set_border_width(4);
    inputframe->add(*inputbox);

    int buses = perf().master_bus().get_num_in_buses(); // input busses
    for (int bus = 0; bus < buses; ++bus)
    {
        Gtk::CheckButton * check = manage
        (
            new Gtk::CheckButton(perf().master_bus().get_midi_in_bus_name(bus), 0)
        );
        add_tooltip
        (
            check, "Select (click/space-bar) to enable/disable this MIDI input."
        );
        check->signal_toggled().connect
        (
            bind(mem_fun(*this, &options::input_callback), bus, check)
        );
        check->set_active(perf().get_input(bus));
        check->set_sensitive(! perf().is_input_system_port(bus));
        inputbox->pack_start(*check, false, false);
    }

    Gtk::Frame * midioptionframe = manage(new Gtk::Frame("Input Options"));
    midioptionframe->set_border_width(4);
    vbox->pack_start(*midioptionframe, Gtk::PACK_SHRINK);

    Gtk::VBox * midioptionbox = manage(new Gtk::VBox());
    midioptionbox->set_border_width(4);
    midioptionframe->add(*midioptionbox);

    Gtk::CheckButton * midioptfilter = manage
    (
        new Gtk::CheckButton
        (
            "Record input into sequences according to channel", true
        )
    );
    midioptfilter->set_active(rc().filter_by_channel());
    add_tooltip
    (
        midioptfilter,
        "If checked, MIDI recording filters each event into the sequence "
        "that uses the MIDI channel of the input event.  This is like the "
        "behavior of Seq32."
    );
    midioptionbox->pack_start(*midioptfilter, Gtk::PACK_SHRINK);
    midioptfilter->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::filter_callback), midioptfilter)
    );
}

/**
 *  Local macro to use in the add_keyboard_page() function.
 */

#define AddKey(text, address) \
    label = manage(new Gtk::Label(text)); \
    hbox->pack_start(*label, false, false, 4); \
    entry = manage(new keybindentry(keybindentry::location, address)); \
    hbox->pack_start(*entry, false, false, 4);

/**
 *  Adds the Keyboard page (tab) to the Options dialog.  This tab is the
 *  setup editor for the <tt> ~/.config/sequencer64/sequencer64.rc </tt>
 *  keybindings.
 */

void
options::add_keyboard_page ()
{
    Gtk::VBox * mainbox = manage(new Gtk::VBox());
    mainbox->set_spacing(6);
    m_notebook->append_page(*mainbox, "_Keyboard", true);

    Gtk::HBox * hbox = manage(new Gtk::HBox());
    Gtk::CheckButton * keycheck = manage
    (
        new Gtk::CheckButton("_Show sequence hot-key labels on sequences", true)
    );
    keycheck->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::input_callback),
            PERFORM_KEY_LABELS_ON_SEQUENCE, keycheck
        )
    );
    keycheck->set_active(perf().show_ui_sequence_key());
    mainbox->pack_start(*keycheck, false, false);

    if (! rc().legacy_format())
    {
        Gtk::CheckButton * numcheck = manage
        (
            new Gtk::CheckButton("Show se_quence numbers on sequences", true)
        );
        numcheck->signal_toggled().connect
        (
            bind
            (
                mem_fun(*this, &options::input_callback),
                PERFORM_NUM_LABELS_ON_SEQUENCE, numcheck
            )
        );
        numcheck->set_active(perf().show_ui_sequence_number());
        mainbox->pack_start(*numcheck, false, false);
    }

    /* Frame for sequence toggle keys */

    Gtk::Frame * controlframe = manage
    (
        new Gtk::Frame("Control keys [keyboard-group]")
    );
    controlframe->set_border_width(4);
    mainbox->pack_start(*controlframe, Gtk::PACK_SHRINK);

    Gtk::Table * controltable = manage(new Gtk::Table(4, 8, false));
    controltable->set_border_width(4);
    controltable->set_spacings(4);
    controlframe->add(*controltable);

    Gtk::Label * label = manage(new Gtk::Label("Start", Gtk::ALIGN_RIGHT));
    keybindentry * entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(start))
    );
    controltable->attach(*label, 0, 1, 0, 1);
    controltable->attach(*entry, 1, 2, 0, 1);

    label = manage(new Gtk::Label("Stop", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(stop))
    );
    controltable->attach(*label, 0, 1, 1, 2);
    controltable->attach(*entry, 1, 2, 1, 2);

    if (! rc().legacy_format())
    {
        label = manage(new Gtk::Label("Pause", Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::location, PREFKEY_ADDR(pause))
        );
        controltable->attach(*label, 0, 1, 2, 3);
        controltable->attach(*entry, 1, 2, 2, 3);
    }

    if (! rc().legacy_format())                 /* variset support  */
    {
        label = manage(new Gtk::Label("Slot Shift", Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::location, PREFKEY_ADDR(pattern_shift))
        );
        controltable->attach(*label, 0, 1, 3, 4);
        controltable->attach(*entry, 1, 2, 3, 4);
    }

    label = manage(new Gtk::Label("Snapshot 1", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(snapshot_1))
    );
    controltable->attach(*label, 2, 3, 0, 1);
    controltable->attach(*entry, 3, 4, 0, 1);

    label = manage(new Gtk::Label("Snapshot 2", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(snapshot_2))
    );
    controltable->attach(*label, 2, 3, 1, 2);
    controltable->attach(*entry, 3, 4, 1, 2);

    label = manage(new Gtk::Label("BPM Up", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(bpm_up))
    );
    controltable->attach(*label, 2, 3, 2, 3);
    controltable->attach(*entry, 3, 4, 2, 3);

    label = manage(new Gtk::Label("BPM Down", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(bpm_dn))
    );
    controltable->attach(*label, 2, 3, 3, 4);
    controltable->attach(*entry, 3, 4, 3, 4);

    label = manage(new Gtk::Label("Replace/Solo", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(replace))
    );
    controltable->attach(*label, 4, 5, 0, 1);
    controltable->attach(*entry, 5, 6, 0, 1);

    label = manage(new Gtk::Label("Queue", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(queue))
    );
    controltable->attach(*label, 4, 5, 1, 2);
    controltable->attach(*entry, 5, 6, 1, 2);

    label = manage(new Gtk::Label("Keep Queue", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(keep_queue))
    );
    controltable->attach(*label, 4, 5, 2, 3);
    controltable->attach(*entry, 5, 6, 2, 3);

    if (! rc().legacy_format())
    {
        label = manage(new Gtk::Label("Pattern Edit", Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::location, PREFKEY_ADDR(pattern_edit))
        );
        controltable->attach(*label, 4, 5, 3, 4);
        controltable->attach(*entry, 5, 6, 3, 4);
    }

    label = manage(new Gtk::Label("Screenset Up", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(screenset_up))
    );
    controltable->attach(*label, 6, 7, 0, 1);
    controltable->attach(*entry, 7, 8, 0, 1);

    label = manage(new Gtk::Label("Screenset Down", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(screenset_dn))
    );
    controltable->attach(*label, 6, 7, 1, 2);
    controltable->attach(*entry, 7, 8, 1, 2);

    label = manage(new Gtk::Label("Set Playing Screenset", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry
        (
            keybindentry::location, PREFKEY_ADDR(set_playing_screenset)
        )
    );
    controltable->attach(*label, 6, 7, 2, 3);
    controltable->attach(*entry, 7, 8, 2, 3);

    if (! rc().legacy_format())
    {
        label = manage(new Gtk::Label("Event Edit", Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::location, PREFKEY_ADDR(event_edit))
        );
        controltable->attach(*label, 6, 7, 3, 4);
        controltable->attach(*entry, 7, 8, 3, 4);
    }

    /* Frame for sequence toggle keys */

    Gtk::Frame * toggleframe = manage
    (
        new Gtk::Frame("Sequence toggle keys [keyboard-control]")
    );
    toggleframe->set_border_width(4);
    mainbox->pack_start(*toggleframe, Gtk::PACK_SHRINK);

    Gtk::Table * toggletable = manage(new Gtk::Table(4, 16, false));
    toggletable->set_border_width(4);
    toggletable->set_spacings(4);
    toggleframe->add(*toggletable);
    for (int i = 0; i < c_max_keys; ++i)            // not c_seqs_in_set
    {
        /*
         * TODO:  evaluate viability against a variable column count.  Also,
         * this stuff could be moved to the nearly identical loop starting
         * around line 693 below!
         */

        int x = i % SEQ64_SET_KEYS_COLUMNS * 2;     // 8 = c_mainwnd_cols ?
        int y = i / SEQ64_SET_KEYS_COLUMNS;
        int slot = x * 2 + y;           // count this way: 0, 4, 8, 16...
        char buf[8];
        snprintf(buf, sizeof buf, "%d", slot);
        Gtk::Label * numlabel = manage(new Gtk::Label(buf, Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::events, NULL, &perf(), slot)
        );
        toggletable->attach(*numlabel, x,   x+1, y, y+1);
        toggletable->attach(*entry,    x+1, x+2, y, y+1);
    }

    /* Frame for mute group slots */

    Gtk::Frame * mutegroupframe = manage
    (
        new Gtk::Frame("Mute-group slots [mute-group]")
    );
    mutegroupframe->set_border_width(4);
    mainbox->pack_start(*mutegroupframe, Gtk::PACK_SHRINK);

    Gtk::Table * mutegrouptable = manage(new Gtk::Table(4, 16, false));
    mutegrouptable->set_border_width(4);
    mutegrouptable->set_spacings(4);
    mutegroupframe->add(*mutegrouptable);
    for (int i = 0; i < c_max_keys; ++i)
    {
        int x = i % SEQ64_SET_KEYS_COLUMNS * 2;     // 8 = c_mainwnd_cols ?
        int y = i / SEQ64_SET_KEYS_COLUMNS;
        char buf[8];
        snprintf(buf, sizeof buf, "%d", i);
        Gtk::Label * numlabel = manage(new Gtk::Label(buf, Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::groups, NULL, &perf(), i)
        );
#ifdef USE_MUTE_GROUP_COUNT_CHECK
        if (i >= perf().group_max())
            entry->set_sensitive(false);
#endif

        mutegrouptable->attach(*numlabel, x, x+1, y, y+1);
        mutegrouptable->attach(*entry, x+1, x+2, y, y+1);
    }
    hbox = manage(new Gtk::HBox());
    AddKey
    (
        "Learn (while pressing a mute-group key):",
        PREFKEY_ADDR(group_learn)
    );
    AddKey("Disable:", PREFKEY_ADDR(group_off));
    AddKey("Enable:", PREFKEY_ADDR(group_on));
    mainbox->pack_start(*hbox, false, false);
}

/**
 *  Adds the Keyboard page (tab) to the Options dialog.  This tab is the
 *  setup editor for the <tt> ~/.config/sequencer64/sequencer64.rc </tt>
 *  keybindings.
 */

void
options::add_extended_keys_page ()
{
    Gtk::VBox * mainbox = manage(new Gtk::VBox());
    mainbox->set_spacing(6);
    m_notebook->append_page(*mainbox, "E_xt Keys", true);

    /* Frame for sequence toggle keys */

    Gtk::Frame * controlframe = manage
    (
        new Gtk::Frame("Extended keys [extended-keys]")
    );
    controlframe->set_border_width(4);
    mainbox->pack_start(*controlframe, Gtk::PACK_SHRINK);

    Gtk::Table * controltable = manage(new Gtk::Table(4, 8, false));
    controltable->set_border_width(4);
    controltable->set_spacings(4);
    controlframe->add(*controltable);

    Gtk::Label * label = manage
    (
        new Gtk::Label("Song/Live toggle", Gtk::ALIGN_RIGHT)
    );
    keybindentry * entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(song_mode))
    );
    controltable->attach(*label, 0, 1, 0, 1);
    controltable->attach(*entry, 1, 2, 0, 1);

    label = manage(new Gtk::Label("Toggle JACK", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(toggle_jack))
    );
    controltable->attach(*label, 0, 1, 1, 2);
    controltable->attach(*entry, 1, 2, 1, 2);

    label = manage(new Gtk::Label("Menu mode", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(menu_mode))
    );
    controltable->attach(*label, 0, 1, 2, 3);
    controltable->attach(*entry, 1, 2, 2, 3);
#if ! defined SEQ64_STAZED_MENU_BUTTONS
    entry->set_sensitive(false);
#endif

    label = manage(new Gtk::Label("Follow transport", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(follow_transport))
    );
    controltable->attach(*label, 2, 3, 0, 1);
    controltable->attach(*entry, 3, 4, 0, 1);

    label = manage(new Gtk::Label("Rewind", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(rewind))
    );
    controltable->attach(*label, 2, 3, 1, 2);
    controltable->attach(*entry, 3, 4, 1, 2);

    label = manage(new Gtk::Label("Fast forward", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(fast_forward))
    );
    controltable->attach(*label, 2, 3, 2, 3);
    controltable->attach(*entry, 3, 4, 2, 3);

    label = manage(new Gtk::Label("Pointer position", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(pointer_position))
    );
    controltable->attach(*label, 2, 3, 3, 4);
    controltable->attach(*entry, 3, 4, 3, 4);

    label = manage(new Gtk::Label("Toggle mutes", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(toggle_mutes))
    );
    controltable->attach(*label, 4, 5, 0, 1);
    controltable->attach(*entry, 5, 6, 0, 1);

    label = manage(new Gtk::Label("Tap BPM", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(tap_bpm))
    );
    controltable->attach(*label, 4, 5, 1, 2);
    controltable->attach(*entry, 5, 6, 1, 2);

#ifdef SEQ64_SONG_RECORDING
    label = manage(new Gtk::Label("Song record", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(song_record))
    );
    controltable->attach(*label, 4, 5, 2, 3);
    controltable->attach(*entry, 5, 6, 2, 3);

    label = manage(new Gtk::Label("One-shot queue", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(oneshot_queue))
    );
    controltable->attach(*label, 4, 5, 3, 4);
    controltable->attach(*entry, 5, 6, 3, 4);
#endif
}

#undef AddKey

/**
 *  Adds the Mouse page (tab) to the Options dialog.  It also creates a
 *  Frame for setting/viewing the mouse-interaction options.
 */

void
options::add_mouse_page ()
{
    const std::string msg =
        "Interaction method (a change requires reopening pattern editors)";

    Gtk::VBox * vbox = manage(new Gtk::VBox());
    m_notebook->append_page(*vbox, "_Mouse", true);

    Gtk::Frame * interactionframe = manage(new Gtk::Frame(msg));
    interactionframe->set_border_width(4);
    vbox->pack_start(*interactionframe, Gtk::PACK_SHRINK);

    Gtk::VBox * interactionbox = manage(new Gtk::VBox());
    interactionbox->set_border_width(4);
    interactionframe->add(*interactionbox);

    Gtk::RadioButton * rb_seq24 = manage
    (
        new Gtk::RadioButton("Se_q24 (original style)", true)
    );
    interactionbox->pack_start(*rb_seq24, Gtk::PACK_SHRINK);

    Gtk::RadioButton * rb_fruity = manage
    (
        new Gtk::RadioButton
        (
            "_Fruity (similar to a certain well-known sequencer)", true
        )
    );
    interactionbox->pack_start(*rb_fruity, Gtk::PACK_SHRINK);

    Gtk::RadioButton::Group group = rb_seq24->get_group();
    rb_fruity->set_group(group);
    if (rc().interaction_method() == e_fruity_interaction)
        rb_fruity->set_active();
    else
        rb_seq24->set_active();

    rb_seq24->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::mouse_seq24_callback), rb_seq24)
    );
    rb_fruity->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::mouse_fruity_callback), rb_fruity)
    );

    Gtk::Frame * seq64frame = manage(new Gtk::Frame("Sequencer64 Options"));
    seq64frame->set_border_width(4);
    vbox->pack_start(*seq64frame, Gtk::PACK_SHRINK);

    Gtk::VBox * seq64box = manage(new Gtk::VBox());
    seq64box->set_border_width(4);
    seq64frame->add(*seq64box);
    Gtk::CheckButton * chk_mod4 = manage
    (
        new Gtk::CheckButton
        (
            "_Mod4 key preserves add (paint) mode in song and pattern editors",
            true
        )
    );
    chk_mod4->set_active(rc().allow_mod4_mode());
    add_tooltip
    (
        chk_mod4,
        "If checked, note-add mode stays active after right-click release "
        "if the Super (Windows) key is pressed .  This works in "
        "the sequence/pattern and song editor piano rolls.  To get out of "
        "note-add mode, right-click again. An alternative is to use the p "
        "key (paint mode), and the x key to exit (xscape) the paint mode."
    );
    seq64box->pack_start(*chk_mod4, Gtk::PACK_SHRINK);
    chk_mod4->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::mouse_mod4_callback), chk_mod4)
    );

    Gtk::CheckButton * chk_snap_split = manage
    (
        new Gtk::CheckButton
        (
            "Middle click (or Ctrl-left-click) splits song trigger "
            "at nearest snap instead of halfway point.", true
        )
    );
    chk_snap_split->set_active(rc().allow_snap_split());
    add_tooltip
    (
        chk_snap_split,
        "If checked, middle-click on a trigger block in the performance "
        "editor splits the trigger block at the nearest snap point. "
        "Otherwise, the split occurs at the halfway point of the trigger "
        "block."
    );
    seq64box->pack_start(*chk_snap_split, Gtk::PACK_SHRINK);
    chk_snap_split->signal_toggled().connect
    (
        sigc::bind
        (
            mem_fun(*this, &options::mouse_snap_split_callback), chk_snap_split
        )
    );

    Gtk::CheckButton * chk_click_edit = manage
    (
        new Gtk::CheckButton
        (
            "Double click brings up sequence/pattern for editing.", true
        )
    );
    chk_click_edit->set_active(rc().allow_click_edit());
    add_tooltip
    (
        chk_click_edit,
        "If checked, double-click on a sequence/pattern in the patterns "
        "panel brings up the pattern for editing. "
        "This can interfere with muting/unmuting, so uncheck this option "
        "if that happens."
    );
    seq64box->pack_start(*chk_click_edit, Gtk::PACK_SHRINK);
    chk_click_edit->signal_toggled().connect
    (
        sigc::bind
        (
            mem_fun(*this, &options::mouse_click_edit_callback), chk_click_edit
        )
    );
}

/**
 *  Adds the JACK Sync page (tab) to the Options dialog.
 *
 *  We'll also stick a LASH option in this page, since there is plenty of room.
 */

void
options::add_jack_sync_page ()
{
    std::string page_title;

#if defined SEQ64_JACK_SUPPORT && defined SEQ64_LASH_SUPPORT
    page_title = "_JACK/LASH";
#elif defined SEQ64_JACK_SUPPORT
    page_title = "_JACK Sync";
#elif defined SEQ64_LASH_SUPPORT
    page_title = "_LASH";
#endif

    Gtk::VBox * vbox = nullptr;
    if (! page_title.empty())
    {
        vbox = manage(new Gtk::VBox());
        vbox->set_border_width(4);
        m_notebook->append_page(*vbox, page_title, true);
    }

#ifdef SEQ64_JACK_SUPPORT

    /* Frame for transport options */

#ifdef SEQ64_RTMIDI_SUPPORT
    Gtk::Frame * transportframe = manage(new Gtk::Frame("JACK Transport/MIDI"));
#else
    Gtk::Frame * transportframe = manage(new Gtk::Frame("JACK Transport Mode"));
#endif

    transportframe->set_border_width(4);
    vbox->pack_start(*transportframe, Gtk::PACK_SHRINK);

    Gtk::VBox * transportbox = manage(new Gtk::VBox());
    transportbox->set_border_width(4);
    transportframe->add(*transportbox);

    m_button_jack_transport->set_active(rc().with_jack_transport());
    add_tooltip
    (
        m_button_jack_transport,
        "Enable slave sync with JACK Transport.  Will be forced on if "
        " the user selected 'Transport Master' or 'Master Conditional'."
    );
    m_button_jack_transport->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_transport, m_button_jack_transport
        )
    );
    transportbox->pack_start(*m_button_jack_transport, false, false);

    m_button_jack_master->set_active(rc().with_jack_master());
    add_tooltip
    (
        m_button_jack_master,
        "Sequencer64 will attempt to serve as JACK Master.  'JACK Transport' "
        "will be forced on, and 'Master Conditional' will be forced off."
    );
    m_button_jack_master->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_master, m_button_jack_master
        )
    );
    transportbox->pack_start(*m_button_jack_master, false, false);

    m_button_jack_master_cond->set_active(rc().with_jack_master_cond());
    add_tooltip
    (
        m_button_jack_master_cond,
        "Sequencer64 will fail to be Master if there is already a Master set. "
        "'JACK Transport' will be forced on, and 'Transport Master' will be "
        "forced off."
    );
    m_button_jack_master_cond->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_master_cond, m_button_jack_master_cond
        )
    );
    transportbox->pack_start(*m_button_jack_master_cond, false, false);

#ifdef SEQ64_RTMIDI_SUPPORT
    m_button_jack_midi->set_active(rc().with_jack_midi());
    add_tooltip
    (
        m_button_jack_midi,
        "Sequencer64 will use JACK MIDI for input/output. "
        "This setting is independent of the 'JACK Transport' and related "
        "settings."
    );
    m_button_jack_midi->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_midi, m_button_jack_midi
        )
    );
    transportbox->pack_start(*m_button_jack_midi, false, false);
#endif

    /*
     * If JACK is already running, we don't want the user modifying these
     * check-boxes.  They'll have to click "JACK Disconnect" to re-enable
     * changing the status of these check-boxes.
     */

    if (perf().is_jack_running())
    {
        m_button_jack_transport->set_sensitive(false);
        m_button_jack_master->set_sensitive(false);
        m_button_jack_master_cond->set_sensitive(false);
    }
    else
    {
        m_button_jack_connect->set_sensitive(false);
        m_button_jack_disconnect->set_sensitive(false);
    }

    /* Frame for jack start mode options */

    Gtk::Frame * modeframe = manage(new Gtk::Frame("JACK Start Mode"));
    modeframe->set_border_width(4);
    vbox->pack_start(*modeframe, Gtk::PACK_SHRINK);

    Gtk::VBox * modebox = manage(new Gtk::VBox());
    modebox->set_border_width(4);
    modeframe->add(*modebox);

    Gtk::RadioButton * rb_live = manage(new Gtk::RadioButton("_Live Mode", true));
    add_tooltip
    (
        rb_live,
        "Playback will be in Live mode.  Use this to "
        "allow live muting and unmuting of patterns (loops) in the "
        "sequence/pattern window (the main window) when running JACK. "
        "If JACK is not running, Live mode occurs only if playback is started "
        "from the main window."
    );

    Gtk::RadioButton * rb_perform = manage
    (
        new Gtk::RadioButton("_Song Mode", true)
    );
    add_tooltip
    (
        rb_perform,
        "Playback will use the Song Editor's layout data.  This data is "
        "used no matter whether the sequence/pattern editor or the song "
        "editor is active, if JACK is running.  If JACK is not running, Song "
        "mode occurs only if playback is started from the song editor."
    );

    Gtk::RadioButton::Group group = rb_live->get_group();
    rb_perform->set_group(group);
    if (perf().song_start_mode())
        rb_perform->set_active(true);
    else
        rb_live->set_active(true);

    rb_perform->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_start_mode_song, rb_perform
        )
    );
    modebox->pack_start(*rb_live, false, false);
    modebox->pack_start(*rb_perform, false, false);

    /* Connection buttons */

    Gtk::HButtonBox * buttonbox = manage(new Gtk::HButtonBox());
    buttonbox->set_layout(Gtk::BUTTONBOX_START);
    buttonbox->set_spacing(12);                      // was 6
    vbox->pack_start(*buttonbox, false, false);

    /*
     * JACK Connect.
     */

    add_tooltip
    (
        m_button_jack_connect,
        "Reconnect to JACK transport. Calls the JACK transport "
        "initialization function, which is "
        "automatically called at Sequencer64 startup, if configured.  Click "
        "this button after making the JACK Transport settings above. "
        "Does not apply to the Native JACK MIDI setting."
    );
    m_button_jack_connect->signal_clicked().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_connect, m_button_jack_connect
        )
    );
    buttonbox->pack_start(*m_button_jack_connect, false, false);
    if (rc().with_jack_transport())
    {
        if (perf().is_jack_running())
            m_button_jack_connect->set_sensitive(false);
        else
            m_button_jack_connect->set_sensitive(true);
    }

    /*
     * JACK Disconnect.  Weird.  When we click this button, we see the
     * following message:
     *
     *   (sequencer64:766): Gtk-CRITICAL **: IA__gtk_toggle_button_get_active:
     *   assertion 'GTK_IS_TOGGLE_BUTTON (toggle_button)' failed
     *
     * This doesn't happen in seq24, or to the Connect button, and the code is
     * identical!  What's up!??
     */

    add_tooltip
    (
        m_button_jack_disconnect,
        "Disconnect JACK transport. Calls the JACK transport "
        "deinitialization function, and enables the "
        "JACK transport buttons.  Click this button to modify "
        "the JACK Transport Mode settings above."
    );
    m_button_jack_disconnect->signal_clicked().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_disconnect, m_button_jack_disconnect
        )
    );
    buttonbox->pack_start(*m_button_jack_disconnect, false, false);
    if (rc().with_jack_transport())
    {
        if (perf().is_jack_running())
            m_button_jack_disconnect->set_sensitive(true);
        else
            m_button_jack_disconnect->set_sensitive(false);
    }

#endif          // SEQ64_JACK_SUPPORT

#ifdef SEQ64_LASH_SUPPORT

    Gtk::Frame * lashframe = manage(new Gtk::Frame("LASH Options"));
    lashframe->set_border_width(4);
    vbox->pack_start(*lashframe, Gtk::PACK_SHRINK);

    Gtk::VBox * lashbox = manage(new Gtk::VBox());
    lashbox->set_border_width(4);
    lashframe->add(*lashbox);
    Gtk::CheckButton * chk_lash = manage
    (
        new Gtk::CheckButton
        (
            "Enables the usage of LASH with Sequencer64. "
            "Requires Sequencer64 to be restarted, to take effect.", true
        )
    );
    chk_lash->set_active(rc().lash_support());
    add_tooltip
    (
        chk_lash,
        "If checked, LASH session support will be used. "
        "This is the same as the [lash-session] option in the rc "
        "configuration file."
    );
    lashbox->pack_start(*chk_lash, Gtk::PACK_SHRINK);
    chk_lash->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::lash_support_callback), chk_lash)
    );

#endif          // SEQ64_LASH_SUPPORT

}

/**
 *  Clock-off callback function.  This and the other callback functions for
 *  clocking are actually called twice; the first time, the button is not
 *  active, and the second time, it is.  Weird.
 *
 * \param bus
 *      The MIDI buss number to be affected.
 *
 * \param button
 *      The status of the radio-button.  If active, then the master buss
 *      set_clock() function is called to turn off the clock (e_clock_off).
 */

void
options::clock_callback_off (int bus, Gtk::RadioButton * button)
{
    if (button->get_active())
        perf().set_clock_bus(bus, e_clock_off);
}

/**
 *  Clock-on position callback function.
 *
 * \param bus
 *      The MIDI buss number to be affected.
 *
 * \param button
 *      The status of the radio-button.  If active, then the master buss
 *      set_clock() function is called to allow the repositioning of the
 *      clock (e_clock_pos).
 */

void
options::clock_callback_on (int bus, Gtk::RadioButton * button)
{
    if (button->get_active())
        perf().set_clock_bus(bus, e_clock_pos);
}

/**
 *  Clock-mod callback function.
 *
 * \param bus
 *      The MIDI buss number to be affected.
 *
 * \param button
 *      The status of the radio-button.  If active, then the master buss
 *      set_clock() function is called to turn on e_clock_mod.
 */

void
options::clock_callback_mod (int bus, Gtk::RadioButton * button)
{
    if (button->get_active())
        perf().set_clock_bus(bus, e_clock_mod);
}

/**
 *  Clock-output-disable callback function.
 *
 * \param bus
 *      The MIDI buss number to be affected.
 *
 * \param button
 *      The status of the radio-button.  If active, then the master buss
 *      set_clock() function is called to turn on e_clock_mod.
 */

void
options::clock_callback_disable (int bus, Gtk::RadioButton * button)
{
    if (button->get_active())
        perf().set_clock_bus(bus, e_clock_disabled);
}

/**
 *  Mod-clock callback function.
 *
 * \param adj
 *      The horizontal adjustment object.  Its get_value() function return
 *      value is pass to set_clock_mod.
 */

void
options::clock_mod_callback (Gtk::Adjustment * adj)
{
    midibus::set_clock_mod(int(adj->get_value()));
}

/**
 *  Provides an option (not recommended, but may be necessary for legacy
 *  tunes) to change the default tempo track from the MIDI-specified 0 (first
 *  track) to some other track.
 *
 * \param text
 *      Provides the text-edit control to change the tempo-track number.
 */

void
options::edit_tempo_track_number (Gtk::Entry * text)
{
    std::string number = text->get_text();
    rc().tempo_track_number(atoi(number.c_str()));
    int track = rc().tempo_track_number();              /* now validated    */
    number = std::to_string(track);
    text->set_text(number);
}

/**
 *  Sets the tempo-track (normally 0) that will be used in runs of
 *  Sequencer64.
 */

void
options::log_tempo_track_number ()
{
    perf().set_tempo_track_number(rc().tempo_track_number());
}

/**
 *  Input callback function.  This is kind of a weird function, but it allows
 *  immediate redrawing of the mainwid and perfnames user-interfaces when this
 *  item is modified in the File / Options / Keyboard tab.  This drawing is
 *  indirect, and triggered by the perform object setting the dirty-flag on
 *  all of the sequences in the bus.
 *
 *  However, this does not affect the empty pattern slots of the mainwid, and
 *  we don't see a way to get mainwid::reset() called, so the mainwid
 *  currently cannot update the empty slots; a restart of the application is
 *  the only way to see the change.
 *
 * \tricky
 *      See the description of the bus parameter.
 *
 * \param bus
 *      If in the normal buss-number range, this serves as a buss setting for
 *      the perform perform object.  If it is a large number (9900 and above),
 *      it serves to modify the "show sequence hot-key" or "show sequence
 *      number" settings (which leads to the set-dirty flag of each sequence
 *      being set, and hence a redraw of each sequence).
 *
 * \param i_button
 *      Provides the check-box object that was changed by a click.  It's
 *      get_active() function provides the current state of the boolean value.
 */

void
options::input_callback (int bus, Gtk::Button * b)
{
    Gtk::CheckButton * button = (Gtk::CheckButton *) b;
    bool input = button->get_active();
    perf().set_input_bus(bus, input);
}

#ifdef USE_MIDI_CLOCK_CONNECT_BUTTON        // NOT YET READY

/**
 *  Sets the output status.  NOT YET READY
 */

void
options::output_callback (int bus, Gtk::Button * button)
{
    // Gtk::CheckButton * button = (Gtk::CheckButton *) button;
    // bool input = button->get_active();
    // perf().set_output_bus(bus, input);
}

#endif  // USE_MIDI_CLOCK_CONNECT_BUTTON

/**
 *  Sets the ability to filter incoming MIDI events by MIDI channel.
 *
 * \param i_button
 *      Provides the check-button object that was changed by a click.  It's
 *      get_active() function provides the current state of the boolean value
 *      to be passed to the "rc" and perform filter_by_channel() functions.
 */

void
options::filter_callback (Gtk::Button * f_button)
{
    Gtk::CheckButton * button = (Gtk::CheckButton *) f_button;
    bool input = button->get_active();
    rc().filter_by_channel(input);
    perf().filter_by_channel(input);
}

/**
 *  Mouse interaction = Seq24 callback function.
 *
 * \param btn
 *      The button that controls the "rc" interaction_method() setting.
 */

void
options::mouse_seq24_callback (Gtk::RadioButton * btn)
{
    if (btn->get_active())
        rc().interaction_method(e_seq24_interaction);
}

/**
 *  Mouse interaction = Fruity callback function.
 *
 * \param btn
 *      The button that controls the "rc" interaction_method() setting.
 */

void
options::mouse_fruity_callback (Gtk::RadioButton * btn)
{
    if (btn->get_active())
        rc().interaction_method(e_fruity_interaction);
}

/**
 *  Mouse interaction, Mod4 option callback function.
 *
 * \param btn
 *      The button that controls the "rc" allow_mod4_mode() setting.
 */

void
options::mouse_mod4_callback (Gtk::CheckButton * btn)
{
    rc().allow_mod4_mode(btn->get_active());
}

/**
 *  Mouse interaction, snap-split option callback function.
 *
 * \param btn
 *      The button that controls the "rc" allow_snap_split() setting.
 */

void
options::mouse_snap_split_callback (Gtk::CheckButton * btn)
{
    rc().allow_snap_split(btn->get_active());
}

/**
 *  Mouse interaction, click-edit option callback function.
 *
 * \param btn
 *      The button that controls the "rc" allow_click_edit() setting.
 */

void
options::mouse_click_edit_callback (Gtk::CheckButton * btn)
{
    rc().allow_click_edit(btn->get_active());
}

/**
 *  Mouse interaction, Mod4 option callback function.
 *
 * \param btn
 *      The button that controls the "rc" lash_support() setting.
 */

void
options::lash_support_callback (Gtk::CheckButton * btn)
{
    rc().lash_support(btn->get_active());
}

/**
 *  Transport callback function.  See the options::button enumeration for the
 *  meaning of the values.  Note that we added the
 *  e_jack_start_mode_live value, for completeness, even though no control
 *  calls this function with that enumeration.
 *
 * \warning
 *      These CheckButtons really need to be radio buttons (along with some
 *      changes in how the various JACK transport flags of Sequencer64 are
 *      juxtaposed).  However, what we do instead is make sure that the
 *      buttons are coordinated properly:
 *
 *          -   JACK Transport (makes Sequencer64 a JACK slave).  Always
 *              active if one of the other two are set, or if set on its own.
 *          -   Transport Master (makes Sequencer64 a JACK master).  Forces
 *              the Master Conditional button off, and the JACK Transport
 *              button on.
 *          -   Master Conditional (makes Sequencer64 a JACK master if it
 *              can).  Forces the Transport Master button off, and the JACK
 *              Transport button on.
 *
 *      The buttons responded to here  can be a ToggleButton, RadioButton, or
 *      a CheckButton!
 *
 * \param type
 *      The type of the button that was pressed.  It is one of:
 *
 *      -   e_jack_transport.  This is a CheckButton.
 *      -   e_jack_master.  This is a CheckButton.
 *      -   e_jack_master_cond.  This is a CheckButton.
 *      -   e_jack_start_mode_live.  This is a RadioButton.
 *      -   e_jack_start_mode_song.  This is a RadioButton.
 *      -   e_jack_connect.  Currently a ToggleButton.
 *      -   e_jack_disconnect.  Currently a ToggleButton.
 *      -   e_jack_midi. This is a CheckButton.
 *
 * \param acheck
 *      Provides the status of the check-button that was clicked.
 */

void
options::transport_callback (button type, Gtk::Button * acheck)
{
    Gtk::CheckButton * ccheck = (Gtk::CheckButton *) acheck;
    Gtk::RadioButton * rcheck = (Gtk::RadioButton *) acheck;
    bool is_active = false;
    switch (type)
    {
    case e_jack_transport:

        is_active = ccheck->get_active();
        if (is_active)
        {
            rc().with_jack_transport(true);
            m_button_jack_connect->set_sensitive(true);     // enable connect
            m_button_jack_disconnect->set_sensitive(false); // disable disconnect
        }
        else
        {
            if (! rc().with_jack_master() && ! rc().with_jack_master_cond())
            {
                rc().with_jack_transport(false);
                m_button_jack_connect->set_sensitive(false);
                m_button_jack_disconnect->set_sensitive(false);
            }
            else
                m_button_jack_transport->set_active(1); // force it back on
        }
        break;

    case e_jack_master:

        is_active = ccheck->get_active();
        rc().with_jack_master(is_active);
        if (is_active)
        {
            rc().with_jack_transport(true);
            m_button_jack_transport->set_active(1);

            rc().with_jack_master_cond(false);
            m_button_jack_master_cond->set_active(0);
        }
        break;

    case e_jack_master_cond:

        is_active = ccheck->get_active();
        rc().with_jack_master_cond(is_active);
        if (is_active)
        {
            rc().with_jack_transport(true);
            m_button_jack_transport->set_active(1);

            rc().with_jack_master(false);
            m_button_jack_master->set_active(0);
        }
        break;

    case e_jack_midi:

        is_active = ccheck->get_active();
        rc().with_jack_midi(is_active);
        break;

    case e_jack_start_mode_live:
    case e_jack_start_mode_song:

        is_active = rcheck->get_active();
        perf().song_start_mode(is_active);
        break;

    case e_jack_connect:

        /*
         * This is legacy behavior.  If Stazed JACK support is enabled,
         * the perform::set_jack_mode() function as called from perfedit
         * will call init_jack().  But that won't affect the buttons here.
         * Needs to be reconciled.  Perhaps read the status during the opening
         * of the JACK options page.
         */

        if (perf().init_jack_transport())                   // true = it worked
        {
            m_button_jack_connect->set_sensitive(false);    // disable connect
            m_button_jack_disconnect->set_sensitive(true);  // enable disconnect
            m_button_jack_transport->set_sensitive(false);
            m_button_jack_master->set_sensitive(false);
            m_button_jack_master_cond->set_sensitive(false);
        }
        break;

    case e_jack_disconnect:

        /*
         * Also legacy behavior, like the comment above.
         */

        if (! perf().deinit_jack_transport())               // false = it worked
        {
            m_button_jack_connect->set_sensitive(true);     // enable connect
            m_button_jack_disconnect->set_sensitive(false); // disable disconnect
            m_button_jack_transport->set_sensitive(true);
            m_button_jack_master->set_sensitive(true);
            m_button_jack_master_cond->set_sensitive(true);
        }
        break;

    default:
        break;
    }
}

}           // namespace seq64

/*
 * options.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

