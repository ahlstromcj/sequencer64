#ifndef SEQ64_QSEQEDITEX_HPP
#define SEQ64_QSEQEDITEX_HPP

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
 * \file          qseditex.hpp
 *
 *  This module declares/defines the base class for the external
 *  sequence-editing window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2018-07-10
 * \license       GNU GPLv2 or above
 *
 *  The sequence editing window is known as the "Pattern Editor".  Kepler34
 *  provides and editor embedded within a tab, but we supplement that with a
 *  more sophisticated external editor, which works a lot more like the Gtkmm
 *  seqedit class.
 */

#include <QWidget>

class QCloseEvent;

namespace Ui
{
    class qseqeditex;
}

namespace seq64
{
    class perform;
    class sequence;
    class qseqeditframe64;
    class qsmainwnd;

/**
 *  Provides a container for a qseqeditframe64 object.  Thus, the Qt 5 version
 *  of Sequencer64 has an external seqedit window like its Gtkmm-2.4
 *  counterpart.
 */

class qseqeditex : public QWidget
{
    Q_OBJECT

public:

    explicit qseqeditex
    (
        perform & p,
        int seqid,
        qsmainwnd * parent = nullptr
    );
    virtual ~qseqeditex ();

protected:

    virtual void closeEvent (QCloseEvent *);

    const perform & perf () const
    {
        return m_perform;
    }

    perform & perf ()
    {
        return m_perform;
    }

private:

    Ui::qseqeditex * ui;
    perform & m_perform;
    int m_seq_id;
    qsmainwnd * m_edit_parent;
    qseqeditframe64 * m_edit_frame;

};              // class qseqeditex

}               // namespace seq64

#endif          // SEQ64_QSEQEDITEX_HPP

/*
 * qseditex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

