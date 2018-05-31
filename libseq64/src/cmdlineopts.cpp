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
 * \updates       2018-05-30
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
#include <stdlib.h>                     /* atoi(), atof(), 32-bit old gcc   */
#include <string.h>                     /* strlen() <gasp!>                 */
#include "easy_macros.h"

#if defined PLATFORM_UNIX || defined PLATFORM_MINGW
#include <getopt.h>
#endif

#include "app_limits.h"                 /* macros for build_details()       */
#include "cmdlineopts.hpp"
#include "daemonize.hpp"                /* seqg4::reroute_stdio()           */
#include "file_functions.hpp"           /* file_accessible()                */
#include "optionsfile.hpp"
#include "perform.hpp"
#include "settings.hpp"
#include "userfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Sets up the "hardwired" version text for Sequencer64.  This value
 *  ultimately comes from the configure.ac script.
 *
 *  This was too redundant:
 *
 *  SEQ64_PACKAGE " " SEQ64_VERSION " (" SEQ64_GIT_VERSION ") " __DATE__ "\n"
 *
 *  This is out-of-date:
 *
 *  SEQ64_PACKAGE " " SEQ64_GIT_VERSION " " __DATE__ "\n";
 */

static const std::string versiontext =
    SEQ64_APP_NAME " " SEQ64_GIT_VERSION " "
    SEQ64_VERSION_DATE_SHORT "\n"
    ;

/**
 *  A structure for command parsing that provides the long forms of
 *  command-line arguments, and associates them with their respective
 *  short form.  Note the terminating null structure..
 */

static struct option long_options [] =
{
    {"help",                0, 0, 'h'},
    {"version",             0, 0, 'V'},
    {"verbose",             0, 0, 'v'},
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
    {"no-jack-midi",        0, 0, 'N'},
    {"jack-midi",           0, 0, 't'},
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

    /*
     * New app-specific options, for easier expansion.  The -o/--option
     * processing is mostly handled outside of the get-opt setup, because it
     * can disable detection of a MIDI file-name argument.
     */

    {"option",              0, 0, 'o'},                 /* expansion!       */
    {0, 0, 0, 0}                                        /* terminator       */
};

/**
 *  Provides a complete list of the short options, and is passed to
 *  getopt_long().  The following string keeps track of the characters used so
 *  far.  An 'x' means the character is used; an 'o' means it is used for the
 *  legacy spelling of the option, which uses underscores instead of hyphens.
 *  An 'a' indicates we could repurpose the key with minimal impact. An
 *  asterisk indicates the option is reserved for application-specific
 *  options.  Currently we will use it for options like "daemonize" in the
 *  seq64cli application.
 *
\verbatim
        0123456789 @AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz#
         ooooooooo oxxxxxx x  xx  xx xxx xxxxxxx *xx xxxxx xxxxx   x    x
\endverbatim
 *
 *  Previous arg-list, items missing! "ChVH:lRrb:q:Lni:jJmaAM:pPusSU:x:"
 *
 *  * Also note that 'o' options argument cannot be included here due to
 *  issues involving parse_o_options(), but it is *reserved* here, without the
 *  argument indicator.
 */

static const std::string s_arg_list =
    "AaB:b:Cc:F:f:H:hi:JjKkLlM:mNnoPpq:RrtSsU:uVvx:#"   /* modern args      */
    "1234:5:67:89@"                                     /* legacy args      */
    ;

/**
 *  Provides help text.
 */

static const char * const s_help_1a =
SEQ64_APP_NAME " v " SEQ64_VERSION
" A reboot of the seq24 live sequencer.\n"
"Usage: " SEQ64_APP_NAME " [options] [MIDI filename]\n\n"
"Options:\n"
"   -h, --help               Show this message and exit.\n"
"   -V, --version            Show program version/build  information and exit.\n"
"   -v, --verbose            Verbose mode, show more data to the console.\n"
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
"   -m, --manual-alsa-ports  Don't attach system ALSA ports. Use virtual ports.\n"
"                            Not supported in the PortMIDI version.\n"
"   -a, --auto-alsa-ports    Attach ALSA ports (overrides the 'rc' file).\n"
"                            Use to expose system ALSA ports to JACK (e.g.\n"
"                            using a2jmidid).\n"
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
"   -N, --no-jack-midi       Use ALSA MIDI, even with JACK Transport. See -A.\n"
"   -t, --jack-midi          Use JACK MIDI; separate option from JACK Transport.\n"
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
 *  Still still more help text.
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
"   -o, --option optoken     Provides app-specific options for expansion.  The\n"
"                            options supported are:\n"
"\n"
    ;

/**
 *  Still still more more help text.
 */

static const char * const s_help_4 =
"              log=filename  Redirect console output to a log file in the\n"
"                            --home directory [$HOME/.config/sequencer64].\n"
"                            If '=filename' is not provided, then the filename\n"
"                            specified in '[user-options]' in the 'usr' file is\n"
"                            used.\n"
#if defined SEQ64_MULTI_MAINWID
"              wid=RxC,F     Show R rows of sets, C columns of sets, and set\n"
"                            the sync-status of the set blocks. R can range\n"
"              (e.g          from 1 to 3, C can range from 1 to 2, and the sync\n"
"               'wid=3x2,t') flag F can be true, false, or 'indep' (the same\n"
"                            as false).  The flag sets the multiple windows so\n"
"                            that they stay in step with each other, and the\n"
"                            multi-windows use consecutive set numbers.\n"
"                            The upper left mainwid is always the active one.\n"
#endif
"              sets=RxC      Modifies the rows and columns in a set from the\n"
"                            default of 4x8.  Supported values of R are 4 to 8,\n"
"                            and C can range from 8 to 12. If not 4x8, seq64 is\n"
"                            in 'variset' mode. Affects mute groups, too.\n"
"\n"
" seq64cli:\n"
"              daemonize     Makes this application fork to the background.\n"
"              no-daemonize  Or not.  These options do not apply to Windows.\n"
"\n"
"The 'daemonize' option works only in the CLI build. The 'sets' option works in\n"
"the CLI build as well.  Specify the '--user-save' option to make these options\n"
"permanent in the sequencer64.usr configuration file.\n"
"\n"
    ;

/**
 *  Still still still more more more help text.
 */

static const char * const s_help_5 =
"--ppqn works pretty well. Saving a MIDI file also saves the PPQN value.\n"
"If no JACK/LASH options are shown above, they were disabled in the build\n"
"configuration. Command-line options can be sticky; most of them\n"
"get saved to the configuration files when Sequencer64 exits.  See the\n"
"user manual at https://github.com/ahlstromcj/sequencer64-doc.\n"
    ;

/**
 *  Gets a compound option argument.  An option argument is a value flagged on
 *  the command line by the -o/--option options.  Each option has a value
 *  associated with it, as the next argument on the command-line.  A compound
 *  option is of the form name=value, of which one example is:
 *
 *      log=filename
 *
 *  This function extracts both the name and the value.  If the name is empty,
 *  then the option is unsupported and should be ignored.  If the value is
 *  empty, then there may be a default value to use.
 *
 * \param compound
 *      The putative compound option.
 *
 * \param [out] optionname
 *      This value is filled in with the name of the option-value, if there
 *      is an equals sign in the \a compound parameter.
 *
 * \return
 *      Returns the value part of the compound option, or just the compound
 *      parameter is there is no '=' sign.  That is, it returns the
 *      option-value.
 *
 * \sideeffect
 *      The name portion is returned in the optionname parameter.
 */

static std::string
get_compound_option
(
    const std::string & compound,
    std::string & optionname
)
{
    std::string value;
    std::size_t eqpos = compound.find_first_of("=");
    if (eqpos == std::string::npos)
    {
        optionname.clear();
        value = compound;
    }
    else
    {
        optionname = compound.substr(0, eqpos); /* beginning to eqpos chars */
        value = compound.substr(eqpos + 1);     /* rest of the string       */
    }
    return value;
}

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
 *      Returns true only if -v, -V, --version, -#, -h, --help, or "?" were
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
            (arg == "-V") || (arg == "--version") || (arg == "--V") ||
            (arg == "-#")       /*  || (arg == "-v") || (arg == "--v")  */
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
            printf(s_help_5);
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Checks the putative command-line arguments for any of the new "options"
 *  options.  These are flagged by "-o" or "--option".  These options are then
 *  passed to the user_settings module.  Multiple options can be supplied; the
 *  whole command-line is processed.
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if any "options" option was found, and false otherwise.
 *      If the options flags ("-o" or "--option") were found, but were not
 *      valid options, then we break out of processing and return false.
 */

bool
parse_o_options (int argc, char * argv [])
{
    bool result = false;
    if (argc > 1 && not_nullptr(argv))
    {
        int argn = 1;
        std::string arg;
        std::string optionname;
        while (argn < argc)
        {
            if (not_nullptr(argv[argn]))
            {
                arg = argv[argn];
                if ((arg == "-o") || (arg == "--option"))
                {
                    ++argn;
                    if (argn < argc && not_nullptr(argv[argn]))
                    {
                        arg = get_compound_option(argv[argn], optionname);
                        if (optionname.empty())
                        {
                            if (arg == "daemonize")
                            {
                                result = true;
                                usr().option_daemonize(true);
                            }
                            else if (arg == "no-daemonize")
                            {
                                result = true;
                                usr().option_daemonize(false);
                            }
                            else if (arg == "log")
                            {
                                /*
                                 * Without a filename, just turn on the
                                 * log-file flag, using the name already read
                                 * from the "[user-options]" section of the
                                 * "usr" file.
                                 */

                                result = true;
                                usr().option_use_logfile(true);
                            }
                        }
                        else
                        {
                            /*
                             * \tricky
                             *      Note that 'arg' is used in the clause
                             *      above, but 'optionname' is used here.
                             */

                            if (optionname == "log")
                            {
                                result = true;
                                usr().option_logfile(arg);
                                if (! arg.empty())
                                    usr().option_use_logfile(true);
                            }
#if defined SEQ64_MULTI_MAINWID
                            else if (optionname == "wid")
                            {
                                /*
                                 * The arg should be of the form "rxc[,b]",
                                 * stricty, 1 digit max each number.
                                 */

                                if (arg.length() >= 3)
                                {
                                    int rows = atoi(arg.c_str());
                                    int cols = atoi(arg.substr(2, 1).c_str());
                                    char flag = arg[4];
                                    result = true;
                                    if (rows > 0)
                                        usr().block_rows(rows);

                                    if (cols > 0)
                                        usr().block_columns(cols);

                                    bool nosync = flag == 'f' || flag == 'i';
                                    usr().block_independent(nosync);
                                }
                            }
#endif  // SEQ64_MULTI_MAINWID
                            else if (optionname == "sets")
                            {
                                if (arg.length() >= 3)
                                {
                                    int rows = atoi(arg.c_str());
                                    std::string::size_type p =
                                        arg.find_first_of("x");

                                    if (p != std::string::npos)
                                    {
                                        int cols = atoi(arg.substr(p+1).c_str());
                                        usr().mainwnd_rows(rows);
                                        usr().mainwnd_cols(cols);
                                        result = true;
                                    }
                                }
                            }
                            else if (optionname == "scale")
                            {
                                if (arg.length() >= 1)
                                {
                                    float scale = atof(arg.c_str());
                                    usr().window_scale(scale);
                                    result = true;
                                }
                            }
                        }
                        if (! result)
                        {
                            printf("Warning: unsupported --option value\n");
                            break;
                        }
                    }
                }
            }
            else
                break;

            ++argn;
        }
    }
    return result;
}

/**
 *  Checks the putative command-line arguments for the "log" option.  Generally,
 *  this function needs to be called near the beginning of main().  See the
 *  rtmidi version of the main() function, for example.
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if stdio was rerouted to the "usr"-specified log-file.
 */

bool
parse_log_option (int argc, char * argv [])
{
    bool result = false;
    if (parse_o_options(argc, argv))
    {
        std::string logfile = usr().option_logfile();
        if (! logfile.empty())
        {
#ifdef PLATFORM_LINUX_XXX                   /* let main() call this */
            (void) reroute_stdio(logfile);
#endif
            result = true;
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
 *  usr().set_defaults() at the appropriate time, which is before any parsing
 *  of the command-line options.  The caller can then use the command-line to
 *  make any modifications to the setting that will be used here.  The biggest
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
 * \param [out] errmessage
 *      Provides a string into which to dump any error-message that might
 *      occur.  Still not thoroughly supported in the "rc" and "user"
 *      configuration files.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns true if the reading of both configuration files succeeded, or
 *      if they did not exist.  In the latter case, they will be saved as new
 *      files upon exit.
 */

bool
parse_options_files
(
    perform & p,
    std::string & errmessage,
    int argc, char * argv []
)
{
    std::string rcname = seq64::rc().config_filespec();
    bool result = true; // ! rcname.empty();
    if (file_accessible(rcname))
    {
        printf("[Reading rc configuration %s]\n", rcname.c_str());
        seq64::optionsfile options(rcname);
        if (options.parse(p))
        {
            // Nothing to do?
        }
        else
        {
            errmessage = options.get_error_message();
            result = false;
        }
    }
    if (result)
    {
        rcname = seq64::rc().user_filespec();
        if (file_accessible(rcname))
        {
            printf("[Reading user configuration %s]\n", rcname.c_str());
            seq64::userfile ufile(rcname);
            if (ufile.parse(p))
            {
                /*
                 * Since we are parsing this file after the creation of the
                 * perform object, we may need to modify some perform members
                 * here.  These changes must also be made after parsing the
                 * command line in the main module (e.g. in seq64rtmidi.cpp).
                 */

                p.seqs_in_set(usr().seqs_in_set());
                p.max_sets(usr().max_sets());
            }
            else
            {
                errmessage = ufile.get_error_message();
                result = false;
            }
        }
    }
    return result;
}

/**
 *  Like parse_options_files(), but reads only the [mute-group] section.
 *
 * \param p
 *      The perform object to alter by reading the mute-groups.
 *
 * \param errmessage
 *      A return parameter for any error message that might occur.
 *
 * \return
 *      Returns true if no errors occurred in reading the mute-groups.
 *      If not true, the caller should output the error message.
 */

bool
parse_mute_groups (perform & p, std::string & errmessage)
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
        printf("[Reading mute-group section from %s]\n", rcname.c_str());
        seq64::optionsfile options(rcname);
        if (options.parse_mute_group_section(p))
        {
            // Nothing to do?
        }
        else
        {
            errmessage = options.get_error_message();
            result = false;
        }
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
 * \param p
 *      The performance object that implements some of the command-line
 *      options.
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
    std::string optionval;                  /* used only with -o options    */
    std::string optionname;                 /* ditto                        */
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
            seq64::rc().with_jack_midi(false);
            printf("Forcing all-ALSA mode.\n");
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
            p.filter_by_channel(false);     /* important! */
            break;

        case 'd':                           /* --record-by-channel option   */
            seq64::rc().filter_by_channel(true);
            p.filter_by_channel(true);      /* important! */
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
            printf("[Activating LASH support]\n");
            break;

        case 'l':
            seq64::rc().legacy_format(true);
            seq64::rc().filter_by_channel(false);
            p.filter_by_channel(false);     /* important! */
            printf("[Setting legacy seq24 format/operation]\n");
            break;

        case 'M':
        case '4':
            p.song_start_mode(atoi(optarg) > 0);
            break;

        case 'm':
        case '8':
            seq64::rc().manual_alsa_ports(true);
            break;

        case 'N':
            seq64::rc().with_jack_midi(false);
            printf("[Deactivating JACK MIDI]\n");
            break;

        case 'n':
            seq64::rc().lash_support(false);
            printf("Deactivating LASH support.\n");
            break;

        case 'o':

            /*
             * We now handle this processing separately and first, in the
             * parse_o_option() function.  Doing it here can mess up parsing.
             * We need to skip the argument in case there are other arguments
             * or a MIDI filename following the compound option.
             */

            ++optind;
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
            printf("[Showing user-configured port names]\n");
            break;

        case 'r':
            seq64::rc().reveal_alsa_ports(true);
            printf("[Showing native system port names]\n");
            break;

        case 'S':
            seq64::rc().stats(true);
            break;

        case 's':
            seq64::rc().show_midi(true);
            break;

        case 't':
            seq64::rc().with_jack_midi(true);
            printf("[Activating native JACK MIDI]\n");
            break;

        case 'U':
        case '5':
            seq64::rc().jack_session_uuid(std::string(optarg));
            break;

        case 'u':
            seq64::usr().save_user_config(true);    /* usr(), not rc()! */
            break;

        case 'v':
            seq64::rc().verbose_option(true);
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

        case '#':
            printf("%s\n", SEQ64_VERSION);
            result = SEQ64_NULL_OPTION_INDEX;
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
            printf("[Setting legacy seq24 format/operation]\n");
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

#ifdef PLATFORM_32_BIT
const static std::string s_bitness = "32-bit";
#else
const static std::string s_bitness = "64-bit";
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
        << "Build features:" << std::endl << std::endl
#ifdef SEQ64_RTMIDI_SUPPORT
        << "Native JACK/ALSA (rtmidi) on" << std::endl
#endif
#ifdef SEQ64_ALSAMIDI_SUPPORT
        << "ALSA-only MIDI support on" << std::endl
#endif
#ifdef SEQ64_PORTMIDI_SUPPORT
        << "PortMIDI support on" << std::endl
#endif
#ifdef SEQ64_ENABLE_EVENT_EDITOR
        << "Event editor on" << std::endl
#endif
#ifdef SEQ64_USE_EVENT_MAP
        << "Event multimap (vs list) on" << std::endl
#endif
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
        << "Follow progress bar on" << std::endl
#endif
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
        << "Highlight edit pattern on" << std::endl
#endif
#ifdef SEQ64_HIGHLIGHT_EMPTY_SEQS
        << "Highlight empty patterns on" << std::endl
#endif
#ifdef SEQ64_JACK_SESSION
        << "JACK session on" << std::endl
#endif
#ifdef SEQ64_JACK_SUPPORT
        << "JACK support on" << std::endl
#endif
#ifdef SEQ64_LASH_SUPPORT
        << "LASH support on" << std::endl
#endif
#ifdef SEQ64_USE_MIDI_VECTOR
        << "MIDI vector (vs list) on" << std::endl
#endif
#ifdef SEQ64_STAZED_CHORD_GENERATOR
        << "Seq32 chord generator on" << std::endl
#endif
#ifdef SEQ64_STAZED_LFO_SUPPORT
        << "Seq32 LFO window on" << std::endl
#endif
#ifdef SEQ64_STAZED_MENU_BUTTONS
        << "Seq32 menu buttons on" << std::endl
#endif
#ifdef SEQ64_STAZED_TRANSPOSE
        << "Seq32 transpose on" << std::endl
#endif
#ifdef SEQ64_MAINWND_TAP_BUTTON
        << "BPM Tap button on" << std::endl
#endif
#ifdef SEQ64_SOLID_PIANOROLL_GRID
        << "Solid piano-roll grid on" << std::endl
#endif
#ifdef SEQ64_JE_PATTERN_PANEL_SCROLLBARS
        << "Main window scroll-bars on" << std::endl
#endif
#ifdef SEQ64_SHOW_COLOR_PALETTE
        << "Optional pattern coloring on" << std::endl
#endif
#ifdef SEQ64_MULTI_MAINWID
        << "Multiple main windows on" << std::endl
#endif
#ifdef SEQ64_SONG_RECORDING
        << "Song performance recording on" << std::endl
#endif
#ifdef SEQ64_SONG_BOX_SELECT
        << "Box song selection on" << std::endl
#endif
#ifdef SEQ64_STATISTICS_SUPPORT
        << "Statistics support on" << std::endl
#endif
#ifdef PLATFORM_WINDOWS
        << "Windows support on" << std::endl
#endif
        << "Pause support on" << std::endl
        << "Save time-sig/tempo on" << std::endl
#ifdef PLATFORM_DEBUG
        << "Debug code on" << std::endl
#endif
        << s_bitness << " support enabled" << std::endl
        << std::endl
    << "Options are enabled/disabled via the configure script," << std::endl
    << "libseq64/include/seq64_features.h, or the build-specific" << std::endl
    << "seq64-config.h file in include or in include/qt/portmidi" << std::endl
    ;
    return result.str();
}

}           // namespace seq64

/*
 * cmdlineopts.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

