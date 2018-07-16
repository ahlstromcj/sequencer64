#ifndef SEQ64_QPERFEDITFRAME_HPP
#define SEQ64_QPERFEDITFRAME_HPP

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
 * \file          qperfeditframe.hpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-07-15
 * \license       GNU GPLv2 or above
 *
 *  Note that, as of version 0.9.11, the z and Z keys, when focus is on the
 *  perfroll (piano roll), will zoom the view horizontally.
 */

#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <qmath.h>

#include "qperfroll.hpp"
#include "qperfnames.hpp"
#include "qperftime.hpp"

namespace Ui
{
    class qperfeditframe;
}

namespace seq64
{
    class perform;

/**
 *
 */

class qperfeditframe : public QFrame
{
    Q_OBJECT

    friend class qsmainwnd;

public:

    explicit qperfeditframe (seq64::perform & p, QWidget * parent);

    /*
     * One thing that bugs me about Qt... this function should be virtual, as
     * should ~QFrame().
     */

    virtual ~qperfeditframe();

    int get_beat_width() const;
    void set_beat_width(int a_beat_width);
    int get_beats_per_measure() const;
    void set_beats_per_measure(int a_beats_per_measure);
    void follow_progress ();

private:

    void update_sizes ();
    void set_snap(int a_snap);
    void set_guides();
    void grow();

    perform & perf ()
    {
        return m_mainperf;
    }

private:

    Ui::qperfeditframe * ui;
    seq64::perform & m_mainperf;
    QGridLayout * m_layout_grid;
    QScrollArea * m_scroll_area;
    QWidget * mContainer;
    QPalette * m_palette;
    int m_snap;                 /* set snap to in pulses */
    int mbeats_per_measure;
    int mbeat_width;
    seq64::qperfroll * m_perfroll;
    seq64::qperfnames * m_perfnames;
    seq64::qperftime * m_perftime;

private slots:

    void updateGridSnap (int snapIndex);
    void zoom_in ();
    void zoom_out ();
    void markerCollapse ();
    void markerExpand ();
    void markerExpandCopy ();
    void markerLoop (bool loop);

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    void follow (bool ischecked);
#endif

};              // class qperfeditframe

}               // namespace seq64

#endif          // SEQ64_QPERFEDITFRAME_HPP

/*
 * qperfeditframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

