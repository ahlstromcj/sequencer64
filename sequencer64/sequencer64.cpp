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
 * \updates       2015-09-25
 * \license       GNU GPLv2 or above
 *
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
    {"lash",                0, 0, 'L'},                 /* new 2015-08-27 */
#endif
    {"legacy",              0, 0, 'l'},                 /* new 2015-08-16 */
    {"showmidi",            0, 0, 's'},
    {"show_keys",           0, 0, 'k'},
    {"stats",               0, 0, 'S'},
    {"priority",            0, 0, 'p'},
    {"ignore",              required_argument, 0, 'i'},
    {"interaction_method",  required_argument, 0, 'x'},
    {"jack_transport",      0, 0, 'j'},
    {"jack_master",         0, 0, 'J'},
    {"jack_master_cond",    0, 0, 'C'},
    {"jack_start_mode",     required_argument, 0, 'M'},
    {"jack_session_uuid",   required_argument, 0, 'U'},
    {"manual_alsa_ports",   0, 0, 'm'},
    {"pass_sysex",          0, 0, 'P'},
    {"version",             0, 0, 'V'},
    {0, 0, 0, 0}                                        /* terminator     */
};

static const std::string versiontext = SEQ64_PACKAGE " " SEQ64_VERSION "\n";

namespace seq64
{

/*
 * Global pointer!  Declared in font.h.
 */

    font * p_font_renderer = nullptr;

}           // namespace seq64

const char * const g_help_1a =
"Usage: sequencer64 [OPTIONS] [FILENAME]\n\n"
"Options:\n"
"   -h, --help               Show this message.\n"
"   -v, --version            Show program version information.\n"
"   -l, --legacy             Write MIDI file in old Seq24 format.  Also set\n"
"                            if Sequencer64 is called as 'seq24'.\n"
#ifdef SEQ64_LASH_SUPPORT
"   -L, --lash               Activate built-in LASH support.\n"
#endif
    ;

const char * const g_help_1b =
"   -m, --manual_alsa_ports  Don't attach ALSA ports.\n"
"   -s, --showmidi           Dump incoming MIDI events to the screen.\n"
"   -p, --priority           Runs higher priority with FIFO scheduler\n"
"                            (must be root)\n"
"   -P, --pass_sysex         Passes incoming SysEx messages to all outputs.\n"
"   -i, --ignore n           Ignore ALSA device number.\n"
    ;

const char * const g_help_2 =
"   -k, --show_keys          Prints pressed key value.\n"
"   -j, --jack_transport     Sync to JACK transport\n"
"   -J, --jack_master        Try to be JACK master\n"
"   -C, --jack_master_cond   JACK master will fail if there's already a master.\n"
"   -M, --jack_start_mode m  When synced to JACK, the following play modes\n"
"                            are available: 0 = live mode;\n"
"                            1 = song mode (the default).\n"
"   -S, --stats              Show global statistics.\n"
"   -x, --interaction_method n  See ~/.config/sequencer64/sequencer64.rc for "
"                            methods to use.\n"
"   -U, --jack_session_uuid u   Set UUID for JACK session\n"
"\n\n\n"
    ;

/**
 *  The standard C/C++ entry point to this application.  This first thing
 *  this function does is scan the argument vector and strip off all
 *  parameters known to GTK+.
 */

int
main (int argc, char * argv [])
{
    Gtk::Main kit(argc, argv);              /* strip GTK+ parameters        */
    int c;
    g_rc_settings.set_defaults();      /* start out with normal values */
    g_user_settings.set_defaults();    /* start out with normal values */
    for (;;)                                /* parse all parameters         */
    {
        int option_index = 0;               /* getopt_long index storage    */
        c = getopt_long
        (
            argc, argv,
            "ChlLi:jJmM:pPsSU:Vx:",         /* wrong: "C:hi:jJmM:pPsSU:Vx:" */
            long_options, &option_index
        );
        if (c == -1)                        /* detect the end of options    */
            break;

        switch (c)
        {
        case '?':
        case 'h':
            printf(g_help_1a);
            printf(g_help_1b);
            printf(g_help_2);
            return EXIT_SUCCESS;
            break;

        case 'l':
            g_rc_settings.legacy_format(true);
            printf("Setting legacy seq24 file format.\n");
            break;

        case 'L':
            g_rc_settings.lash_support(true);
            printf("Activating LASH support.\n");
            break;

        case 'S':
            g_rc_settings.stats(true);
            break;

        case 's':
            g_rc_settings.show_midi(true);
            break;

        case 'p':
            g_rc_settings.priority(true);
            break;

        case 'P':
            g_rc_settings.pass_sysex(true);
            break;

        case 'k':
            g_rc_settings.print_keys(true);
            break;

        case 'j':
            g_rc_settings.with_jack_transport(true);
            break;

        case 'J':
            g_rc_settings.with_jack_master(true);
            break;

        case 'C':
            g_rc_settings.with_jack_master_cond(true);
            break;

        case 'M':
            if (atoi(optarg) > 0)
            {
                g_rc_settings.jack_start_mode(true);
            }
            else
            {
                g_rc_settings.jack_start_mode(false);
            }
            break;

        case 'm':
            g_rc_settings.manual_alsa_ports(true);
            break;

        case 'i':                           /* ignore ALSA device */
            g_rc_settings.device_ignore(true);
            g_rc_settings.device_ignore_num(atoi(optarg));
            break;

        case 'V':
            printf("%s", versiontext.c_str());
            return EXIT_SUCCESS;
            break;

        case 'U':
            g_rc_settings.jack_session_uuid(std::string(optarg));
            break;

        case 'x':
            g_rc_settings.interaction_method
            (
                interaction_method_t(atoi(optarg))
            );
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
        g_rc_settings.legacy_format(true);
        printf("Setting legacy seq24 file format.\n");
    }
    g_rc_settings.set_globals();                /* copy to legacy globals   */
    g_user_settings.set_globals();              /* copy to legacy globals   */

    /*
     * Set up objects that are specific to the Gtk-2 GUI.  Pass them to
     * the perform constructor.  Create a font-render object.
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

    std::string cfg_dir = g_rc_settings.home_config_directory();
    if (cfg_dir.empty())
        return EXIT_FAILURE;

    std::string rcname = g_rc_settings.config_filespec();
    if (Glib::file_test(rcname, Glib::FILE_TEST_EXISTS))
    {
        printf("Reading 'rc' configuration [%s]\n", rcname.c_str());
        seq64::optionsfile options(rcname);
        if (options.parse(p))
            g_rc_settings.last_used_dir(cfg_dir);
        else
            return EXIT_FAILURE;
    }
    rcname = g_rc_settings.user_filespec();
    if (Glib::file_test(rcname, Glib::FILE_TEST_EXISTS))
    {
        printf("Reading 'user' configuration [%s]\n", rcname.c_str());
        seq64::userfile user(rcname);
        if (user.parse(p))
        {
            // Nothing to do, and no exit needed here if it fails
        }
    }
    p.init();
    p.launch_input_thread();
    p.launch_output_thread();
    p.init_jack();

    seq64::mainwnd seq24_window(&p);          /* push a mainwnd onto stack */
    if (optind < argc)
    {
        if (Glib::file_test(argv[optind], Glib::FILE_TEST_EXISTS))
            seq24_window.open_file(argv[optind]);
        else
            printf("? file not found: %s\n", argv[optind]);
    }

#ifdef SEQ64_LASH_SUPPORT
    if (global_lash_support)
    {
        /*
         *  Initialize the lash driver (strips lash-specific command line
         *  arguments), then connect to LASH daemon and poll events.
         */

        seq64::global_lash_driver = new seq64::lash(argc, argv);
        seq64::global_lash_driver->init(&p);
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

    g_rc_settings.get_globals();             /* copy from legacy globals */
//  if (g_rc_settings.legacy_format())
        rcname = g_rc_settings.config_filespec();
//  else
//      rcname = g_rc_settings.config_filespec();

    printf("Writing rc configuration file [%s]\n", rcname.c_str());
    seq64::optionsfile options(rcname);
    if (options.write(p))
    {
        // Anything to do?
    }

    g_user_settings.get_globals();           /* copy from legacy globals */
    rcname = g_rc_settings.user_filespec();
    printf("Writing user configuration file [%s]\n", rcname.c_str());
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
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */

