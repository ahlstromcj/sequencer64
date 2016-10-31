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
 * \updates       2016-10-30
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
#include <gtkmm/label.h>

#include "calculations.hpp"             /* seq64::wave_type_t enumeration   */
#include "lfownd.hpp"
#include "seqdata.hpp"
#include "seqedit.hpp"
#include "sequence.hpp"

namespace seq64
{

/**
 *  Constructs the LFO window.
 *
 * \param p
 *      The performance object, which holds parameters necessary for
 *      manipulating events.
 *
 * \param seq
 *      The sequence/pattern that is to be affected by the LFO window.  It
 *      holds the actual MIDI events being modified.
 *
 * \param sdata
 *      The data pane/panel of the pattern editor window representing the
 *      sequence.  We need to tell it to redraw.
 */

lfownd::lfownd (perform & p, sequence & seq, seqdata & sdata)
 :
    gui_window_gtk2 (p),
    m_seq           (seq),
    m_seqdata       (sdata),
    m_hbox          (manage(new Gtk::HBox(true, 8))),          // (false, 2))),
    m_scale_value   (manage(new Gtk::VScale(0, 127, 0.1))),
    m_scale_range   (manage(new Gtk::VScale(0, 127, 0.1))),
    m_scale_speed   (manage(new Gtk::VScale(0, 16,  0.01))),
    m_scale_phase   (manage(new Gtk::VScale(0, 1,   0.01))),
    m_scale_wave    (manage(new Gtk::VScale(1, 5,   1))),
    m_wave_name     (manage(new Gtk::Label("Sine"))),
    m_value         (0.0),
    m_range         (0.0),
    m_speed         (0.0),
    m_phase         (0.0),
    m_wave          (WAVE_SINE)
{
    std::string title = "Sequencer64 - LFO Editor - ";
    title.append(m_seq.get_name());
    set_title(title);
    set_size_request(400, 300); // set_size_request(150, 200);
    m_scale_value->set_tooltip_text
    (
        "Value: a kind of DC offset for the data value. Starts at 64."
    );
    m_scale_range->set_tooltip_text
    (
        "Range: controls the depth of modulation. Starts at 64."
    );
    m_scale_speed->set_tooltip_text
    (
        "Speed: the number of periods per pattern (divided by beat width, "
        "normally 4).  For long patterns, this parameter needs to be set "
        "high in some cases.  Also subject to an 'anti-aliasing' effect in "
        "some parts of the range, especially for short patterns. "
        "Try it.  For short patterns, try a value of 1."
    );
    m_scale_phase->set_tooltip_text
    (
        "Phase: phase shift in a beat width (quarter note). "
        "A value of 1 is a phase shift of 360 degrees."
    );
    m_scale_wave->set_tooltip_text
    (
        "Wave type: 1 = sine; 2 = ramp sawtooth; 3 = decay sawtooth; "
        "4 = triangle."
    );

    m_scale_value->set_value(64);
    m_scale_range->set_value(64);
    m_scale_speed->set_value(0);
    m_scale_phase->set_value(0);
    m_scale_wave->set_value(1);

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
    Gtk::VBox * vbox1 = manage(new Gtk::VBox(false, 2));
    Gtk::VBox * vbox2 = manage(new Gtk::VBox(false, 2));
    Gtk::VBox * vbox3 = manage(new Gtk::VBox(false, 2));
    Gtk::VBox * vbox4 = manage(new Gtk::VBox(false, 2));
    Gtk::VBox * vbox5 = manage(new Gtk::VBox(false, 2));
    Gtk::Label * label1 = manage(new Gtk::Label("Value"));
    Gtk::Label * label2 = manage(new Gtk::Label("Range"));
    Gtk::Label * label3 = manage(new Gtk::Label("Speed"));
    Gtk::Label * label4 = manage(new Gtk::Label("Phase"));
    Gtk::Label * label5 = manage(new Gtk::Label("Type"));
    m_wave_name->set_width_chars(12);
    vbox1->pack_start(*label1,  false, false, 8);
    vbox1->pack_start(*m_scale_value,  true, true, 0);
    vbox2->pack_start(*label2,  false, false, 8);
    vbox2->pack_start(*m_scale_range,  true, true, 0);
    vbox3->pack_start(*label3,  false, false, 8);
    vbox3->pack_start(*m_scale_speed,  true, true, 0);
    vbox4->pack_start(*label4,  false, false, 8);
    vbox4->pack_start(*m_scale_phase,  true, true, 0);
    vbox5->pack_start(*label5,  false, false, 8);
    vbox5->pack_start(*m_scale_wave,  true, true, 0);
    vbox5->pack_start(*m_wave_name, false, false, 0);
    vbox5->pack_start(*manage(new Gtk::Label(" ")), false, false, 0);
    m_hbox->pack_start(*vbox1);
    m_hbox->pack_start(*vbox2);
    m_hbox->pack_start(*vbox3);
    m_hbox->pack_start(*vbox4);
    m_hbox->pack_start(*vbox5, true, true, 4);
    add(*m_hbox);
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
#ifdef SEQ64_STAZED_LFO_SUPPORT
    int wtype = int(m_scale_wave->get_value());
    m_value = m_scale_value->get_value();
    m_range = m_scale_range->get_value();
    m_speed = m_scale_speed->get_value();
    m_phase = m_scale_phase->get_value();
    m_wave = wave_type_t(wtype);
    m_wave_name->set_text(wave_type_name(wave_type_t(wtype)));
    m_seq.change_event_data_lfo
    (
        m_value, m_range, m_speed, m_phase, m_wave,
        m_seqdata.m_status, m_seqdata.m_cc
    );
    m_seqdata.update_pixmap();
    m_seqdata.draw_pixmap_on_window();
#endif
}

bool
lfownd::on_focus_out_event (GdkEventFocus * /* p0 */)
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

}           /* namespace seq64 */

/*
 * lfownd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

