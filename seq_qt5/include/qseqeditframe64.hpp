#ifndef SEQ64_QSEQEDITFRAME64_HPP
#define SEQ64_QSEQEDITFRAME64_HPP

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
 *  along with seq24; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqeditframe64.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2018-07-05
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>
#include <QLayout>

#include "sequence.hpp"                 /* seq64::edit_mode_t enumeration   */

/*
 *  A bunch of forward declarations.  The Qt header files are moved into the
 *  cpp file.
 */

class QGridLayout;
class QIcon;
class QMenu;
class QPalette;
class QScrollArea;
class QScrollBar;
class QWidget;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qseqeditframe64;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qseqkeys;
    class qseqtime;
    class qseqroll;
    class qseqdata;
    class qstriggereditor;

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
 *  This frame holds tools for editing an individual MIDI sequence.  This frame is
 *  a more advanced version of qseqeditframe, which was based on Kepler34's
 *  EditFrame class.
 */

class qseqeditframe64 : public QFrame
{
    Q_OBJECT

public:

    explicit qseqeditframe64
    (
        perform & p, int seqid, QWidget * parent = nullptr
    );
    ~qseqeditframe64 ();

    void update_draw_geometry ();
    void set_editor_mode (edit_mode_t mode);

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    void follow_progress ();
#endif

private:

    /**
     * \getter m_performance
     *      The const version.
     */

    const perform & perf () const
    {
        return m_performance;
    }

    /**
     * \getter m_performance
     *      The non-const version.
     */

    perform & perf ()
    {
        return m_performance;
    }

    QIcon * create_menu_image (bool state);

private slots:

    void update_seq_name ();
    void update_beats_per_measure (int index);
    void increment_beats_per_measure ();
    void reset_beats_per_measure ();
    void update_beat_width (int index);
    void next_beat_width ();
    void reset_beat_width ();
    void update_measures (int index);
    void next_measures ();
    void reset_measures ();
#ifdef SEQ64_STAZED_TRANSPOSE
    void transpose (bool ischecked);
#endif
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    void update_chord (int index);
    void increment_chord ();
    void reset_chord ();
#endif
    void update_midi_bus (int index);
    void reset_midi_bus ();
    void update_midi_channel (int index);
    void reset_midi_channel ();
    void undo ();
    void redo ();

    void tools ();
    void select_all_notes ();
    void inverse_note_selection ();
    void quantize_notes ();
    void tighten_notes ();
    void transpose_notes ();

    void sequences ();

    void update_grid_snap (int index);
    void reset_grid_snap ();
    void update_note_length (int index);
    void reset_note_length ();
    void update_zoom (int index);
    void reset_zoom ();
    void zoom_in ();
    void zoom_out ();
    void update_key (int index);
    void reset_key ();
    void update_scale (int index);
    void reset_scale ();
    void events ();

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    void follow (bool ischecked);
#endif

private:

    void do_action (edit_action_t action, int var);
    void popup_tool_menu ();
    void popup_sequence_menu ();
    void popup_event_menu ();
    void repopulate_event_menu (int buss, int channel);
    void popup_mini_event_menu ();
    void repopulate_mini_event_menu (int buss, int channel);

private:

    void set_dirty ();
    void set_beats_per_measure (int bpm);
    void set_beat_width (int bw);
    void set_measures (int len);
    int get_measures ();
    void set_midi_channel (int midichannel, bool user_change = false);
    void set_midi_bus (int midibus, bool user_change = false);
    void set_note_length (int nlen);
    void set_snap (int s);
    void set_zoom (int z);
    void set_chord (int chord);
    void set_key (int key);
    void set_scale (int key);
    void set_background_sequence (int seqnum);

#ifdef SEQ64_STAZED_TRANSPOSE
    void set_transpose_image (bool istransposable);
#endif

    void set_event_entry
    (
        QMenu * menu,
        const std::string & text,
        bool present,
        midibyte status,
        midibyte control = 0
    );

    void set_data_type (midibyte status, midibyte control = 0);

private:

    Ui::qseqeditframe64 * ui;
    perform & m_performance;
    sequence * m_seq;
    qseqkeys * m_seqkeys;
    qseqtime * m_seqtime;
    qseqroll * m_seqroll;
    qseqdata * m_seqdata;
    qstriggereditor * m_seqevent;  // qseqevent?

    /**
     *  Menu for Tools.
     */

    QMenu * m_tools_popup;

    /**
     *  Menu for Background Sequences.
     */

    QMenu * m_sequences_popup;

    /**
     *  Menu for the Event Data button.
     */

    QMenu * m_events_popup;

    /**
     *  Menu for the "mini" Event Data button.
     */

    QMenu * m_minidata_popup;

    /**
     *  Holds the current beats-per-measure selection.
     */

    int m_beats_per_bar;

    /**
     *  Holds the current beat-width selection.
     */

    int m_beat_width;

    /**
     *  Provides the initial zoom, used for restoring the original zoom using
     *  the 0 key.
     */

    const int m_initial_zoom;

    /**
     *  Provides the zoom values: 1  2  3  4, and 1, 2, 4, 8, 16.
     *  The value of zoom is the same as the number of pixels per tick on the
     *  piano roll.
     */

    int m_zoom;

    /**
     *  Used in setting the snap-to value in pulses, off = 1.
     */

    int m_snap;

    /**
     *  The default length of a note to be inserted by a right-left-click
     *  operation.
     */

    int m_note_length;

    /**
     *  Setting for the music scale, can now be saved with the sequence.
     */

    int m_scale;

#ifdef SEQ64_STAZED_CHORD_GENERATOR

    /**
     *  Setting for the current chord generation; not now saved with the
     *  sequence.
     */

    int m_chord;

#endif

    /**
     *  Setting for the music key, can now be saved with the sequence.
     */

    int m_key;

    /**
     *  Setting for the background sequence, can now be saved with the
     *  sequence.
     */

    int m_bgsequence;

    /**
     *  Provides the length of the sequence in measures.
     */

    long m_measures;

    /**
     *  Holds a copy of the current PPQN for the sequence (and the entire MIDI
     *  file).
     */

    int m_ppqn;

#ifdef USE_STAZED_ODD_EVEN_SELECTION
    int m_pp_whole;
    int m_pp_eighth;
    int m_pp_sixteenth;
#endif

    /**
     *  Indicates what MIDI event/status the data window currently editing.
     */

    midibyte m_editing_status;

    /**
     *  Indicates what MIDI CC value the data window currently editing.
     */

    midibyte m_editing_cc;

    /**
     *  Indicates the first event found in the sequence while setting up the
     *  data menu via set_event_entry().  If no events exist, the value is
     *  0x00.
     */

    midibyte m_first_event;

    /**
     *  Provides the string describing the first event, or "(no events)".
     */

    std::string m_first_event_name;

    /**
     *  Indicates that the focus has already been changed to this sequence.
     */

    bool m_have_focus;

    /**
     *  Indicates if this sequence is in note-edit more versus drum-edit mode.
     */

    edit_mode_t m_edit_mode;

private:

    /*
     * Documented in the cpp file.
     */

    static int m_initial_snap;
    static int m_initial_note_length;

#ifdef SEQ64_STAZED_CHORD_GENERATOR
    static int m_initial_chord;
#endif

};          // class qseqeditframe64

}           // namespace seq64

#endif      // SEQ64_QSEQEDITFRAME64_HPP

/*
 * qseqeditframe64.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


