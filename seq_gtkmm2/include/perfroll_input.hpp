#ifndef SEQ64_PERFROLL_INPUT_HPP
#define SEQ64_PERFROLL_INPUT_HPP

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
 * \file          perfroll_input.hpp
 *
 *  This module declares/defines the base class for the Performance window
 *  mouse input.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-09-17
 * \license       GNU GPLv2 or above
 *
 *  Now refactored to be derived from perfroll directly; no more passing
 *  events along via a "roll" parameter.
 */

#include "midibyte.hpp"                 /* seq64::midipulse typedef     */
#include "perfroll.hpp"                 /* seq64::perfroll class        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

/*
 * Forward references
 */

namespace Gtk
{
    class Adjustment;
}

namespace seq64
{
    class perform;
    class perfedit;

/**
 *  Implements the default (Seq24) performance input characteristics of this
 *  application.
 */

class Seq24PerfInput : public perfroll
{

private:

    /**
     *  The current tick for the current segment?
     */

    midipulse m_effective_tick;

public:

    Seq24PerfInput
    (
        perform & perf,
        perfedit & parent,
        Gtk::Adjustment & hadjust,
        Gtk::Adjustment & vadjust,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );

    /**
     * virtual destructor
     */

    ~Seq24PerfInput ()
    {
        // no code
    }

protected:

    virtual void activate_adding (bool a_adding);
    virtual bool handle_motion_key (bool is_left);

protected:

    virtual bool on_button_press_event (GdkEventButton * ev);
    virtual bool on_button_release_event (GdkEventButton * ev);
    virtual bool on_motion_notify_event (GdkEventMotion * ev);

};          // class Seq24PerfInput

}           // namespace seq64

#endif      // SEQ64_PERFROLL_INPUT_HPP

/*
 * perfroll_input.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

