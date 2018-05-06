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
 * \file          seq64qt5.cpp
 *
 *  This module declares/defines the main module for the JACK/ALSA "qt5"
 *  implementation of this application.
 *
 * \library       seq64qt5 application
 * \author        Chris Ahlstrom
 * \date          2017-09-05
 * \updates       2018-05-05
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Sequencer64.
 */

#include <stdio.h>
#include <QApplication>

#include "cmdlineopts.hpp"              /* command-line functions           */
#include "daemonize.hpp"                /* seqg4::reroute_stdio()           */
#include "file_functions.hpp"           /* seq64::file_accessible()         */
#include "gui_assistant_qt5.hpp"        /* seq64::gui_assistant_qt5         */

#ifdef PLATFORM_LINUX
#include "lash.hpp"                     /* seq64::lash_driver functions     */
#endif

#include "perform.hpp"                  /* seq64::perform                   */
#include "qsmainwnd.hpp"                /* the main window of seq64qt5      */
#include "settings.hpp"                 /* seq64::usr() and seq64::rc()     */

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
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns EXIT_SUCCESS (0) or EXIT_FAILURE, depending on the status of
 *      the run.
 */

int
main (int argc, char * argv [])
{
    QApplication a(argc, argv);             /* main application object      */
    int exit_status = EXIT_SUCCESS;         /* EXIT_FAILURE                 */
    seq64::rc().set_defaults();             /* start out with normal values */
    seq64::usr().set_defaults();            /* start out with normal values */
    (void) seq64::parse_log_option(argc, argv);   /* -o log=file.ext early  */

    /**
     * Set up objects that are specific to the GUI.  Pass them to the
     * perform constructor.  Then parse any command-line options to see if
     * they might affect what gets read from the 'rc' or 'user' configuration
     * files.  They will be parsed again later so that they can still override
     * whatever other settings were made via the configuration files.
     *
     * However, we currently have a issue where the mastermidibus created by
     * the perform object gets the default PPQN value, because the "user"
     * configuration file has not been read at that point.  See the
     * perform::launch() function.
     */

    seq64::gui_assistant_qt5 gui;               /* GUI-specific objects     */
    seq64::perform p(gui);                      /* main performance object  */
    (void) seq64::parse_command_line_options(p, argc, argv);
    bool is_help = seq64::help_check(argc, argv);
    bool ok = true;
    int optionindex = SEQ64_NULL_OPTION_INDEX;
    if (! is_help)
    {
        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        ok = seq64::parse_options_files(p, errmessage, argc, argv);
        optionindex = seq64::parse_command_line_options(p, argc, argv);
        if (seq64::parse_o_options(argc, argv))
        {
            /**
             * The user may have specified the "wid" or other -o options that
             * are also set up in the "usr" file.  The command line needs to
             * take precedence.  The "log" option is processed early in the
             * startup sequence.  These same settings are made in the
             * cmdlineopts module.
             *
             * Now handled via optind incrementing:     ++optionindex;
             */

            p.seqs_in_set(seq64::usr().seqs_in_set());
            p.max_sets(seq64::usr().max_sets());

            std::string logfile = seq64::usr().option_logfile();
            if (seq64::usr().option_use_logfile() && ! logfile.empty())
                (void) seq64::reroute_stdio(logfile);
        }

        /*
         * Issue #100, moved this call to before creating the qsmainwnd.
         * Otherwise, seq64 will not register with LASH (if enabled) in a
         * timely fashion.
         */

        p.launch(seq64::usr().midi_ppqn());         /* set up performance   */

        /*
         * TODO
         *
        if (seq64::usr().inverse_colors())
            seq64::gui_palette_qt5::load_inverse_palette(true);
         */

        /*
         * Push the qsmainwnd window onto the stack, with an option for
         * allowing a second perfedit to be created.  Also be sure to pass
         * along the PPQN value, which might be different than the default
         * (192), and affects some of the child objects of mainwnd.
         */

        seq64::qsmainwnd seq24_window
        (
            p
#if defined READY_FOR_USE
            ,
            seq64::usr().allow_two_perfedits(),
            seq64::usr().midi_ppqn()
#if defined SEQ64_MULTI_MAINWID
            ,
            seq64::usr().block_rows(),
            seq64::usr().block_columns(),
            seq64::usr().block_independent()
#endif
#endif
        );
        seq24_window.show();

        /*
         * Having this here after creating the main window may cause issue
         * #100, where ladish doesn't see seq64's ports in time.
         *
         *  p.launch(seq64::usr().midi_ppqn());
         *
         * We also check for any "fatal" PortMidi errors, so we can display
         * them.  But we still want to keep going, in order to at least
         * generate the log-files and configuration files to
         * C:/Users/me/AppData/Local/sequencer64 or ~/.config/sequencer64.
         */

        if (Pm_error_present())
        {
            ok = false;
            seq24_window.show_message_box(std::string(Pm_hosterror_message()));
        }

        if (ok)
        {
            if (optionindex < argc)                 /* MIDI filename given? */
            {
                std::string midifilename = argv[optionindex];
                if (seq64::file_accessible(midifilename))
                    seq24_window.open_file(midifilename);
                else
                {
                    char temp[256];
                    (void) snprintf
                    (
                        temp, sizeof temp,
                        "? MIDI file not found: %s\n", midifilename.c_str()
                    );
                    printf(temp);
                    seq24_window.show_message_box(std::string(temp));
                }
            }
        }

#ifdef PLATFORM_LINUX
        if (ok)
        {
            if (seq64::rc().lash_support())
                seq64::create_lash_driver(p, argc, argv);
        }
#endif

        exit_status = a.exec();                 /* run main window loop */
        p.finish();                             /* tear down performer  */
        if (seq64::rc().auto_option_save())
        {
            (void) seq64::write_options_files(p);
        }
        else
            printf("[auto-option-save off, not saving config files]\n");

#ifdef PLATFORM_LINUX
    if (ok)
        seq64::delete_lash_driver();            /* deleted only exists  */
#endif
    }
    return exit_status;
}

/*
 * seq64qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

