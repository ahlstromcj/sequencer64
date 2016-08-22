/*
 *  This file is part of seq42/seq32/sequencer64.
 *
 *  seq32 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq32 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          lfownd.cpp
 *
 *  This module declares/defines the base class for the LFO window of the
 *  application.
 *
 * \library       sequencer64 application
 * \author        Seq42 team; modifications by Chris Ahlstrom
 * \date          2016-07-30
 * \updates       2016-08-21
 * \license       GNU GPLv2 or above
 *
 *  Created on: 22 mar 2013
 *      Author: mattias
 *
 *  The main window holds the menu and the main controls of the application,
 */

#include <string>
#include <sigc++/slot.h>

#include <gtkmm/box.h>
#include <gtkmm/scale.h>

#include "calculations.hpp"             /* seq64::wave_type_t enumeration   */
#include "lfownd.hpp"
#include "seqdata.hpp"
#include "seqedit.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *
 */

lfownd::lfownd (perform & p, sequence & seq, seqdata & sdata)
 :
    gui_window_gtk2  (p),
    m_seq            (seq),
    m_seqdata        (sdata),
    m_hbox           (manage(new Gtk::HBox(false, 2))),
    m_scale_value    (manage(new Gtk::VScale(0, 127, 0.1))),
    m_scale_range    (manage(new Gtk::VScale(0, 127, 0.1))),
    m_scale_speed    (manage(new Gtk::VScale(0, 16,  0.01))),
    m_scale_phase    (manage(new Gtk::VScale(0, 1,   0.01))),
    m_scale_wave     (manage(new Gtk::VScale(1, 5,   1))),
    m_value          (0.0),
    m_range          (0.0),
    m_speed          (0.0),
    m_phase          (0.0),
    m_wave           (WAVE_NONE)
{
    std::string title = "Sequencer64 - LFO Editor - ";
    title.append(m_seq.get_name());
    set_title(title);
    set_size_request(150, 200);

    m_scale_value->set_tooltip_text("value");
    m_scale_range->set_tooltip_text("range");
    m_scale_speed->set_tooltip_text("speed");
    m_scale_phase->set_tooltip_text("phase");
    m_scale_wave->set_tooltip_text("wave");

    m_scale_value->set_value(64);
    m_scale_range->set_value(64);
    m_scale_value->signal_value_changed().connect
    (
        sigc::mem_fun(*this, &lfownd::scale_lfo_change)
    );
    m_scale_range->signal_value_changed().connect
    (
        sigc::mem_fun( *this, &lfownd::scale_lfo_change)
    );
    m_scale_speed->signal_value_changed().connect
    (
        sigc::mem_fun( *this, &lfownd::scale_lfo_change)
    );
    m_scale_phase->signal_value_changed().connect
    (
        sigc::mem_fun( *this, &lfownd::scale_lfo_change)
    );
    m_scale_wave->signal_value_changed().connect
    (
        sigc::mem_fun( *this, &lfownd::scale_lfo_change)
    );

    add(*m_hbox);
    m_hbox->pack_start(*m_scale_value);
    m_hbox->pack_start(*m_scale_range);
    m_hbox->pack_start(*m_scale_speed);
    m_hbox->pack_start(*m_scale_phase);
    m_hbox->pack_start(*m_scale_wave);
}

lfownd::~lfownd ()
{
    // no code
}

void
lfownd::toggle_visible ()
{
    show_all();
    raise();
}

void
lfownd::scale_lfo_change ()
{
    m_value = m_scale_value->get_value();
    m_range = m_scale_range->get_value();
    m_speed = m_scale_speed->get_value();
    m_phase = m_scale_phase->get_value();
    m_wave = wave_type_t(int(m_scale_wave->get_value()));

#ifdef USE_STAZED_LFO_SUPPORT
    m_seq.change_event_data_lfo
    (
        m_value, m_range, m_speed, m_phase, m_wave,
        m_seqdata.m_status, m_seqdata.m_cc
    );
#endif

    m_seqdata.update_pixmap();
    m_seqdata.draw_pixmap_on_window();
}

bool
lfownd::on_focus_out_event (GdkEventFocus * /* p0 */)
{
#ifdef USE_STAZED_LFO_SUPPORT
    if (m_seq.get_hold_undo())
    {
        m_seq.push_undo(true);
        m_seq.set_hold_undo(false);
    }
#endif
    return true;
}

}           /* namespace seq64 */

/*
 * lfownd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

