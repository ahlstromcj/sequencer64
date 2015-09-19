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
 * \updates       2015-09-19
 * \license       GNU GPLv2 or above
 *
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>                  /* for stat() and mkdir() */
#include <sys/stat.h>
#include <unistd.h>

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

#ifdef PLATFORM_WINDOWS
#define HOME "HOMEPATH"
#define SLASH "\\"
#else
#define HOME "HOME"
#define SLASH "/"
#endif

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
"   -x, --interaction_method n  See ~/.config/sequencer64/sequencer64rc for "
"                            methods to use.\n"
"   -U, --jack_session_uuid u   Set UUID for JACK session\n"
"\n\n\n"
    ;

/**
 *  An internal function to ensure that the ~/.config/sequencer64
 *  directory exists.  This function is actually a little more general
 *  than that, but it is not sufficiently general, in general.
 *
 * \param pathname
 *      Provides the name of the path to create.  The parent directory of
 *      the final directory must already exist.
 *
 * \return
 *      Returns true if the path-name exists.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

static bool
make_directory (const std::string & pathname)
{
    bool result = ! pathname.empty();
    if (result)
    {
        static struct stat st =
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 /* and more for Linux! */
        };
        if (stat(pathname.c_str(), &st) == -1)
        {
            int rcode = mkdir(pathname.c_str(), 0700);
            result = rcode == 0;
        }
    }
    return result;
}


/**
 *  The standard C/C++ entry point to this application.  This first thing
 *  this function does is scan the argument vector and strip off all
 *  parameters known to GTK+.
 */

int
main (int argc, char * argv [])
{
    Gtk::Main kit(argc, argv);          /* strip GTK+ parameters    */
    int c;
    while (true)                        /* parse parameters         */
    {
        int option_index = 0;           /* getopt_long stores index here */
        c = getopt_long
        (
            argc, argv,
            "ChlLi:jJmM:pPsSU:Vx:",      /* wrong: "C:hi:jJmM:pPsSU:Vx:" */
            long_options, &option_index
        );
        if (c == -1)                    /* detect the end of the options */
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
            global_legacy_format = true;
            printf("Setting legacy seq24 file format.\n");
            break;

        case 'L':
            global_lash_support = true;
            printf("Activating LASH support.\n");
            break;

        case 'S':
            global_stats = true;
            break;

        case 's':
            global_showmidi = true;
            break;

        case 'p':
            global_priority = true;
            break;

        case 'P':
            global_pass_sysex = true;
            break;

        case 'k':
            global_print_keys = true;
            break;

        case 'j':
            global_with_jack_transport = true;
            break;

        case 'J':
            global_with_jack_master = true;
            break;

        case 'C':
            global_with_jack_master_cond = true;
            break;

        case 'M':
            if (atoi(optarg) > 0)
                global_jack_start_mode = true;
            else
                global_jack_start_mode = false;
            break;

        case 'm':
            global_manual_alsa_ports = true;
            break;

        case 'i':                           /* ignore ALSA device */
            global_device_ignore = true;
            global_device_ignore_num = atoi(optarg);
            break;

        case 'V':
            printf("%s", versiontext.c_str());
            return EXIT_SUCCESS;
            break;

        case 'U':
            global_jack_session_uuid = std::string(optarg);
            break;

        case 'x':
            global_interactionmethod = (interaction_method_e)atoi(optarg);
            break;

        default:
            break;
        }
    }

    std::size_t applen = strlen("seq24");
    std::string appname(argv[0]);           /* "seq24", "./seq24", etc. */
    appname = appname.substr(appname.size()-applen, applen);
    if (appname == "seq24")
    {
        if (! global_legacy_format)
        {
            global_legacy_format = true;
            printf("Setting legacy seq24 file format.\n");
        }
    }

    /*
     *  Prepare global MIDI definitions.
     */

    for (int i = 0; i < c_max_busses; i++)
    {
        for (int j = 0; j < 16; j++)
            global_user_midi_bus_definitions[i].instrument[j] = -1;
    }
    for (int i = 0; i < c_max_instruments; i++)
    {
        for (int j = 0; j < MIDI_COUNT_MAX; j++)
            global_user_instrument_definitions[i].controllers_active[j] = false;
    }

    /*
     * Set up objects that are specific to the Gtk-2 GUI.  Pass them to
     * the perform constructor.  Create a font-render object.
     */

    seq64::gui_assistant_gtk2 gui;              /* GUI-specific objects     */
    seq64::perform p(gui);                      /* main performance object  */
    seq64::p_font_renderer = new seq64::font(); /* set the font renderer    */
    if (getenv(HOME) != NULL)                   /* is $HOME set?            */
    {
        /*
         *  Instead of the Seq24 names, use the new configuration
         *  file-names, located in ~/.config/sequencer64.  If they aren't
         *  found, then fall back to the legacy configuration file-names.
         *  If the --legacy option is in force, use only the legacy
         *  configuration file-name.  Note that we also ensure the
         *  directory exists.  CURRENTLY LINUX-SPECIFIC.
         */

        std::string home(getenv(HOME));
        if (! global_legacy_format)
        {
            std::string cfg_dir = home + SLASH + global_config_directory;
            bool ok = make_directory(cfg_dir);
            if (! ok)
            {
                printf
                (
                    "? error creating [%s]\n", global_config_directory.c_str()
                );
                return EXIT_FAILURE;
            }
        }

        std::string rcname = home + SLASH + global_config_directory +
            SLASH + global_config_filename;

        if
        (
            ! global_legacy_format &&
            Glib::file_test(rcname, Glib::FILE_TEST_EXISTS)
        )
        {
            printf("Reading configuration [%s]\n", rcname.c_str());
            seq64::optionsfile options(rcname);
            if (options.parse(p))
            {
                // nothing to do upon success
            }
            else
                printf("? error reading [%s]\n", rcname.c_str());
        }
        else
        {
            std::string alt_rcname = home + SLASH +
                global_config_filename_alt;

            if (Glib::file_test(alt_rcname, Glib::FILE_TEST_EXISTS))
            {
                printf
                (
                    "Reading alternate configuration [%s]\n",
                    alt_rcname.c_str()
                );
                seq64::optionsfile options(alt_rcname);
                if (options.parse(p))
                    global_last_used_dir = home;
                else
                    printf("? error reading [%s]\n", alt_rcname.c_str());
            }
        }

        rcname = home + SLASH + global_config_directory +
            SLASH + global_user_filename;

        if (Glib::file_test(rcname, Glib::FILE_TEST_EXISTS))
        {
            printf("Reading 'user' configuration [%s]\n", rcname.c_str());
            seq64::userfile user(rcname);
            if (! user.parse(p))
                printf("? error reading [%s]\n", rcname.c_str());
        }
        else
        {
            std::string alt_rcname = home + SLASH +
                global_user_filename_alt;

            if (Glib::file_test(alt_rcname, Glib::FILE_TEST_EXISTS))
            {
                printf
                (
                    "Reading 'user' configuration [%s]\n",
                    alt_rcname.c_str()
                );
                seq64::userfile user(alt_rcname);
                if (! user.parse(p))
                    printf("? error reading [%s]\n", alt_rcname.c_str());
            }
        }
    }
    else
        printf("? error calling getenv(\"%s\")\n", HOME);

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
    if (getenv(HOME) != NULL)
    {
        /*
         * Write the configuration file to the new name, unless the
         * --legacy option is in force.
         */

        std::string home(getenv(HOME));
        std::string rcname;
        if (global_legacy_format)
            rcname = home + SLASH + global_config_filename_alt;
        else
            rcname = home + SLASH + global_config_directory +
                SLASH + global_config_filename;

        printf("Writing configuration [%s]\n", rcname.c_str());
        seq64::optionsfile options(rcname);
        if (!options.write(p))
            printf("? error writing [%s]\n", rcname.c_str());

        /*
         * \todo
         *      Flesh out the userfile writing code and write it.
         */
    }
    else
        printf("? error calling getenv(\"%s\")\n", HOME);

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

