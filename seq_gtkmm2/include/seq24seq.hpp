#ifndef SEQ64_SEQ24SEQ_HPP
#define SEQ64_SEQ24SEQ_HPP

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
 * \file          seq24seq.hpp
 *
 *  This module declares/defines the mouse interactions for the "seq24"
 *  mode.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-02
 * \updates       2016-06-07
 * \license       GNU GPLv2 or above
 *
 */

namespace seq64
{
    class seqevent;

/**
 *  This structure implement the normal interaction methods for Seq24.
 */

struct Seq24SeqEventInput
{
    /**
     *  True if we're adding events via the mouse.
     */

    bool m_adding;

    /**
     * Default constructor.
     */

    Seq24SeqEventInput () : m_adding(false)
    {
        // Empty body
    }

    void set_adding (bool adding, seqevent & ths);
    bool on_button_press_event (GdkEventButton * ev, seqevent & ths);
    bool on_button_release_event (GdkEventButton * ev, seqevent & ths);
    bool on_motion_notify_event (GdkEventMotion * ev, seqevent & ths);
};

}           // namespace seq64

#endif      // SEQ64_SEQ24SEQ_HPP

/*
 * seq24seq.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

