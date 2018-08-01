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
 * \file          qseqframe.cpp
 *
 *  This module declares/defines the base class for managing the editing of
 *  sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-07-27
 * \updates       2018-07-31
 * \license       GNU GPLv2 or above
 *
 *  Sequencer64 (Qt version) has two different pattern editor frames to
 *  support:
 *
 *      -   New.  This pattern-editor frame is used in its own window.  It is
 *          larger and has a lot of functionality.  Furthermore, it
 *          keeps the time, event, and data views in full view at all times
 *          when scrolling, just like the Gtkmm-2.4 version of the pattern
 *          editor.
 *      -   Kepler34.  This frame is not as functional, but it does fit in the
 *          tab better, and it scrolls the time, event, keys, and roll panels
 *          as if they were one object.
 */

#include <QWidget>

#include "perform.hpp"                  /* seq64::perform reference         */
#include "qseqdata.hpp"
#include "qseqframe.hpp"
#include "qseqkeys.hpp"
#include "qseqroll.hpp"
#include "qseqtime.hpp"
#include "qstriggereditor.hpp"
#include "settings.hpp"                 /* usr()                            */

/*
 *  Do not document the name space.
 */

namespace seq64
{

/**
 *
 * \param p
 *      Provides the perform object to use for interacting with this sequence.
 *      Among other things, this object provides the active PPQN.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.  This number is also the pattern-slot number for this
 *      sequence and for this window.  Ranges from 0 to 1024.  The caller
 *      should ensure this is a valid, non-blank sequence.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qseqframe::qseqframe
(
    perform & p,
    int seqid,
    QWidget * parent
) :
    QFrame              (parent),
    m_performance       (p),                            // a reference
    m_seq               (*perf().get_sequence(seqid)),  // a pointer-->reference
    m_seqkeys           (nullptr),
    m_seqtime           (nullptr),
    m_seqroll           (nullptr),
    m_seqdata           (nullptr),
    m_seqevent          (nullptr),
    m_initial_zoom      (SEQ64_DEFAULT_ZOOM),
    m_zoom              (SEQ64_DEFAULT_ZOOM),           // fixed below
    m_ppqn              (p.get_ppqn())                  // MIGHT REMOVE
{
    // bool ok = initialize_panels();
}

/**
 *  A virtual function to halve the zoom and set it.
 */

void
qseqframe::zoom_in ()
{
    int z = m_zoom / 2;
    set_zoom(z);
}

/**
 *  A virtual function to double the zoom and set it.
 */

void
qseqframe::zoom_out ()
{
    int z = m_zoom * 2;
    set_zoom(z);
}

/**
 *  Sets the zoom parameter, z.  If valid, then the m_zoom member is set.
 *  The new setting is passed to the roll, time, data, and event panels
 *  [which each call their own set_dirty() functions].
 *
 * \param z
 *      The desired zoom value, which is checked for validity.
 */

void
qseqframe::set_zoom (int z)
{
    if ((z >= usr().min_zoom()) && (z <= usr().max_zoom()))
    {
        m_zoom = z;
        if (not_nullptr(m_seqroll))
            m_seqroll->set_zoom(z);

        if (not_nullptr(m_seqtime))
            m_seqtime->set_zoom(z);

        if (not_nullptr(m_seqevent))
            m_seqdata->set_zoom(z);

        if (not_nullptr(m_seqdata))
            m_seqevent->set_zoom(z);
    }
}

/**
 *  Sets the dirty status of all of the panels.  However, note that in the
 *  case of zoom, for example, it also sets dirtiness, via qseqbase.
 */

void
qseqframe::set_dirty ()
{
    if (not_nullptr(m_seqroll))
        m_seqroll->set_dirty();

    if (not_nullptr(m_seqtime))
        m_seqtime->set_dirty();

    if (not_nullptr(m_seqevent))
        m_seqevent->set_dirty();

    if (not_nullptr(m_seqdata))
        m_seqdata->set_dirty();
}

}           // namespace seq64

/*
 * qseqframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

