#ifndef SEQ64_SEQ24SEQROLL_HPP
#define SEQ64_SEQ24SEQROLL_HPP

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
 * \file          seq24seqroll.hpp
 *
 *  This module declares/defines the base class for handling the Seq24
 *  mode of mouse interaction in the piano roll of the pattern/sequence
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-07-14
 * \license       GNU GPLv2 or above
 *
 */

#error The seq24seqroll module is now OBSOLETE.

namespace seq64
{
    class seqroll;

/**
 *  Implements the Seq24 mouse interaction paradigm for the seqroll.
 */

class Seq24SeqRollInput
{

private:

    /**
     *  True if adding events to the seqroll via the mouse.

    bool m_adding;
     */

public:

    /**
     * Default constructor.
     */

    Seq24SeqRollInput () // : m_adding(false)
    {
        // Empty body
    }

    void set_adding (bool adding, seqroll & ths);

public:         // callbacks

    bool on_button_press_event (GdkEventButton * ev, seqroll & ths);
    bool on_button_release_event (GdkEventButton * ev, seqroll & ths);
    bool on_motion_notify_event (GdkEventMotion * ev, seqroll & ths);

};

}           // namespace seq64

#endif      // SEQ64_SEQ24SEQROLL_HPP

/*
 * seq24seqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

