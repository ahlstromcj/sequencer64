#ifndef SEQ64_SEQMENU_HPP
#define SEQ64_SEQMENU_HPP

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
 * \file          seqmenu.hpp
 *
 *  This module declares/defines the class that handles the right-click
 *  menu of the sequence slots in the pattern window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include "sequence.hpp"

namespace Gtk
{
    class Menu;
}

namespace seq64
{

class perform;
class seqedit;

/**
 *  This class handles the right-click menu of the sequence slots in the
 *  pattern window.
 *
 *  It is an abstract base class.
 */

class seqmenu : public virtual Glib::ObjectBase
{

private:

    Gtk::Menu * m_menu;
    perform & m_mainperf;
    sequence m_clipboard;

    /**
     * \change
     *      Added by Chris on 2015-08-02 based on compiler warnings and a
     *      comment warning in the seq_edit() function.  We'll save the
     *      result of that function here, and will let valgrind tell us
     *      later if Gtkmm takes care of it.
     */

    seqedit * m_seqedit;

    int m_current_seq;
    bool m_modified;

public:

    seqmenu (perform & a_p);

    /**
     *  Provides a rote base-class destructor.  This is necessary in an
     *  abstraction base class.
     */

    virtual ~seqmenu ();

    /**
     * \getter m_current_seq
     */

    int current_sequence () const
    {
        return m_current_seq;
    }

    /**
     * \getter m_modified
     */

    bool is_modified () const
    {
        return m_modified;
    }

protected:

    /**
     * \setter m_current_seq
     */

    void current_sequence (int seq)
    {
        if (seq >= 0)                   // shall we validate the upper end?
            m_current_seq = seq;
    }

    /**
     * \setter m_modified
     */

    void is_modified (bool flag)
    {
        m_modified = flag;
    }

    void popup_menu ();

private:

    void seq_edit ();
    void seq_new ();
    void seq_copy ();
    void seq_cut ();
    void seq_paste ();
    void seq_clear_perf ();
    void set_bus_and_midi_channel (int a_bus, int a_ch);
    void mute_all_tracks ();

    virtual void redraw (int a_sequence) = 0;   // pure virtual function

private:            // callback

    void on_realize ();

};

}           // namespace seq64

#endif      // SEQ64_SEQMENU_HPP

/*
 * seqmenu.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
