#ifndef SEQ64_QSEQFRAME_HPP
#define SEQ64_QSEQFRAME_HPP

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
 *  along with seq24; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqframe.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-07-27
 * \updates       2018-07-28
 * \license       GNU GPLv2 or above
 *
 *  Provides an abstract base class so that both the old and the new Qt
 *  sequence-edit frames can be supported.
 *
 *  For now, we're only abstracting the zoom functionality.  Later, we
 *  can abstract other code common between the two frames.
 */

#include <QFrame>

/*
 *  Forward declarations.  The Qt header files are in the cpp file.
 */

class QWidget;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qseqframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qseqkeys;
    class qseqtime;
    class qseqroll;
    class qseqdata;
    class qstriggereditor;
    class sequence;

/**
 *  This frame is the basis for editing an individual MIDI sequence.
 */

class qseqframe : public QFrame
{
    Q_OBJECT

public:

    qseqframe
    (
        perform & p,
        int seqid,
        QWidget * parent = nullptr
    );

    virtual ~qseqframe ()
    {
        // no code
    }

protected:

    /**
     * \getter m_performance
     *      The const version.
     */

    const perform & perf () const
    {
        return m_performance;
    }

    /**
     * \getter m_performance
     *      The non-const version.
     */

    perform & perf ()
    {
        return m_performance;
    }

    /**
     * \getter m_seq
     *      The const version.
     */

    const sequence & seq () const
    {
        return m_seq;
    }

    /**
     * \getter m_seq
     *      The non-const version.
     */

    sequence & seq ()
    {
        return m_seq;
    }

    /**
     * \getter m_ppqn
     */

    int ppqn ()
    {
        return m_ppqn;
    }

    /**
     * \getter m_zoom
     */

    int zoom ()
    {
        return m_zoom;
    }

private slots:

public:             // protected:

    virtual void zoom_in ();
    virtual void zoom_out ();

    /**
     *  Sets the zoom to its "default" value.
     */

    virtual void reset_zoom ()
    {
        set_zoom(m_initial_zoom);
    }

    virtual void set_zoom (int z);
    virtual void set_dirty ();

private:

    perform & m_performance;
    sequence & m_seq;

protected:

    qseqkeys * m_seqkeys;
    qseqtime * m_seqtime;
    qseqroll * m_seqroll;
    qseqdata * m_seqdata;
    qstriggereditor * m_seqevent;

    /**
     *  Provides the initial zoom, used for restoring the original zoom using
     *  the 0 key.
     */

    const int m_initial_zoom;

    /**
     *  Provides the zoom values: 1  2  3  4, and 1, 2, 4, 8, 16.
     *  The value of zoom is the same as the number of pixels per tick on the
     *  piano roll.
     */

    int m_zoom;

    /**
     *  Holds a copy of the current PPQN for the sequence (and the entire MIDI
     *  file).
     */

    int m_ppqn;

};          // class qseqframe

}           // namespace seq64

#endif      // SEQ64_QSEQFRAME_HPP

/*
 * qseqframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

