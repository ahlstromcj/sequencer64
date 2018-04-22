/**
 * \file          file_functions.cpp
 *
 *    Provides the implementations for safe replacements for the various C
 *    file functions.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-11-20
 * \updates       2018-04-22
 * \version       $Revision$
 *
 *    We basically include only the functions we need for Sequencer64, not
 *    much more than that.  These functions are adapted from our xpc_basic
 *    project.
 */

#include <algorithm>                    /* std::replace() function          */
#include <stdlib.h>                     /* realpath(3) or _fullpath()       */
#include <string.h>                     /* strlen() etc.                    */
#include <sys/types.h>
#include <sys/stat.h>

#include "easy_macros.h"
#include "file_functions.hpp"           /* free functions in seq64 n'space  */

#if SEQ64_HAVE_LIMITS_H
#include <limits.h>                     /* PATH_MAX                         */
#endif

#if defined _MSC_VER                    /* Microsoft compiler               */

#include <io.h>                         /* _access_s()                      */
#include <share.h>                      /* _SH_DENYNO                       */

#define F_OK         0x00               /* existence                        */
#define X_OK         0x01               /* executable, not useful w/Windows */
#define W_OK         0x02               /* writability                      */
#define R_OK         0x04               /* readability                      */

#define S_ACCESS     _access_s
#define S_STAT       _stat

typedef struct _stat stat_t;

#else                                   /* non-Microsoft stuff follows      */

#include <unistd.h>

#define S_ACCESS     access
#define S_STAT       stat

typedef struct stat stat_t;

#endif                                  /* _MSC_VER                         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *    Checks a file for the desired access modes.
 *    The following modes are defined, and can be OR'd:
 *
\verbatim
      POSIX Value Windows  Meaning
      F_OK    0   0x00     Existence
      X_OK    1   N/A      Executable
      W_OK    2   0x04     Writable
      R_OK    4   0x02     Readable
\endverbatim
 *
 * \win32
 *    Windows does not provide a mode to check for executability.
 *
 * \param filename
 *    Provides the name of the file to be checked.
 *
 * \param mode
 *    Provides the mode of the file to check for.  This value should be in the
 *    cross-platform set of file-mode's for the various versions of the fopen()
 *    function.
 *
 * \return
 *    Returns 'true' if the requested modes are all supported for the file.
 */

#define ERRNO_FILE_DOESNT_EXIST     2  /* true in Windows and Linux  */

bool
file_access (const std::string & filename, int mode)
{
   bool result = ! filename.empty();
   if (result)
   {
#ifdef _MSC_VER

      /**
       * Passing in X_OK here on Windows 7 yields a debug assertion!  For now,
       * we just have to return false if that value is part of the mode mask.
       */

      if (mode & X_OK)
      {
         errprint("cannot test X_OK (executable bit) on Windows");
         result = false;
      }
      else
      {
         int errnum = S_ACCESS(filename.c_str(), mode);
         result = errnum == 0;
      }
#else
      int errnum = S_ACCESS(filename.c_str(), mode);
      result = errnum == 0;
#endif
   }
   return result;
}

/**
 *    Checks a file for existence.
 *
 * \param filename
 *    Provides the name of the file to be checked.
 *
 * \return
 *    Returns 'true' if the file exists.
 *
 */

bool
file_exists (const std::string & filename)
{
   return file_access(filename, F_OK);
}

/**
 *    Checks a file for readability.
 *
 * \param filename
 *    Provides the name of the file to be checked.
 *
 * \return
 *    Returns 'true' if the file is readable.
 *
 */

bool
file_readable (const std::string & filename)
{
   return file_access(filename, R_OK);
}

/**
 *    Checks a file for writability.
 *
 * \param filename
 *    Provides the name of the file to be checked.
 *
 * \return
 *    Returns 'true' if the file is writable.
 *
 */

bool
file_writable (const std::string & filename)
{
   return file_access(filename, W_OK);
}

/**
 *    Checks a file for readability and writability.
 *    An even stronger test than file_exists.  At present, we see no need to
 *    distinguish read and write permissions.  We assume the file is
 *    accessible only if the file has both permissions.
 *
 * \param filename
 *    Provides the name of the file to be checked.
 *
 * \return
 *    Returns 'true' if the file is readable and writable.
 */

bool
file_accessible (const std::string & filename)
{
   return file_access(filename, R_OK|W_OK);
}

/**
 *    Checks a file for the ability to be executed.
 *
 * \param filename
 *    Provides the name of the file to be checked.
 *
 * \return
 *    Returns 'true' if the file exists.
 */

bool
file_executable (const std::string & filename)
{
   bool result = ! filename.empty();
   if (result)
   {
      stat_t statusbuf;
      int statresult = S_STAT(filename.c_str(), &statusbuf);
      if (statresult == 0)                          /* a good file handle? */
      {
#if defined _MSC_VER
         result = (statusbuf.st_mode & _S_IEXEC) != 0;
#else
         result =
         (
            ((statusbuf.st_mode & S_IXUSR) != 0) ||
            ((statusbuf.st_mode & S_IXGRP) != 0) ||
            ((statusbuf.st_mode & S_IXOTH) != 0)
         );
#endif
      }
      else
         result = false;
   }
   return result;
}

/**
 *    Checks a file to see if it is a directory.
 *    This function is also used in the function of the same name
 *    in fileutilities.cpp.
 *
 * \param filename
 *    Provides the name of the directory to be checked.
 *
 * \return
 *    Returns 'true' if the file is a directory.
 */

bool
file_is_directory (const std::string & filename)
{
   bool result = ! filename.empty();
   if (result)
   {
      stat_t statusbuf;
      int statresult = S_STAT(filename.c_str(), &statusbuf);
      if (statresult == 0)                           // a good file handle?
      {
#ifdef _MSC_VER
         result = (statusbuf.st_mode & _S_IFDIR) != 0;
#else
         result = (statusbuf.st_mode & S_IFDIR) != 0;
#endif
      }
      else
         result = false;
   }
   return result;
}

/**
 *  A function to ensure that the ~/.config/sequencer64 directory exists.
 *  This function is actually a little more general than that, but it is not
 *  sufficiently general, in general, General.
 *
 * \param pathname
 *      Provides the name of the path to create.  The parent directory of the
 *      final directory must already exist.
 *
 * \return
 *      Returns true if the path-name exists.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

bool
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
#ifdef PLATFORM_MINGW
            int rcode = mkdir(pathname.c_str());
#else
            int rcode = mkdir(pathname.c_str(), 0700);
#endif
            result = rcode == 0;
        }
    }
    return result;
}

/**
 *  Provides the path name of the current working directory.  This function is a
 *  wrapper for getcwd() and other such functions.  It obtains the current
 *  working directory in the application.
 *
 * \return
 *      The pointer to the string containg the name of the current directory.
 *      This name is the full path name for the directory.  If an error occurs,
 *      then an empty string is returned.
 */

std::string
get_current_directory ()
{
    std::string result;
    char temp[PATH_MAX];
    char * cwd = GETCWD(temp, PATH_MAX);  /* get current directory      */
    if (not_nullptr(cwd))
    {
      size_t len = strlen(cwd);
      if (len > 0)
         result = cwd;
      else
      {
         errprint("empty directory name returned");
      }
   }
   else
   {
      errprint("could not get current directory");
   }
   return result;
}

/**
 *  Given a path, relative or not, this function returns the full path.
 *  It uses the Linux function realpath(3), which returns the canonicalized
 *  absolute path-name.  For Windows, the function _fullpath() is used.
 *
 * \param path
 *      Provides the path, which may be relative.
 *
 * \return
 *      Returns the full path.  If a problem occurs, the result is empty.
 */

std::string
get_full_path (const std::string & path)
{
    std::string result;
    if (! path.empty())
    {
#if defined PLATFORM_WINDOWS
        char temp[256];
        char * resolved_path = _fullpath(temp, path.c_str(), 256);
#else
        char * resolved_path = realpath(path.c_str(), NULL);
#endif
        if (not_NULL(resolved_path))
        {
            result = resolved_path;
            free(resolved_path);
        }
        else
        {
            // TODO: check the errno value and emit a message.
        }
    }
    return result;
}

/**
 *  Makes sure that the path-name is a UNIX path, separated by forward slashes
 *  (the solidus).
 *
 * \param path
 *      Provides the path, which should be a full path-name.
 *
 * \param to_unix
 *      Defaults to true, which converts "\" to "/".  False converts in the
 *      opposite direction.
 *
 * \return
 *      The possibly modified path is returned.
 */

std::string
normalize_path (const std::string & path, bool to_unix)
{
    std::string result;
    if (! path.empty())
    {
        result = path;
        if (to_unix)
        {
            std::string::size_type pos = path.find_first_of("\\");
            if (pos != std::string::npos)
                std::replace(result.begin(), result.end(), '\\', '/');
        }
        else
        {
            std::string::size_type pos = path.find_first_of("/");
            if (pos != std::string::npos)
                std::replace(result.begin(), result.end(), '/', '\\');
        }
    }
    return result;
}

/**
 *  Strips the double quotes from a string.  Meant mainly for removing quotes
 *  around a file-name, so it works only if the first character is a quote,
 *  and the last character is a quote.
 *
 * \param item
 *      The string to be massaged.
 *
 * \return
 *      The string without double quotes.  If it didn't have any, the string
 *      should be unchanged.
 */

std::string
strip_quotes (const std::string & item)
{
    std::string result;
    if (! item.empty())
    {
        result = item;
        std::string::size_type fpos = result.find_first_of("\"");
        if (fpos == 0)
        {
            std::string::size_type lpos = result.find_last_of("\"");
            std::string::size_type end_index = result.length() - 1;
            if (lpos != std::string::npos && lpos == end_index)
                result = result.substr(1, end_index - 1);
        }
    }
    return result;
}

}           // namespace seq64

/*
 * file_functions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

