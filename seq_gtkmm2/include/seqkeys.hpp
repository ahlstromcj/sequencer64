#ifndef SEQ64_SEQKEYS_HPP
#define SEQ64_SEQKEYS_HPP

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
 * \file          seqkeys.hpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>

namespace Gtk
{
    class adjustment;
}

namespace seq64
{

class sequence;

/**
 *  This class implements the left side piano of the pattern/sequence
 *  editor.
 */

class seqkeys : public Gtk::DrawingArea
{

private:

    Glib::RefPtr<Gdk::GC> m_gc;
    Glib::RefPtr<Gdk::Window> m_window;
    Gdk::Color m_black;
    Gdk::Color m_white;
    Gdk::Color m_grey;
    Glib::RefPtr<Gdk::Pixmap> m_pixmap;
    int m_window_x;
    int m_window_y;
    Gtk::Adjustment * m_vadjust;
    sequence * m_seq;
    int m_scroll_offset_key;
    int m_scroll_offset_y;
    bool m_hint_state;
    int m_hint_key;

    /**
     *  What is this?
     */

    bool m_keying;
    int m_keying_note;
    int m_scale;
    int m_key;

public:

    seqkeys (sequence * a_seq, Gtk::Adjustment * a_vadjust);

    void set_scale (int a_scale);
    void set_key (int a_key);
    void set_hint_key (int a_key);       /* sets key to grey */
    void set_hint_state (bool a_state);  /* true == on, false == off */

private:

    void draw_area ();
    void update_pixmap ();
    void convert_y (int a_y, int * a_note);
    void draw_key (int a_key, bool a_state);
    void change_vert ();
    void force_draw ();
    void update_sizes ();
    void reset ();

private:        // callbacks

    void on_realize ();
    bool on_expose_event (GdkEventExpose * a_ev);
    bool on_button_press_event (GdkEventButton * a_ev);
    bool on_button_release_event (GdkEventButton * a_ev);
    bool on_motion_notify_event (GdkEventMotion * a_p0);
    bool on_enter_notify_event (GdkEventCrossing * p0);
    bool on_leave_notify_event (GdkEventCrossing * p0);
    bool on_scroll_event (GdkEventScroll * a_ev);
    void on_size_allocate (Gtk::Allocation &);

};

}           // namespace seq64

#endif      // SEQ64_SEQKEYS_HPP

/*
 * seqkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
