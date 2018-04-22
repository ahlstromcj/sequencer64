#if ! defined SEQ64_FILE_FUNCTIONS_HPP
#define SEQ64_FILE_FUNCTIONS_HPP

/**
 * \file          file_functions.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author        Chris Ahlstrom
 * \date          2015-11-20
 * \updates       2018-04-22
 * \version       $Revision$
 *
 *    Also see the file_functions.cpp module.
 */

#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 * Global function declarations.
 */

extern bool file_access (const std::string & targetfile, int mode);
extern bool file_exists (const std::string & targetfile);
extern bool file_readable (const std::string & targetfile);
extern bool file_writable (const std::string & targetfile);
extern bool file_accessible (const std::string & targetfile);
extern bool file_executable (const std::string & targetfile);
extern bool file_is_directory (const std::string & targetfile);
extern bool make_directory (const std::string & pathname);
extern std::string get_current_directory ();
extern std::string get_full_path (const std::string & path);
extern std::string normalize_path (const std::string & path, bool to_unix = true);
extern std::string strip_quotes (const std::string & item);

#endif      // SEQ64_FILE_FUNCTIONS_HPP

}           // namespace seq64

/*
 * file_functions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

