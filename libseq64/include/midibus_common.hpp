#ifndef SEQ64_MIDIBUS_COMMON_HPP
#define SEQ64_MIDIBUS_COMMON_HPP

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
 * \file          midibus_common.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-05-20
 * \license       GNU GPLv2 or above
 *
 *  Defines some midibus constants and the clock_e enumeration
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Manifest global constants.  These constants were also defined in
 *  midibus_portmidi.h, but we made them common to both implementations here.
 *
 *  The c_midibus_output_size value is passed, in mastermidibus, to
 *  snd_seq_set_output_buffer_size().  Not sure if the value needs to be so
 *  large.
 */

const int c_midibus_output_size = 0x100000;     // 1048576

/**
 *  The c_midibus_input_size value is passed, in mastermidibus,  to
 *  snd_seq_set_input_buffer_size().  Not sure if the value needs to be so
 *  large.
 */

const int c_midibus_input_size  = 0x100000;     // 1048576

/**
 *  Controls the amount a SysEx data sent at one time, in the midibus module.
 */

const int c_midibus_sysex_chunk = 0x100;        // 256

/**
 *  A clock enumeration, as used in the File / Options / MIDI Clock dialog.
 *  This enumeration was also defined in midibus_portmidi.h, but we put it
 *  into this common module to avoid duplication.
 */

enum clock_e
{
    /**
     *  A new, currently-hidden value to indicate to ignore/disable an
     *  output port.  If a port always fails to open, we want to just ignore
     *  it.
     */

    e_clock_disabled = -1,

    /**
     *  Corresponds to the "Off" selection in the MIDI Clock tab.  With this
     *  setting, the MIDI Clock is disabled for the buss using this setting.
     *  Notes will still be sent that buss, of course.  Some software
     *  synthesizers might require this setting in order to make a sound.
     */

    e_clock_off,

    /**
     *  Corresponds to the "Pos" selection in the MIDI Clock tab.  With this
     *  setting, MIDI Clock will be sent to this buss, and, if playback is
     *  starting beyond tick 0, then MIDI Song Position and MIDI Continue will
     *  also be sent on this buss.
     */

    e_clock_pos,

    /**
     *  Corresponds to the "Mod" selection in the MIDI Clock tab.  With this
     *  setting, MIDI Clock and MIDI Start will be sent.  But clocking won't
     *  begin until the Song Position has reached the start modulo (in 1/16th
     *  notes) that is specified.
     */

    e_clock_mod

};          // enum clock_e

}           // namespace seq64

#endif      // SEQ64_MIDIBUS_COMMON_HPP

/*
 * midibus_common.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

