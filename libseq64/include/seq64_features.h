#ifndef SEQ64_FEATURES_H
#define SEQ64_FEATURES_H

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
 * \file          seq64_features.h
 *
 *    This module summarizes or defines all of the configure and build-time
 *    options available for Sequencer64.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-08-19
 * \updates       2016-08-19
 * \license       GNU GPLv2 or above
 *
 *    Some options (the "USE_xxx" options) specify experimental and
 *    unimplemented features.
 *
 *    Some options are available (or can be disabled) by running the
 *    "configure" script generated using the configure.ac file.  These
 *    options are things that a normal user or a seq24 aficianado might want to
 *    disable.  They are defined as desired, in the auto-generated
 *    seq64-config.h file in the top-level "include" directory.
 *
 *    The rest of the options can be modified only by editing the source code
 *    (soon to be this file) to enable or disable features.  These options are
 *    those that we feel more strongly about.
 */

/*
 * Odds and ends that we missed.
 */

#define USE_NON_NOTE_EVENT_ADJUSTMENT   /* see sequence.cpp                 */

/*
 * Currently undefined as tentative or EXPERIMENTAL.
 */

#undef  USE_STAZED_COMPANDING
#undef  USE_STAZED_EXTRAS
#undef  USE_STAZED_EXTRA_SNAPS
#undef  USE_STAZED_FIX

/*
 * TEMPORARILY defined
 */

#define USE_STAZED_JACK_SUPPORT

#undef  USE_STAZED_LFO_SUPPORT
#undef  USE_STAZED_MENU_MODE_BUTTON
#undef  USE_STAZED_MIDIBUS_SUPPORT
#undef  USE_STAZED_MIDI_DUMP
#undef  USE_STAZED_MULTIPLY_PATTERN
#undef  USE_STAZED_ODD_EVEN_SELECTION
#undef  USE_STAZED_PLAYING_CONTROL
#undef  USE_STAZED_RANDOMIZE_SUPPORT
#undef  USE_STAZED_SELECTION_EXTENSIONS
#undef  USE_STAZED_SEQDATA_EXTENSIONS
#undef  USE_STAZED_SHIFT_SUPPORT
#undef  USE_STAZED_SONG_MODE_BUTTON
#undef  USE_STAZED_SYSEX_SUPPORT

/*
 * TEMPORARILY defined
 */

#define USE_STAZED_TRANSPORT

#undef  USE_STAZED_PERF_AUTO_SCROLL

/*
 * Configure-time options.
 *
 *    SEQ64_HAVE_LIBASOUND
 *    SEQ64_HIGHLIGHT_EMPTY_SEQS
 *    SEQ64_JACK_SESSION
 *    SEQ64_JACK_SUPPORT
 *    SEQ64_LASH_SUPPORT
 *    SEQ64_PAUSE_SUPPORT
 *    SEQ64_STAZED_CHORD_GENERATOR
 *    SEQ64_STAZED_TRANSPOSE
 *    SEQ64_STRIP_EMPTY_MUTES
 */

/*
 * Edit-time (permanent) options.
 */

/**
 *  EXPERIMENTAL.  Not yet working.  A very tough problem.
 *  The idea is to go into an auto-screen-set mode via a menu entry, where the
 *  first set is unmuted, and then changes to the screen-set number queue the
 *  previous screen-set for muting, and queue up the current one for
 *  unmuting.
 */

#undef  SEQ64_USE_AUTO_SCREENSET_QUEUE

/**
 *  Try to highlight the selected pattern using black-on-cyan
 *  coloring, in addition to the red progress bar marking that already exists.
 *  Moved from seqmenu.  Seems to work pretty well now.
 */

#define SEQ64_EDIT_SEQUENCE_HIGHLIGHT

/**
 *  This special value of zoom sets the zoom according to a power of two
 *  related to the PPQN value of the song.  Is this really used?
 */

#define SEQ64_USE_ZOOM_POWER_OF_2       0

/*
 * Others
 */

/**
 *  This provides a build option for having the pattern editor window scroll
 *  to keep of with the progress bar, for sequences that are longer than the
 *  measure or two that a pattern window will show.
 *
 *  We thought about making this a configure option or a run-time option, but
 *  this kind of scrolling is a universal convention of MIDI sequencers.  If
 *  you really don't like this feature, let me know, and I will make it a
 *  configure option.  We could also disable it it "legacy" mode, which also
 *  disables a lot of other features.
 *
 * \warning
 *      This code might still have issues with interactions between triggers
 *      and gaps in the performance (song) window when JACK transport is
 *      active.  Still investigating.
 */

#define SEQ64_FOLLOW_PROGRESS_BAR

#define SEQ64_HANDLE_TIMESIG_AND_TEMPO

/**
 *  Let's try using lighter solid lines in the piano rolls and see how it
 *  looks.  It looks a little better.
 */

#define SEQ64_SOLID_PIANOROLL_GRID

#define SEQ64_STATISTICS_SUPPORT
#define SEQ64_STAZED_EDIT_MENU
#define SEQ64_USE_BLACK_SELECTION_BOX

/**
 * This macro indicates an experimental feature where we are tyring to see
 * if using std::multimap as an event-container has any benefits over
 * using std::list.  Define this macro to use the multimap.  So far, we
 * recommend using it.  In debug mode, the b4uacuse MIDI files take about 8
 * seconds (!) to load using the list, but barely any time to load using the
 * multimap.  It turns out the multimap does have issues; one must be careful
 * dealing with insertions since multiple events with the same keys can be
 * load.  This caused an issue with copy/paste leaving unlinked notes that
 * would either play forever or not play at all.  A good fix was provided by
 * user 0rel.
 */

#define SEQ64_USE_EVENT_MAP             /* the map seems to work well!  */

/**
 *  Determins which implementation of a MIDI byte container is used.
 *  See the midifile module.
 */

#define SEQ64_USE_MIDI_VECTOR           /* as opposed to the MIDI list      */

#define SEQ64_USE_TDEAGAN_CODE

/*
 * #define SEQ64_USE_DEBUG_OUTPUT (normally disabled)
 */

#endif      // SEQ64_FEATURES_H

/*
 * seq64_features.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

