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
 * \file          cmdlineopts.cpp
 *
 *  This module moves the command-line options processing in sequencer64.cpp
 *  into the libseq64 library.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-20
 * \updates       2016-09-10
 * \license       GNU GPLv2 or above
 *
 *  The "rc" command-line options override setting that are first read from
 *  the "rc" configuration file.  These modified settings are always saved
 *  when Sequencer64 exits, on the theory that somebody may have modified
 *  these settings in the user-interface (there is currently no "dirty flag"
 *  for this operation), and that command-line modifications to
 *  system-dependent settings such as the JACK setup should be saved for
 *  convenience.
 *
 *  The "user" settings are mostly not available from the command-line
 *  (--bus being one exception).  They, too, are partly system-dependent, but
 *  there is no user-interface for changing the "user" options at this time.
 *  So the "user" configuration file is not saved unless it does not exist in
 *  the first place, or the "--user-save" option isprovided on the ommand
 *  line.
 *
 *  We should backup the old versions of any saved configuration files
 *  at some point.
 */

#include <sstream>
#include "platform_macros.h"

#ifdef PLATFORM_UNIX
#include <getopt.h>
#endif

#include "app_limits.h"                 /* macros for build_details()       */
#include "cmdlineopts.hpp"
#include "file_functions.hpp"           /* file_accessible()                */
#include "optionsfile.hpp"
#include "perform.hpp"
#include "settings.hpp"
#include "userfile.hpp"

namespace seq64
{

/**
 *  Sets up the "hardwired" version text for Sequencer64.  This value
 *  ultimately comes from the configure.ac script.
 *
 *  This was too redundant:
 *
 *  SEQ64_PACKAGE " " SEQ64_VERSION " (" SEQ64_GIT_VERSION ") " __DATE__ "\n"
 */

static const std::string versiontext =
    SEQ64_PACKAGE " " SEQ64_GIT_VERSION " " __DATE__ "\n";

/**
 *  A structure for command parsing that provides the long forms of
 *  command-line arguments, and associates them with their respective
 *  short form.  Note the terminating null structure..
 */

static struct option long_options [] =
{
    {"help",                0, 0, 'h'},
    {"version",             0, 0, 'V'},
    {"home",                required_argument, 0, 'H'}, /* new */
#ifdef SEQ64_LASH_SUPPORT
    {"lash",                0, 0, 'L'},                 /* new */
    {"no-lash",             0, 0, 'n'},                 /* new */
#endif
    {"bus",                 required_argument, 0, 'b'}, /* new */
    {"buss",                required_argument, 0, 'B'}, /* new */
    {"ppqn",                required_argument, 0, 'q'}, /* new */
    {"legacy",              0, 0, 'l'},                 /* new */
    {"show-midi",           0, 0, 's'},
    {"show-keys",           0, 0, 'k'},
    {"inverse",             0, 0, 'K'},
    {"stats",               0, 0, 'S'},
    {"priority",            0, 0, 'p'},
    {"ignore",              required_argument, 0, 'i'},
    {"interaction-method",  required_argument, 0, 'x'},
#ifdef SEQ64_JACK_SUPPORT
    {"jack-transport",      0, 0, 'j'},
    {"jack-master",         0, 0, 'J'},
    {"jack-master-cond",    0, 0, 'C'},
    {"jack-start-mode",     required_argument, 0, 'M'},
    {"jack-session-uuid",   required_argument, 0, 'U'},
#endif
    {"manual-alsa-ports",   0, 0, 'm'},
    {"auto-alsa-ports",     0, 0, 'a'},
    {"reveal-alsa-ports",   0, 0, 'r'},                 /* new */
    {"hide-alsa-ports",     0, 0, 'R'},                 /* new */
    {"alsa",                0, 0, 'A'},                 /* new */
    {"pass-sysex",          0, 0, 'P'},
    {"user-save",           0, 0, 'u'},
    {"record-by-channel",   0, 0, 'd'},                 /* new */
    {"legacy-record",       0, 0, 'D'},                 /* new */
    {"config",              required_argument, 0, 'c'}, /* new */
    {"rc",                  required_argument, 0, 'f'}, /* new */
    {"usr",                 required_argument, 0, 'F'}, /* new */

    /**
     * Legacy command-line options are using underscores, which are confusing
     * and not a GNU standard, as far as we know.  Ugh, prefer the hyphen!
     * But we continue to support them for "backward compatibility".
     */

#ifdef SEQ64_JACK_SUPPORT
    {"jack_transport",      0, 0, '1'},                 /* underscores!     */
    {"jack_master",         0, 0, '2'},                 /* underscores!     */
    {"jack_master_cond",    0, 0, '3'},                 /* underscores!     */
    {"jack_start_mode",     required_argument, 0, '4'}, /* underscores!     */
    {"jack_session_uuid",   required_argument, 0, '5'}, /* underscores!     */
#endif

    {"show_keys",           0, 0, '6'},                 /* underscores!     */
    {"interaction_method",  required_argument, 0, '7'}, /* underscores!     */
    {"manual_alsa_ports",   0, 0, '8'},                 /* underscores!     */
    {"pass_sysex",          0, 0, '9'},                 /* underscores!     */
    {"showmidi",            0, 0, '@'},                 /* no separator!    */

    {0, 0, 0, 0}                                        /* terminator       */
};

/**
 *  Provides a complete list of the short options, and is passed to
 *  getopt_long().  The following string keeps track of the characters used so
 *  far.  An 'x' means the character is used; an 'o' means it is used for the
 *  legacy spelling of the option.
 *
 *      0123456789 @AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz
 *       ooooooooo oxxxxxx x  xx  xx xxx xxxxx x  xx xxxxx  xxx    x
 *
 *  Previous arg-list, items missing! "ChVH:lRrb:q:Lni:jJmaAM:pPusSU:x:"
 */

static const std::string s_arg_list =
    "AaB:b:Cc:F:f:H:hi:JjKkLlM:mnPpq:RrSsU:uVx:"        /* modern args      */
    "1234:5:67:89@"                                     /* legacy args      */
    ;

/**
 *  Provides help text.
 */

static const char * const s_help_1a =
"sequencer64 v 0.9.18  A significant reboot of the seq24 live sequencer.\n"
"Usage: sequencer64 [options] [MIDI filename]\n\n"
"Options:\n"
"   -h, --help               Show this message and exit.\n"
"   -V, --version            Show program version information and exit.\n"
"   -H, --home dir           Set the directory to hold the configuration files,\n"
"                            always relative to $HOME.  The default is\n"
"                            .config/sequencer64.\n"
"   -l, --legacy             Write MIDI file in strict Seq24 format.  Same if\n"
"                            Sequencer64 is run as 'seq24'.  Affects some other\n"
"                            options as well.\n"
#ifdef SEQ64_LASH_SUPPORT
"   -L, --lash               Activate built-in LASH support.\n"
"   -n, --no-lash            Do not activate built-in LASH support.\n"
#endif
"   -m, --manual-alsa-ports  Do not attach ALSA ports.  Use when exposing ALSA\n"
"                            ports to JACK (e.g. using a2jmidid).\n"
"   -a, --auto-alsa-ports    Attach ALSA ports (overrides the 'rc' file).\n"
    ;

/**
 *  More help text.
 */

static const char * const s_help_1b =
"   -r, --reveal-alsa-ports  Do not use the 'user' definitions for port names.\n"
"   -R, --hide-alsa-ports    Use the 'user' definitions for port names.\n"
"   -A, --alsa               Do not use JACK, use ALSA. A sticky option.\n"
"   -b, --bus b              Global override of bus number (for testing).\n"
"   -B, --buss b             Avoids the 'bus' versus 'buss' confusion.\n"
"   -q, --ppqn qn            Specify default PPQN to replace 192.  The MIDI\n"
"                            file might specify its own PPQN.\n"
"   -p, --priority           Run high priority, FIFO scheduler (needs root).\n"
"   -P, --pass-sysex         Passes incoming SysEx messages to all outputs.\n"
"                            Not yet fully implemented.\n"
"   -i, --ignore n           Ignore ALSA device number.\n"
"   -s, --show-midi          Dump incoming MIDI events to the screen.\n"
    ;

/**
 *  Still more help text.
 */

static const char * const s_help_2 =
"   -k, --show-keys          Prints pressed key value.\n"
"   -K, --inverse            Inverse (night) color scheme for seq/perf editors.\n"
"   -S, --stats              Show global statistics.\n"
#ifdef SEQ64_JACK_SUPPORT
"   -j, --jack-transport     Synchronize to JACK transport.\n"
"   -J, --jack-master        Try to be JACK Master. Also sets -j.\n"
"   -C, --jack-master-cond   Fail if there's already a Jack Master; sets -j.\n"
"   -M, --jack-start-mode m  When synced to JACK, the following play modes are\n"
"                            available: 0 = live mode; 1 = song mode (default).\n"
" -U, --jack-session-uuid u  Set UUID for JACK session.\n"
" -x, --interaction-method n Set mouse style: 0 = seq24; 1 = fruity. Note that\n"
"                            fruity does not support arrow keys and paint key.\n"
#endif
"   -d, --record-by-channel  Divert MIDI input by channel into the sequences\n"
"                            that are configured for each channel.\n"
"   -D, --legacy-record      Record all MIDI into the active sequence.  The\n"
"                            default at present.\n"
    ;

/**
 *  Still more help text.
 */

static const char * const s_help_3 =
"   -u, --user-save          Save the 'user' configuration settings.  Normally,\n"
"                            they are saved only if the file does not exist, so\n"
"                            that certain 'user' command-line options, such as\n"
"                            --bus, do not become permanent.\n"
"   -f, --rc filename        Use a different 'rc' configuration file.  It must\n"
"                            be a file in the user's $HOME/.config/sequencer64\n"
"                            (or --home) directory.  Not supported by --legacy.\n"
"                            The '.rc' extension is added if needed.\n"
"   -F, --usr filename       Use a different 'usr' configuration file.  Same\n"
"                            rules as for the --rc option. The '.usr'\n"
"                            extension is added if needed.\n"
"   -c, --config basename    Change both 'rc' and 'usr' files.  Any extension\n"
"                            provided is stripped starting at the last period.\n"
"\n"
    ;

/**
 *  Still more help text.
 */

static const char * const s_help_4 =
"--ppqn works pretty well. Saving a MIDI file also saves the PPQN value.\n"
"If no JACK/LASH options are shown above, they were disabled in the build\n"
"configuration. Command-line options can be sticky; most of them\n"
"get saved to the configuration files when Sequencer64 exits.  See the\n"
"user manual at https://github.com/ahlstromcj/sequencer64-doc.\n"
    ;

/**
 *  Checks to see if the first option is a help or version argument, just so
 *  we can skip the "Reading configuration ..." messages.  Also check for the
 *  --legacy option.  Finally, it also checks for the "?" option that people
 *  sometimes use as a guess to get help.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns true only if -V, --version, -h, --help, or "?" were
 *      encountered.  If the legacy options occurred, then
 *      rc().legacy_format(true) is called, as a side effect, because it will
 *      be needed before we parse the options.
 */

bool
help_check (int argc, char * argv [])
{
    bool result = false;
    for ( ; argc > 1; --argc)
    {
        std::string arg = argv[argc - 1];
        if
        (
            (arg == "-h") || (arg == "--help") ||
            (arg == "-V") || (arg == "--version")
        )
        {
            result = true;
        }
        else if (arg == "-l" || arg == "--legacy")
        {
            seq64::rc().legacy_format(true);
        }
        else if (arg == "?")
        {
            printf(s_help_1a);
            printf(s_help_1b);
            printf(s_help_2);
            printf(s_help_3);
            printf(s_help_4);
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Provides the command-line option support, as well as some setup support,
 *  extracted from the main routine of Sequencer64.
 *
 *  It probably requires this call preceding: Gtk::Main kit(argc, argv),
 *  to strip any GTK+-specific parameters the knowledgeable user may have
 *  added.  Usage:
 *
 *      Gtk::Main kit(argc, argv);
 *      seq64::gui_assistant_gtk2 gui;
 *      seq64::perform p(gui);
 *
 *  It also requires the caller to call rc().set_defaults() and
 *  usr().set_defaults().  The caller can then use the command-line to make
 *  any modifications to the setting that will be used here.  The biggest
 *  example is the -r/--reveal-alsa-ports option, which determines if the MIDI
 *  buss definition strings are read from the 'user' configuration file.
 *
 *  Instead of the legacy Seq24 names, we use the new configuration
 *  file-names, located in the ~/.config/sequencer64 directory. However, if
 *  they are not found, we no longer fall back to the legacy configuration
 *  file-names.  If the --legacy option is in force, use only the legacy
 *  configuration file-name.  The code also ensures the directory exists.
 *  CURRENTLY LINUX-SPECIFIC.  See the rc_settings class for how this works.
 *
\verbatim
        std::string cfg_dir = seq64::rc().home_config_directory();
        if (cfg_dir.empty())
            return EXIT_FAILURE;
\endverbatim
 *
 * \change ca 2016-04-03
 *      We were parsing the user-file first, but we now need to parse
 *      the rc-file first, to get the manual-alsa-ports option, so that we can
 *      avoid overriding the port names that the ALSA system provides, if the
 *      manual-alsa-option is false.
 *
 * \param p
 *      Provides the perform object that will be affected by the new
 *      parameters.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns true if the reading of both configuration files succeeded.
 */

bool
parse_options_files (perform & p, int argc, char * argv [])
{
    bool result = true;
    std::string rcname = seq64::rc().config_filespec();

    /*
     * The caller must make these calls, at the appropriate time, which is
     * before any parsing of the command-line options.
     *
     * seq64::rc().set_defaults();     // start out with normal values
     * seq64::usr().set_defaults();    // ditto
     */

    if (file_accessible(rcname))
    {
        printf("[Reading rc configuration %s]\n", rcname.c_str());
        seq64::optionsfile options(rcname);
        if (options.parse(p))
        {
            // Nothing to do?
        }
        else
            result = false;
    }
    rcname = seq64::rc().user_filespec();
    if (file_accessible(rcname))
    {
        printf("[Reading user configuration %s]\n", rcname.c_str());
        seq64::userfile ufile(rcname);
        if (ufile.parse(p))
        {
            // Nothing to do?
        }
        else
            result = false;
    }
    return result;
}

/**
 *  Parses the command-line options on behalf of the application.  Note that,
 *  since we call this function twice (once before the configuration files are
 *  parsed, and once after), we have to make sure that the global value optind
 *  is reset to 0 before calling this function.  Note that the traditional
 *  reset value for optind is 1, but 0 is used in GNU code to trigger the
 *  internal initialization routine of get_opt().
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns the value of optind if no help-related options were provided.
 */

int
parse_command_line_options (perform & p, int argc, char * argv [])
{
    int result = 0;
    optind = 0;
    for (;;)                                /* parse all command parameters */
    {
        int option_index = 0;               /* getopt_long index storage    */
        int c = getopt_long
        (
            argc, argv,
            s_arg_list.c_str(),             /* "Chlrb:q:Li:jJmaM:pPsSU:Vx:" */
            long_options, &option_index
        );
        if (c == -1)                        /* detect the end of options    */
            break;

        switch (c)
        {
        case 'A':
            seq64::rc().with_jack_transport(false);
            seq64::rc().with_jack_master(false);
            seq64::rc().with_jack_master_cond(false);
            break;

        case 'a':
            seq64::rc().manual_alsa_ports(false);
            break;

        case 'B':                           /* --buss for the oldsters      */
        case 'b':                           /* --bus for the youngsters     */
            seq64::usr().midi_buss_override(char(atoi(optarg)));
            break;

        case 'C':
        case '3':
            seq64::rc().with_jack_transport(true);
            seq64::rc().with_jack_master(false);
            seq64::rc().with_jack_master_cond(true);
            break;

        case 'c':                           /* --config option              */
            seq64::rc().set_config_files(optarg);
            break;

        case 'D':                           /* --legacy-record option       */
            seq64::rc().filter_by_channel(false);
            break;

        case 'd':                           /* --record-by-channel option   */
            seq64::rc().filter_by_channel(true);
            break;

        case 'F':                           /* --usr option                 */
            seq64::rc().user_filename(optarg);
            break;

        case 'f':                           /* --rc option                  */
            seq64::rc().config_filename(optarg);
            break;

        case 'H':
            seq64::rc().config_directory(optarg);
            printf("Set home to %s.\n", seq64::rc().config_directory().c_str());
            break;

        case 'h':
            printf(s_help_1a);
            printf(s_help_1b);
            printf(s_help_2);
            printf(s_help_3);
            printf(s_help_4);
            result = SEQ64_NULL_OPTION_INDEX;
            break;

        case 'i':                           /* ignore ALSA device           */
            seq64::rc().device_ignore(true);
            seq64::rc().device_ignore_num(atoi(optarg));
            break;

        case 'J':
        case '2':
            seq64::rc().with_jack_transport(true);
            seq64::rc().with_jack_master(true);
            seq64::rc().with_jack_master_cond(false);
            break;

        case 'j':
        case '1':
            seq64::rc().with_jack_transport(true);
            break;

        case 'k':
        case '6':
            seq64::rc().print_keys(true);
            break;

        case 'K':
            seq64::usr().inverse_colors(true);
            break;

        case 'L':
            seq64::rc().lash_support(true);
            printf("Activating LASH support.\n");
            break;

        case 'l':
            seq64::rc().legacy_format(true);
            seq64::rc().filter_by_channel(false);
            printf
            (
                "Setting legacy seq24 file format for configuration, "
                "writing, and recording.\n"
            );
            break;

        case 'M':
        case '4':
            p.song_start_mode(atoi(optarg) > 0);
            break;

        case 'm':
        case '8':
            seq64::rc().manual_alsa_ports(true);
            break;

        case 'n':
            seq64::rc().lash_support(false);
            printf("Deactivating LASH support.\n");
            break;

        case 'P':
        case '9':
            seq64::rc().pass_sysex(true);
            break;

        case 'p':
            seq64::rc().priority(true);
            break;

        case 'q':
            seq64::usr().midi_ppqn(atoi(optarg));
            break;

        case 'R':
            seq64::rc().reveal_alsa_ports(false);
            printf("Showing user-configured ALSA ports.\n");
            break;

        case 'r':
            seq64::rc().reveal_alsa_ports(true);
            printf("Showing native ALSA ports.\n");
            break;

        case 'S':
            seq64::rc().stats(true);
            break;

        case 's':
            seq64::rc().show_midi(true);
            break;

        case 'U':
        case '5':
            seq64::rc().jack_session_uuid(std::string(optarg));
            break;

        case 'u':
            seq64::usr().save_user_config(true);    /* usr(), not rc()! */
            break;

        case 'V':
            printf("%s", versiontext.c_str());
            printf("%s", build_details().c_str());
            result = SEQ64_NULL_OPTION_INDEX;
            break;

        case 'x':
        case '7':
            seq64::rc().interaction_method
            (
                seq64::interaction_method_t(atoi(optarg))
            );
            break;

        default:
            break;
        }
    }
    if (result != SEQ64_NULL_OPTION_INDEX)
    {
        std::size_t applen = strlen("seq24");
        std::string appname(argv[0]);           /* "seq24", "./seq24", etc. */
        appname = appname.substr(appname.size()-applen, applen);
        if (appname == "seq24")
        {
            seq64::rc().legacy_format(true);
            printf("Setting legacy seq24 file format.\n");
        }
        result = optind;
    }
    return result;
}

/**
 *  Saves all options to the "rc" and "user" configuration files.
 *  This function gets any legacy global variables, on the theory that they
 *  might have been changed.
 *
 * \param p
 *      Provides the perform object that may provide new values for the
 *      parameters.
 *
 * \return
 *      Returns true if both files were saved successfully.  Otherwise returns
 *      false.  But even if one write failed, the other might have succeeded.
 */

bool
write_options_files (const perform & p)
{
    bool result = true;

    /*
     * Write the configuration file to the new name, unless the
     * --legacy option is in force.
     */

    std::string rcname = seq64::rc().config_filespec();
    printf("[Writing rc configuration %s]\n", rcname.c_str());
    seq64::optionsfile options(rcname);
    if (options.write(p))
    {
        // Anything to do?
    }
    else
        result = false;

    bool cansave = usr().save_user_config();
    rcname = seq64::rc().user_filespec();
    if (! cansave)
        cansave = ! file_exists(rcname);

    if (cansave)
    {
        printf("[Writing user configuration %s]\n", rcname.c_str());
        seq64::userfile userstuff(rcname);
        if (userstuff.write(p))
        {
            // Anything to do?
        }
        else
            result = false;
    }
    return result;
}

/**
 *  This section of variables provide static information about the options
 *  enabled or disabled during the build.
 */

#ifdef SEQ64_HIGHLIGHT_EMPTY_SEQS
const static std::string s_build_highlight_empty = "on";
#else
const static std::string s_build_highlight_empty = "off";
#endif

#ifdef SEQ64_LASH_SUPPORT
const static std::string s_build_lash_support = "on";
#else
const static std::string s_build_lash_support = "off";
#endif

#ifdef SEQ64_JACK_SUPPORT
const static std::string s_build_jack_support = "on";
#else
const static std::string s_build_jack_support = "off";
#endif

#ifdef SEQ64_JACK_SESSION
const static std::string s_build_jack_session = "on";
#else
const static std::string s_build_jack_session = "off";
#endif

#ifdef SEQ64_PAUSE_SUPPORT
const static std::string s_build_pause_support = "on";
#else
const static std::string s_build_pause_support = "off";
#endif

#ifdef SEQ64_USE_EVENT_MAP
const static std::string s_build_use_event_map = "on";
#else
const static std::string s_build_use_event_map = "off";
#endif

#ifdef SEQ64_STAZED_CHORD_GENERATOR
const static std::string s_build_chord_generator = "on";
#else
const static std::string s_build_chord_generator = "off";
#endif

#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
const static std::string s_build_edit_highlight = "on";
#else
const static std::string s_build_edit_highlight = "off";
#endif

const static std::string s_build_timesig_tempo = "on (permanent)";

#ifdef SEQ64_USE_MIDI_VECTOR
const static std::string s_build_midi_vector = "on";
#else
const static std::string s_build_midi_vector = "off";
#endif

#ifdef SEQ64_SOLID_PIANOROLL_GRID
const static std::string s_build_solid_grid = "on";
#else
const static std::string s_build_solid_grid = "off";
#endif

#ifdef SEQ64_FOLLOW_PROGRESS_BAR
const static std::string s_build_follow_progress = "on";
#else
const static std::string s_build_follow_progress = "off";
#endif

#ifdef SEQ64_STATISTICS_SUPPORT
const static std::string s_statistics_support = "on";
#else
const static std::string s_statistics_support = "off";
#endif

/*
 * Still EXPERIMENTAL/UNOFFICIAL support.
 */

#ifdef USE_STAZED_JACK_SUPPORT
const static std::string s_seq32_jack_support = "on";
#else
const static std::string s_seq32_jack_support = "off";
#endif

#ifdef USE_STAZED_TRANSPORT
const static std::string s_seq32_transport = "on";
#else
const static std::string s_seq32_transport = "off";
#endif

#ifdef USE_STAZED_SONG_MODE_BUTTON
const static std::string s_seq32_song_button = "on";
#else
const static std::string s_seq32_song_button = "off";
#endif

/**
 *  Generates a string describing the features of the build.
 *
 * \return
 *      Returns an ordered, human-readable string enumerating the built-in
 *      features of this application.
 */

std::string
build_details ()
{
    std::ostringstream result;
    result
<< "Build features:" << std::endl
<< "  Highlight empty sequences: " << s_build_highlight_empty << std::endl
<< "  LASH support:              " << s_build_lash_support << std::endl
<< "  JACK support:              " << s_build_jack_support << std::endl
<< "  JACK session:              " << s_build_jack_session << std::endl
<< "  Pause support:             " << s_build_pause_support << std::endl
<< "  Stazed chord generator:    " << s_build_chord_generator << std::endl
<< "  Event multimap (vs list):  " << s_build_use_event_map << std::endl
<< "  Highlight pattern in edit: " << s_build_edit_highlight << std::endl
<< "  Save time-signature/tempo: " << s_build_timesig_tempo << std::endl
<< "  Use MIDI vector (vs list): " << s_build_midi_vector << std::endl
<< "  Solid piano-roll grid:     " << s_build_solid_grid << std::endl
<< "  Follow progress bar:       " << s_build_follow_progress << std::endl
<< "  Statistics support:        " << s_statistics_support << std::endl
<< "  Seq32 JACK support (exp):  " << s_seq32_jack_support << std::endl
<< "  Seq32 transport (exp):     " << s_seq32_transport << std::endl
<< "  Seq32 song button (exp):   " << s_seq32_song_button << std::endl
    ;
    return result.str();
}

}           // namespace seq64

/*
 * cmdlineopts.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

