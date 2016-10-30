#ifndef SEQ64_LFOWND_HPP
#define SEQ64_LFOWND_HPP

/*
 *  This file is part of seq42/seq32/sequencer64.
 *
 *  seq32 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq32 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          lfownd.hpp
 *
 *  This module declares/defines the base class for the LFO window of the
 *  application.
 *
 * \library       sequencer64 application
 * \author        Seq42 team; modifications by Chris Ahlstrom
 * \date          2016-07-30
 * \updates       2016-10-30
 * \license       GNU GPLv2 or above
 *
 *  Created on: 22 mar 2013
 *      Author: mattias
 *
 *  The LFO window holds the menu and the controls of the LFO support.
 *
 *  Note that we move the wave_func() function to the calculations module to
 *  make it available without needing to include GUI code in the libseq64
 *  library.
 */

#include <gdkmm/cursor.h>
#include <gtkmm/window.h>

#include "gui_window_gtk2.hpp"          // seq64::qui_window_gtk2

namespace Gtk
{
    class HBox;
    class Label;
    class VScale;
}

namespace seq64
{
    class perform;
    class seqdata;
    class sequence;

/**
 *  One LFO window class.  Personally, it seems a bit of a odd duck to be
 *  included in Sequencer64, so we're thinking of a better way to manage the
 *  data managed by this window.
 */

class lfownd : public gui_window_gtk2
{

private:

    /**
     *  The sequence associated with this window.
     */

    sequence & m_seq;

    /**
     *  The seqdata associated with this window.
     */

    seqdata & m_seqdata;

    /*
     * GUI elements
     */

    Gtk::HBox * m_hbox;             /**< The main horizontal packing box.   */
    Gtk::VScale * m_scale_value;    /**< Vertical slider for value.         */
    Gtk::VScale * m_scale_range;    /**< Vertical slider for range.         */
    Gtk::VScale * m_scale_speed;    /**< Vertical slider for speed.         */
    Gtk::VScale * m_scale_phase;    /**< Vertical slider for phase.         */
    Gtk::VScale * m_scale_wave;     /**< Vertical slider for wave type.     */
    Gtk::Label * m_wave_name;       /**< Human readable name for wave type. */

    /**
     *  Value.
     */

    double m_value;

    /**
     *  Range.
     */

    double m_range;

    /**
     *  Speed.
     */

    double m_speed;

    /**
     *  Phase.
     */

    double m_phase;

    /**
     *  Wave type.
     */

    wave_type_t m_wave;

public:

    lfownd (perform & p, sequence & seq, seqdata & sdata);
    virtual ~lfownd ();

    void toggle_visible ();

private:

    void scale_lfo_change ();

private:            // callbacks

    bool on_focus_out_event (GdkEventFocus * p0);

};

}           // namespace seq64

#endif      // SEQ64_LFOWND_HPP

/*
 * lfownd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

