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
 * \updates       2015-09-10
 * \license       GNU GPLv2 or above
 *
 */

class perfroll;

/**
 *  Provides an abstract base class to provide the minimal interface for
 *  the various "perf input" classes.
 */

class AbstractPerfInput
{

public:

    AbstractPerfInput ()
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
        perfroll& roll
    ) = 0;

};

/**
 *  Implements the performance input of that certain fruity sequencer that
 *  people seem to like.
 */

class FruityPerfInput : public AbstractPerfInput
{

private:

    bool m_adding_pressed;
    long m_current_x;
    long m_current_y;

public:

    FruityPerfInput() :
        AbstractPerfInput   (),
        m_adding_pressed    (false),
        m_current_x         (0),
        m_current_y         (0)
    {
        // Empty body
    }

    bool on_button_press_event (GdkEventButton * a_ev, perfroll & roll);
    bool on_button_release_event (GdkEventButton * a_ev, perfroll & roll);
    bool on_motion_notify_event (GdkEventMotion * a_ev, perfroll & roll);

private:

    void updateMousePtr (perfroll & roll);
    void on_left_button_pressed (GdkEventButton * a_ev, perfroll & roll);
    void on_right_button_pressed (GdkEventButton * a_ev, perfroll & roll);
};

/**
 *  Implements the default performance input characteristics of this
 *  application.
 */

class Seq24PerfInput : public AbstractPerfInput
{

private:

    bool m_adding;
    bool m_adding_pressed;

public:

    Seq24PerfInput() :
        AbstractPerfInput   (),
        m_adding            (false),
        m_adding_pressed    (false)
    {
        // Empty body
    }

    bool on_button_press_event (GdkEventButton * a_ev, perfroll & roll);
    bool on_button_release_event (GdkEventButton * a_ev, perfroll & roll);
    bool on_motion_notify_event (GdkEventMotion * a_ev, perfroll & roll);

private:

    void set_adding (bool a_adding, perfroll & roll);
};

#endif   // SEQ64_PERFROLL_INPUT_HPP

/*
 * perfroll_input.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
