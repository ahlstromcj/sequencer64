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
 * \updates       2018-03-20
 * \license       GNU GPLv2 or above
 *
 *  This object also does some minor coordination of editing a sequence via
 *  the pattern editor versus the event editor.
 *
 * \warning
 *      We have currently disabled the "Event Edit..." menu entry at present,
 *      by default, because of a difficult issue with segfaults caused when
 *      events are deleted by the event editor (classes eventedit and
 *      eventslots).  If you want to enable the event editor for your own
 *      experiments, use the "--enable-eveditor" option with the configure
 *      script.
 */

#include <gtkmm/box.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>

#include "eventedit.hpp"
#include "font.hpp"
#include "seqedit.hpp"
#include "seqmenu.hpp"
#include "settings.hpp"                 /* seq64::usr()                 */

using namespace Gtk::Menu_Helpers;

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The single map of seqedit objects, for seqedit updates and management.
 */

seqmenu::SeqeditMap seqmenu::sm_seqedit_list;

/**
 *  Current saved sequence data.
 */

sequence seqmenu::sm_clipboard;

/**
 *  Indicates if sequence data is present or not.
 */

bool seqmenu::sm_clipboard_empty = true;

/**
 *  Principal constructor.  Apart from filling in some of the members,
 *  this function initializes the clipboard, so that we don't get a crash
 *  on a paste with no previous copy.
 *
 * \param p
 *      The main performance object representing the whole MIDI song.
 */

seqmenu::seqmenu (perform & p)
 :
    m_menu              (nullptr),
    m_mainperf          (p),
    m_seqedit           (nullptr),
    m_eventedit         (nullptr),
    m_current_seq       (SEQ64_ALL_TRACKS)  /* (0) is not really current yet    */
{
    /*
     * Why did we remove this?  I believe it was a pull request.  But let's
     * restore it for some testing of copy/paste, at least.
     */

    sm_clipboard.set_master_midi_bus(&m_mainperf.master_bus());

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
     *      delete(m_seqedit)
     *
     * if (not_nullptr(m_eventedit))
     *      delete(m_eventedit)
     */
}

/**
 *  This function sets up the pattern menu entries.  It also sets up the
 *  pattern popup menu entries that are used in mainwid.  Note that, for the
 *  selected sequence, the "Edit" and "Event Edit" menu entries are not
 *  included if a pattern editor or event editor is already running.
 */

void
seqmenu::popup_menu ()
{
    if (not_nullptr(m_menu))
        delete m_menu;

    m_menu = manage(new Gtk::Menu());
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s) && ! s->get_editing())
        {
            m_menu->items().push_back
            (
                MenuElem("Edit...", mem_fun(*this, &seqmenu::seq_edit))
            );

#if SEQ64_ENABLE_EVENT_EDITOR

            /**
             * The new event editor seems to create far-reaching problems that
             * we do not yet understand, so it is now possible to disable it at
             * build time.  We have mitigated most of those problems by not
             * allowing both a seq_edit() and a seq_event_edit() at the same
             * time.
             */

            m_menu->items().push_back
            (
                MenuElem
                (
                    "Event Edit...", mem_fun(*this, &seqmenu::seq_event_edit)
                )
            );
#endif
            m_menu->items().push_back(SeparatorElem());
        }
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
            MenuElem("New", mem_fun(*this, &seqmenu::seq_edit))
        );
        m_menu->items().push_back(SeparatorElem());
        m_menu->items().push_back
        (
            MenuElem("Paste", mem_fun(*this, &seqmenu::seq_paste))
        );
    }
    m_menu->items().push_back(SeparatorElem());
    Gtk::Menu * menu_song = manage(new Gtk::Menu());
    m_menu->items().push_back(MenuElem("Song", *menu_song));
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        menu_song->items().push_back
        (
            MenuElem
            (
                "Clear This Track's Song Data",
                mem_fun(*this, &seqmenu::seq_clear_perf)
            )
        );
    }
    menu_song->items().push_back
    (
        MenuElem("Mute All Tracks", mem_fun(*this, &seqmenu::mute_all_tracks))
    );
    menu_song->items().push_back
    (
        MenuElem("Unmute All Tracks", mem_fun(*this, &seqmenu::unmute_all_tracks))
    );
    menu_song->items().push_back
    (
        MenuElem("Toggle All Tracks", mem_fun(*this, &seqmenu::toggle_all_tracks))
    );
    menu_song->items().push_back
    (
        MenuElem
        (
            "Toggle Playing Tracks", mem_fun(*this, &seqmenu::toggle_playing_tracks)
        )
    );

#ifdef SEQ64_SHOW_COLOR_PALETTE               // EXPERIMENTAL

    /*
     * Sets up the optional color palette menu.  Currently, we don't access
     * the PaletteColor enumeration, but the numbers should match it.
     *
     * Also, we still need to add the selected color to the mainwid and
     * perfroll drawing mechanisms.
     */

#define SET_COLOR   mem_fun(*this, &seqmenu::set_color)
#define PUSH_COLOR(name, c) \
    menu_color->items().push_back(MenuElem(name, sigc::bind(SET_COLOR, (c))))

    Gtk::Menu * menu_color = manage(new Gtk::Menu());
    m_menu->items().push_back(MenuElem("Color", *menu_color));
    PUSH_COLOR("None", -1);
    menu_color->items().push_back(SeparatorElem());
    /* Not useful: PUSH_COLOR("Black", 0);                                  */
    PUSH_COLOR("Red", 1);
    PUSH_COLOR("Green", 2);
    PUSH_COLOR("Yellow", 3);
    PUSH_COLOR("Blue", 4);
    PUSH_COLOR("Magenta", 5);
    /* Not accessible by this name in Gtkmm: PUSH_COLOR("Cyan", 6);         */
    PUSH_COLOR("White", 7);
    menu_color->items().push_back(SeparatorElem());
    /* Not useful: PUSH_COLOR("Dk Black", 8);   */
    PUSH_COLOR("Dk Red", 9);
    PUSH_COLOR("Dk Green", 10);
    /* Not accessible by this name in Gtkmm: PUSH_COLOR("Dk Yellow", 11);   */
    PUSH_COLOR("Dk Blue", 12);
    PUSH_COLOR("Dk Magenta", 13);
    PUSH_COLOR("Dk Cyan", 14);
    /* Not useful:  PUSH_COLOR("Dk White", 15);                             */
    menu_color->items().push_back(SeparatorElem());
    PUSH_COLOR("Orange", 16);
    /* Not accessible by this name in Gtkmm: PUSH_COLOR("Pink", 17);        */
    /* Conflicts with one-shot: PUSH_COLOR("Grey", 18);                     */
    menu_color->items().push_back(SeparatorElem());
    PUSH_COLOR("Dk Orange", 19);
    /* Not accessible by this name in Gtkmm: PUSH_COLOR("Dk Pink", 20);     */
    /* Conflicts with queuing: PUSH_COLOR("Dk Grey", 21);                   */

#endif  // SEQ64_SHOW_COLOR_PALETTE

    /*
     * This is the bottom part of the menu accessible from a non-empty pattern
     * slot on the main window.  Some of the seqedit settings functions are
     * also exposed here.
     */

    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        m_menu->items().push_back(SeparatorElem());

#ifdef SEQ64_STAZED_TRANSPOSE
#define SET_TRANS   mem_fun(*this, &seqmenu::set_transposable)

        sequence * s = get_current_sequence();
        if (not_nullptr(s) && s->get_transposable())
        {
            m_menu->items().push_back
            (
                MenuElem("Disable Transpose", sigc::bind(SET_TRANS, false))
            );
        }
        else
        {
            m_menu->items().push_back
            (
                MenuElem("Enable Transpose", sigc::bind(SET_TRANS, true))
            );
        }
#endif

        Gtk::Menu * menu_buses = manage(new Gtk::Menu());
        m_menu->items().push_back(MenuElem("MIDI Bus", *menu_buses));

#define SET_BUS     mem_fun(*this, &seqmenu::set_bus_and_midi_channel)

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
 *
 * \param bus
 *      The MIDI buss number to set (bus vs buss?  You decide.)
 *
 * \param ch
 *      The MIDI channel number to set.
 */

void
seqmenu::set_bus_and_midi_channel (int bus, int ch)
{
    if (is_current_seq_active())            /* also checks sequence pointer */
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            if (s->get_midi_bus() != bus || s->get_midi_channel() != ch)
                s->set_dirty();

            s->set_midi_bus(bus, true);       /* now can set perform modify */
            s->set_midi_channel(ch, true);    /* ditto                      */

            /*
             * New for 0.9.15.2:  Let's try to update the seqedit as well, if it
             * is open.  Seems to work!  But not on all systems, as issue 50
             * suggests.  We need a better solution.
             *
             * if (not_nullptr(m_seqedit))
             * {
             *     m_seqedit->set_midi_bus(bus);
             *     m_seqedit->set_midi_channel(ch);
             * }
             */

            iterator sit = sm_seqedit_list.find(s->number());
            if (sit != sm_seqedit_list.end())
            {
                if (not_nullptr(sit->second))
                {
                    sit->second->set_midi_bus(bus);
                    sit->second->set_midi_channel(ch);
                }
            }
        }
    }
}

#ifdef SEQ64_SHOW_COLOR_PALETTE               // EXPERIMENTAL

/**
 *  Sets up or resets the experimental colored sequence feature.
 *
 * \param color
 *      The PaletteColor value to use.
 */

void
seqmenu::set_color (int color)
{
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            s->color(color);
            s->set_dirty();
        }
    }
}

#endif  // SEQ64_SHOW_COLOR_PALETTE

/**
 *  Sets the "is-transposable" flag of the current sequence.
 *
 * \param flag
 *      The value to use to set the flag.
 */

void
seqmenu::set_transposable (bool flag)
{
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
#ifdef SEQ64_STAZED_TRANSPOSE
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            if (s->get_transposable() != flag)
                s->set_dirty();

            s->set_transposable(flag);
        }
#endif
    }
}

/**
 *  This menu callback launches the sequence-editor (pattern editor) window.
 *  If it is already open for that sequence, this function just raises it.
 *
 *  Note that the m_seqedit member to which we save the new pointer is
 *  currently there just to avoid a compiler warning.
 *
 *  Also, if a new sequences is created, we set the m_modified flag to true,
 *  even though the sequence might later be deleted.  Too much modification to
 *  keep track of!
 *
 *  An oddity is that calling show_all() here does not work unless the
 *  seqedit() constructor makes its show_all() call.
 */

void
seqmenu::seq_edit ()
{
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            if (! s->get_editing())
                m_seqedit = create_seqedit(*s);
            else
                s->set_raise(true);
        }
    }
    else
    {
        seq_new();
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
            m_seqedit = create_seqedit(*s);
    }
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
    set_edit_sequence(current_seq());
#endif
}

/**
 *  A wrapper function so that we can not only create a new seqedit object,
 *  but have some management over it.  We don't bother checking here if the
 *  insert succeeded.  If it doesn't, all bets are off.
 *
 * \param s
 *      Provides the sequence for which the seqedit will be created.
 *      The perform object and the current_seq() value are implicit
 *      parameters.  This object can obviously be modified by the sequence
 *      editor, so cannot be constant.
 *
 * \return
 *      Returns the pointer to the new seqedit object.
 */

seqedit *
seqmenu::create_seqedit (sequence & s)
{
    seqedit * result = new seqedit(m_mainperf, s, current_seq());
    if (not_nullptr(result))
    {
#if __cplusplus >= 201103L              /* C++11 */
        SeqeditPair p = std::make_pair(current_seq(), result);
#else
        SeqeditPair p = std::make_pair<int, seqedit *>(current_seq(), result);
#endif
        (void) sm_seqedit_list.insert(p);
    }
    return result;
}


/**
 *  A wrapper function to make sure the seqedit object is removed from the
 *  list when it goes away.  Called by seqedit::on_delete_event().
 */

void
seqmenu::remove_seqedit (sequence & s)
{
    int seqnum = s.number();
    int count = int(sm_seqedit_list.erase(seqnum));
    if (count == 0)
    {
        errprint("seqedit::on_delete_event() found nothing to delete");
    }
}

/**
 *  Sets the current sequence and then acts as if the user had clicked on its
 *  slot.
 *
 *  How do we account for the current screenset?  It might not matter if the
 *  mute/unmute keystrokes were designed to work only with the current
 *  screenset.
 *
 * \change ca 2016-11-01
 *      We would like to be able to right-click on a given pattern slot in
 *      mainwid, and figure out if it has a seqedit window open, so that we
 *      can update that window.  So we need to add that seqedit window to a
 *      map of seqedits, keyed by the slot number.  Then we can look up that
 *      slot and see if it has a seqedit window open.  If the seqedit window
 *      closes, that window needs to remove itself from the map.  This won't
 *      be needed for the event editor, which has no functionality from
 *      seqmenu.
 *
 * \param seqnum
 *      The number of the sequence to edit.
 */

void
seqmenu::seq_set_and_edit (int seqnum)
{
    current_seq(seqnum);
    seq_edit();
}

/**
 *  Sets the current sequence and then acts as if the user had right-clicked
 *  on its slot and selected "Event Edit".
 *
 * \param seqnum
 *      The number of the sequence to event-edit.
 */

void
seqmenu::seq_set_and_eventedit (int seqnum)
{
    current_seq(seqnum);
    seq_event_edit();
}

/**
 *  This menu callback launches the new event editor window.  If it is already
 *  open for that sequence, this function just raises it.
 *
 *  Note that the m_eventedit member to which we save the new pointer is
 *  currently there just to avoid a compiler warning.
 *
 *  This menu entry is available only if the selected sequence is active.
 *  That is, if the sequence has already been created.
 *
 *  An oddity is that we need the show_all() call here in order to see the
 *  dialog.  A situation different from that for seqedit!  However, now it
 *  doesn't seem to be needed, and we have put it back into the eventedit
 *  constructor.
 */

void
seqmenu::seq_event_edit ()
{
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            if (! s->get_editing())
                m_eventedit = new eventedit(m_mainperf, *s);
            else
                s->set_raise(true);
        }
    }
    else
    {
        seq_new();
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
            m_eventedit = new eventedit(m_mainperf, *s);
    }
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
    set_edit_sequence(current_seq());
#endif
}

/**
 *  This function sets the new sequence into the perform object, a bit
 *  prematurely, though.  For one thing, if current_seq() is either a -1
 *  or is greater than the maximum allowed sequence number,
 *  perform::is_active() will return false, and we have no idea whether the
 *  sequence is not active or the sequence number is just invalid.  So we need
 *  to check the pointer we got before trying to use it.
 */

void
seqmenu::seq_new ()
{
    if (! is_current_seq_active())      /* false if not active or if null   */
    {
        new_current_sequence();
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
            s->set_dirty();
    }
}

/**
 *  Copies the selected (current) sequence to the clipboard sequence.
 *  We use a more appropriate function than operator =() here:
 *  sequence::partial_assign().
 *
 * \todo
 *      Can be offloaded to a perform member function that accepts a
 *      sequence clipboard non-const reference parameter.
 */

void
seqmenu::seq_copy ()
{
    if (is_current_seq_active())        /* also checks sequence pointer */
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            sm_clipboard.partial_assign(*s);
            sm_clipboard_empty = false;
        }
    }
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
    if (is_current_seq_active() && ! is_current_seq_in_edit())
    {
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
        {
            sm_clipboard.partial_assign(*s);
            m_mainperf.delete_sequence(current_seq());
            sm_clipboard_empty = false;                  /* ca 2017-05-25    */
            redraw(current_seq());
        }
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
    if (! is_current_seq_active())      /* false if not active or if null   */
    {
        new_current_sequence();
        sequence * s = get_current_sequence();
        if (not_nullptr(s) && ! sm_clipboard_empty)
        {
            s->partial_assign(sm_clipboard);
            s->set_dirty();
        }
    }
}

/**
 *  If the current sequence is active, this function pushes a trigger
 *  undo in the main perform object, clears its sequence triggers for the
 *  current sequence, and sets the dirty flag of the sequence.
 */

void
seqmenu::seq_clear_perf ()
{
    if (is_current_seq_active())                /* check sequence pointer   */
    {
        m_mainperf.push_trigger_undo(m_current_seq);        /* stazed fix   */
        m_mainperf.clear_sequence_triggers(current_seq());
        sequence * s = get_current_sequence();
        if (not_nullptr(s))
            s->set_dirty();
    }
}

}           // namespace seq64

/*
 * seqmenu.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

