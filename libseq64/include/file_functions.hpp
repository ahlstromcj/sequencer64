#ifndef SEQ64_FILE_FUNCTIONS_HPP
#define SEQ64_FILE_FUNCTIONS_HPP

/**
 * \file          file_functions.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author        Chris Ahlstrom
 * \date          2015-11-20
 * \updates       2018-09-30
 * \version       $Revision$
 *
 *    Also see the file_functions.cpp module.
 */

#include <string>

#define SEQ64_TRIM_CHARS                " \t\n\v\f\r"
#define SEQ64_TRIM_CHARS_QUOTES         " \t\n\v\f\r\"'"

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
extern bool name_has_directory (const std::string & filename);
extern bool make_directory (const std::string & pathname);
extern std::string get_current_directory ();
extern std::string get_full_path (const std::string & path);
extern std::string normalize_path
(
    const std::string & path,
    bool tounix = true,
    bool terminate = false
);
extern std::string clean_file (const std::string & path, bool tounix = true);
extern std::string clean_path (const std::string & path, bool tounix = true);
extern std::string filename_concatenate
(
    const std::string & path, const std::string & filebase
);
extern void filename_split
(
    const std::string & fullpath,
    std::string & path,
    std::string & filebase
);
extern bool file_extension_match
(
    const std::string & path, const std::string & target
);
extern std::string file_extension (const std::string & path);

/*
 * String functions.
 */

extern std::string strip_comments (const std::string & item);
extern std::string strip_quotes (const std::string & item);
extern std::string add_quotes (const std::string & item);
extern bool strcasecompare (const std::string & a, const std::string & b);
extern std::string & ltrim
(
    std::string & str, const std::string & chars = SEQ64_TRIM_CHARS
);
extern std::string & rtrim
(
    std::string & str, const std::string & chars = SEQ64_TRIM_CHARS
);
extern std::string & trim
(
    std::string & str, const std::string & chars = SEQ64_TRIM_CHARS
);

#endif      // SEQ64_FILE_FUNCTIONS_HPP

}           // namespace seq64

/*
 * file_functions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

