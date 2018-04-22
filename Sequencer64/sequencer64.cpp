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
 *  This module declares/defines the main module for the original ALSA-only
 *  implementation of this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-04-22
 * \license       GNU GPLv2 or above
 *
 *  Note that there are a number of header files that we don't need to add
 *  here, since other header files include them.
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
#include <gdkmm/cursor.h>
#include <gtkmm/main.h>

#include "cmdlineopts.hpp"              /* command-line functions           */
#include "daemonize.hpp"                /* seq64::reroute_stdio()           */
#include "file_functions.hpp"           /* seq64::file_accessible()         */
#include "gui_assistant_gtk2.hpp"       /* seq64::gui_assistant_gtk2        */
#include "gui_palette_gtk2.hpp"         /* colors and "inverse" colors      */
#include "lash.hpp"                     /* seq64::lash_driver functions     */
#include "mainwid.hpp"                  /* needed to fulfill mainwnd        */
#include "mainwnd.hpp"                  /* the main window of sequencer64   */
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
    Gtk::Main kit(argc, argv);              /* strip GTK+ parameters        */
    seq64::rc().set_defaults();             /* start out with normal values */
    seq64::usr().set_defaults();            /* start out with normal values */
    (void) seq64::parse_log_option(argc, argv);    /* -o log=file.ext early */

    /*
     * Set up objects that are specific to the Gtk-2 GUI.  Pass them to the
     * perform constructor.  Then parse any command-line options to see if
     * they might affect what gets read from the 'rc' or 'user' configuration
     * files.  They will be parsed again later so that they can still override
     * whatever other settings were made via the configuration files.
     */

    seq64::gui_assistant_gtk2 gui;              /* GUI-specific objects     */
    seq64::perform p(gui);                      /* main performance object  */
    (void) seq64::parse_command_line_options(p, argc, argv);
    bool is_help = seq64::help_check(argc, argv);
    bool ok = true;
    int optionindex = SEQ64_NULL_OPTION_INDEX;
    if (! is_help)
    {
        /*
         * \todo
         *      If parsing fails at all, we need to report it and disable
         *      usage of the application and saving bad garbage out when
         *      exiting.
         */

        std::string errmessage;                     /* just in case!        */
        ok = seq64::parse_options_files(p, errmessage, argc, argv);
        optionindex = seq64::parse_command_line_options(p, argc, argv);
        if (seq64::parse_o_options(argc, argv))
        {
            /*
             * Process the -o options, also set up in the "usr" file.
             *
             *  Now handled via incrementing optind:    ++optionindex;
             */

            p.seqs_in_set(seq64::usr().seqs_in_set());
            p.max_sets(seq64::usr().max_sets());

            std::string logfile = seq64::usr().option_logfile();
            if (seq64::usr().option_use_logfile() && ! logfile.empty())
                (void) seq64::reroute_stdio(logfile);
        }
        p.launch(seq64::usr().midi_ppqn());         /* set up performance   */
        if (seq64::usr().inverse_colors())
            seq64::gui_palette_gtk2::load_inverse_palette(true);

        /*
         * Push the mainwnd window onto the stack, with an option for allowing
         * a second perfedit to be created.  Also be sure to pass along the
         * PPQN value, which might be different than the default (192), and
         * affects some of the child objects of mainwnd.
         */

        seq64::mainwnd seq24_window
        (
            p, seq64::usr().allow_two_perfedits(),
            seq64::usr().midi_ppqn()
#if defined SEQ64_MULTI_MAINWID
            ,
            seq64::usr().block_rows(),
            seq64::usr().block_columns(),
            seq64::usr().block_independent()
#endif
        );
        if (ok)
        {
            if (optionindex < argc)                 /* MIDI filename provided?  */
            {
                std::string midifilename = argv[optionindex];
                if (seq64::file_accessible(midifilename))
                    seq24_window.open_file(midifilename);
                else
                    printf("? MIDI file not found: %s\n", midifilename.c_str());
            }

            if (seq64::rc().lash_support())
                seq64::create_lash_driver(p, argc, argv);

            kit.run(seq24_window);                  /* run until user quits  */
            p.finish();                             /* tear down performance */
            if (seq64::rc().auto_option_save())
            {
                if (ok)                             /* don't write bad stuff */
                    ok = seq64::write_options_files(p);
            }
            else
                printf("[auto-option-save is off, so not saving config files]\n");

            seq64::delete_lash_driver();            /* deletes only if exists */
        }
        else
            seq24_window.rc_error_dialog(errmessage);
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE ;
}

/*
 * sequencer64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

