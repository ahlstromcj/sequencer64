#ifndef SEQ64_FRUITYPERFROLL_INPUT_HPP
#define SEQ64_FRUITYPERFROLL_INPUT_HPP

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
 * \file          fruityperfroll_input.hpp
 *
 *  This module declares/defines the class for the "fruity" flavor of the
 *  Performance window mouse input.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-06-08
 * \license       GNU GPLv2 or above
 *
 *  Note that this class doesn't handle keystrokes (directly), so even if the
 *  user chooses it, the Seq24 input object is also needed, to handle those
 *  keystrokes.
 */

#include "perfroll_input.hpp"           /* ABC and Seq24 input class    */

namespace seq64
{
    class perfroll;

/**
 *  Implements the performance input of that certain fruity sequencer that
 *  people seem to like.
 */

class FruityPerfInput : public AbstractPerfInput
{
    friend class perfroll;

private:

    /**
     *  The current x value of the mouse.
     */

    long m_current_x;

    /**
     *  The current y value of the mouse.
     */

    long m_current_y;

public:

    /**
     *  Default constructor.
     */

    FruityPerfInput () :
        AbstractPerfInput   (),
        m_current_x         (0),
        m_current_y         (0)
    {
        // Empty body
    }

    bool on_button_press_event (GdkEventButton * ev, perfroll & roll);
    bool on_button_release_event (GdkEventButton * ev, perfroll & roll);
    bool on_motion_notify_event (GdkEventMotion * ev, perfroll & roll);

private:

    void update_mouse_pointer (perfroll & roll);
    bool on_left_button_pressed (GdkEventButton * ev, perfroll & roll);
    bool on_right_button_pressed (GdkEventButton * ev, perfroll & roll);

};          // FruityPerfInput

}           // namespace seq64

#endif      // SEQ64_FRUITYPERFROLL_INPUT_HPP

/*
 * fruityperfroll_input.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

