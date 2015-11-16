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
 * \updates       2015-11-01
 * \license       GNU GPLv2 or above
 *
 */

namespace seq64
{

class perfroll;

/**
 *  Provides an abstract base class to provide the minimal interface for
 *  the various "perf input" classes.
 */

class AbstractPerfInput
{

protected:

    bool m_adding_pressed;

public:

    AbstractPerfInput () :
        m_adding_pressed    (false)
    {
        // Empty body
    }

    virtual ~AbstractPerfInput ()
    {
        // Empty body
    }

    virtual bool on_button_press_event
    (
        GdkEventButton * a_ev,
        perfroll & roll
    ) = 0;

    virtual bool on_button_release_event
    (
        GdkEventButton * a_ev,
        perfroll & roll
    ) = 0;

    virtual bool on_motion_notify_event
    (
        GdkEventMotion * a_ev,
        perfroll & roll
    ) = 0;

};

/**
 *  Implements the default (Seq24) performance input characteristics of this
 *  application.
 */

class Seq24PerfInput : public AbstractPerfInput
{

    friend class perfroll;

private:

    bool m_adding;
    long m_effective_tick;

public:

    Seq24PerfInput() :
        AbstractPerfInput   (),
        m_adding            (false),
        m_effective_tick    (0)
    {
        // Empty body
    }

    bool on_button_press_event (GdkEventButton * a_ev, perfroll & roll);
    bool on_button_release_event (GdkEventButton * a_ev, perfroll & roll);
    bool on_motion_notify_event (GdkEventMotion * a_ev, perfroll & roll);

private:

    void set_adding (bool a_adding, perfroll & roll);
    bool handle_motion_key (bool is_left, perfroll & roll);

    /**
     * \getter m_adding
     */

    bool is_adding () const
    {
        return m_adding;
    }

};

}           // namespace seq64

#endif      // SEQ64_PERFROLL_INPUT_HPP

/*
 * perfroll_input.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
