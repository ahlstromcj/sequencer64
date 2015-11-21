#if ! defined SEQ64_CMDLINEOPTS_HPP
#define SEQ64_CMDLINEOPTS_HPP

/**
 * \file    file_functions.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author  Chris Ahlstrom
 * \date    2015-11-20
 * \updates 2015-11-20
 * \version $Revision$
 *
 *    Also see the file_functions.cpp module.
 */

#include <string>

/**
 *  Provides a return value for parse_command_line_options() that indicates a
 *  help-related option was specified.
 */

#define SEQ64_NULL_OPTION_INDEX         99999

namespace seq64
{

class perform;                          /* forward reference */

/*
 * Global function declarations.
 */

extern bool help_check (int argc, char * argv []);
extern bool parse_options_files (perform & p, int argc, char * argv []);
extern int parse_command_line_options (int argc, char * argv []);
extern bool write_options_files (const perform & p);

#endif      // SEQ64_CMDLINEOPTS_HPP

}           // namespace seq64

/*
 * file_functions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

