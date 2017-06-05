#ifndef SEQ64_CMDLINEOPTS_HPP
#define SEQ64_CMDLINEOPTS_HPP

/**
 * \file    cmdlineopts.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author  Chris Ahlstrom
 * \date    2015-11-20
 * \updates 2017-06-04
 * \version $Revision$
 *
 *    Also see the file_functions.cpp module.  These modules together simplify
 *    the main() module considerably, which will be useful when we have more
 *    than one "Sequencer64" application.
 */

#include <string>

/**
 *  Provides a return value for parse_command_line_options() that indicates a
 *  help-related option was specified.
 */

#define SEQ64_NULL_OPTION_INDEX         99999

/*
 * This is the main namespace of Sequencer64.  Do not attempt to
 * Doxygenate the documentation here; it breaks Doxygen.
 */

namespace seq64
{
    class perform;                      /* forward reference */

/*
 * Global function declarations.
 */

extern bool help_check (int argc, char * argv []);
extern bool parse_options_files
(
    perform & p, std::string & errmessage, int argc, char * argv []
);
extern bool parse_mute_groups (perform & p, std::string & errmessage);
extern bool parse_o_options (int argc, char * argv []);
extern int parse_command_line_options (perform & p, int argc, char * argv []);
extern bool write_options_files (const perform & p);
extern std::string build_details ();

#endif      // SEQ64_CMDLINEOPTS_HPP

}           // namespace seq64

/*
 * cmdlineopts.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

