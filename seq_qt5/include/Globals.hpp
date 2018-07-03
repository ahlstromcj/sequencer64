#ifndef KEPLER43_GLOBALS_HPP
#define KEPLER43_GLOBALS_HPP

/*
 *  This file is part of seq24.
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

/*
 * DEPRECATED.  Migrate to globals.h or to the modules that use it.
 */

#include <QString>
#include <QMap>
#include <QColor>

using namespace std;

const int c_ppqn         = 192;  /* default - dosnt change */
const int c_bpm          = 120;  /* default */
const int c_maxBuses = 32;

/* trigger width in milliseconds */
const int qc_thread_trigger_width_ms = 4;
const int qc_thread_trigger_lookahead_ms = 2;

/* keyboard */

// padding to the left of the note roll
// to allow 1st tick drum hits
/* events bar */

/* time scale window on top */
const int qc_timearea_y = 18;

/* sequences */
const int qc_midi_notes = 256;

/* maximum size of sequence, default size */
const int qc_maxbeats     = 0xFFFF;   /* max number of beats in a sequence */

/* used in menu to tell setState what to do */
const int qc_adding = 0;
const int qc_normal = 1;
const int qc_paste  = 2;

/* redraw when recording ms */
#ifdef __WIN32__
const int qc_redraw_ms = 20;
#else
const int qc_redraw_ms = 40;
#endif

/* consts for perform editor */
const int qc_names_x = 6 * 24;
const int qc_names_y = 22;
const int qc_perf_scale_x = 32; /*ticks per pixel */

extern bool is_pattern_playing;

const int c_max_instruments = 64;

enum mouse_action_e
{
    e_action_select,
    e_action_draw,
    e_action_grow
};

#endif  // KEPLER43_GLOBALS_HPP
