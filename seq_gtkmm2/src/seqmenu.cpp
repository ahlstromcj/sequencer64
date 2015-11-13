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
 * \file          seqmenu.cpp
 *
 *  This module declares/defines the class that handles the right-click
 *  menu of the sequence slots in the pattern window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-13
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/box.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>

#include "font.hpp"
#include "perform.hpp"
#include "seqedit.hpp"
#include "seqmenu.hpp"

using namespace Gtk::Menu_Helpers;

namespace seq64
{

/**
 *  Principal constructor.  Apart from filling in some of the members,
 *  this function initializes the clipboard, so that we don't get a crash
 *  on a paste with no previous copy.
 */

seqmenu::seqmenu (perform & p)
 :
    m_menu          (nullptr),
    m_mainperf      (p),
    m_clipboard     (),
    m_seqedit       (nullptr),
    m_current_seq   (-1)            /* (0) is not really current yet    */
{
    m_clipboard.set_master_midi_bus(&m_mainperf.master_bus());
}

/**
 *  A rote destructor.  If we determine that we need to delete the
 *  m_seqedit pointer, we can do it here.  But that is not likely,
 *  because we can have many new seqedit objects in play, because we can
 *  edit many at once.
 */

seqmenu::~seqmenu ()
{
    /*
     * if (not_nullptr(m_seqedit))
     *  delete(m_seqedit)
     */
}

/**
 *  This function sets up the File menu entries.
 *  It also sets up the pattern popup menu entries that are used in mainwid.
 */

void
seqmenu::popup_menu ()
{
    if (not_nullptr(m_menu))
        delete m_menu;

    m_menu = manage(new Gtk::Menu());
    if (m_mainperf.is_active(m_current_seq))
    {
        m_menu->items().push_back
        (
            MenuElem("Edit...", mem_fun(*this, &seqmenu::seq_edit))
        );
    }
    else
    {
        m_menu->items().push_back
        (
            MenuElem("New", mem_fun(*this, &seqmenu::seq_edit))
        );
    }
    m_menu->items().push_back(SeparatorElem());
    if (m_mainperf.is_active(m_current_seq))
    {
        m_menu->items().push_back
        (
            MenuElem("Cut", mem_fun(*this, &seqmenu::seq_cut))
        );
        m_menu->items().push_back
        (
            MenuElem("Copy", mem_fun(*this, &seqmenu::seq_copy))
        );
    }
    else
    {
        m_menu->items().push_back
        (
            MenuElem("Paste", mem_fun(*this, &seqmenu::seq_paste))
        );
    }
    m_menu->items().push_back(SeparatorElem());
    Gtk::Menu * menu_song = manage(new Gtk::Menu());
    m_menu->items().push_back(MenuElem("Song", *menu_song));
    if (m_mainperf.is_active(m_current_seq))
    {
        menu_song->items().push_back
        (
            MenuElem("Clear Song Data", mem_fun(*this, &seqmenu::seq_clear_perf))
        );
    }
    menu_song->items().push_back
    (
        MenuElem("Mute All Tracks", mem_fun(*this, &seqmenu::mute_all_tracks))
    );

    /*
     * This is the MIDI channel menu accessible from a non-empty pattern slot
     * on the main window.
     */

    if (m_mainperf.is_active(m_current_seq))
    {
        m_menu->items().push_back(SeparatorElem());
        Gtk::Menu * menu_buses = manage(new Gtk::Menu());
        m_menu->items().push_back(MenuElem("MIDI Bus", *menu_buses));

        /* Get the MIDI buses */

        mastermidibus & masterbus = m_mainperf.master_bus();
        for (int bus = 0; bus < masterbus.get_num_out_buses(); ++bus)
        {
            Gtk::Menu * menu_channels = manage(new Gtk::Menu());
            menu_buses->items().push_back
            (
                MenuElem(masterbus.get_midi_out_bus_name(bus), *menu_channels)
            );

            char b[4];
            for (int channel = 0; channel < 16; channel++) /* channel menu */
            {
                snprintf(b, sizeof(b), "%d", channel + 1);
                std::string name = std::string(b);
                std::string s = usr().instrument_name(bus, channel);
                if (! s.empty())
                    name += (std::string(" ") + s);

#define SET_BUS     mem_fun(*this, &seqmenu::set_bus_and_midi_channel)

                menu_channels->items().push_back
                (
                    MenuElem(name, sigc::bind(SET_BUS, bus, channel))
                );
            }
        }
    }
    m_menu->popup(0, 0);
}

/**
 *  Sets up the bus, MIDI channel, and dirtiness flag of the current
 *  sequence in the main perform object, as per the give parameters.
 */

void
seqmenu::set_bus_and_midi_channel (int a_bus, int a_ch)
{
    if (m_mainperf.is_active(m_current_seq))
    {
        m_mainperf.get_sequence(m_current_seq)->set_midi_bus(a_bus);
        m_mainperf.get_sequence(m_current_seq)->set_midi_channel(a_ch);
        m_mainperf.get_sequence(m_current_seq)->set_dirty();
    }
}

/**
 *  Mutes all tracks in the main perform object.
 */

void
seqmenu::mute_all_tracks ()
{
    m_mainperf.mute_all_tracks();
}

/**
 *  This menu callback launches the sequence-editor (pattern editor)
 *  window.  If it is already open for that sequence, this function just
 *  raises it.
 *
 *  Note that the m_seqedit member to which we save the new pointer is
 *  currently there just to avoid a compiler warning.
 *
 *  Also, if a new sequences is created, we set the m_modified flag to
 *  true, even though the sequence might later be deleted.  Too much
 *  modification to keep track of!
 */

void
seqmenu::seq_edit ()
{
    seqedit * sed;
    if (m_mainperf.is_active(m_current_seq))
    {
        if (! m_mainperf.get_sequence(m_current_seq)->get_editing())
        {
            sed = new seqedit
            (
                m_mainperf,
                *m_mainperf.get_sequence(m_current_seq),
                m_current_seq
            );
            m_seqedit = sed;            /* prevents "unused" warning      */
            m_mainperf.modify();
        }
        else
            m_mainperf.get_sequence(m_current_seq)->set_raise(true);
    }
    else
    {
        seq_new();
        sed = new seqedit
        (
            m_mainperf,
            *m_mainperf.get_sequence(m_current_seq),
            m_current_seq
        );
        m_seqedit = sed;                /* prevents "unused" warning      */
        m_mainperf.modify();
    }
}

/**
 *  This function sets the new sequence into the perform object, a bit
 *  prematurely, though.
 */

void
seqmenu::seq_new ()
{
    if (! m_mainperf.is_active(m_current_seq))
    {
        m_mainperf.new_sequence(m_current_seq);
        m_mainperf.get_sequence(m_current_seq)->set_dirty();
    }
}

/**
 *  Copies the selected (current) sequence to the clipboard sequence.
 *
 * \todo
 *      Can be offloaded to a perform member function that accepts a
 *      sequence clipboard non-const reference parameter.
 */

void
seqmenu::seq_copy ()
{
    if (m_mainperf.is_active(m_current_seq))
        m_clipboard = *(m_mainperf.get_sequence(m_current_seq));
}

/**
 *  Deletes the selected (current) sequence and copies it to the clipboard
 *  sequence, <i> if </i> it is not in edit mode.
 *
 * \todo
 *      A lot of seq_cut() can be offloaded to a (new) perform member
 *      function that takes a sequence clipboard non-const reference
 *      parameter.
 */

void
seqmenu::seq_cut ()
{
    if (m_mainperf.is_active(m_current_seq) &&
            ! m_mainperf.is_sequence_in_edit(m_current_seq))
    {
        m_clipboard = *(m_mainperf.get_sequence(m_current_seq));
        m_mainperf.delete_sequence(m_current_seq);
        redraw(m_current_seq);
    }
}

/**
 *  Pastes the sequence clipboard into the current sequence, if the
 *  current sequence slot is not active.  Then it sets the dirty flag for
 *  the destination sequence.
 *
 * \todo
 *      All of seq_paste() can be offloaded to a (new) perform member
 *      function with a const clipboard reference parameter.
 */

void
seqmenu::seq_paste ()
{
    if (! m_mainperf.is_active(m_current_seq))
    {
        m_mainperf.new_sequence(m_current_seq);
        *(m_mainperf.get_sequence(m_current_seq)) = m_clipboard;
        m_mainperf.get_sequence(m_current_seq)->set_dirty();
    }
}

/**
 *  If the current sequence is active, this function pushes a trigger
 *  undo in the main perform object, clears its sequence triggers for the
 *  current seqeuence, and sets the dirty flag of the sequence.
 *
 * \todo
 *      All of seq_paste() can be offloaded to a (new) perform member
 *      function.
 */

void
seqmenu::seq_clear_perf ()
{
    if (m_mainperf.is_active(m_current_seq))
    {
        m_mainperf.push_trigger_undo();
        m_mainperf.clear_sequence_triggers(m_current_seq);
        m_mainperf.get_sequence(m_current_seq)->set_dirty();
    }
}

}           // namespace seq64

/*
 * seqmenu.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

