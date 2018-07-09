#ifndef SEQ64_QLFOFRAME_HPP
#define SEQ64_QLFOFRAME_HPP

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
 * \file          qlfoframe.hpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-07-08
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QFrame>

#include "calculations.hpp"             /* seq64::wave_type_t               */

/*
 *  Forward declarations for Qt.
 */

class QButtonGroup;

/*
 * This is necessary to keep the compiler from thinking Ui::qlfoframe
 * would be found in the seq64 namespace.
 */

namespace Ui
{
    class qlfoframe;
}

/*
 * Do not document a namespace.  It breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;
    class qseqdata;

/**
 *
 */

class qlfoframe : public QFrame
{
    Q_OBJECT

public:

    qlfoframe
    (
        perform & p, sequence & seq, qseqdata & sdata,
        QWidget * parent = nullptr
    );
    virtual ~qlfoframe ();

    void toggle_visible ();

private:

    /**
     *  Converts a slider value to a double value.  Slider values are a 100 times
     *  (m_scale_factor) what they need to be.
     */

    double to_double (int v)
    {
        return double(v) / m_scale_factor;
    }

    /**
     *  Converts a double value to a slider value.
     */

    int to_slider (double v)
    {
        return int(v * double(m_scale_factor) + 0.5);
    }

private slots:

#if 0
    //  bool on_focus_out_event (GdkEventFocus * p0);
#endif

    /*
     *  We use a lambda function for this slot: void onButtonGroup (int)
     */

    void scale_lfo_change (int);

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qlfoframe * ui;

    /**
     *  Provides a way to treat the wave radio-buttons as a group.  Had issues
     *  trying to set this up in Qt Creator. To get the checked value,
     *  use its checkedButton() function.
     */

    QButtonGroup * m_wave_group;

    /**
     *
     */

    perform & m_perform;

    /**
     *  The sequence associated with this window.
     */

    sequence & m_seq;

    /**
     *  The qseqdata associated with this window.
     */

    qseqdata & m_seqdata;

    /**
     *  We need a scale factor in order to use the integer values of the
     *  sliders with two digits of precision after the decimal.
     */

    const int m_scale_factor = 100;

    /**
     *  Value.  Ranges from 0 to 127 with a precision of 0.1.
     */

    double m_value;

    /**
     *  Range.  Ranges from 0 to 127 with a precision of 0.1.
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

};          // class qlfoframe

}           // namespace seq64

#endif // SEQ64_QLFOFRAME_HPP

/*
 * qlfoframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

