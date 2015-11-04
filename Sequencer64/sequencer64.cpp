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
 * \file          sequencer64.cpp
 *
 *  This module declares/defines the main module for the application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 * \todo
 *      "I don't like seq24's pianoroll editor, the way you do CC envelopes
 *      isn't ideal, it uses alsa-midi, there's unnecessary complexity in
 *      switching from pattern-trigger mode to song mode, and its insistence
 *      on being transport master while not even being able to adjust tempo
 *      when live is annoying."  The JACK support may need updating/upgrading
 *      as well.
 */

#include <stdio.h>

#include "platform_macros.h"

#ifdef PLATFORM_UNIX
#include <getopt.h>
#endif

#include <gdkmm/cursor.h>
#include <gtkmm/main.h>

#include "globals.h"                    // full platform configuration
#include "font.hpp"
#include "gui_assistant_gtk2.hpp"
#include "mainwid.hpp"                  // needed to fulfill mainwnd
#include "mainwnd.hpp"
#include "optionsfile.hpp"
#include "perform.hpp"
#include "userfile.hpp"

#ifdef SEQ64_LASH_SUPPORT
#include "lash.hpp"                     // seq64::lash & global_lash_driver
#endif

/**
 *  A structure for command parsing that provides the long forms of
 *  command-line arguments, and associates them with their respective
 *  short form.  Note the terminating null structure..
 */

static struct option long_options[] =
{
    {"help",                0, 0, 'h'},
#ifdef SEQ64_LASH_SUPPORT
    {"lash",                0, 0, 'L'},                 /* new 2015-08-27   */
#endif
    {"bus",                 required_argument, 0, 'b'}, /* new 2015-10-14   */
    {"ppqn",                required_argument, 0, 'q'}, /* new 2015-10-15   */
    {"legacy",              0, 0, 'l'},                 /* new 2015-08-16   */
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
    {"version",             0, 0, 'V'},

    /*
     * Legacy options using underscores, which are confusing and not a
     * GNU standard, as far as we know.
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

static const std::string versiontext = SEQ64_PACKAGE " " SEQ64_VERSION "\n";

namespace seq64
{

/*
 * Global pointer!  Declared in font.h.
 */

    font * p_font_renderer = nullptr;

}           // namespace seq64

static const char * const s_help_1a =
"sequencer64 v 0.9.9.6  A refactoring of the seq24 live sequencer.\n\n"
"Usage: sequencer64 [options] [MIDI filename]\n\n"
"Options:\n"
"   -h, --help               Show this message.\n"
"   -v, --version            Show program version information.\n"
"   -l, --legacy             Write MIDI file in old Seq24 format.  Also set\n"
"                            if Sequencer64 is called as 'seq24'.\n"
#ifdef SEQ64_LASH_SUPPORT
"   -L, --lash               Activate built-in LASH support.\n"
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
"\n"
    ;

static const char * const s_help_3 =
"Setting --ppqn to double the default causes a MIDI file to play at half\n"
"the speed.  If the file is re-saved with that setting, the double clock is\n"
"saved, but it still plays slowly in Sequencer64, but at its original rate\n"
"in Timidity.  We still have some work to do on PPQN support, obviously.\n"
"\n"
    ;

/**
 *  The standard C/C++ entry point to this application.  This first thing
 *  this function does is scan the argument vector and strip off all
 *  parameters known to GTK+.
 *
 *  The next thing is to set the various settings defaults, and then try to
 *  read the "user" and "rc" configuration files, in that order.  There are
 *  currently no options to change the names of those files.  If we add
 *  that code, we'll move the parsing code to where the configuration
 *  file-names are changed from the command-line.
 *
 *  The last thing is to override any other settings via the command-line
 *  parameters.
 */

int
main (int argc, char * argv [])
{
    Gtk::Main kit(argc, argv);              /* strip GTK+ parameters        */
    int c;
    bool is_help = false;
    for (int a = 1; a < argc; a++)
    {
        if (strcmp(argv[a], "--help") == 0)
        {
            is_help = true;
            break;
        }
    }
    seq64::g_rc_settings.set_defaults();    /* start out with normal values */
    seq64::g_user_settings.set_defaults();  /* start out with normal values */

    /*
     * Set up objects that are specific to the Gtk-2 GUI.  Pass them to
     * the perform constructor.  Create a font-render object.
     *
     * ISSUE:  We really need to create the perform object after reading
     * the configuration, but we also need to fill it in from the
     * configuration, I believe.
     */

    seq64::gui_assistant_gtk2 gui;              /* GUI-specific objects     */
    seq64::perform p(gui);                      /* main performance object  */
    seq64::p_font_renderer = new seq64::font(); /* set the font renderer    */

    /*
     *  Instead of the Seq24 names, use the new configuration file-names,
     *  located in ~/.config/sequencer64. However, if they aren't found,
     *  we no longer fall back to the legacy configuration file-names.  If
     *  the --legacy option is in force, use only the legacy configuration
     *  file-name.  The code also ensures the directory exists.  CURRENTLY
     *  LINUX-SPECIFIC.  See the rc_settings class for how this works.
     */

    std::string cfg_dir = seq64::g_rc_settings.home_config_directory();
    if (cfg_dir.empty())
        return EXIT_FAILURE;

    std::string rcname = seq64::g_rc_settings.user_filespec();
    if (Glib::file_test(rcname, Glib::FILE_TEST_EXISTS))
    {
        if (! is_help)
            printf("[Reading user configuration %s]\n", rcname.c_str());

        seq64::userfile ufile(rcname);
        if (ufile.parse(p))
        {
            // Nothing to do, and no exit needed here if it fails
        }
    }

    rcname = seq64::g_rc_settings.config_filespec();
    if (Glib::file_test(rcname, Glib::FILE_TEST_EXISTS))
    {
        if (! is_help)
            printf("[Reading rc configuration %s]\n", rcname.c_str());

        seq64::optionsfile options(rcname);
        if (options.parse(p))
        {
            /*
             * Not necessary, and it resets the last-used-directory
             * obtained from the "rc" configuration file.
             *
             * g_rc_settings.last_used_dir(cfg_dir);
             */
        }
        else
            return EXIT_FAILURE;
    }

    for (;;)                                /* parse all command parameters */
    {
        int option_index = 0;               /* getopt_long index storage    */
        c = getopt_long
        (
            argc, argv, "Chlb:q:Li:jJmaM:pPsSU:Vx:",
            long_options, &option_index
        );
        if (c == -1)                        /* detect the end of options    */
            break;

        switch (c)
        {
        case '?':
        case 'h':
            printf(s_help_1a);
            printf(s_help_1b);
            printf(s_help_2);
            printf(s_help_3);
            return EXIT_SUCCESS;
            break;

        case 'l':
            seq64::g_rc_settings.legacy_format(true);
            printf("Setting legacy seq24 file format for writing.\n");
            break;

        case 'L':
            seq64::g_rc_settings.lash_support(true);
            printf("Activating LASH support.\n");
            break;

        case 'S':
            seq64::g_rc_settings.stats(true);
            break;

        case 's':
            seq64::g_rc_settings.show_midi(true);
            break;

        case 'p':
            seq64::g_rc_settings.priority(true);
            break;

        case 'P':
            seq64::g_rc_settings.pass_sysex(true);
            break;

        case 'k':
            seq64::g_rc_settings.print_keys(true);
            break;

        case 'j':
            seq64::g_rc_settings.with_jack_transport(true);
            break;

        case 'J':
            seq64::g_rc_settings.with_jack_master(true);
            break;

        case 'C':
            seq64::g_rc_settings.with_jack_master_cond(true);
            break;

        case 'M':
            if (atoi(optarg) > 0)
                seq64::g_rc_settings.jack_start_mode(true);
            else
                seq64::g_rc_settings.jack_start_mode(false);
            break;

        case 'm':
            seq64::g_rc_settings.manual_alsa_ports(true);
            break;

        case 'a':
            seq64::g_rc_settings.manual_alsa_ports(false);
            break;

        case 'i':                           /* ignore ALSA device */
            seq64::g_rc_settings.device_ignore(true);
            seq64::g_rc_settings.device_ignore_num(atoi(optarg));
            break;

        case 'V':
            printf("%s", versiontext.c_str());
            return EXIT_SUCCESS;
            break;

        case 'U':
            seq64::g_rc_settings.jack_session_uuid(std::string(optarg));
            break;

        case 'x':
            seq64::g_rc_settings.interaction_method
            (
                seq64::interaction_method_t(atoi(optarg))
            );
            break;

        case 'b':
            global_buss_override = char(atoi(optarg));
            break;

        case 'q':
            if (std::string(optarg) == std::string("file"))
            {
                global_ppqn = 0;                // NOT READY !!!!!
            }
            else
            {
                global_ppqn = atoi(optarg);
            }
            break;

        default:
            break;
        }
    }

    std::size_t applen = strlen("seq24");
    std::string appname(argv[0]);               /* "seq24", "./seq24", etc. */
    appname = appname.substr(appname.size()-applen, applen);
    if (appname == "seq24")
    {
        seq64::g_rc_settings.legacy_format(true);
        printf("Setting legacy seq24 file format.\n");
    }
    seq64::g_rc_settings.set_globals();         /* copy to legacy globals   */
    seq64::g_user_settings.set_globals();       /* copy to legacy globals   */
    p.init();
    p.launch_input_thread();
    p.launch_output_thread();
#ifdef SEQ64_JACK_SUPPORT
    p.init_jack();
#endif

    seq64::mainwnd seq24_window(p);             /* push mainwnd onto stack  */
    if (optind < argc)                          /* MIDI filename provided?  */
    {
        if (Glib::file_test(argv[optind], Glib::FILE_TEST_EXISTS))
            seq24_window.open_file(argv[optind]);
        else
            printf("? MIDI file not found: %s\n", argv[optind]);
    }

#ifdef SEQ64_LASH_SUPPORT
    if (global_lash_support)
    {
        /*
         *  Initialize the lash driver (strips lash-specific command line
         *  arguments), then connect to the LASH daemon and poll events.
         */

        seq64::global_lash_driver = new seq64::lash(p, argc, argv);
        seq64::global_lash_driver->start();
    }
    else
        seq64::global_lash_driver = nullptr;
#endif

    kit.run(seq24_window);
    p.deinit_jack();

    /*
     * Write the configuration file to the new name, unless the
     * --legacy option is in force.
     */

    seq64::g_rc_settings.get_globals();         /* copy from legacy globals */
    rcname = seq64::g_rc_settings.config_filespec();
    printf("[Writing rc configuration %s]\n", rcname.c_str());
    seq64::optionsfile options(rcname);
    if (options.write(p))
    {
        // Anything to do?
    }

    seq64::g_user_settings.get_globals();       /* copy from legacy globals */
    rcname = seq64::g_rc_settings.user_filespec();
    printf("[Writing user configuration %s]\n", rcname.c_str());
    seq64::userfile userstuff(rcname);
    if (userstuff.write(p))
    {
        // Anything to do?
    }

#ifdef SEQ64_LASH_SUPPORT
    if (not_nullptr(seq64::global_lash_driver))
        delete seq64::global_lash_driver;
#endif

    return EXIT_SUCCESS;
}

/*
 * sequencer64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

