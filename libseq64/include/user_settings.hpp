#ifndef SEQ64_USER_SETTINGS_HPP
#define SEQ64_USER_SETTINGS_HPP

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
 * \file          user_settings.hpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2015-09-27
 * \license       GNU GPLv2 or above
 *
 *  This collection of variables describes some facets of the
 *  "Patterns Panel" or "Sequences Window", which is visually presented by
 *  the Gtk::Window-derived class called mainwnd.
 *
 *  The Patterns Panel contains an 8-by-4 grid of "pattern boxes" or
 *  "sequence boxes".  All of the patterns in this grid comprise what is
 *  called a "set" (in the musical sense) or a "screen set".
 *
 *  We want to be able to change these defaults.
 */

#include <string>
#include <vector>

#include "easy_macros.h"               // with platform_macros.h, too
#include "user_instrument.hpp"
#include "user_midi_bus.hpp"

/**
 *  Manifest constant for the maximum number of "instrument" values in a
 *  user_midi_bus_t structure.
 */

#define MIDI_BUS_CHANNEL_MAX             16

/**
 *  Holds the current values of sequence settings and settings that can
 *  modify the number of sequences and the configuration of the
 *  user-interface.  These settings will eventually be made part of the
 *  "user" settings file.
 */

class user_settings
{
    /**
     *  Internal type for the container of user_midi_bus objects.
     *  Sorry about the "confusion" about "bus" versus "buss".
     *  See Google for arguments about it.
     */

    typedef std::vector<user_midi_bus> Busses;
    typedef std::vector<user_midi_bus>::iterator BussIterator;
    typedef std::vector<user_midi_bus>::const_iterator BussConstIterator;

    /**
     *  Provides data about the MIDI busses, readable from the "user"
     *  configuration file.  Since this object is a vector, its size is
     *  adjustable.
     */

    Busses m_midi_buses;

    /**
     *  Internal type for the container of user_instrument objects.
     */

    typedef std::vector<user_instrument> Instruments;
    typedef std::vector<user_instrument>::iterator InstrumentIterator;
    typedef std::vector<user_instrument>::const_iterator InstrumentConstIterator;

    /**
     *  Provides data about the MIDI instruments, readable from the "user"
     *  configuration file.  The size is adjustable, and grows as objects
     *  are added.
     */

    Instruments m_instruments;

    /**
     *  Number of rows in the Patterns Panel.  The current value is 4, and
     *  if changed, many other values depend on it.  Together with
     *  m_mainwnd_cols, this value fixes the patterns grid into a 4 x 8
     *  set of patterns known as a "screen set".
     */

    int m_mainwnd_rows;

    /**
     *  Number of columns in the Patterns Panel.  The current value is 4,
     *  and probably won't change, since other values depend on it.
     *  Together with m_mainwnd_rows, this value fixes the patterns grid
     *  into a 4 x 8 set of patterns known as a "screen set".
     */

    int m_mainwnd_cols;

    /**
     *  Number of patterns/sequences in the Patterns Panel, also known as
     *  a "set" or "screen set".  This value is 4 x 8 = 32 by default.
     *
     * \warning
     *      Currently part of the "rc" file and rc_settings!
     */

    int m_seqs_in_set;

    /**
     *  Number of group-mute tracks that can be support, which is
     *  m_seqs_in_set squared, or 1024.
     */

    int m_gmute_tracks;

    /**
     *  Maximum number of screen sets that can be supported.  Basically,
     *  that the number of times the Patterns Panel can be filled.  32
     *  sets can be created.
     */

    int m_max_sets;

    /*
     *  The maximum number of patterns supported is given by the number of
     *  patterns supported in the panel (32) times the maximum number of
     *  sets (32), or 1024 patterns.  It is basically the same value as
     *  m_max_sequence by default.
     */

     int m_total_seqs;

    /**
     *  The maximum number of patterns supported is given by the number of
     *  patterns supported in the panel (32) times the maximum number of
     *  sets (32), or 1024 patterns.
     */

    int m_max_sequence;

    /**
     *  Constants for the mainwid class.  The m_text_x and m_text_y
     *  constants help define the "seqarea" size.  It looks like these two
     *  values are the character width (x) and height (y) in pixels.
     *  Thus, these values would be dependent on the font chosen.  But
     *  that, currently, is hard-wired.  See the m_font_6_12[] array for
     *  the default font specification.
     *
     *  However, please not that font files are not used.  Instead, the
     *  fonts are provided by two pixmaps in the <tt> src/pixmap </tt>
     *  directory: <tt> font_b.xpm </tt> (black lettering on a white
     *  background) and <tt> font_w.xpm </tt> (white lettering on a black
     *  background).
     */

    int m_text_x;
    int m_text_y;

    /**
     *  Constants for the mainwid class.  The m_seqchars_x and
     *  m_seqchars_y constants help define the "seqarea" size.  These look
     *  like the number of characters per line and the number of lines of
     *  characters, in a pattern/sequence box.
     */

    int m_seqchars_x;
    int m_seqchars_y;

    /**
     *  The m_seqarea_x and m_seqarea_y constants are derived from the
     *  width and heights of the default character set, and the number of
     *  characters in width, and the number of lines, in a
     *  pattern/sequence box.
     *
     *  Compare these two constants to m_seqarea_seq_x(y), which was in
     *  mainwid.h, but is now in this file.
     */

    int m_seqarea_x;
    int m_seqarea_y;

    /**
     * Area of what?  Doesn't look at all like it is based on the size of
     * characters.  These are used only in the mainwid module.
     */

    int m_seqarea_seq_x;
    int m_seqarea_seq_y;

    /**
     *  These control sizes.  We'll try changing them and see what
     *  happens.  Increasing these value spreads out the pattern grids a
     *  little bit and makes the Patterns panel slightly bigger.  Seems
     *  like it would be useful to make these values user-configurable.
     */

    int m_mainwid_border;             // try 2 or 3 instead of 0
    int m_mainwid_spacing;            // try 4 or 6 instead of 2

    /**
     *  This constants seems to be created for a future purpose, perhaps
     *  to reserve space for a new bar on the mainwid pane.  But it is
     *  used only in this header file, to define m_mainwid_y, but doesn't
     *  add anything to that value.
     */

    int m_control_height;

    /**
     * The width of the main pattern/sequence grid, in pixels.  Affected
     * by the m_mainwid_border and m_mainwid_spacing values.
     */

    int m_mainwid_x;

    /*
     * The height  of the main pattern/sequence grid, in pixels.  Affected by
     * the m_mainwid_border and m_control_height values.
     */

    int m_mainwid_y;

public:

    user_settings ();
    user_settings (const user_settings & rhs);
    user_settings & operator = (const user_settings & rhs);

    void set_defaults ();
    void normalize ();
    void set_globals () const;
    void get_globals ();

    bool add_bus (const std::string & alias);
    bool add_instrument (const std::string & instname);

    /**
     * \getter
     *      Unlike the non-const version this function is public.
     *      Cannot append the const specifier.
     */

    const user_midi_bus & bus (int index) // const
    {
        return private_bus(index);
    }

    /**
     * \getter
     *      Unlike the non-const version this function is public.
     *      Cannot append the const specifier.
     */

    const user_instrument & instrument (int index) // const
    {
        return private_instrument(index);
    }

    /**
     * \getter m_midi_buses.size()
     */

    int bus_count () const
    {
        return int(m_midi_buses.size());
    }

    void set_bus_instrument (int index, int channel, int instrum);

    /**
     * \getter m_midi_buses[buss].instrument[channel]
     * \todo
     *      Do this for controllers values and for user_instrument
     *      members.
     */

     int bus_instrument (int buss, int channel)
     {
          return bus(buss).instrument(channel);
     }

    /**
     * \getter m_midi_buses[buss].name
     */

    const std::string & bus_name (int buss)
    {
        return bus(buss).name();
    }

    /**
     * \getter m_instruments.size()
     */

    int instrument_count () const
    {
        return int(m_instruments.size());
    }

    void set_instrument_controllers
    (
        int index, int cc, const std::string & ccname, bool isactive
    );

    /**
     * \getter m_instruments[instrument].instrument (name of instrument)
     */

    const std::string & instrument_name (int instrum)
    {
        return instrument(instrum).name();
    }

    /**
     * \getter m_instruments[instrument].controllers_active[controller]
     */

    bool instrument_controller_active (int instrum, int c)
    {
        return instrument(instrum).controller_active(c);
    }

    /**
     * \getter m_instruments[instrument].controllers_active[controller]
     */

    const std::string & instrument_controller_name (int instrum, int c)
    {
        return instrument(instrum).controller_name(c);
    }

public:

    /**
     * \getter m_mainwnd_rows
     */

    int mainwnd_rows () const
    {
        return m_mainwnd_rows;
    }

    /**
     * \getter m_mainwnd_cols
     */

    int mainwnd_cols () const
    {
        return m_mainwnd_cols;
    }

    /**
     * \getter m_seqs_in_set
     */

    int seqs_in_set () const
    {
        return m_seqs_in_set;
    }

    /**
     * \getter m_gmute_tracks
     */

    int gmute_tracks () const
    {
        return m_gmute_tracks;
    }

    /**
     * \getter m_max_sets
     */

    int max_sets () const
    {
        return m_max_sets;
    }

    /**
     * \getter m_max_sequence
     */

    int max_sequence () const
    {
        return m_max_sequence;
    }

    /**
     * \getter m_text_x
     */

    int text_x () const
    {
        return m_text_x;
    }

    /**
     * \getter m_text_y
     */

    int text_y () const
    {
        return m_text_y;
    }

    /**
     * \getter m_seqchars_x
     */

    int seqchars_x () const
    {
        return m_seqchars_x;
    }

    /**
     * \getter m_seqchars_y
     */

    int seqchars_y () const
    {
        return m_seqchars_y;
    }

    /**
     * \getter m_seqarea_x
     */

    int seqarea_x () const
    {
        return m_seqarea_x;
    }

    /**
     * \getter m_seqarea_y
     */

    int seqarea_y () const
    {
        return m_seqarea_y;
    }

    /**
     * \getter m_seqarea_seq_x
     */

    int seqarea_seq_x () const
    {
        return m_seqarea_seq_x;
    }

    /**
     * \getter m_seqarea_seq_y
     */

    int seqarea_seq_y () const
    {
        return m_seqarea_seq_y;
    }

    /**
     * \getter m_mainwid_border
     */

    int mainwid_border () const
    {
        return m_mainwid_border;
    }

    /**
     * \getter m_mainwid_spacing
     */

    int mainwid_spacing () const
    {
        return m_mainwid_spacing;
    }

    /**
     * \getter m_control_height
     */

    int control_height () const
    {
        return m_control_height;
    }

    /**
     * \getter m_mainwid_x
     */

    int mainwid_x () const
    {
        return m_mainwid_x;
    }

    /**
     * \getter m_mainwid_y
     */

    int mainwid_y () const
    {
        return m_mainwid_y;
    }

    void mainwnd_rows (int value);
    void mainwnd_cols (int value);
    void max_sets (int value);
    void text_x (int value);
    void text_y (int value);
    void seqchars_x (int value);
    void seqchars_y (int value);
    void seqarea_x (int value);
    void seqarea_y (int value);
    void seqarea_seq_x (int value);
    void seqarea_seq_y (int value);
    void mainwid_border (int value);
    void mainwid_spacing (int value);
    void control_height (int value);

    /*
     *  These values are calculated from other values in the normalize()
     *  function:
     *
     *  void seqs_in_set (int value);
     *  void gmute_tracks (int value);
     *  void max_sequence (int value);
     *  void mainwid_x (int value);
     *  void mainwid_y (int value);
     */

    void dump_summary();

private:

#if 0
    void bus_alias (int index, const std::string & alias);
    void instrument_name (int index, const std::string & instname);
#endif

    user_midi_bus & private_bus (int buss);
    user_instrument & private_instrument (int instrum);

};

#endif  // SEQ64_USER_SETTINGS_HPP

/*
 * user_settings.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
