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
 * \updates       2018-06-28
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

class QWidget;
class QGridLayout;
class QScrollArea;
class QScrollBar;
class QPalette;
class QMenu;

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

private slots:

    void update_seq_name ();
    void update_beats_per_measure (int index);
    void increment_beats_per_measure ();
    void update_beat_width (int index);
    void next_beat_width ();
    void update_measures (int index);
    void next_measures ();
    void update_grid_snap (int index);
    void update_note_length (int index);
    void update_grid_zoom (int index);
    void zoom_in ();
    void zoom_out ();

private:

    void set_snap (int s);
    void set_note_length (int nlen);
    void set_zoom (int z);
    void set_dirty ();
    void set_beats_per_measure (int bpm);
    void set_beat_width (int bw);
    void set_measures (int len);
    int get_measures ();

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


