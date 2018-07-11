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
 * \updates       2018-07-10
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
class QLineEdit;

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
    class qseqeditframe64;

/**
 *  This class is the Qt 5 version of the lfownd class.  It has one important
 *  difference, in that the wave type is chosen via radio-buttons, rather than
 *  a slider.  And the numbers can be edited directly.
 */

class qlfoframe : public QFrame
{
    Q_OBJECT

public:

    qlfoframe
    (
        perform & p,
        sequence & seq,
        qseqdata & sdata,
        qseqeditframe64 * editparent    = nullptr,
        QWidget * parent                = nullptr
    );
    virtual ~qlfoframe ();

    void toggle_visible ();

protected:

    virtual void closeEvent (QCloseEvent *);

private:

    /**
     *  Converts a slider value to a double value.  Slider values are a 100
     *  times (m_scale_factor) what they need to be.
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

    void set_value_text (double value, QLineEdit * textline);

private slots:

#if 0
    // TODO:  implement undo via selection of "None" for the wave type.
    //  bool on_focus_out_event (GdkEventFocus * p0);
#endif

    /*
     *  We use a lambda function for the slot for the QButtonGroup ::
     *  buttonClicked() signal.
     */

    void scale_lfo_change (int);
    void value_text_change ();
    void range_text_change ();
    void speed_text_change ();
    void phase_text_change ();

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
     *  The seqedit frame that owns (sort of) this LFO window.
     */

    qseqeditframe64 * m_edit_frame;

    /**
     *  We need a scale factor in order to use the integer values of the
     *  sliders with two digits of precision after the decimal.
     */

    const int m_scale_factor = 100;

    /**
     *  Value.  Ranges from 0.0 to 127.0. It is initialized to its starting
     *  value, 64.0.  Also defined are the minimum and maximum.
     */

    double m_value;
    static double m_value_min;      /**< The minimum value allowed. */
    static double m_value_max;      /**< The maximum value allowed. */

    /**
     *  Range.  Ranges from 0.0 to 127.0. It is initialized to its starting
     *  value, 64.0.  Also defined are the minimum and maximum.
     */

    double m_range;
    static double m_range_min;      /**< The minimum range allowed. */
    static double m_range_max;      /**< The maximum range allowed. */

    /**
     *  Speed.
     */

    double m_speed;
    static double m_speed_min;      /**< The minimum speed allowed. */
    static double m_speed_max;      /**< The maximum speed allowed. */

    /**
     *  Phase.
     */

    double m_phase;
    static double m_phase_min;      /**< The minimum phase allowed. */
    static double m_phase_max;      /**< The maximum phase allowed. */

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

