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
 * \updates       2016-01-01
 * \license       GNU GPLv2 or above
 *
 *  Note that this representation is, in a sense, inside the mainwnd
 *  implementation.  While mainwid represents the pattern slots, mainwnd
 *  represents the menu and surrounding elements.
 *
 *  There are a number of issue where active, but non-existent (null
 *  pointered) sequences are accessed, and we're fixing them, but need
 *  to fix the root causes as well.
 */

#include "calculations.hpp"             /* seq64::shorten_file_spec()       */
#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.         */
#include "font.hpp"
#include "mainwid.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 * The width of the main pattern/sequence grid, in pixels.  Affected by
 * the c_mainwid_border and c_mainwid_spacing values.
 */

const int c_mainwid_x =
(
    2 + (c_seqarea_x + c_mainwid_spacing) * c_mainwnd_cols -
        c_mainwid_spacing + c_mainwid_border * 2
);

/*
 * The height  of the main pattern/sequence grid, in pixels.  Affected by
 * the c_mainwid_border and c_control_height values.
 */

const int c_mainwid_y =
(
    (c_seqarea_y + c_mainwid_spacing) * c_mainwnd_rows +
         c_control_height + c_mainwid_border * 2
);

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
    m_text_size_x           (font_render().char_width()),       // c_text_x
    m_text_size_y           (font_render().padded_height()),    // c_text_y
    m_max_sets              (c_max_sets),
    m_screenset_slots       (m_mainwnd_rows * m_mainwnd_cols),
    m_screenset_offset      (m_screenset * m_screenset_slots)
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
 *  This function fills the pixmap with sequences.  Please note that
 *  draw_sequence_on_pixmap() also draws the empty slots of inactive
 *  sequences, so we cannot take shortcuts here.
 */

void
mainwid::draw_sequences_on_pixmap ()
{
    int offset = m_screenset_offset;                // m_screenset * slots
    for (int s = 0; s < m_screenset_slots; ++s, ++offset)
    {
        draw_sequence_on_pixmap(offset);
        m_last_tick_x[offset] = 0;
    }
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
 *  Also, we now ignore the sequence if it does not exist.  :-D
 *
 * \note
 *      If only the main window is up, then the sequences just play (muted by
 *      default) -- the progress bars move in each pattern.  Gaps in the
 *      sequence in the Song (performance) Editor don't change the appearance
 *      of the patterns if only the main window is up.  But, if the Song
 *      Editor window is up, and the song is started using the controls in the
 *      Song Editor, then the active patterns are black while playing, and
 *      white when gaps in the sequence are encountered.  The muting status in
 *      the main window is ignored.  The muting in the Song (performance)
 *      windows is in force.
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
        calculate_base_sizes(seqnum, base_x, base_y);   /* side-effects     */
        ++base_x;                                       /* overall fix-up   */
        ++base_y;                                       /* overall fix-up   */

        /*
         * If we contract this one pixel on every side, the brackets
         * disappear.  This box is one pixel bigger on each side than
         * the pixmap showing the sequence information.
         */

        draw_rectangle_on_pixmap                        /* filled black box */
        (
            black(), base_x, base_y, m_seqarea_x, m_seqarea_y
        );
        if (perf().is_active(seqnum))
        {
            sequence * seq = perf().get_sequence(seqnum);
            if (is_nullptr(seq))                        /* non-existent?    */
                return;                                 /* yes, ignore it   */

#ifdef SEQ64_USE_DEBUG_OUTPUT
            if (seqnum != seq->number() && seq->number() != (-1))
                printf("seq# mismatch: %d-%d\n", seqnum, seq->number());
#endif

            bool high_light = perf().highlight(*seq);
            bool smf_0 = perf().is_smf_0(*seq);
            if (high_light)
            {
                m_last_playing[seqnum] = false;         /* active, no play  */
                if (seq->get_playing())
                {
                    bg_color(black());
                    fg_color(yellow());
                }
                else
                {
                    bg_color(yellow());
                    fg_color(black());
                }
            }
            else if (smf_0)
            {
                bg_color(dark_cyan());
                fg_color(black());
            }
            else
            {
                if (seq->get_playing())
                {
                    m_last_playing[seqnum] = true;      /* active & playing */
                    bg_color(black());
                    fg_color(white());
                }
                else
                {
                    m_last_playing[seqnum] = false;     /* active, no play  */
                    bg_color(white());
                    fg_color(black());
                }
            }

            /*
             * Draw a normal background box that is one pixel less on all
             * sides than the black box that was drawn initially.  If not
             * drawn, then the sequence shows as a black box with only the
             * text showing.
             */

            draw_rectangle_on_pixmap
            (
                bg_color(), base_x+1, base_y+1, m_seqarea_x-2, m_seqarea_y-2
            );
            m_gc->set_foreground(fg_color());

            font::Color col = font::BLACK;
            if (high_light)
            {
                if (fg_color() == black())
                    col = font::BLACK_ON_YELLOW;
                else if (fg_color() == yellow())
                    col = font::YELLOW_ON_BLACK;
            }
            else if (smf_0)
            {
                col = font::BLACK_ON_CYAN;
            }
            else
            {
                if (fg_color() == black())
                    col = font::BLACK;
                else if (fg_color() == white())
                    col = font::WHITE;
            }

            char temp[32];
            snprintf(temp, sizeof temp, "%.13s", seq->get_name());
            render_string_on_pixmap                 // seqnum:name of pattern
            (
                base_x + m_text_size_x - 3, base_y + 4, temp, col
            );

            /*
             * MIDI buss + channel + timesig + seqnum + ui key
             * Compensates for text-width using actual character width.
             */

            if (perf().show_ui_sequence_key())
            {
                char key = char(perf().lookup_keyevent_key(seqnum));
                int charx = base_x + m_seqarea_x - 3;
                int chary = base_y + m_text_size_y * 4 - 2;
                snprintf(temp, sizeof temp, "%c", key);
                charx -= strlen(temp) * m_text_size_x;
                render_string_on_pixmap(charx, chary, temp, col);
            }

            /*
             * Get the format of the bottom-left line from the perform module,
             * and display it in the active pattern slots.
             */

            std::string label = perf().sequence_label(*seq);
            render_string_on_pixmap                         // bus, ch, etc.
            (
                base_x + m_text_size_x - 3, base_y + m_text_size_y * 4 - 2,
                label, col
            );

            int rectangle_x = base_x + m_text_size_x - 1;
            int rectangle_y = base_y + m_text_size_y + m_text_size_x - 1;
            int x = rectangle_x - 2;
            int y = rectangle_y - 1;
            int lx = m_seqarea_seq_x + 3;
            int ly = m_seqarea_seq_y + 3;

            /*
             * Draw the inner rectangle containing the notes of a sequence.
             * If queued, color the rectangle grey.
             */

            if (seq->get_queued())
            {
                draw_rectangle_on_pixmap(grey(), x, y, lx, ly);
                fg_color(black());
            }
            draw_rectangle_on_pixmap(fg_color(), x, y, lx, ly, false);

            int lowest_note = seq->get_lowest_note_event();
            int highest_note = seq->get_highest_note_event();
            int height = highest_note - lowest_note + 2;
            int len  = seq->get_length();
            midipulse tick_s;
            midipulse tick_f;
            int note;
            bool selected;
            int velocity;
            draw_type dt;
            seq->reset_draw_marker();

            /*
             * Doesn't prevent segfault.
             * seq->lock();                            // EXPERIMENTAL
             */

            while                       /* draws note marks in inner box    */
            (
                (
                    dt = seq->get_next_note_event(
                        &tick_s, &tick_f, &note, &selected, &velocity)
                ) != DRAW_FIN
            )
            {
                int note_y = m_seqarea_seq_y -
                     (m_seqarea_seq_y * (note + 1 - lowest_note)) / height;

                int tick_s_x = (tick_s * m_seqarea_seq_x) / len;
                int tick_f_x = (tick_f * m_seqarea_seq_x) / len;
                if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                    tick_f_x = tick_s_x + 1;

                if (tick_f_x <= tick_s_x)
                    tick_f_x = tick_s_x + 1;

                draw_line_on_pixmap
                (
                    fg_color(),
                    rectangle_x + tick_s_x, rectangle_y + note_y,
                    rectangle_x + tick_f_x, rectangle_y + note_y
                );
            }

            /*
             * Doesn't prevent segfault.
             *
             * seq->unlock();                            // EXPERIMENTAL
             */
        }
        else                                /* sequence not active          */
        {
            /*
             *  Draws grids that contain no sequence.  The first section
             *  colors the whole grid area grey, surrounded by a black
             *  outline.  The second section draws a narrower, but taller grey
             *  box, that yields the outlining "brackets" on each side of the
             *  grid area.  Without either of this drawing, an empty grid is
             *  all black boxes.  We now offer a drawing option for
             *  "black-grid", "boxed-grid", or "normal-grid" for the empty
             *  sequence box.  Now we draw a box, thicker if we're adding
             *  thickness to the grid bracket.
             *
             *  Old:  base_x + 4, base_y, m_seqarea_x - 8, m_seqarea_y
             */

            int gbt = usr().grid_brackets();            /* gb thickness     */
            bool do_brackets = gbt > 0;
            if (! do_brackets)
                gbt = -gbt;

            int offset = gbt > 1 ?  gbt - 1 : 0 ;       /* x, y offset      */
            int reduction = 2 * offset;                 /* size reduction   */
            int x = base_x + offset + 1;
            int y = base_y + offset + 1;
            int lx = m_seqarea_x - 2 - reduction;
            int ly = m_seqarea_y - 2 - reduction;
            if (usr().grid_is_normal())
                draw_normal_rectangle_on_pixmap(x, y, lx, ly);
            else if (usr().grid_is_white())
                draw_rectangle_on_pixmap(white(), x, y, lx, ly);

            if (do_brackets)
            {
                int offset = 2 * gbt + 1;               /* 2*thickness + 1  */
                x = base_x + offset;                    /* L bracket hook   */
                y = base_y;                             /* top of box       */
                lx = m_seqarea_x - 2 * offset;          /* minus 2 brackets */
                ly = m_seqarea_y;                       /* to bottom of box */
                if (usr().grid_is_white())
                    draw_rectangle_on_pixmap(white(), x, y, lx, ly);
                else if (usr().grid_is_normal())
                    draw_normal_rectangle_on_pixmap(x, y, lx, ly);
                else
                    draw_rectangle_on_pixmap(black(), x, y, lx, ly);
            }
            if (perf().show_ui_sequence_number())
            {
                char snum[8];
                snprintf(snum, sizeof(snum), "%d", seqnum);
                x = strlen(snum) * m_text_size_x / 2;
                y = font_render().char_height() / 2;
                lx = base_x + m_seqarea_x / 2 - x;              /* center x */
                ly = base_y + m_seqarea_y / 2 - y;              /* center y */
                if (usr().grid_is_black())
                    render_string_on_pixmap(lx, ly, snum, font::YELLOW_ON_BLACK);
                else
                    render_string_on_pixmap(lx, ly, snum, font::BLACK);
            }
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
    return
    (
        seqnum >= m_screenset_offset &&
        seqnum < (m_screenset_offset + m_screenset_slots)
    );
}

/**
 *  This function draws a sequence pixmap in the Patterns Panel.  The sequence
 *  is drawn only if it is in the current screen set (indicated by
 *  m_screenset.  This function is used when dragging a pattern from one
 *  pattern-slot to another pattern-slot.
 *
 *  We have to add 1 pixel to the y height in order to avoid leaving behind
 *  a line at the bottom of an empty pattern-slot.
 *
 * \param seqnum
 *      Provides the number of the sequence to draw.
 */

void
mainwid::draw_sequence_pixmap_on_window (int seqnum)
{
    if (valid_sequence(seqnum))
    {
        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);   /* side-effects */
        draw_drawable
        (
            base_x, base_y, base_x, base_y, m_seqarea_x, m_seqarea_y + 1
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
 *  This virtual function, overridden from the seqmenu base class, draws the
 *  the given pattern/sequence again.
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
    for (int s = 0; s < m_screenset_slots; ++s)
        draw_marker_on_sequence(m_screenset_offset + s, ticks);
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
        redraw(seqnum);

    if (perf().is_active(seqnum))
    {
        sequence * seq = perf().get_sequence(seqnum);
        if (is_nullptr(seq))
            return;                         /* active but non-existent!     */

        if (seq->event_count() == 0)        /* an event-free track          */
            return;                         /* new 2015-08-23 don't update  */

        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);    // side-effects

        int rectangle_x = base_x + m_text_size_x - 1;
        int rectangle_y = base_y + m_text_size_y + m_text_size_x - 1;
        int len  = seq->get_length();
        tick += len - seq->get_trigger_offset();
        tick %= len;

        midipulse tick_x = tick * m_seqarea_seq_x / len;
        draw_drawable
        (
            rectangle_x + m_last_tick_x[seqnum], rectangle_y + 1,
            rectangle_x + m_last_tick_x[seqnum], rectangle_y + 1,
            1, m_seqarea_seq_y
        );
        m_last_tick_x[seqnum] = tick_x;
        if (seqnum == current_sequence())
        {
            m_gc->set_foreground(red());
        }
        else
        {
            if (seq->get_queued())
                m_gc->set_foreground(black());
            else
                m_gc->set_foreground(seq->get_playing() ? white() : black());
        }
        draw_line
        (
            rectangle_x + tick_x, rectangle_y + 1,
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
mainwid::seq_from_xy (int x, int y)
{
    int sequence = -1;
    int slot_x = m_seqarea_x + m_mainwid_spacing;
    int slot_y = m_seqarea_y + m_mainwid_spacing;
    x -= m_mainwid_border;                      // adjust for border
    y -= m_mainwid_border;
    if                                          // is it in the box?
    (
        x >= 0 && x < (slot_x * m_mainwnd_cols) &&
        y >= 0 && y < (slot_y * m_mainwnd_rows)
    )
    {
        int box_x = x % slot_x;                 // box coordinate
        int box_y = y % slot_y;                 // box coordinate
        if (box_x <= m_seqarea_x && box_y <= m_seqarea_y)
        {
            x /= slot_x;
            y /= slot_y;
            sequence = m_screenset_offset + (x * m_mainwnd_rows + y);
        }
    }
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
mainwid::set_screenset (int ss)
{
    m_screenset = ss;
    if (m_screenset < 0)
        m_screenset = m_max_sets - 1;

    if (m_screenset >= m_max_sets)
        m_screenset = 0;

    m_screenset_offset = m_screenset * m_mainwnd_rows * m_mainwnd_cols;
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
 *
 *  This function used to call font::init(), and was the only place where the
 *  font::init() function was called.  The init() function gets a color-map
 *  from the window.  We need a more fool-proof was to do this!
 */

void
mainwid::on_realize ()
{
    gui_drawingarea_gtk2::on_realize();
    set_flags(Gtk::CAN_FOCUS);
    font_render().init(m_window);       /* see complaint in function banner */
    m_pixmap = Gdk::Pixmap::create(m_window, m_mainwid_x, m_mainwid_y, -1);
    fill_background_window();
    draw_sequences_on_pixmap();
}

/**
 *  Implements the GTK expose event callback.
 *
 * \param ev
 *      The expose event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_expose_event (GdkEventExpose * ev)
{
    draw_drawable
    (
        ev->area.x, ev->area.y,
        ev->area.x, ev->area.y,
        ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  Handles a press of a mouse button.
 *
 *  If the press is a single left-click, and no Ctrl key is pressed, then this
 *  function grabs the focus, calculates the pattern/sequence over which the
 *  button press occurred, and sets the m_button_down flag if it is over a
 *  pattern.  In the release event callback, this then causes the sequence
 *  arming/muting to be toggled.
 *
 *  If the press is a single Ctrl-left-click, this function brings up the New
 *  or Edit menu.  The New menu is brought up if the grid slot is empty, and
 *  the Edit menu otherwise.  Another way to bring up the same functionality is
 *  described in the next paragraph.
 *
 *  If the press is a double-click, it first acts just like two single-clicks
 *  (which might confuse the user at first).  Then it brings up the Edit menu
 *  for the sequence.  This new behavior is closer to what users have come to
 *  expect from a double-click.
 *
 *  We also handle a Ctrl-double-click as a signal to do an event edit, instead
 *  of a sequence edit.  The event editor provides a way to look at all events
 *  in detail, without having to select the type of event to see.
 *
 * \param p
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_button_press_event (GdkEventButton * p)
{
    grab_focus();
    if (CAST_EQUIVALENT(p->type, SEQ64_2BUTTON_PRESS))  /* double-click?    */
    {
        /*
         * Doesn't work, the event is treated like a ctrl-single-click.
         * And we use the Alt key to enable window movement or resizing in our
         * window manager.
         *
         * if (p->state & SEQ64_CONTROL_MASK)
         *    seq_event_edit();
         * else
         */

            seq_edit();
    }
    else
    {
        current_sequence(seq_from_xy(int(p->x), int(p->y)));
        if (p->state & SEQ64_CONTROL_MASK)
        {
            seq_edit();
        }
        else
        {
            if (current_sequence() >= 0 && SEQ64_CLICK_LEFT(p->button))
                m_button_down = true;
        }
    }
    return true;
}

/**
 *  Handles a release of a mouse button.  This event is a lot more complex
 *  than a press.  The left button toggles playback status. The right
 *  button brings up a popup menu.  If the slot is empty, then a "New" popup
 *  is presented, otherwise an "Edit" and selection popup is presented.
 *
 * \param p
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_button_release_event (GdkEventButton * p)
{
    current_sequence(seq_from_xy(int(p->x), int(p->y)));
    m_button_down = false;
    if (current_sequence() < 0)
        return true;

    if (SEQ64_CLICK_LEFT(p->button))
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

                /*
                 * Instead of using operator equal, use a better function.
                 *
                 * *(perf().get_sequence(current_sequence())) = m_moving_seq;
                 */

                perf().get_sequence(current_sequence())->
                    partial_assign(m_moving_seq);

                redraw(current_sequence());
            }
            else
            {
                perf().new_sequence(m_old_seq);

                /*
                 * *(perf().get_sequence(m_old_seq)) = m_moving_seq;
                 */

                perf().get_sequence(m_old_seq)->partial_assign(m_moving_seq);
                redraw(m_old_seq);
            }
        }
        else
        {
            if (perf().is_active(current_sequence()))
            {
                perf().sequence_playing_toggle(current_sequence());
                redraw(current_sequence());
            }
        }
    }
    else if (SEQ64_CLICK_RIGHT(p->button))
        popup_menu();

    return true;
}

/**
 *  Handle the motion of the mouse if a mouse button is down and in
 *  another sequence and if the current sequence is not in edit mode.
 *  This function moves the selected pattern to another pattern slot.
 *
 *  The perform::delete_sequence() function sets the perform modification
 *  flag.
 *
 * \param p
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_motion_notify_event (GdkEventMotion * p)
{
    int seq = seq_from_xy(int(p->x), int(p->y));
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

                /*
                 * m_moving_seq = *(perf().get_sequence(current_sequence()));
                 */

                m_moving_seq.partial_assign
                (
                    *(perf().get_sequence(current_sequence()))
                );
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

