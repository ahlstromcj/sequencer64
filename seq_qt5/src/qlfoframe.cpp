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
 * \file          qlfoframe.cpp
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

#include <QButtonGroup>
#include <QSlider>

#include "calculations.hpp"                /* seq64::wave_type_t values     */
#include "perform.hpp"
#include "qlfoframe.hpp"
#include "qseqdata.hpp"
#include "sequence.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qlfoframe.h"
#else
#include "forms/qlfoframe.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq64
{

/*
 *  Signal buttonClicked(int) is overloaded in this class. To connect to this
 *  signal by using the function pointer syntax, Qt provides a convenient
 *  helper for obtaining the function pointer as used below, using a lambda
 *  function.
 */

qlfoframe::qlfoframe
(
    perform & p, sequence & seq, qseqdata & sdata,
    QWidget * parent
) :
    QFrame          (parent),
    ui              (new Ui::qlfoframe),
    m_wave_group    (nullptr),
    m_perform       (p),
    m_seq           (seq),
    m_seqdata       (sdata),
    m_value         (0.0),
    m_range         (0.0),
    m_speed         (0.0),
    m_phase         (0.0),
    m_wave          (WAVE_SINE)
{
    ui->setupUi(this);

    m_wave_group = new QButtonGroup(this);
    m_wave_group->addButton(ui->m_radio_wave_none, int(WAVE_NONE));
    m_wave_group->addButton(ui->m_radio_wave_sine, int(WAVE_SINE));
    m_wave_group->addButton(ui->m_radio_wave_saw, int(WAVE_SAWTOOTH));
    m_wave_group->addButton(ui->m_radio_wave_revsaw, int(WAVE_REVERSE_SAWTOOTH));
    m_wave_group->addButton(ui->m_radio_wave_triangle, int(WAVE_TRIANGLE));

    /*
     * TODO:  set default radio button to WAVE_SINE !!!!!!!!!!!
     * TODO:  add this objects pointer to qseqeditframe64 destructor !!!!
     */

    connect
    (
        m_wave_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
        [=](int id)
        {
            m_wave = wave_type_t(id);
        }
    );

    ui->m_wave_type_group->setToolTip
    (
        "Wave type: 1 = sine; 2 = ramp sawtooth; 3 = decay sawtooth; "
        "4 = triangle."
    );

    ui->m_value_slider->setToolTip
    (
        "Value: a kind of DC offset for the data value. Starts at 64."
    );
    ui->m_value_slider->setMinimum(to_slider(0));
    ui->m_value_slider->setMaximum(to_slider(127));
    ui->m_value_slider->setValue(to_slider(64));
    connect
    (
        ui->m_value_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change(int))
    );

    ui->m_range_slider->setToolTip
    (
        "Range: controls the depth of modulation. Starts at 64."
    );
    ui->m_range_slider->setMinimum(to_slider(0));
    ui->m_range_slider->setMaximum(to_slider(127));
    ui->m_range_slider->setValue(to_slider(64));
    connect
    (
        ui->m_range_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change(int))
    );

    ui->m_speed_slider->setToolTip
    (
        "Speed: the number of periods per pattern (divided by beat width, "
        "normally 4).  For long patterns, this parameter needs to be set "
        "high in some cases.  Also subject to an 'anti-aliasing' effect in "
        "some parts of the range, especially for short patterns. "
        "Try it.  For short patterns, try a value of 1."
    );
    ui->m_speed_slider->setMinimum(to_slider(0));
    ui->m_speed_slider->setMaximum(to_slider(16));
    ui->m_speed_slider->setValue(to_slider(1));
    connect
    (
        ui->m_speed_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change(int))
    );

    ui->m_phase_slider->setToolTip
    (
        "Phase: phase shift in a beat width (quarter note). "
        "A value of 1 is a phase shift of 360 degrees."
    );
    ui->m_phase_slider->setMinimum(to_slider(0));
    ui->m_phase_slider->setMaximum(to_slider(1));
    ui->m_speed_slider->setValue(to_slider(0));
    connect
    (
        ui->m_phase_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change(int))
    );
}

/**
 *
 */

qlfoframe::~qlfoframe()
{
    delete ui;
}

/**
 *  Changes the scaling provided by this window.  Changes take place right
 *  away in this callback.
 */

void
qlfoframe::scale_lfo_change (int /*v*/)
{
#ifdef SEQ64_STAZED_LFO_SUPPORT
    m_value = to_double(ui->m_value_slider->value());
    m_range = to_double(ui->m_range_slider->value());
    m_speed = to_double(ui->m_speed_slider->value());
    m_phase = to_double(ui->m_phase_slider->value());
    m_seq.change_event_data_lfo
    (
        m_value, m_range, m_speed, m_phase, m_wave,
        m_seqdata.status(), m_seqdata.cc(), true
    );
    m_seqdata.set_dirty();

    char tmp[16];
    snprintf(tmp, sizeof tmp, "%g", m_value);
    ui->m_value_text->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", m_range);
    ui->m_range_text->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", m_speed);
    ui->m_speed_text->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", m_phase);
    ui->m_phase_text->setText(tmp);
#endif
}

/**
 *

void
qlfoframe::scale_lfo_edit ()
{
}
 */

#if 0

/**
 *  Undoes the LFO changes if there is undo available.
 *
 * \return
 *      Always returns true.
 */

bool
qlfoframe::on_focus_out_event (GdkEventFocus * /* p0 */)
{
#ifdef SEQ64_STAZED_LFO_SUPPORT
    if (m_seq.get_hold_undo())
    {
        m_seq.push_undo(true);
        m_seq.set_hold_undo(false);
    }
#endif
    return true;
}

#endif  // 0

}               // namespace seq64

/*
 * qlfoframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

