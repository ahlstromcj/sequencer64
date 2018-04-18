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
 * \updates       2018-03-31
 * \license       GNU GPLv2 or above
 *
 *  Note that this representation is, in a sense, inside the mainwnd
 *  implementation.  While mainwid represents the pattern slots, mainwnd
 *  represents the menu and surrounding elements.
 *
 *  There are a number of issues where active, but non-existent (null pointer)
 *  sequences are accessed, and we've fixed them, but need to fix the root
 *  causes as well.
 *
 *  To retest:
 *
 *      -   Sequence pattern drag and drop.
 *      -   Sequence pattern playback toggling.
 *      -   Menu entry access for each slot.
 *      -   Current-edit highlighting.
 */

#include "calculations.hpp"             /* seq64::shorten_file_spec()       */
#include "click.hpp"                    /* SEQ64_CLICK_LEFT(), etc.         */
#include "font.hpp"                     /* access to font bitmap functions  */
#include "gui_key_tests.hpp"            /* is_ctrl_key(), etc.              */
#include "mainwid.hpp"                  /* seq64::mainwid (patterns panel)  */
#include "perform.hpp"                  /* seq64::perform music control     */
#include "settings.hpp"                 /* seq64::usr()                     */

/*
 *  We don't document namespaces because it screws up Doxygen.
 */

namespace seq64
{

/**
 *  Holds a pointer to the single instance of mainwnd for the entire
 *  application, once it is created.  We have decided that passing along a
 *  mainwnd reference among a number of constructors is too much and actually
 *  harder to understand and more error prone.  This value is set at the end
 *  of the mainwnd constructor, but only the first time that constructor is
 *  called.
 */

static mainwid * gs_mainwid_pointer = nullptr;

/**
 *  This global function in the seq64 namespace calls mainwid ::
 *  update_sequences_on_window(), if the global mainwid object exists.  It is
 *  used by other objects that can modify the currently-edited sequence shown
 *  in the mainwid (main window).
 */

void
update_mainwid_sequences ()
{
    if (not_nullptr(gs_mainwid_pointer))
        gs_mainwid_pointer->update_sequences_on_window();
}

/**
 *  This constructor sets all of the members.  And it asks for its size from
 *  usr().mainwid_width() and usr().mainwid_height functions.  It adds GDK
 *  masks for button presses, releases, motion, key presses, and focus
 *  changes.  Also logs a self-referential singleton pointer to use for the
 *  current-edit highlighting support.
 *
 * \param p
 *      Provides the reference to the all-important perform object.
 *
 * \param ss
 *      Indicates the screen we start out at.  This is really only useful if
 *      SEQ64_MULTI_MAINWND is defined, but doesn't hurt much to have it
 *      as a parameter here, and it could be a good feature independent of
 *      multi-mainwid support.  The default value is 0.
 */

mainwid::mainwid (perform & p, int ss)
 :
    gui_drawingarea_gtk2    (p, usr().mainwid_width(), usr().mainwid_height()),
    seqmenu                 (p),
    m_armed_progress_color
    (
        progress_color() == black() ? white() : progress_color()
    ),
    m_moving_seq            (),                 // a moving sequence object
    m_button_down           (false),
    m_moving                (false),
    m_old_seq               (0),
    m_screenset             ((ss > 0 && ss < SEQ64_DEFAULT_SET_MAX) ? ss : 0),
    m_last_tick_x           (),                 // array of size c_max_sequence
    m_mainwnd_rows          (usr().mainwnd_rows()),
    m_mainwnd_cols          (usr().mainwnd_cols()),
    m_seqarea_x             (usr().seqarea_x()),
    m_seqarea_y             (usr().seqarea_y()),
    m_seqarea_seq_x         (usr().seqarea_seq_x()),
    m_seqarea_seq_y         (usr().seqarea_seq_y()),
#if defined SEQ64_MULTI_MAINWID
    m_mainwid_x             (usr().mainwid_width()),
    m_mainwid_y             (usr().mainwid_height()),
#else
    m_mainwid_x             (usr().mainwid_x()),
    m_mainwid_y             (usr().mainwid_y()),
#endif
    m_mainwid_border_x      (usr().mainwid_border_x()),
    m_mainwid_border_y      (usr().mainwid_border_y()),
    m_mainwid_spacing       (usr().mainwid_spacing()),
    m_text_size_x           (font_render().char_width()),       // unscalable
    m_text_size_y           (font_render().padded_height()),    // unscalable
    m_max_sets              (c_max_sets),
    m_screenset_slots       (m_mainwnd_rows * m_mainwnd_cols),
    m_screenset_offset      (m_screenset * m_screenset_slots),
    m_progress_height       (usr().seqarea_seq_y() + 3)
{
    if (is_nullptr(gs_mainwid_pointer))
        gs_mainwid_pointer = this;
}

/**
 *  A rote destructor.
 */

mainwid::~mainwid ()
{
    /* Empty body   */
}

/**
 *  This function fills the pixmap with sequences.  Please note that
 *  draw_sequence_on_pixmap() also draws the empty slots of inactive
 *  sequences, so we cannot take shortcuts here.
 */

void
mainwid::draw_sequences_on_pixmap ()
{
    int offset = m_screenset_offset;                /* m_screenset * slots  */
    for (int s = 0; s < m_screenset_slots; ++s, ++offset)
        draw_sequence_on_pixmap(offset);
}

/**
 *  Provides a stock callback, because some kind of callback is needed.
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
 *  Picks the foreground and background colors based on the sequence in edit
 *  and the SEQ64_EDIT_SEQUENCE_HIGHLIGHT macro.
 */

void
mainwid::select_fg_bg_colors (int seqnum)
{
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
    if (is_edit_sequence(seqnum))
    {
        bg_color(dark_cyan());
        fg_color(black());
    }
    else
    {
        bg_color(white());
        fg_color(black());
    }
#else
    bg_color(white());
    fg_color(black());
#endif
}

/**
 *  This function draws a specific pattern/sequence on the pixmap located
 *  in the main window of the application, the Patterns Panel.  The
 *  sequence is drawn only if it is in the current screen set (indicated
 *  by m_screenset).  Also, we ignore the sequence if it does not exist.
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
 *      windows is in force.  This setup holds for ALSA, but not for JACK
 *      transport.
 *
 * \param seqnum
 *      Provides the number of the sequence slot that needs to be drawn.  It
 *      is checked for validity before usage.
 */

void
mainwid::draw_sequence_on_pixmap (int seqnum)
{
    if (valid_sequence(seqnum))
    {
        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);   /* side-effects     */

        /*
         * If we contract this one pixel on every side, the brackets
         * disappear.  This box is one pixel bigger on each side than
         * the pixmap showing the sequence information.
         */

        ++base_x;                                       /* overall fix-up   */
        ++base_y;                                       /* overall fix-up   */
        draw_rectangle_on_pixmap                        /* filled black box */
        (
            black(), base_x, base_y, m_seqarea_x, m_seqarea_y
        );
        sequence * seq = perf().get_sequence(seqnum);
        if (not_nullptr(seq))                       // perf().is_active(seqnum)
        {
            bool empty_highlight = perf().highlight(*seq);
            bool smf_0 = perf().is_smf_0(*seq);
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
            bool current_highlight = smf_0 || is_edit_sequence(seqnum);
#else
            bool current_highlight = smf_0;
#endif

            if (empty_highlight)
            {
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
            else if (current_highlight)
            {
                /*
                 * \change ca 2016-05-08 Let's try to preserve the muting
                 * status as well.  Idea from muranyia on GitHub.
                 */

                if (seq->get_playing())
                {
                    bg_color(black());
                    fg_color(dark_cyan());
                }
                else
                {
                    bg_color(dark_cyan());
                    fg_color(black());
                }
            }
            else
            {
                if (seq->get_playing())
                {
                    /*
                     * We need to change the foreground color to black if the
                     * sequence has no color, which mandates a white
                     * background.
                     */

                    bg_color(black());                  /* never inversed   */
                    fg_color(white());                  /* ditto            */
                }
                else
                    select_fg_bg_colors(seqnum);
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
            if (empty_highlight)
            {
                if (fg_color() == black())
                    col = font::BLACK_ON_YELLOW;
                else if (fg_color() == yellow())
                    col = font::YELLOW_ON_BLACK;
            }
            else if (current_highlight)
            {
                if (seq->get_playing())
                    col = font::CYAN_ON_BLACK;
                else
                    col = font::BLACK_ON_CYAN;
            }
            else
            {
                if (fg_color() == black())
                    col = font::BLACK;
                else if (fg_color() == white())
                    col = font::WHITE;
            }

            std::string title = perf().sequence_title(*seq);
            render_string_on_pixmap             /* seqnum:name of pattern   */
            (
                base_x + m_text_size_x - 3, base_y + 4, title, col
            );

            /*
             * MIDI buss + channel + timesig + seqnum + ui key
             * Compensates for text-width using actual character width.
             */

            if (perf().show_ui_sequence_key())
            {
                /*
                 * Here, we want to show the hot-keys for each pattern in the
                 * current set.  The default number of sequences in a set is
                 * c_seqs_in_set, but we can support up to 3 times that value
                 * depending on the varisets setting.  Here, c_seqs_in_set is
                 * really "number of hot-keys supported" value.
                 */

                int charx = base_x + m_seqarea_x - 3;
                int chary = base_y + m_text_size_y * 4 - 2;
                int slot = perf().slot_number(seqnum);
                char key = char(perf().lookup_slot_key(seqnum));
                if (key > 0)
                {
                    char temp[8];
                    const int lower_slot = 2 * c_seqs_in_set;
                    const int upper_slot = 2 * c_seqs_in_set;
                    char sh = char(PREFKEY(pattern_shift));
                    if ((sh > 0) && usr().is_variset())
                    {
                        if (slot >= lower_slot && slot < upper_slot)
                            snprintf(temp, sizeof temp, "%c%c%c", sh, sh, key);
                        else if (slot >= c_seqs_in_set && slot < lower_slot)
                            snprintf(temp, sizeof temp, "%c%c", sh, key);
                        else
                            snprintf(temp, sizeof temp, "%c", key);
                    }
                    else
                        snprintf(temp, sizeof temp, "%c", key);

                    charx -= strlen(temp) * m_text_size_x;
                    render_string_on_pixmap(charx, chary, temp, col);
                }
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
             * If queued, color the rectangle grey.  If one-shot queued, color
             * it light grey.
             */

            if (seq->get_queued())
            {
                draw_rectangle_on_pixmap(grey_paint(), x, y, lx, ly);
                fg_color(black());
            }
#ifdef SEQ64_SONG_RECORDING
            else if (seq->one_shot())
            {
                draw_rectangle_on_pixmap(light_grey_paint(), x, y, lx, ly);
                fg_color(black());
            }
#endif
            else
            {
#ifdef SEQ64_SHOW_COLOR_PALETTE

                /*
                 * Draws a filled-in rectangle to hold the event marks.  We
                 * might make this conditional to preserve the full coloring
                 * of empty or in-edit sequences. TBD.
                 */

                int c = seq->color();
                Color color = get_color_ex(PaletteColor(c), 40.0, 0.20, 0.5);
                if (c == SEQ64_COLOR_NONE)
                    color = bg_color();     /* preserve normal coloring     */

                draw_rectangle_on_pixmap(color, x, y, lx, ly);
#endif
                /*
                 * Draws a rectangular outline around the event marks.
                 */

                draw_rectangle_on_pixmap(fg_color(), x, y, lx, ly, false);
            }

            int low_note;                                   // for side-effect
            int high_note;                                  // ditto
            bool have_notes = seq->get_minmax_note_events(low_note, high_note);
            if (have_notes)
            {
                int height = high_note - low_note + 2;      // 2-pixel border
                int len = seq->get_length();
                midipulse tick_s;
                midipulse tick_f;
                int note;
                bool selected;
                int velocity;
                draw_type_t dt;
                Color drawcolor = fg_color();
                Color eventcolor = fg_color();

#ifdef SEQ64_STAZED_TRANSPOSE
                if (! seq->get_transposable())
                {
                    eventcolor = red();
                    drawcolor = red();
                }
#endif
                /*
                 * Draw the note events in the sequence.
                 */

                seq->reset_draw_marker();       /* reset container iterator */
                do
                {
                    dt = seq->get_next_note_event           /* side-effects */
                    (
                        tick_s, tick_f, note, selected, velocity
                    );
                    if (dt == DRAW_FIN)
                        break;

                    int tick_s_x = tick_s * m_seqarea_seq_x / len;
                    int tick_f_x = tick_f * m_seqarea_seq_x / len;
                    int note_y;
                    if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                        tick_f_x = tick_s_x + 1;

                    if (tick_f_x <= tick_s_x)
                        tick_f_x = tick_s_x + 1;

                    if (dt == DRAW_TEMPO)
                    {
                        /*
                         * Do not scale by the note range here.
                         */

                        set_line(Gdk::LINE_SOLID, 2);
                        drawcolor = tempo_paint();
                        note_y = m_seqarea_seq_y -
                             m_seqarea_seq_y * (note + 1) / SEQ64_MAX_DATA_VALUE;
                    }
                    else
                    {
                        note_y = m_seqarea_seq_y -
                             m_seqarea_seq_y * (note + 1 - low_note) / height;
                    }

                    int sx = rectangle_x + tick_s_x;            /* start x  */
                    int fx = rectangle_x + tick_f_x;            /* finish x */
                    int sy = rectangle_y + note_y;              /* start y  */
                    int fy = sy;                                /* finish y */
                    draw_line_on_pixmap(drawcolor, sx, sy, fx, fy);

                    if (dt == DRAW_TEMPO)
                    {
                        /*
                         * We would like to also draw a line from the end of
                         * the current tempo to the start of the next one.
                         * But we currently have only the x value of the next
                         * tempo.
                         *
                         * sx = fx;
                         * fy = next tempo value scaled to 127;
                         */

                        set_line(Gdk::LINE_SOLID, 1);
                        drawcolor = eventcolor;
                    }

                } while (dt != DRAW_FIN);
            }
        }
        else                                            /* sequence inactive */
        {
            /*
             *  Draws grids that contain no sequence.  The first section
             *  colors the whole grid area grey, surrounded by a black
             *  outline.  The second section draws a narrower, but taller grey
             *  box, that yields the outlining "brackets" on each side of the
             *  grid area.  Without either of this drawing, an empty grid is
             *  all black boxes.  We offer a drawing option for "black-grid",
             *  "boxed-grid", or "normal-grid" for the empty sequence box.  We
             *  draw a box, thicker if adding thickness to the grid bracket.
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
                char snum[16];
                snprintf(snum, sizeof snum, "%d", seqnum);
                x = strlen(snum) * m_text_size_x / 2;
                y = font_render().char_height() / 2;
                lx = base_x + m_seqarea_x / 2 - x;              /* center x */
                ly = base_y + m_seqarea_y / 2 - y;              /* center y */

                font::Color col = font::BLACK;
                if (usr().grid_is_black())
                    col = font::YELLOW_ON_BLACK;

                render_string_on_pixmap(lx, ly, snum, col);
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
        int x, y;
        calculate_base_sizes(seqnum, x, y);                 /* side-effects */
        draw_drawable(x, y, x, y, m_seqarea_x, m_seqarea_y + 1);
    }
}

/**
 *  Provides a way to calculate the base x and y size values for the
 *  pattern map.  The values are returned as side-effects.
 *
 * \param seqnum
 *      Provides the number of the sequence to calculate.
 *
 * \param [out] basex
 *      A return parameter for the x coordinate of the base size.
 *
 * \param [out] basey
 *      A return parameter for the y coordinate of the base size.
 */

void
mainwid::calculate_base_sizes (int seqnum, int & basex, int & basey)
{
    int i = (seqnum / m_mainwnd_rows) % m_mainwnd_cols;
    int j =  seqnum % m_mainwnd_rows;
    basex = m_mainwid_border_x + (m_seqarea_x + m_mainwid_spacing) * i;
    basey = m_mainwid_border_y + (m_seqarea_y + m_mainwid_spacing) * j;
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
    draw_sequence_pixmap_on_window(seqnum);
}

/**
 *  Draw the cursors (long vertical bars) on each sequence, so that they
 *  follow the playing progress of each sequence in the mainwid (Patterns
 *  Panel).
 *
 * \param tick
 *      Starting point for drawing the markers.
 */

void
mainwid::update_markers (int tick)
{
    for (int s = 0; s < m_screenset_slots; ++s)
        draw_marker_on_sequence(m_screenset_offset + s, tick);
}

/**
 *  Does the actual drawing of one pattern/sequence position marker, a
 *  vertical progress bar.  If the sequence has no events, this function
 *  doesn't bother drawing a position marker.
 *
 *  Note that, when Sequencer64 first comes up, and perform::is_dirty_main()
 *  is called, no sequences exist yet.  Also, currently the redraw() is hit
 *  when seq_edit() is called, but not when seq_event_edit() is called, which
 *  makes the latter not paint the in-edit highlight colors (if enabled).
 *  Why?
 *
 * \param seqnum
 *      Provides the number of the sequence to draw.
 *
 * \param tick
 *      Provides the location to draw the marker.  If pause support is
 *      compiled in (i.e. no --disable-pause in the configuration), then this
 *      parameter is ignored, and is replaced by the sequences'
 *      get_lask_tick() value.  This causes correct stop/pause/play
 *      progress-bar behavior in each pattern slot.  Note: This is now
 *      independent of the --disable-pause option!
 */

void
mainwid::draw_marker_on_sequence (int seqnum, int tick)
{
    if (perf().is_dirty_main(seqnum))
        redraw(seqnum);

    if (perf().is_active(seqnum))           /* also checks for nullptr      */
    {
        sequence * seq = perf().get_sequence(seqnum);

        /*
         * If this is commented out, a non-moving progress-bar appears at the
         * left of each empty track.  We do want to show the moving progress
         * bar in seqroll, so this note here is an investigation into that
         * issue.
         */

        if (seq->event_count() == 0)        /* an event-free track          */
            return;                         /* new 2015-08-23 don't update  */

        int base_x, base_y;
        calculate_base_sizes(seqnum, base_x, base_y);    /* side-effects    */

        int rect_x = base_x + m_text_size_x - 1;
        int rect_y = base_y + m_text_size_y + m_text_size_x - 1;
        int len = seq->get_length();
        tick = int(seq->get_last_tick());   /* seems to work, see banner    */
        tick += len - seq->get_trigger_offset();
        tick %= len;

        long tick_x = tick * m_seqarea_seq_x / len;
        int bar_x = rect_x + int(m_last_tick_x[seqnum]);
        int thickness = 1;
        if (usr().progress_bar_thick())
        {
            --bar_x;
            thickness = 2;
            set_line(Gdk::LINE_SOLID, 2);
        }
        draw_drawable
        (
            bar_x, rect_y + 1, bar_x, rect_y + 1, thickness, m_progress_height
        );
        m_last_tick_x[seqnum] = tick_x;
        if (seqnum == current_seq())        /* is this good enough?     */
        {
            m_gc->set_foreground(red());    /* red is easiest to see    */
        }
        else
        {
            if (seq->get_queued())
            {
                m_gc->set_foreground(black());
            }
#ifdef SEQ64_SONG_RECORDING
            else if (seq->one_shot())
            {
                m_gc->set_foreground(blue());
            }
#endif
            else
            {
                /*
                 * We like the look of a cyan-colored progress bar in both the
                 * muted and unmuted sequence slots.  We have made the color
                 * of the progress bar configurable.  However, when the
                 * background is black (armed), white is still needed, at
                 * least when the progress bar is black.
                 */

                m_gc->set_foreground
                (
                    seq->get_playing() ? m_armed_progress_color : progress_color()
                );
            }
        }
        draw_line
        (
            rect_x + tick_x, rect_y + 1,
            rect_x + tick_x, rect_y + m_progress_height
        );
        if (usr().progress_bar_thick())
            set_line(Gdk::LINE_SOLID, 1);
    }
}

/**
 *  Translates XY coordinates in the Patterns Panel to a sequence number.
 *
 * \param x
 *      Provides the x coordinate.
 *
 * \param y
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
    x -= m_mainwid_border_x;                    // adjust for border
    y -= m_mainwid_border_y;
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
 *  Set the current screen-set.  Note that m_screenset_slots =
 *  m_mainwnd_rows * m_mainwnd_cols and this will also match
 *  perform::m_seqs_in_set.
 *
 *  This function calls perform::set_screenset(), which recapitulates the old
 *  code above completely, whereas perform::set_offset() recapitulates only
 *  the line of code immediately above it.  However, note that there is a
 *  back-and-forth between setting the screenset via perform (using MIDI
 *  control) versus the GUI in the mainwnd class.  Probably useful to add a
 *  default boolean to prevent circular manipulation.
 *
 * \param ss
 *      Provides the screen-set number to set.
 *
 * \param setperf
 *      If true, then also call perform::set_screenset(), even if multi-wid
 *      mode is not in force.  Defaults to false.  It might be better if it
 *      defaults to true.
 *
 * \return
 *      Returns the (new) value of m_screenset so that the main window can set
 *      the set-number in the Set spinner.
 */

int
mainwid::set_screenset (int ss)
{
    if (ss != m_screenset)
        log_screenset(ss);          /* changes m_screenset and the offset   */

    return m_screenset;
}

/**
 * \setter m_screenset
 *      This function is used for altering the current screen-set
 *      displayed by a single mainwid in multi-mainwid mode.
 */

void
mainwid::log_screenset (int ss)
{
    m_screenset = ss;
    m_screenset_offset = m_screenset_slots * ss;  // perf().screenset_offset(ss);
    reset();
}

/**
 *  Calculates the sequence number based on the screenset and then
 *  calls the base-class function to bring up the pattern/sequence editor.
 *  Used with the '=' key selection, by default.
 */

void
mainwid::seq_set_and_edit (int seqnum)
{
    seqmenu::seq_set_and_edit(seqnum + m_screenset_offset);
}

/**
 *  Calculates the sequence number based on the screenset and then
 *  calls the base-class function to bring up the event editor.
 *  Used with the '-' key selection, by default.
 */

void
mainwid::seq_set_and_eventedit (int seqnum)
{
    seqmenu::seq_set_and_eventedit(seqnum + m_screenset_offset);
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
        ev->area.x, ev->area.y, ev->area.x, ev->area.y,
        ev->area.width, ev->area.height
    );
    return true;
}

/**
 *  Handles a press of a mouse button in one of the sequence/pattern slots.
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
 *  (which might confuse the user at first, because it toggles the mute state
 *  twice).  Then it brings up the Edit menu for the sequence.  This new
 *  behavior is closer to what users have come to expect from a double-click.
 *  I miss the double-click when running seq24.
 *
 *  We also try to handle a Ctrl-double-click as a signal to do an event edit,
 *  instead of a sequence edit.  The event editor provides a way to look at
 *  all events in detail, without having to select the type of event to see.
 *  However, this doesn't work, the event is treated like a ctrl-single-click.
 *  And we use the Alt key to enable window movement or resizing in our window
 *  manager, so that's out.
 *
 * \param ev
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_button_press_event (GdkEventButton * ev)
{
    grab_focus();
    int seqnum = seq_from_xy(int(ev->x), int(ev->y));
    if (CAST_EQUIVALENT(ev->type, SEQ64_2BUTTON_PRESS)) /* double-click?    */
    {
        if (rc().allow_click_edit())                    /* now optional     */
            seq_edit();                                 /* seqmenu function */

        update_sequences_on_window();
    }
    else
    {
        current_seq(seqnum);

#ifdef USE_CTRL_KEY_TO_EDIT_SEQUENCE

        /*
         * This feature will interfere with user setups that (unwisely)
         * use the Ctrl keys as sequencer control keys.  We can already
         * double-click to open a sequence for editing, so let's be satisfied
         * with that.
         */

        if (is_ctrl_key(ev))
        {
            seq_edit();                                 /* seqmenu function */
            update_sequences_on_window();
        }
        else
        {
            if (current_seq() >= 0 && SEQ64_CLICK_LEFT(ev->button))
            {
                m_button_down = true;
                update_sequences_on_window();
            }
        }
#else
        /*
         * Do not let a click toggle the mute state if the control key is
         * pressed.
         */

        bool ok = ! is_ctrl_key(ev);
        if (ok)
            ok = current_seq() >= 0 && SEQ64_CLICK_LEFT(ev->button);

        if (ok)
        {
            m_button_down = true;
            update_sequences_on_window();
        }
#endif

    }
    return true;
}

/**
 *  Handles a release of a mouse button.  This event is a lot more complex
 *  than a press.  The left button toggles playback status. The right
 *  button brings up a popup menu.  If the slot is empty, then a "New" popup
 *  is presented, otherwise an "Edit" and selection popup is presented.
 *
 *  Also now implements the new "toggle all other patterns" action, initiated
 *  via Shift-Left-Click.
 *
 * \param ev
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_button_release_event (GdkEventButton * ev)
{
    /**
     * Tried disabling the setting of the current sequence; it completely
     * disables drag-n-drop.  But leaving it in removes the current-sequence
     * highlighting, which otherwise is fine.  So we do it only if moving a
     * pattern (drag-and-drop).
     */

    if (m_moving)
        current_seq(seq_from_xy(int(ev->x), int(ev->y)));

    m_button_down = false;
    if (current_seq() < 0)
        return true;

    if (SEQ64_CLICK_LEFT(ev->button))
    {
        if (m_moving)
        {
            /*
             * Hmmm, seq24 also tests for m_current_seq == -1.
             */

            m_moving = false;
            if (! is_current_seq_active() && ! is_current_seq_in_edit())
            {
                new_current_sequence();
                get_current_sequence()->partial_assign(m_moving_seq);
                redraw(current_seq());
            }
            else
            {
                new_sequence(m_old_seq);
                get_sequence(m_old_seq)->partial_assign(m_moving_seq);
                redraw(m_old_seq);
            }
        }
        else
        {
            /*
             * If shift is held, toggle all the other sequences.
             *
             * \note ca 2017-03-27 This causes Options / Keyboard key settings
             *      involving shifted keys to misbehave.  Specifically, Mod
             *      Replace!
             */

            bool shifted = is_shift_key(ev);
            bool done = perf().toggle_other_seqs(current_seq(), shifted);
            if (! done)
            {
                if (! is_ctrl_key(ev))
                {
                    /*
                     * This is the original action, a toggle of one pattern.
                     */

                    if (is_current_seq_active())    /* toggle playing status */
                    {
                        toggle_current_sequence();
                        redraw(current_seq());
                    }
                }
            }
        }
    }
    else if (SEQ64_CLICK_RIGHT(ev->button))
        popup_menu();

    return true;
}

/**
 *  Handle the motion of the mouse if a mouse button is down and in another
 *  sequence and if the current sequence is not in edit mode.  This function
 *  moves the selected pattern to another pattern slot.  The
 *  perform::delete_sequence() function sets the perform modification flag.
 *
 * \param ev
 *      Provides the parameters of the button event.
 *
 * \return
 *      Always returns true.
 */

bool
mainwid::on_motion_notify_event (GdkEventMotion * ev)
{
    int seq = seq_from_xy(int(ev->x), int(ev->y));
    if (m_button_down)
    {
        if (seq != current_seq() && ! m_moving && ! is_current_seq_in_edit())
        {
            if (is_current_seq_active())
            {
                m_old_seq = current_seq();
                m_moving = true;
                m_moving_seq.partial_assign(*(get_current_sequence()));
                delete_current_sequence();
                draw_sequence_on_pixmap(current_seq());
                draw_sequence_pixmap_on_window(current_seq());
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

