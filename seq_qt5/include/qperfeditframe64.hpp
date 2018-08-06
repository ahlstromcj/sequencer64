#ifndef SEQ64_QPERFEDITFRAME64_HPP
#define SEQ64_QPERFEDITFRAME64_HPP

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
 * \file          qperfeditframe64.hpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-07-18
 * \updates       2018-08-04
 * \license       GNU GPLv2 or above
 *
 *  Note that, as of version 0.9.11, the z and Z keys, when focus is on the
 *  perfroll (piano roll), will zoom the view horizontally.
 */

#include <QFrame>
#include <QScrollArea>
#include <qmath.h>

#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN           */

/*
 *  A bunch of forward declarations.  The Qt header files are moved into the
 *  cpp file.
 */

class QScrollArea;
class QScrollBar;
class qscrollmaster;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qperfeditframe64;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qsmainwnd;
    class qperfroll;
    class qperfnames;
    class qperftime;

/**
 *  This class is an improved version of qperfeditframe.
 */

class qperfeditframe64 : public QFrame
{
    Q_OBJECT

    friend class qsmainwnd;
    friend class qperfroll;

public:

    qperfeditframe64 (seq64::perform & p, QWidget * parent = nullptr);

    /*
     * One thing that bugs me about Qt... this function should be virtual, as
     * should ~QFrame().
     */

    virtual ~qperfeditframe64 ();

    int get_beat_width () const;
    void set_beat_width (int beat_width);
    int get_beats_per_measure () const;
    void set_beats_per_measure (int beats_per_measure);
    void follow_progress ();
    void update_sizes ();

private:

    void set_snap (int a_snap);
    void set_guides ();
    void grow ();

    perform & perf ()
    {
        return m_mainperf;
    }

    void reset_zoom ();

private:

    Ui::qperfeditframe64 * ui;
    seq64::perform & m_mainperf;
    QPalette * m_palette;
    int m_snap;                 /* set snap-to in pulses/ticks  */
    int m_beats_per_measure;
    int m_beat_width;
    int m_ppqn;                 /* might not need this          */
    qperfroll * m_perfroll;
    qperfnames * m_perfnames;
    qperftime * m_perftime;

private slots:

    void updateGridSnap (int snapIndex);
    void zoom_in ();
    void zoom_out ();
    void markerCollapse ();
    void markerExpand ();
    void markerExpandCopy ();
    void markerLoop (bool loop);
    void follow (bool ischecked);

};              // class qperfeditframe64

}               // namespace seq64

#endif          // SEQ64_QPERFEDITFRAME64_HPP

/*
 * qperfeditframe64.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

