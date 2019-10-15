/*
 * JACK-Transport MIDI Beat Clock Generator
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2009 Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file          midiclocker64.cpp
 *
 *  This module defines the class for the midi_clocker.
 *
 * \library       midiclocker64 application
 * \author        TODO team; refactoring by Chris Ahlstrom
 * \date          2017-11-10
 * \updates       2017-11-19
 * \license       GNU GPLv2 or above
 *
 */

#include <stdlib.h>                     /* exit(3), EXIT_SUCCESS            */
#include <getopt.h>

#include "midi_clocker.hpp"             /* seq64::midi_clocker class        */

/**
 *  Lists the options (long and short) for the midiclocker application.
 */

static struct option const long_options [] =
{
    { "bpm",            required_argument,  0, 'b'  },
    { "force-bpm",      no_argument,        0, 'B'  },
    { "resync-delay",   required_argument,  0, 'd'  },
    { "jitter-level",   required_argument,  0, 'J'  },
    { "help",           no_argument,        0, 'h'  },
    { "no-position",    no_argument,        0, 'P'  },
    { "no-transport",   no_argument,        0, 'T'  },
    { "strict-bpm",     no_argument,        0, 's'  },
    { "version",        no_argument,        0, 'V'  },
    { NULL,             0,                  NULL, 0 }
};

/**
 *  Help strings.
 */

static const std::string s_help_intro =
"A JACK application to generate MIDI Clock and other system events via JACK\n"
"transport.  Based on jack_midi_clock, but using Sequencer64 libraries.\n\n"
"Usage: midiclocker [ options ] [JACK-port] *\n\n"
;

static const std::string s_help_options =
"Options:\n"
"\n"
"  -b bpm, --bpm          Default BPM (if JACK timecode master not available).\n"
"  -B, --force-bpm        Ignore JACK timecode master.\n"
"  -d sec, --resync-delay Seconds between 'Song Position' & 'Continue' message.\n"
"  -J percent,            Add artificial jitter to the signal from 0 to 20%%.\n"
"   --jitter-level        Default: off (0)\n"
"  -P, --no-position      Do not send Song Position messages.\n"
"  -T, --no-transport     Do not send Start/Stop/Continue messages.\n"
"  -s, --strict-bpm       Interpret tempo strictly as beats per minute (default\n"
"                         is quarter-notes per minute).\n"
"  -h, --help             Display this help and exit.\n"
"  -V, --version          Print version information and exit.\n"
"\n"
;

static const std::string s_help_paragraph_1 =
"Midiclocker sends MIDI beat clock messages if JACK transport is rolling. It\n"
"also sends Start, Continue, and Stop MIDI realtime messages whenever transport\n"
"changes state, unless the -T option is used. For midiclocker to send clock\n"
"messages, a JACK timecode Master must be present, and provide the position:\n"
"bar|beat|tick, i.e. \"BBT\".  -b can be used to set a default value.  If a\n"
"value larger than 0 is given, it is used if no timecode master is present.\n"
"Combined with -B, it will override/ignore the JACK timecode master, and only\n"
"act on transport state alone. Midiclocker never acts as timecode master.\n"
"\n"
;

static const std::string s_help_paragraph_2 =
"Song position information is sent only if a timecode master is present and\n"
"the -P option is not given.\n"
"\n"
;

static const std::string s_help_paragraph_3 =
"To allow external synths to sync accurately to song-position, there is a 2-\n"
"second delay between the 'song position changed' message (not a MIDI realtime\n"
"message) and the 'Continue transport' message. The -d option can change this\n"
"delay, and is only relevant if playback starts at a bar|beat|tick other than\n"
"1|1|0, in which case a 'start' message is sent immediately.\n"
"\n"
;

static const std::string s_help_paragraph_4 =
"Midiclocker runs until it receives a HUP or INT signal, or jackd terminates.\n"
// "\n"
// "See also: jack_transport(1), jack_mclk_dump(1)\n"
"\n"
;

static const std::string s_help_bug_reports =
"Report bugs to Chris Ahlstrom <ahlstromcj@gmail.com>\n"
"Website: https://github.com/ahlstromcj/Sequencer64/\n"
;

/**
 *  Version information.
 */

static const std::string s_help_version_info =
"Copyright (C) GPL 2009 Gabriel M. Beddingfield <gabriel@teuton.org>\n"
"Copyright (C) GPL 2013 Robin Gareus <robin@gareus.org>\n"
"Refactored for Sequencer64 GPL 2017 Chris Ahlstrom <ahlstromcj@gmail.com>\n"
;

/**
 *  Prints the help text for this application.
 */

static void
usage (int status)
{
    printf
    (
        "%s%s%s%s%s%s%s",
        s_help_intro.c_str(),
        s_help_options.c_str(),
        s_help_paragraph_1.c_str(),
        s_help_paragraph_2.c_str(),
        s_help_paragraph_3.c_str(),
        s_help_paragraph_4.c_str(),
        s_help_bug_reports.c_str()
    );
    exit(status);
}

/**
 *
 */

static int
decode_switches (seq64::midi_clocker mc, int argc, char ** argv)
{
    int c;
    while
    (
        (
            c = getopt_long
            (
                argc, argv,
                "b:"                            /* bpm              */
                "B"                             /* force-bpm        */
                "d:"                            /* resync-delay     */
                "J:"                            /* jittery output   */
                "h"                             /* help             */
                "P"                             /* no-position      */
                "T"                             /* no-transport     */
                "s"                             /* strict-bpm       */
                "V"                             /* version          */
                , long_options, (int *) 0
            )
        ) != EOF
    )
    {
        switch (c)
        {
        case 'b':
            mc.user_bpm(atof(optarg));
            break;

        case 'B':
            mc.force_bpm(true);
            break;

        case 'P':
            mc.no_song_position();
            break;

        case 'd':
            mc.resync_delay(atof(optarg));
            break;

        case 'J':
            mc.jitter_level(atof(optarg));
            break;

        case 'T':
            mc.no_song_transport();
            break;

        case 's':
            mc.tempo_in_qnpm(0);
            break;

        case 'V':
            printf
            (
                "midiclocker version %s\n%s",
                SEQ64_VERSION, s_help_version_info.c_str()
            );
            exit(0);
            break;

        case 'h':
            usage(EXIT_SUCCESS);
            break;

        default:
            usage(EXIT_FAILURE);
            break;
        }
    }
    return optind;
}

/**
 *
 */

int
main (int argc, char ** argv)
{
    seq64::midi_clocker mc;
    decode_switches(mc, argc, argv);
    if (mc.initialize())
    {
        while (optind < argc)
            mc.port_connect(argv[optind++]);

        mc.run();
        mc.cleanup(0);
    }
    return EXIT_SUCCESS;
}

/*
 * midiclocker64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
