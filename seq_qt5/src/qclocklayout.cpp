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
 * \file          qclocklayout.hpp
 *
 *  This class supports a MIDI Clocks label and a set of radio-buttons for
 *  selecting the clock style (off, on POS, on MOD), associating it with a
 *  particular output buss.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-05-19
 * \updates       2018-05-20
 * \license       GNU GPLv2 or above
 *
 */

#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>

#include "qclocklayout.hpp"
#include "perform.hpp"

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *  Creates a single line in the MIDI Clocks "Clock" group-box.  We will use
 *  the words "clock" or "port" for the MIDI output port represented by this
 *  widget.  Here are the jobs we have to do:
 *
 *      -#    Get the label for the port and set it.
 *      -#  Add the tooltips for the clock radio-buttons.
 *      -#  Add the clock radio-buttons to m_horizlayout_clocklive.
 *        -#    Connect to the radio-button slots:
 *            -    clock_callback_disable().
 *            -    clock_callback_off().
 *            -    clock_callback_on().
 *            -    clock_callback_mod().
 */

qclocklayout::qclocklayout
(
    QWidget * parent,                     // QGroupBox, QObject * parent
    perform & p,
    int bus

) :
    QWidget                     (parent),
    m_performance               (p),
    m_bus                       (bus),
    m_parent_widget             (parent),      // m_groupbox_clocks
    m_horizlayout_clockline     (nullptr),
    m_spacer_clock              (nullptr),
    m_label_outputbusname       (nullptr),
    m_rbutton_portdisabled      (nullptr),
    m_rbutton_clockoff          (nullptr),
    m_rbutton_clockonpos        (nullptr),
    m_rbutton_clockonmod        (nullptr)
{
    setup_ui();

    bool ok = connect
    (
        m_rbutton_group, SIGNAL(buttonClicked(int)),
        this, SLOT(clock_callback_clicked(int))
    );
    if (! ok)
    {
        errprint("qclocklayout: clock-group slot failed to connect");
    }
}

/*
    The tool-tips

"This setting disables the usage of this output port, completely.  "
"It is needed in some cases for devices that are detected, but "
"cannot be used (e.g. devices locked by another application)."

"MIDI Clock will be disabled. Used for conventional playback."

"MIDI Clock will be sent. MIDI Song Position and MIDI Continue "
"will be sent if starting after tick 0 in song mode; otherwise "
"MIDI Start is sent."

"MIDI Clock will be sent.  MIDI Start will be sent and clocking "
"will begin once the song position has reached the modulo of "
"the specified Size. Use for gear that doesn't respond to Song "
"Position."

 */

void
qclocklayout::setup_ui ()
{
    m_horizlayout_clockline = new QHBoxLayout();
    m_horizlayout_clockline->setContentsMargins(0, 0, 0, 0);
    m_spacer_clock = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum
    );

    QString busname = perf().master_bus().get_midi_out_bus_name(m_bus).c_str();
    m_label_outputbusname = new QLabel();
    m_label_outputbusname->setText(busname);
    m_rbutton_portdisabled = new QRadioButton("Port disabled");
    m_rbutton_clockoff = new QRadioButton("Off");
    m_rbutton_clockonpos = new QRadioButton("On (Pos)");
    m_rbutton_clockonmod = new QRadioButton("On (Mod)");

    m_rbutton_group = new QButtonGroup();
    m_rbutton_group->addButton(m_rbutton_portdisabled, int(e_clock_disabled));
    m_rbutton_group->addButton(m_rbutton_clockoff, int(e_clock_off));
    m_rbutton_group->addButton(m_rbutton_clockonpos, int(e_clock_pos));
    m_rbutton_group->addButton(m_rbutton_clockonmod, int(e_clock_mod));

    m_horizlayout_clockline->addWidget(m_label_outputbusname);
    m_horizlayout_clockline->addItem(m_spacer_clock);
    m_horizlayout_clockline->addWidget(m_rbutton_portdisabled);
    m_horizlayout_clockline->addWidget(m_rbutton_clockoff);
    m_horizlayout_clockline->addWidget(m_rbutton_clockonpos);
    m_horizlayout_clockline->addWidget(m_rbutton_clockonmod);

    switch (perf().master_bus().get_clock(m_bus))
    {
    case e_clock_disabled:
        m_rbutton_portdisabled->setChecked(true);
        break;

    case e_clock_off:
        m_rbutton_clockoff->setChecked(true);
        break;

    case e_clock_pos:
        m_rbutton_clockonpos->setChecked(true);
        break;

    case e_clock_mod:
        m_rbutton_clockonmod->setChecked(true);
        break;
    }
}

/**
 *  Sets the clocking value based on in incoming parameter.  We have to use
 *  this particular slot in order to handle all of the radio-buttons.
 *
 * \param id
 *      Provides the ID code of the button that was clicked.  We set these ID
 *      values explicitly, via addButton(ptrbutton, int(e_clock_disabled)).
 *      For some reason, probably because -1 is a special flag for this
 *      callback, -1 [e_clock_disabled] gets converted to -2.  So we have to
 *      adjust.
 */

void
qclocklayout::clock_callback_clicked (int id)
{
    if (id == (-2))
        id = -1;

    /*
     * For debug only: infoprintf("%d clicked\n", id);
     */

    perf().set_clock_bus(m_bus, clock_e(id));
}

}           // namespace seq64

/*
 * qclocklayout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

