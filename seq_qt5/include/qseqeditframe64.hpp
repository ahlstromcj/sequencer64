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
 * \updates       2019-12-16
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>

#include "qseqframe.hpp"                /* seq64::qseqframe                 */
#include "sequence.hpp"                 /* seq64::edit_mode_t enumeration   */

/*
 * We have a weird issues with the automake build (but not the Qt build),
 * where these macros appear to be defined, but at run time, Qt says
 * that the reset_chord() callback, for example, does not exist.  Still
 * trying to figure that one out.
 *
 * This does not work for the "chord" feature!
 *
 * #include "seq64_features.h"          // includes seq64-config.h //
 *
 * In the meantime....
 */

/**
 *  Specifies the reported final size of the main window when the larger edit
 *  frame "kicks in".  See the comments for qsmainwnd::refresh().  The final
 *  vertical size of the main window ends up at around 700, puzzling!  The
 *  vertical size of the "external" edit-frame is only about 600.  Here are
 *  the current measured (via kruler) heights:
 *
 *      -   Top panel: 90
 *      -   Time pane: 20
 *      -   Roll pane: 250
 *      -   Event pane: 27
 *      -   Data pane: 128
 *      -   Bottom panel: 57
 *
 *  That total is 572.
 *
 *      -   qseqframe_height = 558, qseqeditframe64.ui
 *      -   qsmainwnd_height = 580, qsmainwnd.ui
 */

#define QSEQEDITFRAME64_WIDTH         680
#define QSEQEDITFRAME64_HEIGHT        920
#define QSEQEDITFRAME64_BASE_HEIGHT   572
#define QSEQEDITFRAME64_ROLL_HEIGHT   250

/*
 *  A few forward declarations.  The Qt header files are in the cpp file.
 */

class QIcon;
class QMenu;
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
    class qlfoframe;
    class sequence;

/**
 *  This frame holds tools for editing an individual MIDI sequence.  This
 *  frame is a more advanced version of qseqeditframe, which was based on
 *  Kepler34's EditFrame class.
 */

class qseqeditframe64 : public qseqframe
{
    friend class qlfoframe;

    Q_OBJECT

public:

    qseqeditframe64
    (
        perform & p,
        int seqid,
        QWidget * parent = nullptr
    );
    virtual ~qseqeditframe64 ();

    void initialize_panels ();
    void update_draw_geometry ();
    void set_editor_mode (edit_mode_t mode);
    void follow_progress (bool expand = false);

private:

    void remove_lfo_frame ();
    QIcon * create_menu_image (bool state);

private slots:

    virtual void zoom_in ();
    virtual void zoom_out ();
    virtual void update_zoom (int index);
    virtual void reset_zoom ();

    void conditional_update ();
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
    void transpose (bool ischecked);
    void update_chord (int index);
#ifdef SEQ64_QSEQEDIT_BUTTON_INCREMENT
    void increment_chord ();
#else
    void reset_chord ();
#endif
    void update_midi_bus (int index);
    void reset_midi_bus ();
    void update_midi_channel (int index);
    void reset_midi_channel ();
    void undo ();
    void redo ();

    /*
     * Tools button and handlers.
     */

    void tools ();
    void select_all_notes ();
    void inverse_note_selection ();
    void quantize_notes ();
    void tighten_notes ();
    void transpose_notes ();

    /*
     * More slots.
     */

    void sequences ();
    void update_grid_snap (int index);
    void reset_grid_snap ();
    void update_note_length (int index);
    void reset_note_length ();
    void update_key (int index);
    void reset_key ();
    void update_scale (int index);
    void reset_scale ();
    void editor_mode (bool ischecked);
    void events ();
    void data ();
    void show_lfo_frame ();
    void play_change (bool ischecked);
    void thru_change (bool ischecked);
    void record_change (bool ischecked);
    void q_record_change (bool ischecked);
    void update_record_type (int index);
    void update_recording_volume (int index);
    void reset_recording_volume ();
    void follow (bool ischecked);

private:

    /*
     * Slot helper functions.
     */

    void do_action (edit_action_t action, int var);
    void popup_tool_menu ();
    void popup_sequence_menu ();
    void repopulate_event_menu (int buss, int channel);
    void repopulate_mini_event_menu (int buss, int channel);
    void repopulate_midich_combo (int buss);

private:

    virtual void update_midi_buttons ();
    virtual void set_dirty ();

    void set_beats_per_measure (int bpm);
    void set_beat_width (int bw);
    void set_measures (int len);
    int get_measures ();
    void set_midi_channel (int midichannel, bool user_change = false);
    void set_midi_bus (int midibus, bool user_change = false);
    void set_note_length (int nlen);
    void set_snap (int s);
    void set_chord (int chord);
    void set_key (int key);
    void set_scale (int key);
    void set_background_sequence (int seqnum);
    void set_transpose_image (bool istransposable);
    void set_event_entry
    (
        QMenu * menu,
        const std::string & text,
        bool present,
        midibyte status,
        midibyte control = 0
    );
    void set_data_type (midibyte status, midibyte control = 0);
    void set_recording_volume (int recvol);

private:

    Ui::qseqeditframe64 * ui;

    /**
     *  The LFO window object used by the pattern editor.  This item get the
     *  seqdata window hooked into it, and so must follow that item in the C++
     *  initializer list.
     */

    qlfoframe * m_lfo_wnd;

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

    /**
     *  Setting for the current chord generation; not now saved with the
     *  sequence.
     */

    int m_chord;

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

    int m_ppqn;
     */

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

    /**
     *  Update timer for pass-along to the roll, event, and data classes.
     */

    QTimer * m_timer;

private:

    /*
     * Documented in the cpp file.
     */

    static int m_initial_snap;
    static int m_initial_note_length;
    static int m_initial_chord;

};          // class qseqeditframe64

}           // namespace seq64

#endif      // SEQ64_QSEQEDITFRAME64_HPP

/*
 * qseqeditframe64.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

