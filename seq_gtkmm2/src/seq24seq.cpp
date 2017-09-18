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
 * \file          seq24seq.cpp
 *
 *  This module declares/defines the mouse interactions for the "seq24"
 *  mode in the pattern/sequence editor's event panel, the narrow string
 *  between the piano roll and the data panel that's at the bottom.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-02
 * \updates       2016-07-16
 * \license       GNU GPLv2 or above
 *
 *  This code was extracted from seqevent to make that module more
 *  manageable.
 *
 *  One thing to note is that the seqevent user-interface isn't very high, so
 *  that y values don't mean anything in it.  It's just high enough to be
 *  visible and move the mouse horizontally in it.
 *
 * \obsolete
 *      This module has been "merged" into seqevent.
 */

#include <gdkmm/cursor.h>
#include <gtkmm/button.h>

#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.     */
#include "gui_key_tests.hpp"            /* seq64::is_no_modifier()      */
#include "seq24seq.hpp"
#include "seqevent.hpp"
#include "sequence.hpp"                 /* for full usage of seqevent   */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

Seq24SeqEventInput::Seq24SeqEventInput
(
    perform & p,
    sequence & seq,
    int zoom,
    int snap,
    seqdata & seqdata_wid,
    Gtk::Adjustment & hadjust
) :
    seqevent    (p, seq, zoom, snap, seqdata_wid, hadjust),
    m_adding    (false)
{
    //
}

/**
 *  Implements the on-button-press event callback.  Set values for dragging,
 *  then reset the box that holds dirty redraw spot.  Then do the rest.
 *
 * \param ev
 *      The button event for the press of a mouse button.
 *
 * \return
 *      Returns true if a likely modification was made.  This function used to
 *      return true all the time.
 */

bool
Seq24SeqEventInput::on_button_press_event (GdkEventButton * ev)
{
    return true;
}

/**
 *  Implements the on-button-release callback.
 *
 * \param ev
 *      The button event for the release of a mouse button.
 *
 * \return
 *      Returns true if a likely modification was made.  This function used to
 *      return true all the time.
 */

bool
Seq24SeqEventInput::on_button_release_event (GdkEventButton * ev)
{
    return true;
}

/**
 *  Implements the on-motion-notify event.
 *
 * \param ev
 *      The button event for the motion of the mouse cursor.
 *
 * \return
 *      Returns true if a likely modification was made.  This function used to
 *      return true all the time.
 */

bool
Seq24SeqEventInput::on_motion_notify_event (GdkEventMotion * ev)
{
    return true;
}

}           // namespace seq64

/*
 * seq24seq.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

