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
 * \updates       2015-11-21
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
 *  So the "user" configuration file is not saved unless it doesn't exist in
 *  the first place, or the "--user-save" option isprovided on the ommand
 *  line.
 *
 *  We should backup the old versions of any saved configuration files
 *  at some point.
 */

#include "platform_macros.h"
#include "cmdlineopts.hpp"
#include "file_functions.hpp"           /* file_accessible()                */

#ifdef PLATFORM_UNIX
#include <getopt.h>
#endif

#include "globals.h"                    /* full platform configuration      */
#include "optionsfile.hpp"
#include "perform.hpp"
#include "userfile.hpp"

namespace seq64
{

static const std::string versiontext =
    SEQ64_PACKAGE " " SEQ64_VERSION " " __DATE__ "\n";

/**
 *  A structure for command parsing that provides the long forms of
 *  command-line arguments, and associates them with their respective
 *  short form.  Note the terminating null structure..
 */

static struct option long_options [] =
{
    {"help",                0, 0, 'h'},
    {"version",             0, 0, 'V'},
#ifdef SEQ64_LASH_SUPPORT
    {"lash",                0, 0, 'L'},                 /* new */
    {"no-lash",             0, 0, 'n'},                 /* new */
#endif
    {"bus",                 required_argument, 0, 'b'}, /* new */
    {"ppqn",                required_argument, 0, 'q'}, /* new */
    {"legacy",              0, 0, 'l'},                 /* new */
    {"show-midi",           0, 0, 's'},
    {"show-keys",           0, 0, 'k'},
    {"stats",               0, 0, 'S'},
    {"priority",            0, 0, 'p'},
    {"ignore",              required_argument, 0, 'i'},
    {"interaction-method",  required_argument, 0, 'x'},
#ifdef SEQ64_JACK_SUPPORT
    {"jack-transport",      0, 0, 'j'},
    {"jack-master",         0, 0, 'J'},
    {"jack-master_cond",    0, 0, 'C'},
    {"jack-start_mode",     required_argument, 0, 'M'},
    {"jack-session_uuid",   required_argument, 0, 'U'},
#endif
    {"manual-alsa-ports",   0, 0, 'm'},
    {"auto-alsa-ports",     0, 0, 'a'},
    {"pass-sysex",          0, 0, 'P'},
    {"user-save",           0, 0, 'u'},

    /**
     * Legacy command-line options are using underscores, which are confusing
     * and not a GNU standard, as far as we know.  Ugh, prefer the hyphen!
     * But we continue to support them for "backwards compatibility".
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
 *  getopt_long().
 */

static const std::string s_arg_list =
    "Chlb:q:Lni:jJmaM:pPusSU:Vx:"                        /* good args        */
    "1234:5:67:89@"                                     /* legacy args      */
    ;

static const char * const s_help_1a =
"sequencer64 v 0.9.9.10 A significant refactoring of the seq24 live sequencer.\n"
"\n"
"Usage: sequencer64 [options] [MIDI filename]\n\n"
"Options:\n"
"   -h, --help               Show this message.\n"
"   -V, --version            Show program version information.\n"
"   -l, --legacy             Write MIDI file in old Seq24 format.  Also set\n"
"                            if Sequencer64 is called as 'seq24'.\n"
#ifdef SEQ64_LASH_SUPPORT
"   -L, --lash               Activate built-in LASH support.\n"
"   -n, --no-lash            Do not activate built-in LASH support.\n"
#endif
"   -m, --manual-alsa-ports  Don't attach ALSA ports.\n"
"   -a, --auto-alsa-ports    Attach ALSA ports (overrides the 'rc' file).\n"
    ;

static const char * const s_help_1b =
"   -b, --bus b              Global override of bus number (for testing).\n"
"   -q, --ppqn qn            Specify new default PPQN, or 'file'.  Note that\n"
"                            the legacy default is 192.\n"
"   -p, --priority           Run high priority, FIFO scheduler (needs root).\n"
"   -P, --pass-sysex         Passes incoming SysEx messages to all outputs.\n"
"                            IS THIS SUPPORTED?\n"
"   -i, --ignore n           Ignore ALSA device number.\n"
"   -s, --show-midi          Dump incoming MIDI events to the screen.\n"
    ;

static const char * const s_help_2 =
"   -k, --show-keys          Prints pressed key value.\n"
"   -S, --stats              Show global statistics.\n"
#ifdef SEQ64_JACK_SUPPORT
"   -j, --jack-transport     Synchronize to JACK transport.\n"
"   -J, --jack-master        Try to be JACK master.\n"
"   -C, --jack-master-cond   JACK master will fail if there's already a master.\n"
"   -M, --jack-start-mode m  When synced to JACK, the following play modes are\n"
"                            available: 0 = live mode; 1 = song mode (default).\n"
" -U, --jack-session-uuid u  Set UUID for JACK session.\n"
#endif
" -x, --interaction-method n Set mouse style: 0 = seq24; 1 = fruity. Note that\n"
"                            fruity doesn't support arrow keys and paint key.\n"
    ;

static const char * const s_help_3 =
"   -u, --user-save          Save the 'user' configuration settings.  Normally,\n"
"                            they are saved only if the file does not exist, so\n"
"                            that certain 'user' command-line options, such as\n"
"                            --bus, do not become permanent.\n"
"\n"
    ;

static const char * const s_help_4 =
"Setting --ppqn to double the default causes a MIDI file to play at half\n"
"the speed.  If the file is re-saved with that setting, the double clock is\n"
"saved, but it still plays slowly in Sequencer64, but at its original rate\n"
"in Timidity.  We still have some work to do on PPQN support, obviously.\n"
"\n"
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

/*
 * TRIAL FEATURE.  Back up the data read from the two configuration files.
 *                 THIS NEEDS MORE THOUGHT!
 *
 *  The issue is that Sequencer64 saves all changes to parameters in the
 *  "rc" and "user" configuration files, even ones that originate as
 *  supposedly temporary overrides on the command line.  The most
 *  notorious for me is the buss-override features.
 *
 * seq64::rc_settings rc_backup = seq64::rc();
 * seq64::user_settings usr_backup = seq64::usr();
 */

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
 *  Instead of the Seq24 names, use the new configuration file-names, located
 *  in the ~/.config/sequencer64 directory. However, if they aren't found, we
 *  no longer fall back to the legacy configuration file-names.  If the
 *  --legacy option is in force, use only the legacy configuration file-name.
 *  The code also ensures the directory exists.  CURRENTLY LINUX-SPECIFIC.
 *  See the rc_settings class for how this works.
 *
 *      std::string cfg_dir = seq64::rc().home_config_directory();
 *      if (cfg_dir.empty())
 *          return EXIT_FAILURE;
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
    std::string rcname = seq64::rc().user_filespec();
    seq64::rc().set_defaults();     /* start out with normal values */
    seq64::usr().set_defaults();    /* start out with normal values */
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
    rcname = seq64::rc().config_filespec();
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
    return result;
}

/**
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
parse_command_line_options (int argc, char * argv [])
{
    int result = 0;
    for (;;)                                /* parse all command parameters */
    {
        int option_index = 0;               /* getopt_long index storage    */
        int c = getopt_long
        (
            argc, argv,
            s_arg_list.c_str(),             // "Chlb:q:Li:jJmaM:pPsSU:Vx:",
            long_options, &option_index
        );
        if (c == -1)                        /* detect the end of options    */
            break;

        switch (c)
        {
        case 'h':
            printf(s_help_1a);
            printf(s_help_1b);
            printf(s_help_2);
            printf(s_help_3);
            printf(s_help_4);
            result = SEQ64_NULL_OPTION_INDEX;
            break;

        case 'l':
            seq64::rc().legacy_format(true);
            printf("Setting legacy seq24 file format for writing.\n");
            break;

        case 'L':
            seq64::rc().lash_support(true);
            printf("Activating LASH support.\n");
            break;

        case 'n':
            seq64::rc().lash_support(false);
            printf("Deactivating LASH support.\n");
            break;

        case 'S':
            seq64::rc().stats(true);
            break;

        case 's':
            seq64::rc().show_midi(true);
            break;

        case 'p':
            seq64::rc().priority(true);
            break;

        case 'P':
            seq64::rc().pass_sysex(true);
            break;

        case 'u':
            seq64::usr().save_user_config(true);    /* usr(), not rc()! */
            break;

        case 'k':
            seq64::rc().print_keys(true);
            break;

        case 'j':
            seq64::rc().with_jack_transport(true);
            break;

        case 'J':
            seq64::rc().with_jack_master(true);
            break;

        case 'C':
            seq64::rc().with_jack_master_cond(true);
            break;

        case 'M':
            if (atoi(optarg) > 0)
                seq64::rc().jack_start_mode(true);
            else
                seq64::rc().jack_start_mode(false);
            break;

        case 'm':
            seq64::rc().manual_alsa_ports(true);
            break;

        case 'a':
            seq64::rc().manual_alsa_ports(false);
            break;

        case 'i':                           /* ignore ALSA device */
            seq64::rc().device_ignore(true);
            seq64::rc().device_ignore_num(atoi(optarg));
            break;

        case 'V':
            printf("%s", versiontext.c_str());
            result = SEQ64_NULL_OPTION_INDEX;
            break;

        case 'U':
            seq64::rc().jack_session_uuid(std::string(optarg));
            break;

        case 'x':
            seq64::rc().interaction_method
            (
                seq64::interaction_method_t(atoi(optarg))
            );
            break;

        case 'b':
            seq64::usr().midi_buss_override(char(atoi(optarg)));
            break;

        case 'q':
            if (std::string(optarg) == std::string("file"))
            {
                seq64::usr().midi_ppqn(0);      /* NOT READY */
            }
            else
            {
                seq64::usr().midi_ppqn(atoi(optarg));
            }
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
        seq64::usr().set_globals();             /* copy to legacy globals   */
        result = optind;
    }
    return result;
}

/*
 * TRIAL FEATURE.  Restore the data read from the two configuration files.
 *                 This will remove any "rc" edits made in the Options
 *                 dialog, though.
 *
 *                 THIS NEEDS MORE THOUGHT!
 *
 * seq64::rc_settings & rc_ref = seq64::rc();
 * seq64::user_settings & usr_ref = seq64::usr();
 * rc_ref = rc_backup;
 * usr_ref = usr_backup;
 */

/**
 *  Saves all options to the "rc" and "user" configuration files.
 *
 *  This function gets any legacy global variables, on the theory that they
 *  might have been changed.
 *
 * \return
 *      Returns true if both files were saved successfully.  Even if one write
 *      failed, the other might have succeeded.
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
        seq64::usr().get_globals();             /* copy from legacy globals */
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

}           // namespace seq64

/*
 * cmdlineopts.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

