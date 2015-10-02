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
 * \updates       2015-10-02
 * \license       GNU GPLv2 or above
 *
 *  Here is a list of the global variables used/stored/modified by this
 *  module:
 *
 *      -   c_max_sequence
 *      -   e_fruity_interaction and e_seq24_interation
 *      -   e_clock_off, e_clock_pos, e_clock_mod
 *      -   e_keylabelsonsequence
 *      -   e_jack_transport, e_jack_master,
 *          e_jack_master_cond, e_jack_master_connect,
 *          e_jack_master_disconnect, e_jack_master_song_mode,
 *      -   global_interactionmethod
 *      -   global_with_jack_transport);
 *      -   global_with_jack_master);
 *      -   global_with_jack_master_cond);
 *      -   global_jack_start_mode)
 *      -   global_interactionmethod = e_seq24_interaction;
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

namespace seq64
{

/**
 *  Yet another way to define a constant.
 */

enum
{
    e_keylabelsonsequence = 9999
};

/**
 *  The principal constructor for the options class creates a number of
 *  dialog elements.
 */

options::options
(
    Gtk::Window & parent,
    perform & a_p
) :
    Gtk::Dialog                     ("Options", parent, true, true),
#if GTK_MINOR_VERSION < 12
    m_tooltips                      (manage(new Gtk::Tooltips()),
#endif
    m_mainperf                      (a_p),
    m_button_ok                     (manage(new Gtk::Button(Gtk::Stock::OK))),
    m_notebook                      (manage(new Gtk::Notebook()))
{
    Gtk::HBox * hbox = manage(new Gtk::HBox());
    get_vbox()->pack_start(*hbox, false, false);
    get_action_area()->set_border_width(2);
    hbox->set_border_width(6);
    get_action_area()->pack_end(*m_button_ok, false, false);
    m_button_ok->signal_clicked().connect(mem_fun(*this, &options::hide));
    hbox->pack_start(*m_notebook);
    add_midi_clock_page();
    add_midi_input_page();
    add_keyboard_page();
    add_mouse_page();
    add_jack_sync_page();
}

/**
 *  Adds the MIDI Clock page (tab) to the Options dialog.
 *  It counts the MIDI Output clock busses, among other things.
 */

void
options::add_midi_clock_page ()
{
    int buses = perf().master_bus().get_num_out_buses();
    Gtk::VBox * vbox = manage(new Gtk::VBox());
    vbox->set_border_width(6);
    m_notebook->append_page(*vbox, "MIDI _Clock", true);
#if GTK_MINOR_VERSION < 12
    manage(new Gtk::Tooltips());
#endif
    for (int i = 0; i < buses; i++)
    {
        Gtk::HBox * hbox2 = manage(new Gtk::HBox());
        Gtk::Label * label = manage
        (
            new Gtk::Label
            (
                perf().master_bus().get_midi_out_bus_name(i), 0
            )
        );
        hbox2->pack_start(*label, false, false);

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
            "the specified Size. For gear that doesn't respond to Song "
            "Position."
        );

        Gtk::RadioButton::Group group = rb_off->get_group();
        rb_on->set_group(group);
        rb_mod->set_group(group);

        /*
         * Wire in some clock callbacks.
         */

        rb_off->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::clock_callback_off), i, rb_off)
        );
        rb_on->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::clock_callback_on),  i, rb_on)
        );
        rb_mod->signal_toggled().connect
        (
            sigc::bind(mem_fun(*this, &options::clock_callback_mod), i, rb_mod)
        );
        hbox2->pack_end(*rb_mod, false, false);
        hbox2->pack_end(*rb_on, false, false);
        hbox2->pack_end(*rb_off, false, false);
        vbox->pack_start(*hbox2, false, false);
        switch (perf().master_bus().get_clock(i))
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
    vbox->pack_start(*hbox2, false, false);
    clock_mod_adj->signal_value_changed().connect
    (
        sigc::bind(mem_fun(*this, &options::clock_mod_callback), clock_mod_adj)
    );
}

/**
 *  Adds the MIDI Input page (tab) to the Options dialog.
 */

void
options::add_midi_input_page ()
{
    int buses = perf().master_bus().get_num_in_buses(); // input busses
    Gtk::VBox * vbox = manage(new Gtk::VBox());
    vbox->set_border_width(6);
    m_notebook->append_page(*vbox, "MIDI _Input", true);
    for (int i = 0; i < buses; i++)
    {
        Gtk::CheckButton * check = manage
        (
            new Gtk::CheckButton
            (
                perf().master_bus().get_midi_in_bus_name(i), 0
            )
        );
        check->signal_toggled().connect
        (
            bind(mem_fun(*this, &options::input_callback), i, check)
        );
        check->set_active(perf().master_bus().get_input(i));
        vbox->pack_start(*check, false, false);
    }
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
    Gtk::CheckButton * check = manage
    (
        new Gtk::CheckButton("_Show key labels on sequences", true)
    );
    check->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::input_callback),
            int(e_keylabelsonsequence), check
        )
    );
    check->set_active(perf().show_ui_sequence_key());
    mainbox->pack_start(*check, false, false);

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

    label = manage(new Gtk::Label("BPM down", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(bpm_dn))
    );
    controltable->attach(*label, 2, 3, 3, 4);
    controltable->attach(*entry, 3, 4, 3, 4);

    label = manage(new Gtk::Label("BPM up", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(bpm_up))
    );
    controltable->attach(*label, 2, 3, 2, 3);
    controltable->attach(*entry, 3, 4, 2, 3);

    label = manage(new Gtk::Label("Replace", Gtk::ALIGN_RIGHT));
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

    label = manage(new Gtk::Label("Keep queue", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(keep_queue))
    );
    controltable->attach(*label, 4, 5, 2, 3);
    controltable->attach(*entry, 5, 6, 2, 3);

    label = manage(new Gtk::Label("Screenset up", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(screenset_up))
    );
    controltable->attach(*label, 6, 7, 0, 1);
    controltable->attach(*entry, 7, 8, 0, 1);

    label = manage(new Gtk::Label("Screenset down", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry(keybindentry::location, PREFKEY_ADDR(screenset_dn))
    );
    controltable->attach(*label, 6, 7, 1, 2);
    controltable->attach(*entry, 7, 8, 1, 2);

    label = manage(new Gtk::Label("Set playing screenset", Gtk::ALIGN_RIGHT));
    entry = manage
    (
        new keybindentry
        (
            keybindentry::location, PREFKEY_ADDR(set_playing_screenset)
        )
    );
    controltable->attach(*label, 6, 7, 2, 3);
    controltable->attach(*entry, 7, 8, 2, 3);

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
    for (int i = 0; i < 32; i++)        // c_seqs_in_set ?
    {
        int x = i % 8 * 2;
        int y = i / 8;
        int slot = x * 2 + y;           // count this way: 0, 4, 8, 16...
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", slot);
        Gtk::Label * numlabel = manage(new Gtk::Label(buf, Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::events, NULL, &perf(), slot)
        );
        toggletable->attach(*numlabel, x, x+1, y, y+1);
        toggletable->attach(*entry, x+1, x+2, y, y+1);
    }

    /* Frame for mute group slots */

    Gtk::Frame * mutegroupframe = manage
    (
        new Gtk::Frame("Mute-group slots [keyboard-group]")
    );
    mutegroupframe->set_border_width(4);
    mainbox->pack_start(*mutegroupframe, Gtk::PACK_SHRINK);

    Gtk::Table * mutegrouptable = manage(new Gtk::Table(4, 16, false));
    mutegrouptable->set_border_width(4);
    mutegrouptable->set_spacings(4);
    mutegroupframe->add(*mutegrouptable);
    for (int i = 0; i < 32; i++)        // c_seqs_in_set ?
    {
        int x = i % 8 * 2;
        int y = i / 8;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", i);
        Gtk::Label * numlabel = manage(new Gtk::Label(buf, Gtk::ALIGN_RIGHT));
        entry = manage
        (
            new keybindentry(keybindentry::groups, NULL, &perf(), i)
        );
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

#undef AddKey

/**
 *  Adds the Mouse page (tab) to the Options dialog.  It also creates a
 *  Frame for setting/viewing the mouse-interaction options.
 */

void
options::add_mouse_page ()
{
    Gtk::VBox * vbox = manage(new Gtk::VBox());
    m_notebook->append_page(*vbox, "_Mouse", true);

    Gtk::Frame * interactionframe = manage(new Gtk::Frame("Interaction method"));
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
    switch (global_interactionmethod)
    {
    case e_fruity_interaction:
        rb_fruity->set_active();
        break;

    case e_seq24_interaction:
    default:
        rb_seq24->set_active();
        break;
    }
    rb_seq24->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::mouse_seq24_callback), rb_seq24)
    );
    rb_fruity->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::mouse_fruity_callback), rb_fruity)
    );

    /**
     * \todo
     *      Add a "Mod4" checkbox section here.
     */

    Gtk::Frame * mod4frame = manage(new Gtk::Frame("Sequencer64 Options"));
    mod4frame->set_border_width(4);
    vbox->pack_start(*mod4frame, Gtk::PACK_SHRINK);

    Gtk::VBox * mod4box = manage(new Gtk::VBox());
    mod4box->set_border_width(4);
    mod4frame->add(*mod4box);
    Gtk::CheckButton * chk_mod4 = manage
    (
        new Gtk::CheckButton
        (
            "_Mod4 key preserves note-add mode in song and pattern editors",
            true
        )
    );
    chk_mod4->set_active(global_allow_mod4_mode);
    add_tooltip
    (
        chk_mod4,
        "If checked, note-add mode stays active after right-click release "
        "if 'Windows' key is pressed in seq24 mode.  This works in the "
        "pattern and song editor piano rolls."
    );
    mod4box->pack_start(*chk_mod4, Gtk::PACK_SHRINK);
    chk_mod4->signal_toggled().connect
    (
        sigc::bind(mem_fun(*this, &options::mouse_mod4_callback), chk_mod4)
    );
}

/**
 *  Adds the JACK Sync page (tab) to the Options dialog.
 */

void
options::add_jack_sync_page ()
{
#ifdef JACK_SUPPORT
    Gtk::VBox * vbox = manage(new Gtk::VBox());
    vbox->set_border_width(4);
    m_notebook->append_page(*vbox, "_JACK Sync", true);

    /* Frame for transport options */

    Gtk::Frame * transportframe = manage(new Gtk::Frame("Transport"));
    transportframe->set_border_width(4);
    vbox->pack_start(*transportframe, Gtk::PACK_SHRINK);

    Gtk::VBox * transportbox = manage(new Gtk::VBox());
    transportbox->set_border_width(4);
    transportframe->add(*transportbox);

    Gtk::CheckButton * check = manage
    (
        new Gtk::CheckButton("JACK _Transport", true)
    );
    check->set_active(global_with_jack_transport);
    add_tooltip(check, "Enable sync with JACK Transport.");
    check->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_transport, check
        )
    );
    transportbox->pack_start(*check, false, false);

    check = manage(new Gtk::CheckButton("Trans_port Master", true));
    check->set_active(global_with_jack_master);
    add_tooltip(check, "Sequencer64 will attempt to serve as JACK Master.");
    check->signal_toggled().connect
    (
        bind(mem_fun(*this, &options::transport_callback), e_jack_master, check)
    );
    transportbox->pack_start(*check, false, false);

    check = manage(new Gtk::CheckButton("Master C_onditional", true));
    check->set_active(global_with_jack_master_cond);
    add_tooltip
    (
        check,
        "Sequencer64 will fail to be Master if there is already a Master set."
    );
    check->signal_toggled().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_master_cond, check
        )
    );
    transportbox->pack_start(*check, false, false);

    /* Frame for jack start mode options */

    Gtk::Frame * modeframe = manage(new Gtk::Frame("JACK Start mode"));
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
        "allow muting and unmuting of patterns (loops)."
    );

    Gtk::RadioButton * rb_perform = manage(new Gtk::RadioButton("_Song Mode", true));
    add_tooltip(rb_perform, "Playback will use the Song Editor's data.");

    Gtk::RadioButton::Group group = rb_live->get_group();
    rb_perform->set_group(group);
    if (global_jack_start_mode)
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
    buttonbox->set_spacing(6);
    vbox->pack_start(*buttonbox, false, false);

    Gtk::Button * button = manage(new Gtk::Button("Co_nnect", true));
    add_tooltip(button, "Connect to JACK.");
    button->signal_clicked().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_connect, button
        )
    );
    buttonbox->pack_start(*button, false, false);

    button = manage(new Gtk::Button("_Disconnect", true));
    add_tooltip(button, "Disconnect JACK.");
    button->signal_clicked().connect
    (
        bind
        (
            mem_fun(*this, &options::transport_callback),
            e_jack_disconnect, button
        )
    );
    buttonbox->pack_start(*button, false, false);
#endif
}

/**
 *  Clock-off callback function.
 */

void
options::clock_callback_off (int a_bus, Gtk::RadioButton * a_button)
{
    if (a_button->get_active())
        perf().master_bus().set_clock(a_bus, e_clock_off);
}

/**
 *  Clock-on callback function.
 */

void
options::clock_callback_on (int a_bus, Gtk::RadioButton * a_button)
{
    if (a_button->get_active())
        perf().master_bus().set_clock(a_bus, e_clock_pos);
}

/**
 *  Clock-mod callback function.
 */

void
options::clock_callback_mod (int a_bus, Gtk::RadioButton * a_button)
{
    if (a_button->get_active())
        perf().master_bus().set_clock(a_bus, e_clock_mod);
}

/**
 *  Mod-clock callback function.
 */

void
options::clock_mod_callback (Gtk::Adjustment * adj)
{
    midibus::set_clock_mod((int)adj->get_value());
}

/**
 *  Input callback function.
 */

void
options::input_callback (int a_bus, Gtk::Button * i_button)
{
    Gtk::CheckButton * a_button = (Gtk::CheckButton *) i_button;
    bool input = a_button->get_active();
    if (9999 == a_bus)                  // another manifest constant needed
    {
        perf().show_ui_sequence_key(input);
        for (int i = 0; i < c_max_sequence; i++)
        {
            if (perf().get_sequence(i))
                perf().get_sequence(i)->set_dirty();
        }
        return;
    }
    perf().master_bus().set_input(a_bus, input);
}

/**
 *  Mouse interaction = Seq24 callback function.
 */

void
options::mouse_seq24_callback (Gtk::RadioButton * btn)
{
    if (btn->get_active())
        global_interactionmethod = e_seq24_interaction;
}

/**
 *  Mouse interaction = Fruity callback function.
 */

void
options::mouse_fruity_callback (Gtk::RadioButton * btn)
{
    if (btn->get_active())
        global_interactionmethod = e_fruity_interaction;
}

/**
 *  Mouse interaction, Mod4 option callback function.
 */

void
options::mouse_mod4_callback (Gtk::CheckButton * btn)
{
    global_allow_mod4_mode = btn->get_active();
}

/**
 *  Transport callback function.
 */

void
options::transport_callback (button a_type, Gtk::Button * a_check)
{
    Gtk::CheckButton * check = (Gtk::CheckButton *) a_check;
    switch (a_type)
    {
    case e_jack_transport:
        global_with_jack_transport = check->get_active();
        break;

    case e_jack_master:
        global_with_jack_master = check->get_active();
        break;

    case e_jack_master_cond:
        global_with_jack_master_cond = check->get_active();
        break;

    case e_jack_start_mode_song:
        global_jack_start_mode = check->get_active();
        break;

    case e_jack_connect:
        perf().init_jack();
        break;

    case e_jack_disconnect:
        perf().deinit_jack();
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
