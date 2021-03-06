#ifndef SEQ64_QPLAYLISTFRAME_HPP
#define SEQ64_QPLAYLISTFRAME_HPP

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
 * \file          qsplaylistframe.hpp
 *
 *  This module declares/defines the base class for a simple playlist editor based
 *  on Qt 5.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-09-04
 * \updates       2018-11-06
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>
#include "easy_macros.hpp"              /* nullptr and related macros   */

class QTableWidgetItem;
class QTimer;

/*
 * Do not document namespaces.
 */

namespace Ui
{
    class qplaylistframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qsmainwnd;

/**
 *
 */

class qplaylistframe : public QFrame
{
    friend class qsmainwnd;

private:

    /**
     *  Provides human-readable names for the columns of the playlist and song
     *  tables.
     */

    enum column_id_t
    {
        CID_MIDI_NUMBER,
        CID_ITEM_NAME
    };

    Q_OBJECT

public:

    qplaylistframe
    (
        perform & p,
        qsmainwnd * window,
        QWidget * parent = nullptr
    );

    virtual ~qplaylistframe ();

private:

    void set_row_heights (int height);
    void set_column_widths ();
    void reset_playlist ();
    void set_current_playlist ();
    void set_current_song ();
    void fill_playlists ();
    void fill_songs ();
    QTableWidgetItem * cell (bool isplaylist, int row, column_id_t col);

    /**
     * \getter m_perform
     */

    perform & perf ()
    {
        return m_perform;
    }

    bool load_playlist (const std::string & fullfilespec = "");

private slots:

    void handle_list_click_ex (int, int, int, int);
    void handle_song_click_ex (int, int, int, int);
    void handle_list_load_click ();
    void handle_list_add_click ();
    void handle_list_remove_click ();
    void handle_list_save_click ();
    void handle_song_add_click ();
    void handle_song_remove_click ();
    void handle_playlist_active_click ();
    void conditional_update ();

protected:                          // overrides of event handlers

    virtual void keyPressEvent (QKeyEvent * event);
    virtual void keyReleaseEvent (QKeyEvent * event);

private:

    Ui::qplaylistframe * ui;

private:

    /**
     *  A timer for screen refreshing.
     */

    QTimer * m_timer;

    /**
     *  The perform object.
     */

    perform & m_perform;

    /**
     *  The main window parent of this frame.
     */

    qsmainwnd * m_parent;

};          // class qplaylistframe

}           // namespace seq64

#endif      // SEQ64_QPLAYLISTFRAME_HPP

/*
 * qplaylistframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

