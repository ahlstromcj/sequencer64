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
 * \file          mainwid.cpp
 *
 *  This module declares/defines the base class for drawing
 *  patterns/sequences in the Patterns Panel grid.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-10-17
 * \license       GNU GPLv2 or above
 *
 *  Note that this representation is, in a sense, inside the mainwnd
 *  implementation.  While mainwid represents the pattern slots, mainwnd
 *  represents the menu and surrounding elements.
 *
 * \todo
 *      BUG!  During playing with the "Show keys" options, dragging and
 *      dropping sequences, we got a segfault in the application! Not yet sure
 *      what caused it.
 */

#include <gtkmm/combo.h>                /* Gtk::Entry                   */
#include <gtkmm/menubar.h>

#include "click.hpp"                    /* SEQ64_CLICK_IS_LEFT(), etc.  */
#include "font.hpp"
#include "mainwid.hpp"
#include "perform.hpp"

using namespace Gtk::Menu_Helpers;

/**
 *  Adjustments to the main window.  Trying to get sequences that don't
 *  have events to show up as black-on-yellow.  This works now, and is
 *  enabled by default.  To disable this feature, configure the build with
 *  the --disable-highlight option.
 *
 *      #define HIGHLIGHT_EMPTY_SEQS    // undefine for normal empty seqs
 *
 *  The USE_GREY_GRID and USE_NORMAL_GRID can be undefined in combination
 *  to obtain different kinds of looks for the main (patterns) window at
 *  build time.  Try it and see for yourself!
 */

#define USE_GREY_GRID                   // undefine for black boxes
#define USE_NORMAL_GRID                 // undefine for black box, black outline

/*
 *  Static array of characters for use in toggling patterns.
 *  These look like the "Sequence toggle keys" in the Options / Keyboard
 *  dialog, except that they are upper-case here, and lower-case in that
 *  configuration dialog.
 *
 * \obsolete
 *      Its only use was in this module, and is commented out below,
 *      replaced by another lookup method.
 *
\verbatim
    const char mainwid::m_seq_to_char[c_seqs_in_set] =
    {
        '1', 'Q', 'A', 'Z',
        '2', 'W', 'S', 'X',
        '3', 'E', 'D', 'C',
        '4', 'R', 'F', 'V',
        '5', 'T', 'G', 'B',
        '6', 'Y', 'H', 'N',
        '7', 'U', 'J', 'M',
        '8', 'I', 'K', ','
    };
\endverbatim
 *
 * WARNING:  If you make this comment a Doxygen comment, one which precedes
 *           the following namespace specification, it breaks the creation of
 *           the reference-manual PDF file by Doxygen!!!
 */

namespace seq64
{

/**
 *  This constructor sets a lot of the members, but not all.  And it asks
 *  for a size of c_mainwid_x by c_mainwid_y.  It adds GDK masks for
 *  button presses, releases, and motion, and key presses and focus
 *  changes.
 *
 * \param p
 *      Provides the reference to the all-important perform object.
 */

mainwid::mainwid (perform & p)
 :
    gui_drawingarea_gtk2    (p, c_mainwid_x, c_mainwid_y),
    seqmenu                 (p),
    m_moving_seq            (),
    m_button_down           (false),
    m_moving                (false),
    m_old_seq               (0),
    m_screenset             (0),
    m_last_tick_x           (),     // an array of size c_max_sequence
    m_last_playing          (),     // an array of size c_max_sequence
    m_mainwnd_rows          (c_mainwnd_rows),
    m_mainwnd_cols          (c_mainwnd_cols),
    m_seqarea_x             (c_seqarea_x),
    m_seqarea_y             (c_seqarea_y),
    m_seqarea_seq_x         (c_seqarea_seq_x),
    m_seqarea_seq_y         (c_seqarea_seq_y),
    m_mainwid_x             (c_mainwid_x),
    m_mainwid_y             (c_mainwid_y),
    m_mainwid_border        (c_mainwid_border),
    m_mainwid_spacing       (c_mainwid_spacing),
    m_text_size_x           (c_text_x),
    m_text_size_y           (c_text_y),
    m_max_sets              (c_max_sets)
{
    // It's all done in the base classes and the initializer list.
}

/**
 *  A rote destructor.
 */

mainwid::~mainwid ()
{
    // Empty body
}

/**
 *  This function fills the pixmap with sequences.
 */

void
mainwid::draw_sequences_on_pixmap ()
{
    int slots = m_mainwnd_rows * m_mainwnd_cols;
    for (int s = 0; s < slots; s++)
    {
        int offset = (m_screenset * slots) + s;
        draw_sequence_on_pixmap(offset);
        m_last_tick_x[offset] = 0;
    }
}

/**
 *  This function updates the background window, clearing it.
 */

void
mainwid::fill_background_window ()
{
    m_pixmap->draw_rectangle
    (
        get_style()->get_bg_gc(Gtk::STATE_NORMAL),      // this->
        true, 0, 0, m_window_x, m_window_y
    );
}

/**
 *  Provides a stock callback, because some kind of callback is need.
 *
 *
 * \todo
 *      We should use this callback to display the current time in the
 *      playback.
 *
 * \return
 *      Always returns true.
 */

int
mainwid::timeout ()
{
    return true;
}

/**
 *  This function draws a specific pattern/sequence on the pixmap located
 *  in the main window of the application, the Patterns Panel.  The
 *  sequence is drawn only if it is in the current screen set (indicated
 *  by m_screenset).
 *
 * \note
 *      If only the main window is up, then the sequences just appear to
 *      play -- the progress bars move in each pattern.  Gaps in the song
 *      don't change the appearance of the patterns.  But, if the Song
 *      (performance) Editor window is up, and the song is started using
 *      the controls in the Song (performance) Editor windows, then the
 *      active patterns are black (!) while playing, and white when gaps
 *      in the song are encountered.  Also, the muting status in the main
 *      window seems to be ignored (based on coloring, anyway).  However,
 *      the muting in the Song (performance) windows does seem to be in
 *      force.
 *
 * \param seqnum
 *      Provides the number of the sequence slot that needs to be drawn.
 */

void
mainwid::draw_sequence_on_pixmap (int seqnum)
{
    if (valid_sequence(seqnum))
    {
        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);    // side-effects
        m_gc->set_foreground(black());
        m_pixmap->draw_rectangle                // outer border of box
        (
            m_gc, true, base_x, base_y, m_seqarea_x, m_seqarea_y
        );
        if (perf().is_active(seqnum))
        {
            sequence * seq = perf().get_sequence(seqnum);
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            if (seq->event_count() > 0)
            {
#endif
                if (seq->get_playing())
                {
                    m_last_playing[seqnum] = true;   // active and playing
                    bg_color(black());
                    fg_color(white());
                }
                else
                {
                    m_last_playing[seqnum] = false;  // active and not playing
                    bg_color(white());
                    fg_color(black());
                }
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            }
            else
            {
                m_last_playing[seqnum] = false;      // active and not playing
                if (seq->get_playing())
                {
                    m_last_playing[seqnum] = false;  // active and playing
                    bg_color(black());
                    fg_color(yellow());
                }
                else
                {
                    m_last_playing[seqnum] = false;  // active and not playing
                    bg_color(yellow());
                    fg_color(black());
                }
            }
#endif          // SEQ64_HIGHLIGHT_EMPTY_SEQS

            m_gc->set_foreground(bg_color());
            m_pixmap->draw_rectangle
            (
                m_gc, true, base_x+1, base_y+1, m_seqarea_x-2, m_seqarea_y-2
            );
            m_gc->set_foreground(fg_color());

            char temp[64];                          // used a lot below
            snprintf(temp, sizeof temp, "%.13s", seq->get_name());
            font::Color col = font::BLACK;
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            if (seq->event_count() > 0)
            {
#endif
                if (fg_color() == black())
                    col = font::BLACK;

                if (fg_color() == white())
                    col = font::WHITE;
#if SEQ64_HIGHLIGHT_EMPTY_SEQS
            }
            else
            {
                if (fg_color() == black())
                    col = font::BLACK_ON_YELLOW;

                if (fg_color() == yellow())
                    col = font::YELLOW_ON_BLACK;
            }
#endif

            p_font_renderer->render_string_on_drawable      // name of pattern
            (
                m_gc, base_x+m_text_size_x, base_y+4, m_pixmap, temp, col
            );

            /*
             * midi channel + key + timesig
             * char key =  m_seq_to_char[local_seq];        // obsolete
             */

            if (perf().show_ui_sequence_key())
            {
                snprintf
                (
                    temp, sizeof temp, "%c",
                    (char) perf().lookup_keyevent_key(seqnum)
                );
                p_font_renderer->render_string_on_drawable  // shortcut key
                (
                    m_gc, base_x+m_seqarea_x-7, base_y + m_text_size_y*4 - 2,
                    m_pixmap, temp, col
                );
            }
            snprintf
            (
                temp, sizeof temp, "%d-%d %ld/%ld",
                seq->get_midi_bus(), seq->get_midi_channel() + 1,
                seq->get_bpm(), seq->get_bw()
            );
            p_font_renderer->render_string_on_drawable      // bus, ch, etc.
            (
                m_gc,
                base_x + m_text_size_x,
                base_y + m_text_size_y * 4 - 2,
                m_pixmap, temp, col
            );

            int rectangle_x = base_x + m_text_size_x - 1;
            int rectangle_y = base_y + m_text_size_y + m_text_size_x - 1;
            if (seq->get_queued())
            {
                m_gc->set_foreground(grey());
                m_pixmap->draw_rectangle
                (
                    m_gc, true, rectangle_x - 2, rectangle_y - 1,
                    m_seqarea_seq_x + 3, m_seqarea_seq_y + 3
                );
                fg_color(black());
            }

            /*
             * Draws the inner rectangle for all sequences.
             */

            m_gc->set_foreground(fg_color());
            m_pixmap->draw_rectangle                        // ditto, unqueued
            (
                m_gc, false, rectangle_x - 2, rectangle_y - 1,
                m_seqarea_seq_x + 3, m_seqarea_seq_y + 3
            );

            int lowest_note = seq->get_lowest_note_event();
            int highest_note = seq->get_highest_note_event();
            int height = highest_note - lowest_note + 2;
            int length = seq->get_length();
            long tick_s;
            long tick_f;
            int note;
            bool selected;
            int velocity;
            draw_type dt;
            seq->reset_draw_marker();
            while                           // draws the note marks in inner box
            (
                (
                    dt = seq->get_next_note_event(
                        &tick_s, &tick_f, &note, &selected, &velocity)
                ) != DRAW_FIN
            )
            {
                int note_y = m_seqarea_seq_y -
                     (m_seqarea_seq_y * (note + 1 - lowest_note)) / height;

                int tick_s_x = (tick_s * m_seqarea_seq_x)  / length;
                int tick_f_x = (tick_f * m_seqarea_seq_x)  / length;
                if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                    tick_f_x = tick_s_x + 1;

                if (tick_f_x <= tick_s_x)
                    tick_f_x = tick_s_x + 1;

                m_gc->set_foreground(fg_color());
                m_pixmap->draw_line
                (
                    m_gc, rectangle_x + tick_s_x,
                    rectangle_y + note_y, rectangle_x + tick_f_x,
                    rectangle_y + note_y
                );
            }
        }
        else                            /* sequence not active */
        {
            /*
             *  Draws the grid areas that do not contain a sequence.
             *  The first section colors the whole grid area grey,
             *  surrounded by a thin black outline.  The second section
             *  draws a slightly narrower, but taller grey box, that
             *  yields the outlining "brackets" on each side of the grid
             *  area.  Without either of these sections, an empty grid is
             *  all black.
             *
             *  It might be cool to offer a drawing option for
             *  "black-grid", "boxed-grid", or "normal-grid" for the
             *  empty sequence box.
             */

#ifdef USE_GREY_GRID                        /* otherwise, leave it black    */
            m_gc->set_foreground(grey());
            m_pixmap->draw_rectangle
            (
                get_style()->get_bg_gc(Gtk::STATE_NORMAL),        // this->
                true, base_x + 4, base_y, m_seqarea_x - 8, m_seqarea_y
            );
#endif

#ifdef USE_NORMAL_GRID                      /* change box to "brackets"     */
            m_pixmap->draw_rectangle
            (
                get_style()->get_bg_gc(Gtk::STATE_NORMAL),       // this->
                true, base_x + 1, base_y + 1, m_seqarea_x - 2, m_seqarea_y - 2
            );
#endif

        }
    }
}

/**
 *  Common-code helper function.
 *
 * \param seqnum
 *      Provides the number of the sequence to validate.
 *
 * \return
 *      Returns true if the sequence number is valid for the current
 *      m_screenset value.
 */

bool
mainwid::valid_sequence (int seqnum)
{
    int slots = m_mainwnd_rows * m_mainwnd_cols;
    return seqnum >= (m_screenset * slots) && seqnum < ((m_screenset+1) * slots);
}

/**
 *  This function draws something in the Patterns Panel.  The sequence is
 *  drawn only if it is in the current screen set (indicated by
 *  m_screenset.  However, if we comment out this code, we can't see any
 *  difference in the Patterns Panel, even when playback is ongoing!
 *
 * \param seqnum
 *      Provides the number of the sequence to draw.
 */

void
mainwid::draw_sequence_pixmap_on_window (int seqnum)         // effective?
{
    if (valid_sequence(seqnum))
    {
        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);    // side-effects
        m_window->draw_drawable
        (
            m_gc, m_pixmap, base_x, base_y, base_x, base_y,
            m_seqarea_x, m_seqarea_y
        );
    }
}

/**
 *  Provides a way to calculate the base x and y size values for the
 *  pattern map.  The values are returned as side-effects.
 *
 * \param seqnum
 *      Provides the number of the sequence to calculate.
 *
 * \param basex
 *      A return parameter for the x coordinate of the base size.
 *
 * \param basey
 *      A return parameter for the y coordinate of the base size.
 */

void
mainwid::calculate_base_sizes (int seqnum, int & basex, int & basey)
{
    int i = (seqnum / m_mainwnd_rows) % m_mainwnd_cols;
    int j =  seqnum % m_mainwnd_rows;
    basex = m_mainwid_border + (m_seqarea_x + m_mainwid_spacing) * i;
    basey = m_mainwid_border + (m_seqarea_y + m_mainwid_spacing) * j;
}

/**
 *  Draw the the given pattern/sequence again.
 *
 * \param seqnum
 *      Provides the number of the sequence to draw.
 */

void
mainwid::redraw (int seqnum)
{
    draw_sequence_on_pixmap(seqnum);
    draw_sequence_pixmap_on_window(seqnum);         // effective?
}

/**
 *  Draw the cursors (long vertical bars) on each sequence, so that they
 *  follow the playing progress of each sequence in the mainwid (Patterns
 *  Panel.)
 *
 * \param ticks
 *      Starting point for drawing the markers.
 */

void
mainwid::update_markers (int ticks)
{
    int slots = m_mainwnd_rows * m_mainwnd_cols;
    for (int s = 0; s < slots; s++)
        draw_marker_on_sequence(s + (m_screenset * slots), ticks);
}

/**
 *  Does the actual drawing of one pattern/sequence position marker, a
 *  vertical progress bar.
 *
 *  If the sequence has no events, this function doesn't bother even
 *  drawing a position marker.
 *
 *  Note that, when Sequencer64 first comes up, and perform::is_dirty_main()
 *  is called, no sequences exist yet.
 *
 * \param seqnum
 *      Provides the number of the sequence to draw.
 *
 * \param tick
 *      Provides the location to draw the marker.
 */

void
mainwid::draw_marker_on_sequence (int seqnum, int tick)
{
    if (perf().is_dirty_main(seqnum))
        update_sequence_on_window(seqnum);

    if (perf().is_active(seqnum))
    {
        sequence * seq = perf().get_sequence(seqnum);
        if (seq->event_count() ==  0)
            return;                         /* new 2015-08-23 don't update */

        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);    // side-effects

        int rectangle_x = base_x + m_text_size_x - 1;
        int rectangle_y = base_y + m_text_size_y + m_text_size_x - 1;
        int length = seq->get_length();
        tick += (length - seq->get_trigger_offset());
        tick %= length;

        long tick_x = tick * m_seqarea_seq_x / length;
        m_window->draw_drawable
        (
            m_gc, m_pixmap,
            rectangle_x + m_last_tick_x[seqnum], rectangle_y + 1,
            rectangle_x + m_last_tick_x[seqnum], rectangle_y + 1,
            1, m_seqarea_seq_y
        );
        m_last_tick_x[seqnum] = tick_x;
        if (seq->get_playing())
            m_gc->set_foreground(white());
        else
            m_gc->set_foreground(black());

        if (seq->get_queued())
            m_gc->set_foreground(black());

        m_window->draw_line
        (
            m_gc, rectangle_x + tick_x, rectangle_y + 1,
            rectangle_x + tick_x, rectangle_y + m_seqarea_seq_y
        );
    }
}

/**
 *  Updates the image of multiple sequencers.
 */

void
mainwid::update_sequences_on_window ()
{
    draw_sequences_on_pixmap();
    draw_pixmap_on_window();
}

/**
 *  Updates the image of one sequencer.
 *
 * \param seqnum
 *      Provides the number of the sequence to update.
 */

void
mainwid::update_sequence_on_window (int seqnum)
{
    draw_sequence_on_pixmap(seqnum);
    draw_sequence_pixmap_on_window(seqnum);          // effective?
}

/**
 *  This function queues the blit of pixmap to window.
 */

void
mainwid::draw_pixmap_on_window ()
{
    queue_draw();
}

/**
 *  Translates XY coordiinates in the Patterns Panel to a sequence number.
 *
 * \param a_x
 *      Provides the x coordinate.
 *
 * \param a_y
 *      Provides the y coordinate.
 *
 * \return
 *      Returns -1 if the sequence number cannot be calculated.
 */

int
mainwid::seq_from_xy (int a_x, int a_y)
{
    int x = a_x - m_mainwid_border;         // adjust for border
    int y = a_y - m_mainwid_border;
    if                                      // is it in the box?
    (
        x < 0 || x >= ((m_seqarea_x + m_mainwid_spacing) * m_mainwnd_cols) ||
        y < 0 || y >= ((m_seqarea_y + m_mainwid_spacing) * m_mainwnd_rows)
    )
    {
        return -1;                          // no
    }

    int box_test_x = x % (m_seqarea_x + m_mainwid_spacing); // box coordinate
    int box_test_y = y % (m_seqarea_y + m_mainwid_spacing); // box coordinate
    if (box_test_x > m_seqarea_x || box_test_y > m_seqarea_y)
        return -1;                          // right inactive side of area

    x /= (m_seqarea_x + m_mainwid_spacing);
    y /= (m_seqarea_y + m_mainwid_spacing);
    int sequence =
    (
        (x * m_mainwnd_rows + y) +
            (m_screenset * m_mainwnd_rows * m_mainwnd_cols)
    );
    return sequence;
}

/**
 *  This function redraws everything and queues up a redraw operation.
 */

void
mainwid::reset ()
{
    draw_sequences_on_pixmap();
    draw_pixmap_on_window();
}

/**
 *  Set the current screen-set.
 *
 * \param a_ss
 *      Provides the screen-set number to set.
 */

void
mainwid::set_screenset (int a_ss)
{
    m_screenset = a_ss;
    if (m_screenset < 0)
        m_screenset = m_max_sets - 1;

    if (m_screenset >= m_max_sets)
        m_screenset = 0;

    perf().set_offset(m_screenset);
    reset();
}

/*
 * Event-handler section:
 */

/**
 *  For this GTK callback, on realization of window, initialize the shiz.
 *  It allocates any additional resources that weren't initialized in the
 *  constructor.
 */

void
mainwid::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    p_font_renderer->init(m_window);
    m_pixmap = Gdk::Pixmap::create(m_window, m_mainwid_x, m_mainwid_y, -1);
    fill_background_window();
    draw_sequences_on_pixmap();
}

/**
 *  Implements the GTK expose event callback.
 *
 * \param a_e
 *      The expose event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_expose_event (GdkEventExpose * a_e)
{
    m_window->draw_drawable
    (
        m_gc, m_pixmap,
        a_e->area.x, a_e->area.y,
        a_e->area.x, a_e->area.y,
        a_e->area.width, a_e->area.height
    );
    return true;
}

/**
 *  Handles a press of a mouse button.  It grabs the focus, calculates
 *  the pattern/sequence over which the button press occurred, and sets
 *  the m_button_down flag if it is over a pattern.
 *
 * \param p0
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_button_press_event (GdkEventButton * p0)
{
    grab_focus();
    current_sequence(seq_from_xy(int(p0->x), int(p0->y)));
    if (current_sequence() >= 0 && SEQ64_CLICK_IS_LEFT(p0->button))
        m_button_down = true;

    return true;
}

/**
 *  Handles a release of a mouse button.  This event is a lot more complex
 *  than a press.  The left button toggles playback status. The right
 *  button brings up a popup menu.  If the slot is empty, then a "New" popup
 *  is presented, otherwise an "Edit" and selection popup is presented.
 *
 * \param p0
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_button_release_event (GdkEventButton * p0)
{
    current_sequence(seq_from_xy(int(p0->x), int(p0->y)));
    m_button_down = false;
    if (current_sequence() < 0)
        return true;

    if (SEQ64_CLICK_IS_LEFT(p0->button))
    {
        if (m_moving)
        {
            m_moving = false;
            if              // in a pattern, it's active, not in edit mode...
            (
                ! perf().is_active(current_sequence()) &&
                ! perf().is_sequence_in_edit(current_sequence())
            )
            {
                perf().new_sequence(current_sequence());
                *(perf().get_sequence(current_sequence())) = m_moving_seq;
                draw_sequence_on_pixmap(current_sequence());
                draw_sequence_pixmap_on_window(current_sequence());  // effective?
            }
            else
            {
                perf().new_sequence(m_old_seq);
                *(perf().get_sequence(m_old_seq)) = m_moving_seq;
                draw_sequence_on_pixmap(m_old_seq);
                draw_sequence_pixmap_on_window(m_old_seq);          // effective?
            }
        }
        else
        {
            if (perf().is_active(current_sequence()))
            {
                perf().sequence_playing_toggle(current_sequence());
                draw_sequence_on_pixmap(current_sequence());
                draw_sequence_pixmap_on_window(current_sequence());  // effective?
            }
        }
    }
    else if (SEQ64_CLICK_IS_RIGHT(p0->button))
        popup_menu();

    return true;
}

/**
 *  Handle the motion of the mouse if a mouse button is down and in
 *  another sequence and if the current sequence is not in edit mode.
 *  This function moves the selected pattern to another pattern slot.
 *
 * \param p0
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_motion_notify_event (GdkEventMotion * p0)
{
    int seq = seq_from_xy((int) p0->x, (int) p0->y);
    if (m_button_down)
    {
        if
        (
            seq != current_sequence() && ! m_moving &&
            ! perf().is_sequence_in_edit(current_sequence())
        )
        {
            if (perf().is_active(current_sequence()))
            {
                m_old_seq = current_sequence();
                m_moving = true;
                m_moving_seq = *(perf().get_sequence(current_sequence()));
                perf().delete_sequence(current_sequence());
                draw_sequence_on_pixmap(current_sequence());
                draw_sequence_pixmap_on_window(current_sequence());
            }
        }
    }
    return true;
}

/**
 *  Handles an on-focus event.  Just sets the Gtk::HAS_FOCUS flag.
 *
 * \return
 *      Always returns false.
 */

bool
mainwid::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    return false;
}

/**
 *  Handles an out-of-focus event.  Just unsets the Gtk::HAS_FOCUS flag.
 *
 * \return
 *      Always returns false.
 */

bool
mainwid::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    return false;
}

}           // namespace seq64

/*
 * mainwid.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

