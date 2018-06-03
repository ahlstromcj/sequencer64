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
 * \file          seqedit.cpp
 *
 *  This module declares/defines the base class for editing a
 *  pattern/sequence.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-04-13
 * \license       GNU GPLv2 or above
 *
 *  Compare this class to eventedit, which has to do some similar things,
 *  though it has a lot fewer user-interface controls to manage.
 *
 *  There's one big mistake in this module... making the size of the grid
 *  dependent on the PPQN values.  As the PPQN gets larger, so does the size
 *  of each measure, until too big to appear in whole on the screen.  Luckily,
 *  we can get around this issue by changing the zoom range.
 *
 *  We've made the Stazed thru/record fix permanent.  See issue #56 re MIDI
 *  control.  We weren't turning off thru!
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 *
 * Table.
 *
 *      This dialog is layed out as a Gtk::Table, which is deprecated in
 *      preference to Gtk::Grid.  The attachment numbers have the following
 *      meanings:
 *
 *      -   left_attach   Column number to attach left side of a child widget.
 *      -   right_attach  Column number to attach right side of a child widget.
 *      -   top_attach    Row number to attach top of a child widget.
 *      -   bottom_attach Row number to attach bottom of a child widget.
 *
 *      The rows and columns of the seqedit main portion are:
 *
 *  Column:         0       1                                         2
 *  Row:         -------------------------------------------------------
 *          0   |         | seqtime  1 2 0 1                        | b |
 *              |---------------------------------------------------|---|
 *              |         |                                         | v |
 *          1   | seqkeys | seqroll  1 2 1 2                        | s |
 *              | 0 1 1 2 |                                         |   |
 *              |---------------------------------------------------|---|
 *          2   | blank   | seqevent 1 2 2 3                        | b |
 *              |---------------------------------------------------|---|
 *              |         |                                         |   |
 *          3   | blank   | seqdata  1 2 3 4                        | b |
 *              | 0 1 3 4 |                                         |   |
 *              |---------------------------------------------------|---|
 *          4   | icon??? | dhbox    1 2 4 5 event to rec-volume    | b |
 *              |---------------------------------------------------|---|
 *          5   | blank   | horizontal scrollbar                    | b |
 *               -------------------------------------------------------
 *
 * Menu refinements.
 *
 *      We are fixing the popup menus so that they are created only once.
 *      Here is the progress:
 *
 *      -   m_menu_bpm.
 *      -   m_menu_bw.
 *      -   m_menu_chords.
 *      -   m_menu_data.
 *      -   m_menu_key.
 *      -   m_menu_length.
 *      -   m_menu_midibus.  Done.
 *      -   m_menu_midich.  Done.
 *      -   m_menu_note_length.
 *      -   m_menu_rec_type.
 *      -   m_menu_rec_vol.
 *      -   m_menu_scale.
 *      -   m_menu_scale.
 *      -   m_menu_sequences.  Done.
 *      -   m_menu_snap.
 *      -   m_menu_tools.  Done.
 *      -   m_menu_zoom.
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/combo.h>
#include <gtkmm/label.h>
#include <gtkmm/separator.h>
#include <gtkmm/table.h>
#include <gtkmm/tooltips.h>
#include <sigc++/bind.h>

#include "calculations.hpp"             /* measures_to_ticks()          */
#include "controllers.hpp"
#include "event.hpp"
#include "fruityseq.hpp"                /* seq64::FruitySeqEventInput   */
#include "fruityseqroll.hpp"            /* seq64::FruitySeqRollInput    */
#include "keystroke.hpp"                /* uses "gdk_basic_keys.h"      */
#include "globals.h"
#include "gtk_helpers.h"
#include "gui_key_tests.hpp"            /* is_ctrl_key(), etc.          */
#include "mainwid.hpp"
#include "options.hpp"
#include "perfedit.hpp"
#include "perform.hpp"
#include "scales.h"
#include "seqdata.hpp"
#include "seqedit.hpp"
#include "seqevent.hpp"
#include "seqkeys.hpp"
#include "seqroll.hpp"
#include "seqtime.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::rc() or seq64::usr()  */
#include "user_instrument.hpp"          /* seq64::user_instrument       */

#ifdef SEQ64_STAZED_LFO_SUPPORT
#include "lfownd.hpp"
#endif

#include "pixmaps/follow.xpm"
#include "pixmaps/fruity.xpm"
#include "pixmaps/tux.xpm"
#include "pixmaps/play.xpm"
#include "pixmaps/q_rec.xpm"
#include "pixmaps/rec.xpm"
#include "pixmaps/thru.xpm"
#include "pixmaps/bus.xpm"
#include "pixmaps/midi.xpm"
#include "pixmaps/snap.xpm"
#include "pixmaps/zoom.xpm"
#include "pixmaps/length_short.xpm"
#include "pixmaps/scale.xpm"
#include "pixmaps/key.xpm"
#include "pixmaps/down.xpm"
#include "pixmaps/note_length.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/redo.xpm"
#include "pixmaps/quantize.xpm"
#include "pixmaps/menu_empty.xpm"
#include "pixmaps/menu_full.xpm"
#include "pixmaps/sequences.xpm"
#include "pixmaps/tools.xpm"
#include "pixmaps/seq-editor.xpm"

#ifdef SEQ64_STAZED_CHORD_GENERATOR
#include "pixmaps/chord3-inv.xpm"
#endif

#ifdef SEQ64_STAZED_TRANSPOSE
#include "pixmaps/drum.xpm"
#include "pixmaps/transpose.xpm"
#endif

/**
 *  Manifest constants for the top panel text sizes.
 */

#define SEQ64_ENTRY_SIZE_SEQNUMBER       4
#define SEQ64_ENTRY_SIZE_SEQNAME        20
#define SEQ64_ENTRY_SIZE_BUSNAME        32

/*
 * Saves some typing.  We could, like Stazed, limit the scope of this to
 * popup_sequence_menu(), popup_event_menu(), popup_record_menu(),
 * create_menus(), popup_tool_menu(), etc.
 */

using namespace Gtk::Menu_Helpers;

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Static data members.  These items apply to all of the instances of seqedit,
 *  and are passed on to the following constructors:
 *
 *  -   seqdata
 *  -   seqevent
 *  -   seqroll
 *  -   seqtime
 *
 *  The snap and note-length defaults would be good to write to the "user"
 *  configuration file.  The scale and key would be nice to write to the
 *  proprietary section of the MIDI song.  Or, even more flexibly, to each
 *  sequence, if that makes sense to do, since all tracks would generally be
 *  in the same key.  Right, Charles Ives?
 *
 *  Note that, currently, that some of these "initial values" are modified, so
 *  that they are "contagious".  That is, the next sequence to be opened in
 *  the sequence editor will adopt these values.  This is a long-standing
 *  feature of Seq24, but strikes us as a bit surprising.
 *
 * \change ca 2016-04-10
 *      If we just double the PPQN, then the snap divisor becomes 32, and the
 *      snap interval is a 32nd note.  We would like to keep it at a 16th
 *      note.  We correct the snap ticks to the actual PPQN ratio.
 */

int seqedit::m_initial_snap                 = SEQ64_DEFAULT_PPQN / 4;
int seqedit::m_initial_note_length          = SEQ64_DEFAULT_PPQN / 4;

#ifdef SEQ64_STAZED_CHORD_GENERATOR
int seqedit::m_initial_chord = 0;
#endif

/**
 * Actions.  These variables represent actions that can be applied to a
 * selection of notes.  One idea would be to add a swing-quantize action.
 * We will reserve the value here, for notes only; not yet used or part of the
 * action menu.
 */

enum edit_action_t
{
    c_select_all_notes         =  1,
    c_select_all_events        =  2,
    c_select_inverse_notes     =  3,
    c_select_inverse_events    =  4,
    c_quantize_notes           =  5,
    c_quantize_events          =  6,
#ifdef USE_STAZED_RANDOMIZE_SUPPORT
    c_randomize_events         =  7,
#endif
    c_tighten_events           =  8,
    c_tighten_notes            =  9,
    c_transpose_notes          = 10,    /* basic transpose      */
    c_reserved                 = 11,
    c_transpose_h              = 12,    /* harmonic transpose   */
    c_expand_pattern           = 13,
    c_compress_pattern         = 14,
    c_select_even_notes        = 15,
    c_select_odd_notes         = 16,
    c_swing_notes              = 17     /* swing quantize       */
};

/**
 *  Principal constructor.
 *
 *  If provided, override the scale, key, and background-sequence with the
 *  values stored in the file with the sequence, if they are set to
 *  non-default values.  This is a new feature.
 *
 * \todo
 *      Offload most of the work into an initialization function like
 *      options does.
 *
 *  Horizontal Gtk::Adjustment constructor: The initial value was 0 on a range
 *  from 0 to 1, with step and page increments of 1, and a page_size of 1.  We
 *  can fix these values here, or create an h_adjustment() function similar to
 *  eventedit::v_adjustment(), which first gets called in on_realize().
 *
 * \param p
 *      The performance object of which the sequence is a part.
 *
 * \param seq
 *      The seqeuence object this window object represents.
 *
 * \param pos
 *      The sequence number (pattern slot number) for this sequence and
 *      window.
 *
 * \param ppqn
 *      The optional PPQN parameter for this sequence.  Warning:  not really
 *      used by the caller, need to square that!
 */

seqedit::seqedit
(
    perform & p,
    sequence & seq,
    int pos,
    int ppqn
) :
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    gui_window_gtk2     (p, 900, 500),                  // size request
#else
    gui_window_gtk2     (p, 750, 500),                  // size request
#endif
    m_initial_zoom      (SEQ64_DEFAULT_ZOOM),           // constant
    m_zoom              (SEQ64_DEFAULT_ZOOM),           // fixed below
    m_snap              (m_initial_snap),
    m_note_length       (m_initial_note_length),
    m_scale             (usr().seqedit_scale()),        // m_initial_scale
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    m_chord             (0),    // (usr().seqedit_chord()),  // m_initial_chord
#endif
    m_key               (usr().seqedit_key()),          // m_initial_key
    m_bgsequence        (usr().seqedit_bgsequence()),   // m_initial_sequence
    m_measures          (0),                            // fixed below
    m_ppqn              (0),                            // fixed below
#ifdef USE_STAZED_ODD_EVEN_SELECTION
    m_pp_whole          (0),
    m_pp_eighth         (0),
    m_pp_sixteenth      (0),
#endif
    m_seq               (seq),
    m_menubar           (manage(new Gtk::MenuBar())),
    m_menu_tools        (nullptr),              // (manage(new Gtk::Menu())),
    m_menu_zoom         (manage(new Gtk::Menu())),
    m_menu_snap         (manage(new Gtk::Menu())),
    m_menu_note_length  (manage(new Gtk::Menu())),
    m_menu_length       (manage(new Gtk::Menu())),
#ifdef SEQ64_STAZED_TRANSPOSE
    m_toggle_transpose  (manage(new Gtk::ToggleButton())),
    m_image_transpose   (nullptr),
#endif
    m_menu_midich       (nullptr),
    m_menu_midibus      (nullptr),
    m_menu_data         (nullptr),                  // see m_button_data
    m_menu_minidata     (nullptr),                  // see m_button_minidata
    m_menu_key          (manage(new Gtk::Menu())),
    m_menu_scale        (manage(new Gtk::Menu())),
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    m_menu_chords       (manage(new Gtk::Menu())),
#endif
    m_menu_sequences    (nullptr),
    m_menu_bpm          (manage(new Gtk::Menu())),
    m_menu_bw           (manage(new Gtk::Menu())),
    m_menu_rec_vol      (manage(new Gtk::Menu())),
    m_menu_rec_type     (nullptr),
    m_vadjust           (manage(new Gtk::Adjustment(55, 0, c_num_keys, 1, 1, 1))),
    m_hadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll_new       (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_hscroll_new       (manage(new Gtk::HScrollbar(*m_hadjust))),
    m_seqkeys_wid       (manage(new seqkeys(m_seq, p, *m_vadjust))),
    m_seqtime_wid       (manage(new seqtime(m_seq, p, m_zoom, *m_hadjust))),
    m_seqdata_wid       (manage(new seqdata(m_seq, p, m_zoom, *m_hadjust))),
    m_seqevent_wid
    (
        manage
        (
            (rc().interaction_method() == e_fruity_interaction) ?
                new FruitySeqEventInput
                (
                    p, m_seq, m_zoom, m_snap, *m_seqdata_wid, *m_hadjust
                ) :
                new seqevent
                (
                    p, m_seq, m_zoom, m_snap, *m_seqdata_wid, *m_hadjust
                )
        )
    ),
    m_seqroll_wid
    (
        manage
        (
            (rc().interaction_method() == e_fruity_interaction) ?
                new FruitySeqRollInput
                (
                    p, m_seq, m_zoom, m_snap, *m_seqkeys_wid, pos,
                    *m_hadjust, *m_vadjust
                ) :
                new seqroll
                (
                    p, m_seq, m_zoom, m_snap, *m_seqkeys_wid, pos,
                    *m_hadjust, *m_vadjust
                )
        )
    ),
#ifdef SEQ64_STAZED_LFO_SUPPORT
    m_button_lfo        (manage(new Gtk::Button("LFO"))),
    m_lfo_wnd           (new lfownd(p, m_seq, *m_seqdata_wid)),
#endif
    m_table             (manage(new Gtk::Table(7, 4, false))),
    m_vbox              (manage(new Gtk::VBox(false, 2))),
    m_hbox              (manage(new Gtk::HBox(false, 2))),
    m_hbox2             (manage(new Gtk::HBox(false, 2))),
#if USE_THIRD_SEQEDIT_BUTTON_ROW
    m_hbox3             (manage(new Gtk::HBox(false, 2))),
#endif
    m_button_undo       (nullptr),
    m_button_redo       (nullptr),
    m_button_quantize   (nullptr),
    m_button_tools      (nullptr),
    m_button_sequence   (nullptr),
    m_entry_sequence    (nullptr),
    m_button_bus        (nullptr),
    m_entry_bus         (nullptr),
    m_button_channel    (nullptr),
    m_entry_channel     (nullptr),
    m_button_snap       (nullptr),
    m_entry_snap        (nullptr),
    m_button_note_length(nullptr),
    m_entry_note_length (nullptr),
    m_button_zoom       (nullptr),
    m_entry_zoom        (nullptr),
    m_button_length     (nullptr),
    m_entry_length      (nullptr),
    m_button_key        (nullptr),
    m_entry_key         (nullptr),
    m_button_scale      (nullptr),
    m_entry_scale       (nullptr),
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    m_button_chord      (nullptr),
    m_entry_chord       (nullptr),
#endif
    m_tooltips          (manage(new Gtk::Tooltips())),
    m_button_data       (manage(new Gtk::Button("Event"))),
    m_button_minidata   (manage(new Gtk::Button())),
    m_entry_data        (manage(new Gtk::Entry())),
    m_button_bpm        (nullptr),
    m_entry_bpm         (nullptr),
    m_button_bw         (nullptr),
    m_entry_bw          (nullptr),
    m_button_rec_vol    (manage(new Gtk::Button())),
    m_button_rec_type   (manage(new Gtk::Button())),
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    m_toggle_follow     (manage(new Gtk::ToggleButton())),
#endif
    m_toggle_play       (manage(new Gtk::ToggleButton())),
    m_toggle_record     (manage(new Gtk::ToggleButton())),
    m_toggle_q_rec      (manage(new Gtk::ToggleButton())),
    m_toggle_thru       (manage(new Gtk::ToggleButton())),
#if USE_THIRD_SEQEDIT_BUTTON_ROW
    m_radio_select      (nullptr),
    m_radio_grow        (nullptr),
    m_radio_draw        (nullptr),
#endif
    m_entry_seqnumber   (nullptr),
    m_entry_name        (nullptr),
    m_image_mousemode   (nullptr),
    m_editing_status    (0),
    m_editing_cc        (0),
    m_first_event       (0),
    m_first_event_name  ("(no events)"),
    m_have_focus        (false)
{
    std::string title = SEQ64_APP_NAME " #";        /* main window title    */
    title += m_seq.seq_number();
    title += " \"";
    title += m_seq.name();
    title += "\"";
    set_title(title);
    set_icon(Gdk::Pixbuf::create_from_xpm_data(seq_editor_xpm));
    m_seq.set_editing(true);
    m_ppqn          = choose_ppqn(ppqn);
#ifdef USE_STAZED_ODD_EVEN_SELECTION
    m_pp_whole      = m_ppqn * 4;
    m_pp_eighth     = m_ppqn / 2;
    m_pp_sixteenth  = m_ppqn / 4;
#endif
    create_menus();

    Gtk::HBox * dhbox = manage(new Gtk::HBox(false, 2));
    m_vbox->set_border_width(2);

    /*
     * Fill the (deprecated) Table.  The numbers are the L, R, T, and B
     * attachment locations.  See the banner notes for this module.
     *
     *                              L  R  T  B
     */

    m_table->attach(*m_seqkeys_wid, 0, 1, 1, 2, Gtk::SHRINK, Gtk::FILL);
    m_table->attach(*m_seqtime_wid, 1, 2, 0, 1, Gtk::FILL, Gtk::SHRINK);
    m_table->attach
    (
        *m_seqroll_wid, 1, 2, 1, 2, Gtk::FILL |  Gtk::SHRINK,
        Gtk::FILL |  Gtk::SHRINK
    );
    m_table->attach(*m_seqevent_wid, 1, 2, 2, 3, Gtk::FILL, Gtk::SHRINK);
    m_table->attach(*m_seqdata_wid, 1, 2, 3, 4, Gtk::FILL, Gtk::SHRINK);
    m_table->attach
    (
        *dhbox, 1, 2, 4, 5, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK, 0, 2
    );

    m_image_mousemode = manage
    (
        (rc().interaction_method() == e_fruity_interaction) ?
            new PIXBUF_IMAGE(fruity_xpm) :
            new PIXBUF_IMAGE(tux_xpm)
    );
    m_table->attach
    (
        *m_image_mousemode, 0, 1, 4, 5, Gtk::SHRINK, Gtk::SHRINK, 0, 2
    );

    m_table->attach
    (
        *m_vscroll_new, 2, 3, 1, 2, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND
    );
    m_table->attach
    (
        *m_hscroll_new, 1, 2, 5, 6, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK
    );

    /* no expand, just fit the widgets */

    m_vbox->pack_start(*m_hbox,  false, false, 0);
    m_vbox->pack_start(*m_hbox2, false, false, 0);
#if USE_THIRD_SEQEDIT_BUTTON_ROW
    m_vbox->pack_start(*m_hbox3, false, false, 0);
#endif

    /* expand, cause rollview expands */

    m_vbox->pack_start(*m_table, true, true, 0);
    m_button_data->signal_clicked().connect                 /* data button */
    (
        mem_fun(*this, &seqedit::popup_event_menu)
    );
    m_button_minidata->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::popup_mini_event_menu)
    );
    m_entry_data->set_size_request(40, -1);
    m_entry_data->set_editable(false);

    dhbox->pack_start(*m_button_data, false, false);
    dhbox->pack_start(*m_button_minidata, false, false);
    dhbox->pack_start(*m_entry_data, true, true);

#ifdef SEQ64_STAZED_LFO_SUPPORT
    dhbox->pack_start(*m_button_lfo, false, false);
    m_button_lfo->signal_clicked().connect
    (
        mem_fun(m_lfo_wnd, &lfownd::toggle_visible)
    );
#endif

#ifdef SEQ64_STAZED_TRANSPOSE
    m_toggle_transpose->add(*manage(new PIXBUF_IMAGE(transpose_xpm)));
    m_toggle_transpose->set_focus_on_click(false); // set_can_focus(false);
    m_toggle_transpose->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::transpose_change_callback)
    );
    add_tooltip
    (
        m_toggle_transpose,
        "Sequence is allowed to be transposed if button is highighted/checked."
    );
    m_toggle_transpose->set_active(m_seq.get_transposable());
    if (! usr().work_around_transpose_image())
        set_transpose_image(m_seq.get_transposable());
#endif

    /* play, rec, thru */

    m_toggle_play->add(*manage(new PIXBUF_IMAGE(play_xpm)));
    m_toggle_play->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::play_change_callback)
    );
    add_tooltip
    (
        m_toggle_play,
        "If active, sequence is unmuted, and dumps data to MIDI bus."
    );
    m_toggle_record->add(*manage(new PIXBUF_IMAGE(rec_xpm)));
    m_toggle_record->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::record_change_callback)
    );
    add_tooltip(m_toggle_record, "If active, records incoming MIDI data.");
    m_toggle_q_rec->add(*manage(new PIXBUF_IMAGE(q_rec_xpm)));
    m_toggle_q_rec->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::q_rec_change_callback)
    );
    add_tooltip(m_toggle_q_rec, "If active, quantized record.");

    /*
     * Provides a button to set the recording style to "legacy" (when looping,
     * merge new incoming events into the patter), "overwrite" (replace events
     * with incoming events), and "expand" (increase the size of the loop to
     * accomodate new events).
     */

    m_button_rec_type = manage(new Gtk::Button("Merge"));
    m_button_rec_type->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::popup_record_menu)
    );
    add_tooltip
    (
        m_button_rec_type,
        "Select recording type for patterns: merge events; overwrite events; "
        "or expand the pattern size while recording."
    );

#define SET_POPUP   mem_fun(*this, &seqedit::popup_menu)

    m_button_rec_vol->add(*manage(new Gtk::Label("Vol")));
    m_button_rec_vol->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_rec_vol)
    );
    add_tooltip(m_button_rec_vol, "Select recording/generation volume.");
    m_toggle_thru->add(*manage(new PIXBUF_IMAGE(thru_xpm)));
    m_toggle_thru->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::thru_change_callback)
    );
    add_tooltip
    (
        m_toggle_thru,
        "Incoming MIDI data passes through to sequence's MIDI bus and channel."
    );
    m_toggle_play->set_active(m_seq.get_playing());
    m_toggle_record->set_active(m_seq.get_recording());
    m_toggle_thru->set_active(m_seq.get_thru());
    dhbox->pack_end(*m_button_rec_vol, false, false, 4);
    dhbox->pack_end(*m_button_rec_type, false, false, 4);
    dhbox->pack_end(*m_toggle_q_rec, false, false, 4);
    dhbox->pack_end(*m_toggle_record, false, false, 4);
    dhbox->pack_end(*m_toggle_thru, false, false, 4);
    dhbox->pack_end(*m_toggle_play, false, false, 4);
    dhbox->pack_end(*(manage(new Gtk::VSeparator())), false, false, 4);
    fill_top_bar();
    set_rec_vol(usr().velocity_override());

#ifdef USE_STAZED_EXTRAS
    if (! m_seq.is_default_name())
    {
        m_seqroll_wid->set_can_focus();
        m_seqroll_wid->grab_focus();
    }
#endif

    add(*m_vbox);                              /* add table */

    /*
     * If this is not present, even if we call it after creating in seqmenu,
     * we get a segfault.
     */

    show_all();

    /*
     * OPTION?  Sets vertical scroll bar to the middle:
     *
     * gfloat middle = m_vscroll->get_adjustment()->get_upper() / 3;
     * m_vscroll->get_adjustment()->set_value(middle);
     * m_seqroll_wid->set_ignore_draw(true);        // WE MISSED THIS!
     */

    set_snap(m_initial_snap * m_ppqn / SEQ64_DEFAULT_PPQN);
    set_note_length(m_initial_note_length * m_ppqn / SEQ64_DEFAULT_PPQN);

    int zoom = usr().zoom();
    if (usr().zoom() == SEQ64_USE_ZOOM_POWER_OF_2)      /* i.e. 0 */
        zoom = zoom_power_of_2(m_ppqn);

    set_zoom(zoom);
    set_beats_per_bar(m_seq.get_beats_per_bar());
    set_beat_width(m_seq.get_beat_width());
    m_seq.set_unit_measure();              /* must precede set_measures()  */
    set_measures(get_measures());
    set_midi_channel(m_seq.get_midi_channel());

    /*
     * HERE, if JACK, we get bus == 1, when it should be 0.
     * If ALSA, bus == 1 works.
     */

    set_midi_bus(m_seq.get_midi_bus());
    set_data_type(EVENT_NOTE_ON);
    if (m_seq.musical_scale() != int(c_scale_off))
        set_scale(m_seq.musical_scale());
    else
        set_scale(m_scale);

    if (m_seq.musical_key() != SEQ64_KEY_OF_C)
        set_key(m_seq.musical_key());
    else
        set_key(m_key);

#ifdef SEQ64_STAZED_CHORD_GENERATOR
    set_chord(m_chord);
#endif

    if (SEQ64_IS_VALID_SEQUENCE(m_seq.background_sequence()))
        m_bgsequence = m_seq.background_sequence();

    set_background_sequence(m_bgsequence);
    repopulate_mini_event_menu(m_seq.get_midi_bus(), m_seq.get_midi_channel());

    /*
     * These calls work if called here, in the parent of the seqroll.
     * But we want the focus to be conditional; see the fill_top_bar()
     * function.
     *
     *      m_seqroll_wid->set_can_focus();
     *      m_seqroll_wid->grab_focus();
     *
     * Not needed: m_seqroll_wid->set_ignore_redraw(false);
     */

}

/**
 *  A rote destructor.
 */

seqedit::~seqedit()
{
    // Empty body
}

/**
 *  Creates the various menus by pushing menu elements into the menus.  The
 *  first menu is the Zoom menu, represented in the pattern/sequence editor by
 *  a button with a magnifying glass.  The values are "pixels to ticks", where
 *  "ticks" are actually the "pulses" of "pulses per quarter note".  We would
 *  prefer the notation "n" instead of "1:n", as in "n pulses per pixel".
 *
 *  Note that many of the setups here could be loops through data structures.
 */

void
seqedit::create_menus ()
{
    char b[8];
    for (int z = usr().min_zoom(); z <= usr().max_zoom(); z *= 2)
    {
        snprintf(b, sizeof b, "1:%d", z);
        m_menu_zoom->items().push_back      /* add an entry to zoom menu    */
        (
            MenuElem(b, sigc::bind(mem_fun(*this, &seqedit::set_zoom), z))
        );
    }

    /**
     *  The Snap menu is actually the Grid Snap button, which shows two
     *  arrows pointing to a central bar.  This menu somewhat duplicates the
     *  same menu in perfedit.
     */

#define SET_SNAP    mem_fun(*this, &seqedit::set_snap)
#define SET_NOTE    mem_fun(*this, &seqedit::set_note_length)

    /**
     * To reduce the amount of written code, we now use a static array to
     * initialize some of the seqedit menu entries.  0 denotes the separator.
     * This same setup is used to set up both the snap and note menu, since
     * they are exactly the same.  Saves a *lot* of code.
     */

    static const int s_snap_items [] =
    {
        1, 2, 4, 8, 16, 32, 64, 128, 0, 3, 6, 12, 24, 48, 96, 192
    };
    static const int s_snap_count = sizeof(s_snap_items) / sizeof(int);
    int qnfactor = m_ppqn * 4;
    for (int si = 0; si < s_snap_count; ++si)
    {
        int item = s_snap_items[si];
        char fmt[8];
        if (item > 1)
            snprintf(fmt, sizeof fmt, "1/%d", item);
        else
            snprintf(fmt, sizeof fmt, "%d", item);

        if (item == 0)
        {
            m_menu_snap->items().push_back(SeparatorElem());
            m_menu_note_length->items().push_back(SeparatorElem());
            continue;
        }
        else
        {
            int v = qnfactor / item;
            m_menu_snap->items().push_back                  /* snap length  */
            (
                MenuElem(fmt, sigc::bind(SET_SNAP, v))
            );
            m_menu_note_length->items().push_back           /* note length  */
            (
                MenuElem(fmt, sigc::bind(SET_NOTE, v))
            );
        }
    }

    /**
     *  This menu lets one set the key of the sequence, and is brought up
     *  by the button with the "golden key" image on it.
     */

#define SET_KEY     mem_fun(*this, &seqedit::set_key)

    /*
     * To reduce the amount of written code, we now use a loop.
     */

    for (int key = 0; key < SEQ64_OCTAVE_SIZE; ++key)
    {
        m_menu_key->items().push_back
        (
            MenuElem(c_key_text[key], sigc::bind(SET_KEY, key))
        );
    }

    /**
     *  This button shows a down around for the bottom half of the time
     *  signature.  It's tooltip is "Time signature.  Length of beat."
     *  But it is called bw, or beat width, in the code.
     */

#define SET_BW      mem_fun(*this, &seqedit::set_beat_width)

    /*
     * To reduce the amount of written code, we now use a static array to
     * initialize some of the menu entries.  We use the same list for the snap
     * menu and for the beat-width menu.  This adds a beat-width of 32 to the
     * beat-width menu, just like in the perfedit menu.  A new feature!  :-D
     */

    static const int s_width_items [] = { 1, 2, 4, 8, 16, 32 };
    static const int s_width_count = sizeof(s_width_items) / sizeof(int);
    for (int si = 0; si < s_width_count; ++si)
    {
        int item = s_width_items[si];
        char fmt[8];
        snprintf(fmt, sizeof fmt, "%d", item);
        m_menu_bw->items().push_back(MenuElem(fmt, sigc::bind(SET_BW, item)));
    }

    /**
     *  This menu is shown when pressing the button at the bottom of the
     *  window that has "Vol" as its label.  Let's show the numbers as
     *  well to help the user.  And we'll have to document this change.
     */

#define SET_REC_VOL     mem_fun(*this, &seqedit::set_rec_vol)

    m_menu_rec_vol->items().push_back                   /* record volume    */
    (
        MenuElem("Free", sigc::bind(SET_REC_VOL, SEQ64_PRESERVE_VELOCITY))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed: 127", sigc::bind(SET_REC_VOL, 127))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed: 112", sigc::bind(SET_REC_VOL, 112))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed:  96", sigc::bind(SET_REC_VOL, 96))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed:  80", sigc::bind(SET_REC_VOL, 80))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed:  64", sigc::bind(SET_REC_VOL, 64))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed:  48", sigc::bind(SET_REC_VOL, 48))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed:  32", sigc::bind(SET_REC_VOL, 32))
    );
    m_menu_rec_vol->items().push_back
    (
        MenuElem("Fixed:  16", sigc::bind(SET_REC_VOL, 16))
    );

    /**
     *  This menu sets the scale to show on the panel, and the button
     *  shows a "staircase" image.  See the c_music_scales enumeration
     *  defined in the globals module.
     */

#define SET_SCALE   mem_fun(*this, &seqedit::set_scale)

    for (int i = int(c_scale_off); i < int(c_scale_size); ++i)
    {
        m_menu_scale->items().push_back                 /* music scale      */
        (
            MenuElem(c_scales_text[i], sigc::bind(SET_SCALE, i))
        );
    }

#ifdef SEQ64_STAZED_CHORD_GENERATOR
#define SET_CHORD   mem_fun(*this, &seqedit::set_chord)

    for (int i = 0; i < c_chord_number; ++i)
    {
        m_menu_chords->items().push_back                 /* Chords     */
        (
            MenuElem(c_chord_table_text[i], sigc::bind(SET_CHORD, i))
        );
    }

#endif  // SEQ64_STAZED_CHORD_GENERATOR

    /**
     *  This section sets up two different menus.  The first is m_menu_length.
     *  This menu lets one set the sequence length in bars.  The second menu is
     *  the m_menu_bpm, or BPM, which here means "beats per measure" (not
     *  "beats per minute").
     */

#define SET_BPM         mem_fun(*this, &seqedit::set_beats_per_bar)
#define SET_MEASURES    mem_fun(*this, &seqedit::set_measures)

    for (int i = 0; i < 16; ++i)                        /* seq length menu   */
    {
        int len = i + 1;
        snprintf(b, sizeof b, "%d", len);
        m_menu_length->items().push_back                /* length            */
        (
            MenuElem(b, sigc::bind(SET_MEASURES, len))
        );
        m_menu_bpm->items().push_back                   /* beats per measure */
        (
            MenuElem(b, sigc::bind(SET_BPM, len))
        );
    }
    m_menu_length->items().push_back
    (
        MenuElem("32", sigc::bind(SET_MEASURES, 32))
    );
    m_menu_length->items().push_back
    (
        MenuElem("64", sigc::bind(SET_MEASURES, 64))
    );
}

/**
 *  Sets up the pop-up menus that are brought up by pressing the Tools
 *  button, which shows a hammer image.  This button shows three sub-menus
 *  that need to be filled in by this function.  All the functions
 *  accessed here seem to be implemented by the do_action() function.
 */

void
seqedit::popup_tool_menu ()
{
    if (not_nullptr(m_menu_tools))
    {
        m_menu_tools->popup(0, 0);
        return;
    }

    Gtk::Menu * holder = manage(new Gtk::Menu());
    m_menu_tools = manage(new Gtk::Menu());             // swapped

#define DO_ACTION       mem_fun(*this, &seqedit::do_action)

    holder->items().push_back
    (
        MenuElem("All notes", sigc::bind(DO_ACTION, c_select_all_notes, 0))
    );
    holder->items().push_back
    (
        MenuElem("Inverse notes", sigc::bind(DO_ACTION, c_select_inverse_notes, 0))
    );

#ifdef USE_STAZED_ODD_EVEN_SELECTION

    holder->items().push_back
    (
        MenuElem
        (
            "Even 1/4 Note Beats",
            sigc::bind(DO_ACTION, c_select_even_notes, m_ppqn)
        )
    );
    holder->items().push_back
    (
        MenuElem
        (
            "Odd 1/4 Note Beats",
            sigc::bind(DO_ACTION, c_select_odd_notes, m_ppqn)
        )
    );
    holder->items().push_back
    (
        MenuElem
        (
            "Even 1/8 Note Beats",
            sigc::bind(DO_ACTION, c_select_even_notes, m_pp_eighth)
        )
    );
    holder->items().push_back
    (
        MenuElem
        (
            "Odd 1/8 Note Beats",
            sigc::bind(DO_ACTION, c_select_odd_notes, m_pp_eighth)
        )
    );
    holder->items().push_back
    (
        MenuElem
        (
            "Even 1/16 Note Beats",
            sigc::bind(DO_ACTION, c_select_even_notes, m_pp_sixteenth)
        )
    );
    holder->items().push_back
    (
        MenuElem
        (
            "Odd 1/16 Note Beats",
            sigc::bind(DO_ACTION, c_select_odd_notes, m_pp_sixteenth)
        )
    );

#endif  // USE_STAZED_ODD_EVEN_SELECTION

    /*
     * This is an interesting wrinkle to document.
     */

    if (! event::is_note_msg(m_editing_status))
    {
        holder->items().push_back(SeparatorElem());
        holder->items().push_back
        (
            MenuElem("All events", sigc::bind(DO_ACTION, c_select_all_events, 0))
        );
        holder->items().push_back
        (
            MenuElem("Inverse events",
                sigc::bind(DO_ACTION, c_select_inverse_events, 0))
        );
    }
    m_menu_tools->items().push_back(MenuElem("Select", *holder));

    holder = manage(new Gtk::Menu());           /* another menu */
    holder->items().push_back
    (
        MenuElem("Quantize selected notes",
            sigc::bind(DO_ACTION, c_quantize_notes, 0))
    );
    holder->items().push_back
    (
        MenuElem("Tighten selected notes",
            sigc::bind(DO_ACTION, c_tighten_notes, 0))
    );
    if (! event::is_note_msg(m_editing_status))
    {
        /*
         *  The action code here is c_quantize_events, not c_quantize_notes.
         */

        holder->items().push_back(SeparatorElem());
        holder->items().push_back
        (
            MenuElem("Quantize selected events",
                sigc::bind(DO_ACTION, c_quantize_events, 0))
        );
        holder->items().push_back
        (
            MenuElem("Tighten selected events",
                sigc::bind(DO_ACTION, c_tighten_events, 0))
        );
    }

#ifdef USE_STAZED_COMPANDING

    holder->items().push_back(SeparatorElem());
    holder->items().push_back
    (
        MenuElem("Expand pattern (double)",
            sigc::bind(DO_ACTION, c_expand_pattern, 0))
    );

    holder->items().push_back
    (
        MenuElem("Compress pattern (halve)",
            sigc::bind(DO_ACTION, c_compress_pattern, 0))
    );

#endif

    m_menu_tools->items().push_back(MenuElem("Modify time", *holder));
    holder = manage(new Gtk::Menu());

    char num[16];
    Gtk::Menu * holder2 = manage(new Gtk::Menu());
    for (int i = -SEQ64_OCTAVE_SIZE; i <= SEQ64_OCTAVE_SIZE; ++i)
    {
        if (i != 0)
        {
            snprintf(num, sizeof num, "%+d [%s]", i, c_interval_text[abs(i)]);
            holder2->items().push_front
            (
                MenuElem(num, sigc::bind(DO_ACTION, c_transpose, i))
            );
        }
    }
    holder->items().push_back(MenuElem("Transpose selected", *holder2));
    holder2 = manage(new Gtk::Menu());
    for (int i = -7; i <= 7; ++i)
    {
        if (i != 0)
        {
            snprintf
            (
                num, sizeof num, "%+d [%s]",
                (i < 0) ? i-1 : i+1, c_chord_text[abs(i)]
            );
            holder2->items().push_front
            (
                MenuElem(num, sigc::bind(DO_ACTION, c_transpose_h, i))
            );
        }
    }
    if (m_scale != 0)
    {
        holder->items().push_back
        (
            MenuElem("Harmonic-transpose selected", *holder2)
        );
    }
    m_menu_tools->items().push_back(MenuElem("Modify pitch", *holder));

#ifdef USE_STAZED_RANDOMIZE_SUPPORT

    holder = manage(new Gtk::Menu());
    for (int i = 1; i < 17; ++i)
    {
        snprintf(num, sizeof(num), "+/- %d", i);
        holder->items().push_back
        (
            MenuElem(num, sigc::bind(DO_ACTION, c_randomize_events, i))
        );
    }
    m_menu_tools->items().push_back
    (
        MenuElem("Randomize event values", *holder)
    );

#endif

    m_menu_tools->popup(0, 0);
}

/**
 *  Implements the actions brought forth from the Tools (hammer) button.
 *
 *  Note that the push_undo() calls push all of the current events (in
 *  sequence::m_events) onto the stack (as a single entry).
 */

void
seqedit::do_action (int action, int var)
{
    switch (action)
    {
    case c_select_all_notes:
        m_seq.select_all_notes();
        break;

    case c_select_inverse_notes:
        m_seq.select_all_notes(true);
        break;

    case c_select_all_events:
        m_seq.select_events(m_editing_status, m_editing_cc);
        break;

    case c_select_inverse_events:
        m_seq.select_events(m_editing_status, m_editing_cc, true);
        break;

#ifdef USE_STAZED_ODD_EVEN_SELECTION

    case c_select_even_notes:
        m_seq.select_even_or_odd_notes(var, true);
        break;

    case c_select_odd_notes:
        m_seq.select_even_or_odd_notes(var, false);
        break;

#endif

#ifdef USE_STAZED_RANDOMIZE_SUPPORT

    case c_randomize_events:
        m_seq.randomize_selected(m_editing_status, m_editing_cc, var);
        break;

#endif

    case c_quantize_notes:

        /*
         * sequence::quantize_events() is used in recording as well, so we do
         * not want to incorporate sequence::push_undo() into it.  So we make
         * a new function to do that.
         */

        m_seq.push_quantize(EVENT_NOTE_ON, 0, m_snap, 1, true);
        break;

    case c_quantize_events:
        m_seq.push_quantize(m_editing_status, m_editing_cc, m_snap, 1);
        break;

    case c_tighten_notes:
        m_seq.push_quantize(EVENT_NOTE_ON, 0, m_snap, 2, true);
        break;

    case c_tighten_events:
        m_seq.push_quantize(m_editing_status, m_editing_cc, m_snap, 2);
        break;

    case c_transpose:                           /* regular transpose    */
        m_seq.transpose_notes(var, 0);
        break;

    case c_transpose_h:                         /* harmonic transpose   */
        m_seq.transpose_notes(var, m_scale);
        break;

#ifdef USE_STAZED_COMPANDING

    case c_expand_pattern:
        m_seq.multiply_pattern(2.0);
        break;

    case c_compress_pattern:
        m_seq.multiply_pattern(0.5);
        break;
#endif

    default:
        break;
    }
    m_seqroll_wid->redraw();
    m_seqtime_wid->redraw();
    m_seqdata_wid->redraw();
    m_seqevent_wid->redraw();
}

/**
 *  This function inserts the user-interface items into the top bar or
 *  panel of the pattern editor; this bar has two rows of user interface
 *  elements.
 *
 *  Note that, if a non-default title for the sequence is in force, then
 *  we immediately force the focus to the seqroll "widget", so that the space
 *  bar can be used to control playback, instead of immediately erasing the
 *  name of the sequence.
 */

void
seqedit::fill_top_bar ()
{
    /*
     *  First row of top bar
     */

    m_entry_seqnumber = manage(new Gtk::Entry());       /* sequence number  */
    m_entry_seqnumber->set_width_chars(SEQ64_ENTRY_SIZE_SEQNUMBER);
    m_entry_seqnumber->set_text(m_seq.seq_number());
    m_entry_seqnumber->set_sensitive(false);

    m_entry_name = manage(new Gtk::Entry());            /* sequence name    */
    m_entry_name->set_width_chars(SEQ64_ENTRY_SIZE_SEQNAME);
    m_entry_name->set_text(m_seq.name());
    m_entry_name->signal_changed().connect
    (
        mem_fun(*this, &seqedit::name_change_callback)
    );

    /*
     * If a new sequence (the name is "Untitled"), put the focus on the entry
     * field for the sequence name, and select the whole thing for easy
     * replacement.  Otherwise, unselect the name field and put the focus
     * (unseen) on the seqroll, so that the start/stop characters (Space and
     * Esc by default) can be used immediately to control playback.
     */

    if (! m_seq.is_default_name())
    {
        m_entry_name->set_position(-1);                 /* unselect text    */
        m_seqroll_wid->set_can_focus();
        m_seqroll_wid->grab_focus();
    }
    else
    {
        m_entry_name->set_position(0);
        m_entry_name->set_can_focus();
        m_entry_name->grab_focus();
        m_entry_name->select_region(0, 0);              /* select text      */
    }

    m_hbox->pack_start(*m_entry_seqnumber, true, true);
    m_hbox->pack_start(*m_entry_name, true, true);
    m_hbox->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);

    m_button_bpm = manage(new Gtk::Button());           /* beats per measure */
    m_button_bpm->add(*manage(new PIXBUF_IMAGE(down_xpm)));
    m_button_bpm->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_bpm)
    );
    add_tooltip
    (
        m_button_bpm, "Time signature: beats per measure, beats per bar."
    );
    m_entry_bpm = manage(new Gtk::Entry());
    m_entry_bpm->set_width_chars(2);
    m_entry_bpm->set_editable(true);
    m_entry_bpm->signal_activate().connect
    (
        mem_fun(*this, &seqedit::set_beats_per_bar_manual)  /* issue #77    */
    );
    m_entry_bpm->signal_changed().connect                   /* issue #77    */
    (
        mem_fun(*this, &seqedit::set_beats_per_bar_manual)
    );
    m_hbox->pack_start(*m_button_bpm , false, false);
    m_hbox->pack_start(*m_entry_bpm , false, false);
    m_hbox->pack_start(*(manage(new Gtk::Label("/"))), false, false, 4);
    m_button_bw = manage(new Gtk::Button());                /* beat width   */
    m_button_bw->add(*manage(new PIXBUF_IMAGE(down_xpm)));
    m_button_bw->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_bw)
    );
    add_tooltip(m_button_bw, "Time signature: the length or width of beat.");
    m_entry_bw = manage(new Gtk::Entry());
    m_entry_bw->set_width_chars(2);
    m_entry_bw->set_editable(false);
    m_hbox->pack_start(*m_button_bw , false, false);
    m_hbox->pack_start(*m_entry_bw , false, false);
    m_button_length = manage(new Gtk::Button());            /* pattern length */
    m_button_length->add(*manage(new PIXBUF_IMAGE(length_short_xpm)));
    m_button_length->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_length)
    );
    add_tooltip(m_button_length, "Sequence length in measures or bars.");
    m_entry_length = manage(new Gtk::Entry());
    m_entry_length->set_width_chars(3);
    m_entry_length->set_editable(true);
    m_entry_length->signal_activate().connect
    (
        mem_fun(*this, &seqedit::set_measures_manual)       /* issue #77    */
    );
    m_entry_length->signal_changed().connect
    (
        mem_fun(*this, &seqedit::set_measures_manual)       /* issue #77    */
    );
    m_hbox->pack_start(*m_button_length , false, false);
    m_hbox->pack_start(*m_entry_length , false, false);

#ifdef SEQ64_STAZED_TRANSPOSE
    m_hbox->pack_start(*m_toggle_transpose, false, false, 4);
#endif

    /*
     * We need the space this takes up:
     * m_hbox->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
     */

    m_button_bus = manage(new Gtk::Button());           /* MIDI output bus   */
    m_button_bus->add(*manage(new PIXBUF_IMAGE(bus_xpm)));
    m_button_bus->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::popup_midibus_menu)
    );
    add_tooltip(m_button_bus, "Select MIDI output bus.");
    m_entry_bus = manage(new Gtk::Entry());
    m_entry_bus->set_width_chars(SEQ64_ENTRY_SIZE_BUSNAME);
    m_entry_bus->set_editable(false);
    m_hbox->pack_start(*m_button_bus , false, false);
    m_hbox->pack_start(*m_entry_bus , true, true);
    m_button_channel = manage(new Gtk::Button());       /* MIDI channel      */
    m_button_channel->add(*manage(new PIXBUF_IMAGE(midi_xpm)));
    m_button_channel->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::popup_midich_menu)
    );
    add_tooltip(m_button_channel, "Select MIDI output channel.");
    m_entry_channel = manage(new Gtk::Entry());
    m_entry_channel->set_width_chars(2);
    m_entry_channel->set_editable(false);
    m_hbox->pack_start(*m_button_channel , false, false);
    m_hbox->pack_start(*m_entry_channel , false, false);

    /*
     *  Second row of top bar
     */

    m_button_undo = manage(new Gtk::Button());              /* undo         */
    m_button_undo->set_can_focus(false);                    /* stazed       */
    m_button_undo->add(*manage(new PIXBUF_IMAGE(undo_xpm)));
    m_button_undo->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::undo_callback)
    );
    add_tooltip(m_button_undo, "Undo the last action (Ctrl-Z).");
    m_hbox2->pack_start(*m_button_undo , false, false);
    m_button_redo = manage(new Gtk::Button());              /* redo         */
    m_button_redo->set_can_focus(false);                    /* stazed       */
    m_button_redo->add(*manage(new PIXBUF_IMAGE(redo_xpm)));
    m_button_redo->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::redo_callback)
    );
    add_tooltip(m_button_redo, "Redo the last undone action (Ctrl-R).");
    m_hbox2->pack_start(*m_button_redo , false, false);

    /*
     * Quantize shortcut.  This is the "Q" button, and indicates to
     * quantize (just?) notes.  Compare it to the Quantize menu entry,
     * which quantizes events.
     */

    m_button_quantize = manage(new Gtk::Button());          /* Quantize      */
    m_button_quantize->add(*manage(new PIXBUF_IMAGE(quantize_xpm)));
    m_button_quantize->signal_clicked().connect
    (
        sigc::bind(mem_fun(*this, &seqedit::do_action), c_quantize_notes, 0)
    );
    add_tooltip(m_button_quantize, "Quantize the selection.");
    m_hbox2->pack_start(*m_button_quantize , false, false);

#if ! defined SEQ64_STAZED_CHORD_GENERATOR
    m_hbox2->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
#endif

    m_button_tools = manage(new Gtk::Button());             /* tools button  */
    m_button_tools->add(*manage(new PIXBUF_IMAGE(tools_xpm)));
    m_button_tools->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::popup_tool_menu)
    );
    m_tooltips->set_tip(*m_button_tools, "Tools");
    m_hbox2->pack_start(*m_button_tools , false, false);

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    m_toggle_follow->set_image(*manage(new PIXBUF_IMAGE(follow_xpm)));
    add_tooltip
    (
        m_toggle_follow,
        "If active, the piano roll follows the progress bar while playing."
    );
    m_toggle_follow->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::follow_change_callback)
    );
    m_button_redo->set_can_focus(false);
    m_toggle_follow->set_active(m_seqroll_wid->get_progress_follow());
    m_hbox2->pack_start(*m_toggle_follow, false, false);
#endif

#if ! defined SEQ64_STAZED_CHORD_GENERATOR
    m_hbox2->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
#endif

    m_button_snap = manage(new Gtk::Button());              /* snap          */
    m_button_snap->add(*manage(new PIXBUF_IMAGE(snap_xpm)));
    m_button_snap->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_snap)
    );
    add_tooltip(m_button_snap, "Grid snap.");
    m_entry_snap = manage(new Gtk::Entry());
    m_entry_snap->set_width_chars(5);
    m_entry_snap->set_editable(false);
    m_hbox2->pack_start(*m_button_snap , false, false);
    m_hbox2->pack_start(*m_entry_snap , false, false);
    m_button_note_length = manage(new Gtk::Button());       /* note_length   */
    m_button_note_length->add(*manage(new PIXBUF_IMAGE(note_length_xpm)));
    m_button_note_length->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_note_length)
    );
    add_tooltip(m_button_note_length, "Note length for click-to-insert.");
    m_entry_note_length = manage(new Gtk::Entry());
    m_entry_note_length->set_width_chars(5);
    m_entry_note_length->set_editable(false);
    m_hbox2->pack_start(*m_button_note_length , false, false);
    m_hbox2->pack_start(*m_entry_note_length , false, false);
    m_button_zoom = manage(new Gtk::Button());              /* zoom pixels   */
    m_button_zoom->add(*manage(new PIXBUF_IMAGE(zoom_xpm)));
    m_button_zoom->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_zoom)
    );
    add_tooltip(m_button_zoom, "Zoom, units of pixels:ticks (pixels:pulses).");
    m_entry_zoom = manage(new Gtk::Entry());
    m_entry_zoom->set_width_chars(5);
    m_entry_zoom->set_editable(false);
    m_hbox2->pack_start(*m_button_zoom , false, false);
    m_hbox2->pack_start(*m_entry_zoom , false, false);

#if ! defined SEQ64_STAZED_CHORD_GENERATOR
    m_hbox2->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
#endif

    m_button_key = manage(new Gtk::Button());               /* musical key   */
    m_button_key->add(*manage(new PIXBUF_IMAGE(key_xpm)));
    m_button_key->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_key)
    );
    add_tooltip(m_button_key, "Select the musical key of sequence.");
    m_entry_key = manage(new Gtk::Entry());
    m_entry_key->set_width_chars(2);
    m_entry_key->set_editable(false);
    m_hbox2->pack_start(*m_button_key , false, false);
    m_hbox2->pack_start(*m_entry_key , false, false);
    m_button_scale = manage(new Gtk::Button());             /* musical scale */
    m_button_scale->add(*manage(new PIXBUF_IMAGE(scale_xpm)));
    m_button_scale->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_scale)
    );
    add_tooltip(m_button_scale, "Select the musical scale for sequence.");
    m_entry_scale = manage(new Gtk::Entry());
    m_entry_scale->set_width_chars(10);      // 5
    m_entry_scale->set_editable(false);
    m_hbox2->pack_start(*m_button_scale , false, false);
    m_hbox2->pack_start(*m_entry_scale , true, true);

#if ! defined SEQ64_STAZED_CHORD_GENERATOR
    m_hbox2->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
#endif

    m_button_sequence = manage(new Gtk::Button());      /* background sequence */
    m_button_sequence->add(*manage(new PIXBUF_IMAGE(sequences_xpm)));
    m_button_sequence->signal_clicked().connect
    (
        mem_fun(*this, &seqedit::popup_sequence_menu)
    );
    add_tooltip(m_button_sequence, "Select a background sequence to display.");
    m_entry_sequence = manage(new Gtk::Entry());
    m_entry_sequence->set_width_chars(10);          /* 14 */
    m_entry_sequence->set_editable(false);
    m_hbox2->pack_start(*m_button_sequence, false, false);
    m_hbox2->pack_start(*m_entry_sequence, true, true);

#ifdef SEQ64_STAZED_CHORD_GENERATOR

    m_button_chord = manage(new Gtk::Button());
    m_button_chord->add
    (
        *manage(new Gtk::Image(Gdk::Pixbuf::create_from_xpm_data(chord3_inv_xpm)))
    );
    m_button_chord->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>
        (
            mem_fun(*this, &seqedit::popup_menu), m_menu_chords
        )
    );
    add_tooltip(m_button_chord, "Select a chord type to generate.");
    m_entry_chord = manage(new Gtk::Entry());
    m_entry_chord->set_width_chars(10); // m_entry_chord->set_size_request(10,-1)
    m_entry_chord->set_editable(false);
    m_hbox2->pack_start( *m_button_chord, false, false );
    m_hbox2->pack_start( *m_entry_chord, true, true );

#endif  // SEQ64_STAZED_CHORD_GENERATOR

    /**
     * The following commented radio-buttons were a visual way to select the
     * modes of note editing (select, draw, and grow).  These can easily be
     * done with the left mouse button, keystrokes, or some other tricks,
     * though.
     */

#if USE_THIRD_SEQEDIT_BUTTON_ROW

    m_radio_select = manage(new Gtk::RadioButton("Sel", true)); /* Select */
    m_radio_select->signal_clicked().connect
    (
        sigc::bind(mem_fun(*this, &seqedit::mouse_action), e_action_select)
    );
    m_hbox3->pack_start(*m_radio_select, false, false);

    m_radio_draw = manage(new Gtk::RadioButton("Draw"));        /* Draw */
    m_radio_draw->signal_clicked().connect
    (
        sigc::bind(mem_fun(*this, &seqedit::mouse_action), e_action_draw)
    );
    m_hbox3->pack_start(*m_radio_draw, false, false);

    m_radio_grow = manage(new Gtk::RadioButton("Grow"));        /* Grow */
    m_radio_grow->signal_clicked().connect
    (
        sigc::bind(mem_fun(*this, &seqedit::mouse_action), e_action_grow)
    );
    m_hbox3->pack_start(*m_radio_grow, false, false);

    /* Stretch */

    Gtk::RadioButton::Group g = m_radio_select->get_group();
    m_radio_draw->set_group(g);
    m_radio_grow->set_group(g);

#endif
}

/**
 *  Pops up the given pop-up menu.  At construction, this function is set as
 *  the signal handler for each Gtk::Menu object, and that function is bound
 *  with the Menu object as a parameter.
 */

void
seqedit::popup_menu (Gtk::Menu * menu)
{
    menu->popup(0, 0);
}

/**
 *  Populates the MIDI Output buss pop-up menu.  The MIDI busses are
 *  obtained by getting the mastermidibus object, and iterating through
 *  the busses that it contains.
 *
 *  However, JACK counts the playback ports, such as "yoshimi:midi in",
 *  as "input" ports... the application outputs to the input ports.
 *  So we have to deal with that somehow.
 */

void
seqedit::popup_midibus_menu ()
{
    if (not_nullptr(m_menu_midibus))
    {
        m_menu_midibus->popup(0, 0);
        return;
    }

    mastermidibus & masterbus = perf().master_bus();
    m_menu_midibus = manage(new Gtk::Menu());

#define SET_BUS         mem_fun(*this, &seqedit::set_midi_bus)

    for (int i = 0; i < masterbus.get_num_out_buses(); ++i)
    {
        m_menu_midibus->items().push_back
        (
            MenuElem
            (
                masterbus.get_midi_out_bus_name(i), sigc::bind(SET_BUS, i, true)
            )
        );
    }
    m_menu_midibus->popup(0, 0);
}

/**
 *  Populates the MIDI Channel pop-up menu, if necessary, and then reveals the
 *  menu to the user.
 */

void
seqedit::popup_midich_menu ()
{
    if (not_nullptr(m_menu_midich))
    {
        m_menu_midich->popup(0, 0);
    }
    else
    {
        repopulate_midich_menu(m_seq.get_midi_bus());
        m_menu_midich->popup(0, 0);
    }
}

/**
 *  Populates the MIDI Channel pop-up menu.  This action is needed at startup
 *  of the seqedit window, and when the user changes the active buss for the
 *  sequence.
 *
 *  When the output buss or channel are changed, we get the 16 "channels" from
 *  the new buss's definition, get the corresponding instrument, and load its
 *  name into this midich popup.  Then we need to go to the instrument/channel
 *  that has been selected, and repopulate the event menu with that item's
 *  controller values/names.
 *
 * \param buss
 *      The new value for the buss from which to get the [user-instrument-N]
 *      settings in the [user-instrument-definitions] section.
 */

void
seqedit::repopulate_midich_menu (int buss)
{
    if (not_nullptr(m_menu_midich))
        delete m_menu_midich;

    m_menu_midich = manage(new Gtk::Menu());
    for (int channel = 0; channel < SEQ64_MIDI_BUS_CHANNEL_MAX; ++channel)
    {
        char b[4];                                  /* 2 digits or less  */
        snprintf(b, sizeof b, "%2d", channel + 1);
        std::string name = std::string(b);
        std::string s = usr().instrument_name(buss, channel);
        if (! s.empty())
        {
            name += " [";
            name += s;
            name += "]";
        }

#define SET_CH         mem_fun(*this, &seqedit::set_midi_channel)

        m_menu_midich->items().push_back
        (
            MenuElem(name, sigc::bind(SET_CH, channel, true))
        );
    }
}

/**
 *  Populates the "set background sequence" menu (drops from the button
 *  that has some note-bars on it at the right of the second row of the
 *  top bar).  It is populated with an "Off" menu entry, and a second
 *  "[0]" menu entry that pulls up a drop-down menu of all of the
 *  patterns/sequences that are present in the MIDI file for screen-set 0.  If
 *  more screensets have active sequences, then their screen-set number
 *  appears in the screen-set section of the menu.
 *
 *  Now, at present, we can only save background sequence numbers that are
 *  less than 128, which means the sequences from 0 to 127, or the first four
 *  screen sets.  Higher sequences can be selected, but, right now, they
 *  cannot be saved.  We'll probably fix that at some point, low priority.
 */

void
seqedit::popup_sequence_menu ()
{
    if (not_nullptr(m_menu_sequences))
    {
        m_menu_sequences->popup(0, 0);
        return;
    }

    m_menu_sequences = manage(new Gtk::Menu());

#define SET_BG_SEQ     mem_fun(*this, &seqedit::set_background_sequence)

    m_menu_sequences->items().push_back
    (
        MenuElem("Off", sigc::bind(SET_BG_SEQ, SEQ64_SEQUENCE_LIMIT))
    );
    m_menu_sequences->items().push_back(SeparatorElem());
    int seqsinset = usr().seqs_in_set();
    for (int ss = 0; ss < c_max_sets; ++ss)
    {
        Gtk::Menu * menuss = nullptr;
        bool inserted = false;
        for (int seq = 0; seq < seqsinset; ++seq)
        {
            char name[32];
            int i = ss * seqsinset + seq;
            if (perf().is_active(i))
            {
                if (! inserted)
                {
                    inserted = true;
                    snprintf(name, sizeof name, "[%d]", ss);
                    menuss = manage(new Gtk::Menu());
                    m_menu_sequences->items().push_back(MenuElem(name, *menuss));
                }
                sequence * seq = perf().get_sequence(i);
                snprintf(name, sizeof name, "[%d] %.13s", i, seq->name().c_str());
                menuss->items().push_back
                (
                    MenuElem(name, sigc::bind(SET_BG_SEQ, i))
                );
            }
        }
    }
    m_menu_sequences->popup(0, 0);
}

/**
 *  Draws the given background sequence on the Pattern editor so that the
 *  musician has something to see that can be played against.  As a new
 *  feature, it is also passed to the sequence, so that it can be saved as
 *  part of the sequence data, but only if less or equal to the maximum
 *  single-byte MIDI value, 127.
 *
 *  Note that the "initial value" for this parameter is a static variable that
 *  gets set to the new value, so that opening up another sequence causes the
 *  sequence to take on the new "initial value" as well.  A feature, but
 *  should it be optional?  Now it is, based on the setting of
 *  usr().global_seq_feature().
 */

void
seqedit::set_background_sequence (int seqnum)
{
    m_bgsequence = seqnum;              /* we should check this value!  */
    if (usr().global_seq_feature())
        usr().seqedit_bgsequence(seqnum);

    if (SEQ64_IS_DISABLED_SEQUENCE(seqnum) || ! perf().is_active(seqnum))
    {
        m_entry_sequence->set_text("Off");
        m_seqroll_wid->set_background_sequence(false, SEQ64_SEQUENCE_LIMIT);
    }
    if (perf().is_active(seqnum))
    {
        char name[24];
        sequence * seq = perf().get_sequence(seqnum);
        snprintf(name, sizeof name, "[%d] %.13s", seqnum, seq->name().c_str());
        m_entry_sequence->set_text(name);
        m_seqroll_wid->set_background_sequence(true, seqnum);
        if (seqnum < usr().max_sequence())      /* even more restrictive */
            m_seq.background_sequence(long(seqnum));
    }
}

/**
 *  Sets the menu pixmap depending on the given state, where true is a
 *  full menu (black background), and empty menu (gray background).
 */

Gtk::Image *
seqedit::create_menu_image (bool state)
{
    return manage(new PIXBUF_IMAGE(state ? menu_full_xpm : menu_empty_xpm));
}

/**
 *  Populates the event-selection menu, in necessary, and then pops it up.
 *  Also see the handling of the m_button_data and m_entry_data objects.
 */

void
seqedit::popup_event_menu ()
{
    if (not_nullptr(m_menu_data))
        delete m_menu_data;

    int buss = m_seq.get_midi_bus();
    int channel = m_seq.get_midi_channel();
    repopulate_event_menu(buss, channel);
    m_menu_data->popup(0, 0);
}

/**
 *  Local define used for setting the m_entry_data textbox.
 */

#define SET_DATA_TYPE(x)    mem_fun(*this, &seqedit::set_data_type), x, 0

/**
 *  Function to create event menu entries.  Too damn big!
 */

void
seqedit::set_event_entry
(
    Gtk::Menu * menu,
    const std::string & text,
    bool present,
    midibyte status,
    midibyte control    // = 0
)
{
    menu->items().push_back
    (
        ImageMenuElem
        (
            text, *create_menu_image(present),
            sigc::bind(mem_fun(*this, &seqedit::set_data_type), status, control)
        )
    );
    if (present && m_first_event == 0x00)
    {
        m_first_event = status;
        m_first_event_name = text;
        set_data_type(status, 0);       // need m_first_control value!
    }
}

/**
 *  Populates the event-selection menu that drops from the "Event" button
 *  in the bottom row of the Pattern editor.
 *
 *  This menu has a large number of items.  They are filled in by
 *  code, but can also be loaded from sequencer64.usr.
 *
 *  This function first loops through all of the existing events in the
 *  sequence in order to determine what events exist in it.  If any of the
 *  following events are found, their entry in the menu is marked by a filled
 *  square, rather than a hollow square:
 *
 *      -   Note On
 *      -   Note off
 *      -   Aftertouch
 *      -   Program Change
 *      -   Channel Pressure
 *      -   Pitch Wheel
 *      -   Control Changes from 0 to 127
 *
 * \param buss
 *      The selected bus number.
 *
 * \param channel
 *      The selected channel number.
 */

void
seqedit::repopulate_event_menu (int buss, int channel)
{
    bool ccs[SEQ64_MIDI_COUNT_MAX];
    bool note_on = false;
    bool note_off = false;
    bool aftertouch = false;
    bool program_change = false;
    bool channel_pressure = false;
    bool pitch_wheel = false;
    midibyte status, cc;
    memset(ccs, false, sizeof(bool) * SEQ64_MIDI_COUNT_MAX);
    m_seq.reset_draw_marker();
    while (m_seq.get_next_event(status, cc))            /* used only here!  */
    {
        switch (status)
        {
        case EVENT_NOTE_OFF:
            note_off = true;
            break;

        case EVENT_NOTE_ON:
            note_on = true;
            break;

        case EVENT_AFTERTOUCH:
            aftertouch = true;
            break;

        case EVENT_CONTROL_CHANGE:
            ccs[cc] = true;
            break;

        case EVENT_PITCH_WHEEL:
            pitch_wheel = true;
            break;

        case EVENT_PROGRAM_CHANGE:
            program_change = true;
            break;

        case EVENT_CHANNEL_PRESSURE:
            channel_pressure = true;
            break;
        }
    }

    m_menu_data = manage(new Gtk::Menu());
    set_event_entry(m_menu_data, "Note On Velocity", note_on, EVENT_NOTE_ON);
    m_menu_data->items().push_back(SeparatorElem());
    set_event_entry(m_menu_data, "Note Off Velocity", note_off, EVENT_NOTE_OFF);
    set_event_entry(m_menu_data, "Aftertouch", aftertouch, EVENT_AFTERTOUCH);
    set_event_entry
    (
        m_menu_data, "Program Change", program_change, EVENT_PROGRAM_CHANGE
    );
    set_event_entry
    (
        m_menu_data, "Channel Pressure", channel_pressure, EVENT_CHANNEL_PRESSURE
    );
    set_event_entry(m_menu_data, "Pitch Wheel", pitch_wheel, EVENT_PITCH_WHEEL);
    m_menu_data->items().push_back(SeparatorElem());

    /**
     *  Create the 8 sub-menus for the various ranges of controller
     *  changes, shown 16 per sub-menu.
     */

    const int menucount = 8;
    const int itemcount = 16;
    char b[32];
    for (int submenu = 0; submenu < menucount; ++submenu)
    {
        int offset = submenu * itemcount;
        snprintf(b, sizeof b, "Controls %d-%d", offset, offset + itemcount - 1);
        Gtk::Menu * menucc = manage(new Gtk::Menu());
        for (int item = 0; item < itemcount; ++item)
        {
            /*
             * Do we really want the default controller name to start?
             * That's what the legacy Seq24 code does!  We need to document
             * it in the seq24-doc and sequencer64-doc projects.  Also, there
             * was a bug in Seq24 where the instrument number was use re 1
             * to get the proper instrument... it needs to be decremented to
             * be re 0.
             */

            std::string controller_name(c_controller_names[offset + item]);
            const user_midi_bus & umb = usr().bus(buss);
            int inst = umb.instrument(channel);
            const user_instrument & uin = usr().instrument(inst);
            if (uin.is_valid())                             // redundant check
            {
                if (uin.controller_active(offset + item))
                    controller_name = uin.controller_name(offset + item);
            }
            set_event_entry
            (
                menucc, controller_name, ccs[offset+item],
                EVENT_CONTROL_CHANGE, offset + item
            );
        }
        m_menu_data->items().push_back(MenuElem(std::string(b), *menucc));
    }
}

/**
 *  Populates the event-selection menu, in necessary, and then pops it up.
 *  Also see the handling of the m_button_minidata and m_entry_data objects.
 */

void
seqedit::popup_mini_event_menu ()
{
    if (not_nullptr(m_menu_minidata))
        delete m_menu_minidata;

    int buss = m_seq.get_midi_bus();
    int channel = m_seq.get_midi_channel();
    repopulate_mini_event_menu(buss, channel);
    m_menu_minidata->popup(0, 0);
}

/**
 *  Populates the mini event-selection menu that drops from the mini-"Event"
 *  button in the bottom row of the Pattern editor.
 *  This menu has a much smaller number of items, only the ones that actually
 *  exist in the track/pattern/loop/sequence.
 *
 * \param buss
 *      The selected bus number.
 *
 * \param channel
 *      The selected channel number.
 */

void
seqedit::repopulate_mini_event_menu (int buss, int channel)
{
    bool ccs[SEQ64_MIDI_COUNT_MAX];
    bool note_on = false;
    bool note_off = false;
    bool aftertouch = false;
    bool program_change = false;
    bool channel_pressure = false;
    bool pitch_wheel = false;
    midibyte status, cc;
    memset(ccs, false, sizeof(bool) * SEQ64_MIDI_COUNT_MAX);
    m_seq.reset_draw_marker();
    while (m_seq.get_next_event(status, cc))            /* used only here!  */
    {
        switch (status)
        {
        case EVENT_NOTE_OFF:
            note_off = true;
            break;

        case EVENT_NOTE_ON:
            note_on = true;
            break;

        case EVENT_AFTERTOUCH:
            aftertouch = true;
            break;

        case EVENT_CONTROL_CHANGE:
            ccs[cc] = true;
            break;

        case EVENT_PITCH_WHEEL:
            pitch_wheel = true;
            break;

        case EVENT_PROGRAM_CHANGE:
            program_change = true;
            break;

        case EVENT_CHANNEL_PRESSURE:
            channel_pressure = true;
            break;
        }
    }
    m_menu_minidata = manage(new Gtk::Menu());
    bool any_events = false;
    if (note_on)
    {
        any_events = true;
        set_event_entry(m_menu_minidata, "Note On Velocity", true, EVENT_NOTE_ON);
    }
    if (note_off)
    {
        any_events = true;
        set_event_entry
        (
            m_menu_minidata, "Note Off Velocity", true, EVENT_NOTE_OFF
        );
    }
    if (aftertouch)
    {
        any_events = true;
        set_event_entry(m_menu_minidata, "Aftertouch", true, EVENT_AFTERTOUCH);
    }
    if (program_change)
    {
        any_events = true;
        set_event_entry
        (
            m_menu_minidata, "Program Change", true, EVENT_PROGRAM_CHANGE
        );
    }
    if (channel_pressure)
    {
        any_events = true;
        set_event_entry
        (
            m_menu_minidata, "Channel Pressure", true, EVENT_CHANNEL_PRESSURE
        );
    }
    if (pitch_wheel)
    {
        any_events = true;
        set_event_entry
        (
            m_menu_minidata, "Pitch Wheel", true, EVENT_PITCH_WHEEL
        );
    }

    m_menu_minidata->items().push_back(SeparatorElem());

    /**
     *  Create the one menu for the controller changes that actually exist in
     *  the track, if any.
     */

    const int itemcount = SEQ64_MIDI_COUNT_MAX;             /* 128 */
    for (int item = 0; item < itemcount; ++item)
    {
        std::string controller_name(c_controller_names[item]);
        const user_midi_bus & umb = usr().bus(buss);
        int inst = umb.instrument(channel);
        const user_instrument & uin = usr().instrument(inst);
        if (uin.is_valid())                             // redundant check
        {
            if (uin.controller_active(item))
                controller_name = uin.controller_name(item);
        }
        if (ccs[item])
        {
            any_events = true;
            set_event_entry
            (
                m_menu_minidata, controller_name, true,
                EVENT_CONTROL_CHANGE, item
            );
        }
    }
    if (any_events)
    {
        // Here, we would like to pre-select the first kind of event found,
        // somehow.
    }
    else
        set_event_entry(m_menu_minidata, "(no events)", false, 0);

    Gtk::Image * eventflag = manage(create_menu_image(any_events));
    if (not_nullptr(eventflag))
        m_button_minidata->set_image(*eventflag);
}

void
seqedit::popup_record_menu()
{
    bool legacy = !
    (
        m_seq.get_overwrite_rec() ||
        m_seqroll_wid->get_expanded_record()
    );
    m_menu_rec_type = manage(new Gtk::Menu());
    m_menu_rec_type->items().push_back
    (
        ImageMenuElem
        (
            "Merge notes in loop recording",
            *create_menu_image(legacy),
            sigc::bind
            (
                mem_fun(*this, &seqedit::set_rec_type), LOOP_RECORD_LEGACY
            )
        )
    );

    m_menu_rec_type->items().push_back
    (
        ImageMenuElem
        (
            "Replace notes in loop recording",
            *create_menu_image(m_seq.get_overwrite_rec()),
            sigc::bind
            (
                mem_fun(*this, &seqedit::set_rec_type), LOOP_RECORD_OVERWRITE
            )
        )
    );

    m_menu_rec_type->items().push_back
    (
        ImageMenuElem
        (
            "Expand length in loop recording",
            *create_menu_image(m_seqroll_wid->get_expanded_record()),
            sigc::bind
            (
                mem_fun(*this, &seqedit::set_rec_type), LOOP_RECORD_EXPAND
            )
        )
    );
    m_menu_rec_type->popup(0, 0);
}

/**
 *  Selects the given MIDI channel parameter in the main sequence object,
 *  so that it will use that channel.
 *
 *  Should this change set the is-modified flag?  Where should validation
 *  occur?
 *
 * \param midichannel
 *      The MIDI channel  value to set.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.
 */

void
seqedit::set_midi_channel (int midichannel, bool user_change)
{
    char b[8];
    snprintf(b, sizeof b, "%d", midichannel + 1);
    m_entry_channel->set_text(b);
    m_seq.set_midi_channel(midichannel, user_change); /* user-modified value? */
}

/**
 *  Selects the given MIDI buss parameter in the main sequence object,
 *  so that it will use that buss.
 *
 *  Should this change set the is-modified flag?  Where should validation
 *  against the ALSA or JACK buss limits occur?
 *
 *  Also, it would be nice to be able to update this display of the MIDI bus
 *  in the field if we set it from the seqmenu.
 *
 * \param bus
 *      The buss value to set.  If this value changes the selected buss, then
 *      the MIDI channel popup menu is repopulated.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.
 */

void
seqedit::set_midi_bus (int bus, bool user_change)
{
    int initialbus = m_seq.get_midi_bus();
    m_seq.set_midi_bus(bus, user_change);           /* user-modified value? */
    mastermidibus & mmb = perf().master_bus();
    m_entry_bus->set_text(mmb.get_midi_out_bus_name(bus));
    if (bus != initialbus)
    {
        int channel = m_seq.get_midi_channel();
        repopulate_midich_menu(bus);
        repopulate_event_menu(bus, channel);
    }
}

/**
 *  Selects the given zoom value.  It is passed to the seqroll, seqtime,
 *  seqdata, and seqevent objects, as well.  This function doesn't check if
 *  the zoom will change, because this function might be used to initialize
 *  the zoom of the children.
 *
 *  The notation for zoom in the user-interface is in pixels:ticks, but I
 *  would prefer to use pulses/pixel (pulses per pixel).  Oh well.  Note that
 *  this value of zoom is saved to the "user" configuration file when
 *  Sequencer64 exit.
 *
 * \param z
 *      The prospective zoom value to set.  It is applied only if between the
 *      minimum and maximum allowed zoom values, inclusive.  See the
 *      usr().min_zoom() and usr().max_zoom() function.
 */

void
seqedit::set_zoom (int z)
{
    if ((z >= usr().min_zoom()) && (z <= usr().max_zoom()))
    {
        char b[16];
        snprintf(b, sizeof b, "1:%d", z);
        m_entry_zoom->set_text(b);
        m_zoom = z;
        m_seqroll_wid->set_zoom(z);
        m_seqtime_wid->set_zoom(z);
        m_seqdata_wid->set_zoom(z);
        m_seqevent_wid->set_zoom(z);
    }
}

/**
 *  Selects the given snap value, which is the number of ticks in a snap-sized
 *  interval.  It is passed to the seqroll, seqevent, and sequence objects, as
 *  well.
 *
 *  The default initial snap is the default PPQN divided by 4, or the
 *  equivalent of a 16th note (48 ticks).  The snap divisor is 192 * 4 / 48 or
 *  16.
 *
 * \param s
 *      The prospective snap value to set.  It is checked only to make sure it
 *      is greater than 0, to avoid a numeric exception.
 */

void
seqedit::set_snap (int s)
{
    if (s > 0)
    {
        char b[16];
        snprintf(b, sizeof b, "1/%d", m_ppqn * 4 / s);
        m_entry_snap->set_text(b);
        m_snap = s;
        m_initial_snap = s;
        m_seqroll_wid->set_snap(s);
        m_seqevent_wid->set_snap(s);
        m_seq.set_snap_tick(s);
    }
}

/**
 *  Selects the given note-length value.  It is passed to the seqroll
 *  object, as well.
 *
 * \warning
 *      Currently, we don't handle changes in the global PPQN after the
 *      creation of the menu.  The creation of the menu hard-wires the values
 *      of note-length.  To adjust for a new global PQN, we will need to store
 *      the original PPQN (m_original_ppqn = m_ppqn), and then adjust the
 *      notelength based on the new PPQN.  For example if the new PPQN is
 *      twice as high as 192, then the notelength should double, though the
 *      text displayed in the "Note length" field should remain the same.
 *      However, we do adjust for a non-default PPQN at startup time.
 *
 * \param notelength
 *      Provides the note length in units of MIDI pulses.
 */

void
seqedit::set_note_length (int notelength)
{
    char b[8];
    snprintf(b, sizeof b, "1/%d", m_ppqn * 4 / notelength);
    m_entry_note_length->set_text(b);

#ifdef CAN_MODIFY_GLOBAL_PPQN
    if (m_ppqn != m_original_ppqn)
    {
        double factor = double(m_ppqn) / double(m_original);
        notelength = int(notelength * factor + 0.5);
    }
#endif

    m_note_length = notelength;
    m_initial_note_length = notelength;
    m_seqroll_wid->set_note_length(notelength);
}

/**
 *  Selects the given scale value.  It is passed to the seqroll and
 *  seqkeys objects, as well.  As a new feature, it is also passed to the
 *  sequence, so that it can be saved as part of the sequence data.
 *
 *  Note that the "initial value" for this parameter is a static variable that
 *  gets set to the new value, so that opening up another sequence causes the
 *  sequence to take on the new "initial value" as well.  A feature, but
 *  should it be optional?  Now it is, based on the setting of
 *  usr().global_seq_feature().
 */

void
seqedit::set_scale (int scale)
{
    m_entry_scale->set_text(c_scales_text[scale]);
    m_seqroll_wid->set_scale(scale);
    m_seqkeys_wid->set_scale(scale);
    m_seq.musical_scale(scale);
    m_scale = scale;
    if (usr().global_seq_feature())
        usr().seqedit_scale(scale);
}

/**
 *  Selects the given key (signature) value.  It is passed to the seqroll
 *  and seqkeys objects, as well.  As a new feature, it is also passed to the
 *  sequence, so that it can be saved as part of the sequence data.
 *
 *  Note that the "initial value" for this parameter is a static variable that
 *  gets set to the new value, so that opening up another sequence causes the
 *  sequence to take on the new "initial value" as well.  A feature, but
 *  should it be optional?  Now it is, based on the setting of
 *  usr().global_seq_feature().
 */

void
seqedit::set_key (int key)
{
    m_entry_key->set_text(c_key_text[key]);
    m_seqroll_wid->set_key(key);
    m_seqkeys_wid->set_key(key);
    m_seq.musical_key(key);
    m_key = key;
    if (usr().global_seq_feature())
        usr().seqedit_key(key);
}

#ifdef SEQ64_STAZED_CHORD_GENERATOR

void
seqedit::set_chord (int chord)
{
    if (chord >= 0 && chord < c_chord_number)
    {
        m_entry_chord->set_text(c_chord_table_text[chord]);
        m_chord = m_initial_chord = chord;
        m_seqroll_wid->set_chord(m_chord);
    }
}

#endif  // SEQ64_STAZED_CHORD_GENERATOR

/**
 *  Sets the sequence length based on the three given parameters.  There's an
 *  implicit "adjust-triggers = true" parameter used in
 *  sequence::set_length().  Then the seqroll, seqtime, seqdata, and seqevent
 *  objects are reset(), to cause redraw operations.
 *
 * \param bpb
 *      Provides the beats per bar.
 *
 * \param bw
 *      Provides the beatwidth (typically 4) from the time signature.
 *
 * \param measures
 *      Provides the number of measures the sequence should cover, obtained
 *      from the user-interface.
 */

void
seqedit::apply_length (int bpb, int bw, int measures)
{
    m_seq.apply_length(bpb, m_ppqn, bw, measures);
    m_seqroll_wid->reset();
    m_seqtime_wid->reset();
    m_seqdata_wid->reset();
    m_seqevent_wid->reset();
}

/**
 *  Calculates the measures value based on the bpm (beats per measure),
 *  ppqn (parts per quarter note), and bw (beat width) values, and returns
 *  the resultant measures value.
 *
 * \todo
 *      Create a sequence::set_units() function or a
 *      sequence::get_measures() function to forward to.
 */

int
seqedit::get_measures ()
{
    return m_seq.calculate_measures();
}

/**
 *  Set the measures value, using the given parameter, and some internal
 *  values passed to apply_length().
 *
 * \todo
 *      Check if verification is needed at this point.
 *
 * \param len
 *      Provides the sequence length, in measures.
 */

void
seqedit::set_measures (int len)
{
    char b[8];
    snprintf(b, sizeof b, "%d", len);
    m_entry_length->set_text(b);
    m_measures = len;

    /*
     * In lieu of:
     *
     * apply_length(m_seq.get_beats_per_bar(), m_seq.get_beat_width(), len);
     */

    m_seq.apply_length(len);
    m_seqroll_wid->reset();                 /* see seqedit::apply_length()  */
    m_seqtime_wid->reset();
    m_seqdata_wid->reset();
    m_seqevent_wid->reset();
}

/**
 *  Set the measures value manually.  For issue #77, pulled from the
 *  jean-emmanual-manul-bpm-and-measure branch.  The setting is limited to 0
 *  to 1024.
 */

void
seqedit::set_measures_manual ()
{
    int len = atoi(m_entry_length->get_text().c_str());
    if (len > 0 && len <= 1024)
        set_measures(len);
}

/**
 *  Set the bpm (beats per measure) value, using the given parameter, and
 *  some internal values passed to apply_length().
 *
 * \todo
 *      Check if verification is needed at this point.
 *
 * \param bpb
 *      Provides the BPM (beats per measure) value to set.  Not beats/minute!
 */

void
seqedit::set_beats_per_bar (int bpb)
{
    char b[8];
    snprintf(b, sizeof b, "%d", bpb);
    m_entry_bpm->set_text(b);
    if (bpb != m_seq.get_beats_per_bar())
    {
        long len = get_measures();
        m_seq.set_beats_per_bar(bpb);

        /*
         * In lieu of:
         *
         * apply_length(bpb, m_seq.get_beat_width(), len);
         */

        m_seq.apply_length(len);
    }
}

/**
 *  Set the bpm (beats per measure) value manually.  For issue #77, pulled
 *  from the jean-emmanual-manul-bpm-and-measure branch.  The setting is
 *  limited to 0 to 128.
 */

void
seqedit::set_beats_per_bar_manual ()
{
    int bpm = atoi(m_entry_bpm->get_text().c_str());
    if (bpm > 0 && bpm <= 128)
        set_beats_per_bar(bpm);
 }

/**
 *  Set the bw (beat width) value, using the given parameter, and
 *  some internal values passed to apply_length().
 *
 * \todo
 *      Check if verification is needed at this point.
 *
 * \param bw
 *      Provides the beat-width value to set.
 */

void
seqedit::set_beat_width (int bw)
{
    char b[8];
    snprintf(b, sizeof b, "%d", bw);
    m_entry_bw->set_text(b);
    if (bw != m_seq.get_beat_width())
    {
        long len = get_measures();
        m_seq.set_beat_width(bw);
        apply_length(m_seq.get_beats_per_bar(), bw, len);
    }
}

/**
 *  Set the name for the main sequence to this object's entry name.
 *  That name is the name the user has given to the sequence being edited.
 */

void
seqedit::name_change_callback ()
{
    m_seq.set_name(m_entry_name->get_text());
}

#ifdef SEQ64_STAZED_TRANSPOSE

/**
 *  Passes the transpose status to the sequence object.
 */

void
seqedit::transpose_change_callback ()
{
    bool istransposable = m_toggle_transpose->get_active();
    m_seq.set_transposable(istransposable);
    if (! usr().work_around_transpose_image())
        set_transpose_image(istransposable);
}

/**
 *  Changes the image used for the transpose button.
 *
 *  Can we leverage this variation?
 *
 *      m_toggle_transpose->add_pixlabel("info.xpm", "duh");
 *
 * \param istransposable
 *      If true, set the image to the "Transpose" icon.  Otherwise, set it to
 *      the "Drum" (not transposable) icon.
 */

void
seqedit::set_transpose_image (bool istransposable)
{
    if (istransposable)
    {
        add_tooltip(m_toggle_transpose, "Sequence is transposable.");
        m_image_transpose = manage(new(std::nothrow) PIXBUF_IMAGE(transpose_xpm));
    }
    else
    {
        add_tooltip(m_toggle_transpose, "Sequence is not transposable.");
        m_image_transpose = manage(new(std::nothrow) PIXBUF_IMAGE(drum_xpm));
    }
    if (not_nullptr(m_image_transpose))
        m_toggle_transpose->set_image(*m_image_transpose);
}

#endif

/**
 *  Changes the image used for the mouse-mode indicator.
 *
 * \param isfruity
 *      If true, set the image to the "Fruity" icon.  Otherwise, set it to the
 *      "Tux" icon.
 */

void
seqedit::set_mousemode_image (bool isfruity)
{
    /*
     * Not necessary:
     *
     *      m_table->remove(*m_image_mousemode);
     *      delete m_image_mousemode;
     */

    if (isfruity)
        m_image_mousemode = manage(new PIXBUF_IMAGE(fruity_xpm));
    else
        m_image_mousemode = manage(new PIXBUF_IMAGE(tux_xpm));

    m_table->attach
    (
        *m_image_mousemode, 0, 1, 4, 5, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK, 0, 2
    );
}

/**
 *  Passes the play status to the sequence object.
 */

void
seqedit::play_change_callback ()
{
    m_seq.set_playing(m_toggle_play->get_active());
}

/**
 *  Passes the recording status to the sequence object.
 *
 * Stazed:
 *
 *      Both record_change_callback() and thru_change_callback() will call
 *      set_sequence_input() for the same sequence. We only need to call it if
 *      it is not already set, if setting. And, we should not unset it if the
 *      m_toggle_thru->get_active() is true.
 */

void
seqedit::record_change_callback ()
{
    bool thru_active = m_toggle_thru->get_active();
    bool record_active = m_toggle_record->get_active();
    perf().set_recording(record_active, thru_active, &m_seq);
}

/**
 *  Passes the quantized-recording status to the sequence object.
 *
 * Stazed fix:
 *
 *      If we set Quantized recording, then also set recording, but do not
 *      unset recording if we unset Quantized recording.
 *
 *  This is not necessarily the most intuitive thing to do.  See
 *  midi_record.txt.
 */

void
seqedit::q_rec_change_callback ()
{
    // m_seq.set_quantized_recording(m_toggle_q_rec->get_active());

    perf().set_quantized_recording(m_toggle_q_rec->get_active(), &m_seq);
    if (m_toggle_q_rec->get_active() && ! m_toggle_record->get_active())
        m_toggle_record->activate();
}

/**
 *  Pops an undo operation from the sequence object, and then tells the
 *  segroll, seqtime, seqdata, and seqevent objects to redraw.
 */

void
seqedit::undo_callback ()
{
    m_seq.pop_undo();
    m_seqroll_wid->redraw();
    m_seqtime_wid->redraw();
    m_seqdata_wid->redraw();
    m_seqevent_wid->redraw();
}

/**
 *  Pops a redo operation from the sequence object, and then tell the
 *  segroll, seqtime, seqdata, and seqevent objects to redraw.
 */

void
seqedit::redo_callback ()
{
    m_seq.pop_redo();
    m_seqroll_wid->redraw();
    m_seqtime_wid->redraw();
    m_seqdata_wid->redraw();
    m_seqevent_wid->redraw();
}

/**
 *  Passes the MIDI Thru status to the sequence object.
 *
 * Stazed:
 *
 *      Both record_change_callback() and thru_change_callback() will call
 *      set_sequence_input() for the same sequence. We only need to call it if
 *      it is not already set, if setting. And, we should not unset it if the
 *      m_toggle_thru->get_active() is true.
 */

void
seqedit::thru_change_callback ()
{
    bool thru_active = m_toggle_thru->get_active();
    bool record_active = m_toggle_record->get_active();
    perf().set_thru(record_active, thru_active, &m_seq);
}

#ifdef SEQ64_FOLLOW_PROGRESS_BAR

/**
 *  Passes the Follow status to the seqroll object.
 */

void
seqedit::follow_change_callback ()
{
    m_seqroll_wid->set_progress_follow(m_toggle_follow->get_active());
}

#endif  // SEQ64_FOLLOW_PROGRESS_BAR

/**
 *  Passes the given parameter to sequence::set_rec_vol().  This function also
 *  changes the button's text to match the selection, and also changes the
 *  global velocity-override setting in user_settings.  Note that the setting
 *  will not be saved to the "usr" configuration file unless Sequencer64 was
 *  run with the "--user-save" option.
 *
 * \param recvol
 *      The setting to be made, obtained from the recording-volume ("Vol")
 *      menu.
 */

void
seqedit::set_rec_vol (int recvol)
{
    char selection[16];
    if (recvol == SEQ64_PRESERVE_VELOCITY)
        snprintf(selection, sizeof selection, "Free");
    else
        snprintf(selection, sizeof selection, "%d", recvol);

    Gtk::Label * lbl(dynamic_cast<Gtk::Label *>(m_button_rec_vol->get_child()));
    if (not_nullptr(lbl))
        lbl->set_text(selection);

    m_seq.set_rec_vol(recvol);          /* save to the sequence settings    */
    usr().velocity_override(recvol);    /* save to the "usr" config file    */
}

/**
 *
 */

void
seqedit::set_rec_type (loop_record_t rectype)
{
    std::string label = "Merge";
    switch (rectype)
    {
    case LOOP_RECORD_LEGACY:

        m_seq.set_overwrite_rec(false);
        m_seqroll_wid->set_expanded_recording(false);
        break;

    case LOOP_RECORD_OVERWRITE:

        m_seq.set_overwrite_rec(true);
        m_seqroll_wid->set_expanded_recording(false);
        label = "Replace";
        break;

    case LOOP_RECORD_EXPAND:

        m_seq.set_overwrite_rec(false);
        m_seqroll_wid->set_expanded_recording(true);
        label = "Expand";
        break;

    default:

        /*
         * We could also offer a true-true setting to overwrite and expand.
         * :-)
         */

        break;
    }

    Gtk::Label * ptr(dynamic_cast<Gtk::Label *>(m_button_rec_type->get_child()));
    if (not_nullptr(ptr))
    {
        char temp[8];
        snprintf(temp, sizeof(temp), "%s", label.c_str());
        ptr->set_text(temp);
    }
}

/**
 *  Sets the data type based on the given parameters.  This function uses the
 *  hardwired array c_controller_names.
 *
 * \param status
 *      The current editing status.
 *
 * \param control
 *      The control value.  However, we really need to validate it!
 */

void
seqedit::set_data_type (midibyte status, midibyte control)
{
    m_editing_status = status;
    m_editing_cc = control;
    m_seqevent_wid->set_data_type(status, control);
    m_seqdata_wid->set_data_type(status, control);
    m_seqroll_wid->set_data_type(status, control);

    char hex[8];
    char type[80];
    snprintf(hex, sizeof hex, "[0x%02X]", status);
    if (status == EVENT_NOTE_OFF)
        snprintf(type, sizeof type, "Note Off");
    else if (status == EVENT_NOTE_ON)
        snprintf(type, sizeof type, "Note On");
    else if (status == EVENT_AFTERTOUCH)
        snprintf(type, sizeof type, "Aftertouch");
    else if (status == EVENT_CONTROL_CHANGE)
    {
        int bus = m_seq.get_midi_bus();
        int channel = m_seq.get_midi_channel();
        std::string ccname(c_controller_names[control]);
        if (usr().controller_active(bus, channel, control))
            ccname = usr().controller_name(bus, channel, control);

        snprintf(type, sizeof type, "Control Change - %s", ccname.c_str());
    }
    else if (status == EVENT_PROGRAM_CHANGE)
        snprintf(type, sizeof type, "Program Change");
    else if (status == EVENT_CHANNEL_PRESSURE)
        snprintf(type, sizeof type, "Channel Pressure");
    else if (status == EVENT_PITCH_WHEEL)
        snprintf(type, sizeof type, "Pitch Wheel");
    else
        snprintf(type, sizeof type, "Unknown MIDI Event");

    char text[80];
    snprintf(text, sizeof text, "%s %s", hex, type);
    m_entry_data->set_text(text);
}

/**
 *  Update the window after a time out, based on dirtiness and on playback
 *  progress.  Note the new call to seqroll::follow_progress().  This allows
 *  the seqroll to pop to the next frame of events to continue to show the
 *  moving progress bar.  Does this need to be an option?  It only affects
 *  patterns longer than a measure or two, whatever the width of the seqroll
 *  window is.  This is a new feature that is not in seq24.
 *
 *  What about seqtime?  That doesn't change.
 */

bool
seqedit::timeout ()
{
    if (m_seq.get_raise())
    {
        m_seq.set_raise(false);
        raise();
    }
    m_seqroll_wid->draw_progress_on_window();

#if 0
    /*
     * Seq32 has this, but we are not sure why it is needed.
     * The set_active call triggers the button callback.
     */

    if (perf().get_sequence_record())
    {
        perf().set_sequence_record(false);
        m_toggle_record->set_active(! m_toggle_record->get_active());
    }
#endif

    /*
     * This was ours, did not seem to work.
     *
    if (m_seq.recording_next_measure() && m_seqroll_wid->get_expanded_record())
    {
        set_measures(get_measures() + 1);
        m_seqroll_wid->follow_progress();
    }
     */

    /*
     * This is the way Seq32 does it now, and it seems to work for Sequencer64.
     * However, the hardwired "4" is suspicious.
     */

    bool expandrec = m_seq.get_recording() && m_seqroll_wid->get_expanded_record();
    bool expand = m_seq.get_last_tick() >=
        (m_seq.get_length() - m_seq.get_unit_measure() / 4);

    if (expandrec && expand)
    {
        set_measures(get_measures() + 1);
        m_seqroll_wid->follow_progress();
    }

    if (perf().follow_progress() && ! expandrec)
        m_seqroll_wid->follow_progress();       /* keep up with progress    */

    if (m_seq.is_dirty_edit())                  /* m_seq.is_dirty_main()    */
    {
        m_seqroll_wid->redraw_events();
        m_seqevent_wid->redraw();
        m_seqdata_wid->redraw();
    }
    m_seqroll_wid->draw_progress_on_window();

    bool undo_on = m_button_undo->get_sensitive();
    if (m_seq.have_undo() && ! undo_on)
        m_button_undo->set_sensitive(true);
    else if (! m_seq.have_undo() && undo_on)
        m_button_undo->set_sensitive(false);

    bool redo_on = m_button_redo->get_sensitive();
    if (m_seq.have_redo() && ! redo_on)
        m_button_redo->set_sensitive(true);
    else if (! m_seq.have_redo() && redo_on)
        m_button_redo->set_sensitive(false);

    /*
     * Let the toggle-play button track the sequence's mute state. Also, since
     * we can control recording and thru via MIDI now, make sure those buttons
     * are correct.  Still need to handle quantized record (m_toggle_q_rec).
     */

    if (m_seq.get_playing() != m_toggle_play->get_active())
        m_toggle_play->set_active(m_seq.get_playing());

    if (m_seq.get_recording() != m_toggle_record->get_active())
        m_toggle_record->set_active(m_seq.get_recording());

    if (m_seq.get_thru() != m_toggle_thru->get_active())
        m_toggle_thru->set_active(m_seq.get_thru());

    return true;
}

/**
 *  Changes what perform and mainwid see as the "current sequence".  Similar to
 *  the same function in eventedit.
 *
 * \param set_it
 *      If true (the default value), indicates we want focus, otherwise we
 *      want to give up focus.
 */

void
seqedit::change_focus (bool set_it)
{
    if (set_it)
    {
        if (! m_have_focus)
        {
            perf().set_edit_sequence(m_seq.number());
            update_mainwid_sequences();
            update_perfedit_sequences();
            m_have_focus = true;
        }
    }
    else
    {
        if (m_have_focus)
        {
            perf().unset_edit_sequence(m_seq.number());
            update_mainwid_sequences();
            update_perfedit_sequences();
            m_have_focus = false;
        }
    }
}

/**
 *  Handles closing the sequence editor.
 */

void
seqedit::handle_close ()
{
    /*
     * Stazed fix, change this line:
     *
     * perf().master_bus().set_sequence_input(false, nullptr);
     */

//  perf().master_bus().set_sequence_input(false, &m_seq);

    perf().set_sequence_input(false, &m_seq);
    m_seq.set_recording(false);
    m_seq.set_editing(false);
    change_focus(false);
}

#ifdef USE_STAZED_PLAYING_CONTROL

void
seqedit::start_playing ()
{
    if(! perf().song_start_mode())
        m_seq.set_playing(m_toggle_play->get_active());

    perf().start_playing();
}

void
seqedit::stop_playing()
{
    perf().stop_playing();
}

#endif

/**
 *  On realization, calls the base-class version, and connects the redraw
 *  timeout signal, timed at redraw_period_ms().
 */

void
seqedit::on_realize ()
{
    gui_window_gtk2::on_realize();
    Glib::signal_timeout().connect
    (
        mem_fun(*this, &seqedit::timeout), redraw_period_ms()
    );
}

/**
 *  On receiving focus, attempt to tell mainwid that this sequence is now the
 *  current sequence.  Only works in certain circumstances.
 */

void
seqedit::on_set_focus (Widget * focus)
{
    gui_window_gtk2::on_set_focus(focus);
    change_focus();
}

/**
 *  Implements the on-focus event handling.
 */

bool
seqedit::on_focus_in_event (GdkEventFocus *)
{
    set_flags(Gtk::HAS_FOCUS);
    change_focus();
    return false;
}

/**
 *  Implements the on-unfocus event handling.
 */

bool
seqedit::on_focus_out_event (GdkEventFocus *)
{
    unset_flags(Gtk::HAS_FOCUS);
    change_focus(false);
    return false;
}

/**
 *  Handles an on-delete event.  It tells the sequence to stop recording,
 *  tells the perform object's mastermidibus to stop processing input,
 *  and sets the sequence object's editing flag to false.
 *
 * \warning
 *      This function also calls "delete this"!
 *
 * \return
 *      Always returns false.
 */

bool
seqedit::on_delete_event (GdkEventAny *)
{
    handle_close();

#ifdef SEQ64_STAZED_LFO_SUPPORT
    delete m_lfo_wnd;
#endif

    /*
     * We need to see if this object is in the map of seqedits so that we can
     * remove it from that list.
     */

    seqmenu::remove_seqedit(m_seq);
    delete this;
    return false;
}

/**
 *  Handles an on-scroll event.  This handles moving the scroll wheel on a
 *  mouse or do a two-fingered scrolling action on a touchpad.  If no modifier
 *  key is pressed, this moves the view up or down on the "notes" coordinate,
 *  showing different piano keys.  This behavior is implemented in
 *  seqkeys::on_scroll_event(), and is called into play by returning false
 *  here.
 *
 *  If the Ctrl key is pressed, then the scrolling action causes the view to
 *  zoom in or out.  This behavior is implemented here.
 *
 *  If the Shift key is pressed, then the scrolling action moves the view
 *  horizontally on the time-line (measures-line) of the piano roll.  This
 *  behavior is implemented here.
 */

bool
seqedit::on_scroll_event (GdkEventScroll * ev)
{
    if (is_ctrl_key(ev))
    {
        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            set_zoom(m_zoom * 2);       /* validates the new zoom value */
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            set_zoom(m_zoom / 2);       /* validates the new zoom value */
        return true;
    }
    else if (is_shift_key(ev))
    {
        double step = m_hadjust->get_step_increment();
        if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_DOWN))
            horizontal_adjust(step);
        else if (CAST_EQUIVALENT(ev->direction, SEQ64_SCROLL_UP))
            horizontal_adjust(-step);
        return true;
    }
    else
        return gui_window_gtk2::on_scroll_event(ev); /* instead of false */
}

/**
 *  Handles a key-press event.  A number of new keystrokes are processed, so
 *  that we can lessen the reliance on the mouse and work a little faster.
 *
 *      -   Ctrl-W keypress.  This keypress closes the sequence/pattern editor
 *          window by way of calling on_delete_event().  We could apply this
 *          convention to all the other windows.
 *      -   z 0 Z zoom keys.  "z" zooms out, "Z" (Shift-z) zooms in, and "0"
 *          resets the zoom to the default.
 *      -   Page-Up and Page-Down.  Moves up and down in the piano roll.
 *      -   Home and End.  Page to the top or the bottom of the piano roll.
 *      -   Shift-Page-Up and Shift-Page-Down.  Move left and right in the
 *          piano roll.
 *      -   Shift-Home and Shift-End.  Page to the start or the end of the
 *          piano roll.
 *      -   Ctrl-Page-Up and Ctrl-Page-Down.  Mirrors the zoom-in and zoom-out
 *          capabilities of scrolling up and down with the mouse while the
 *          Ctrl key is pressed.
 *
 *  The Keypad-End key is an issue on our ASUS "gaming" laptop.  Whether it is
 *  seen as a "1" or an "End" key depends on an interaction between the Shift
 *  and the Num Lock key.  Annoying, takes some time to get used to.
 *
 * \change layk 2016-10-17
 *      Issue #46.  Undoing (ctrl-z) removes two instances of history.  To
 *      reproduce this bug, if one makes three notes one at a time and presses
 *      ctrl-z once only the first one remains. Same goes for moving notes.
 *      This is due to this else-if statement where we call
 *      seqroll::on_key_press_event() making first removal.  This if statement
 *      is never true and seqroll::on_key_press_event() is called again as
 *      Gtk::Window::on_key_press_event(), making another m_seq.pop_undo() in
 *      seqroll.  Note that the code here was an (ill-advised) attempt to
 *      avoid the pattern title field from grabbing the initial keystrokes;
 *      better to just get used to clicking the piano roll first.  Finally,
 *      fixing the undo bug also let's ctrl-page-up/page-down change the zoom.
 *      Lastly, we've removed the undo here... seqroll already handles both
 *      undo and redo keystrokes.
 *
 * \change ca 2016-10-18
 *      Issue #46.  In addition to layk's fixes, we have to properly determine
 *      if we're inside the "Sequence Name" ("GtkEntry") field, as opposed to
 *      the "GtkDrawingArea" field, to avoid grabbing and using keystrokes
 *      intended for the text-entry field.  We may have to rethink the whole
 *      seqroll vs. seqedit key-press process at some point, as this is a bit
 *      too tricky.  Please note that the name "gtkmm__GtkEntry" likely
 *      applies only to GNU's C++ compiler, g++.  This will be an issue in any
 *      port to Microsoft's C++ compiler.
 *
 * \param ev
 *      Provides the keystroke event to be handled.
 *
 * \return
 *      Returns true if we handled the keystroke here.  Otherwise, returns the
 *      value of Gtk::Window::on_key_press_event(ev).
 */

bool
seqedit::on_key_press_event (GdkEventKey * ev)
{
    bool result = false;
    std::string focus_name = get_focus()->get_name();
    bool in_name_field = focus_name == "gtkmm__GtkEntry";   /* g++ only!    */
    keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS, ev->state);
    if (is_ctrl_key(ev))
    {
        if (k.is(SEQ64_w))
        {
            /*
             * Here, we must return immediately, since this function deletes
             * "this", and we cannot access this object anymore.  Segfault!
             */

            return on_delete_event((GdkEventAny *)(ev));
        }
        else if (k.is(SEQ64_Page_Up))               /* zoom in              */
        {
            set_zoom(m_zoom / 2);
            result = true;
        }
        else if (k.is(SEQ64_Page_Down))             /* zoom out             */
        {
            set_zoom(m_zoom * 2);
            result = true;
        }
#ifdef SEQ64_STAZED_LFO_SUPPORT
        else if (k.is(SEQ64_l))
        {
            m_lfo_wnd->toggle_visible();
        }
#endif
    }
    else if (is_shift_key(ev))
    {
        if (k.is(SEQ64_Page_Down))                  /* scroll rightward     */
        {
            double step = m_hadjust->get_page_increment();
            horizontal_adjust(step);
            result = true;
        }
        else if (k.is(SEQ64_End, SEQ64_KP_End))
        {
            horizontal_set(9999999.0);              /* scroll to the end    */
            result = true;
        }
        else if (k.is(SEQ64_Page_Up))               /* scroll leftward      */
        {
            double step = m_hadjust->get_page_increment();
            horizontal_adjust(-step);
            result = true;
        }
        else if (k.is(SEQ64_Home, SEQ64_KP_Home))
        {
            horizontal_set(0);                      /* scroll to beginning  */
            result = true;
        }
        else if (k.is(SEQ64_Z))                     /* zoom in               */
        {
            if (! in_name_field)
            {
                set_zoom(m_zoom / 2);
                result = true;
            }
        }
    }
    else
    {
#ifdef USE_UNHANDLED_SHIFT_KEY

        /*
         * Handled in Shift key handling above now.
         */

        if (k.is(SEQ64_Z))                          /* zoom in              */
        {
            if (! in_name_field)
            {
                set_zoom(m_zoom / 2);
                result = true;
            }
        }
        else
#endif
        if (k.is(SEQ64_0))                          /* reset to normal zoom */
        {
            if (! in_name_field)
            {
                set_zoom(m_initial_zoom);           /* not usr().zoom())    */
                result = true;
            }
        }
        else if (k.is(SEQ64_z))                     /* zoom out             */
        {
            if (! in_name_field)
            {
                set_zoom(m_zoom * 2);
                result = true;
            }
        }
        else if (k.is(SEQ64_Page_Down))             /* scroll downward      */
        {
            double step = m_vadjust->get_page_increment();
            vertical_adjust(step);
            result = true;
        }
        else if (k.is(SEQ64_End, SEQ64_KP_End))
        {
            vertical_set(9999999.0);                /* scroll to the end    */
            result = true;
        }
        else if (k.is(SEQ64_Page_Up))               /* scroll upward        */
        {
            double step = m_vadjust->get_page_increment();
            vertical_adjust(-step);
            result = true;
        }
        else if (k.is(SEQ64_Home, SEQ64_KP_Home))
        {
            vertical_set(0);                        /* scroll to beginning  */
            result = true;
        }
    }
    if (! result)
        result = Gtk::Window::on_key_press_event(ev);

    return result;
}

}           // namespace seq64

/*
 * seqedit.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

