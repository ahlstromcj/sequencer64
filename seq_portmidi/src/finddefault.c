/*
 *  This file is part of sequencer64, adapted from the PortMIDI project.
 *
 *  sequencer64 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  sequencer64 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file        finddefault.c
 *
 *      Provides the find_default_device() implementation.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-04-24
 * \license     GNU GPLv2 or above
 *
 *  Roger Dannenberg, Jan 2009, some fixes and reformatting by Chris Ahlstrom.
 *  However, since this uses Java "prefs", and Sequencer64 already has its
 *  own configuration file, this code will not be used. Kept for any
 *  "cover-your-ass" situations.
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "portmidi.h"
#include "finddefault.h"
#include "pminternal.h"

/**
 * skip over spaces, return first non-space.
 */

void
skip_spaces (FILE * inf)
{
    char c;
    while (isspace(c = getc(inf)))
        ;

    ungetc(c, inf);
}

/**
 * trim leading spaces and match a string
 */

int
match_string (FILE * inf, char * s)
{
    skip_spaces(inf);
    while (*s && *s == getc(inf))
        s++;

    return *s == 0;
}

/**
 *  Parse preference files, find default device, search devices.
 *
 * TODO:  REPLACE THIS WITH Sequencer64 options.
 *
 * \param path
 *      The name of the preference we are searching for.
 *
 * \param input
 *      Set to true if this is an input device.
 *
 * \param id
 *      Current default device ID.
 *
 * \return
 *      Matching device ID if found, otherwise the ID parameter.
 */

PmDeviceID
find_default_device (char * path, int input, PmDeviceID id)
{
    static char * pref_2 = "/.java/.userPrefs/";
    static char * pref_3 = "prefs.xml";
    char * pref_1 = getenv("HOME");
    char * full_name;
    char * path_ptr;
    FILE * inf;
    int c, i;
    if (! pref_1)
        goto nopref; // cannot find preference file

    // full_name will be larger than necessary

    size_t namelen = strlen(pref_1) + strlen(pref_2) +
        strlen(pref_3) + strlen(path) + 2;

    full_name = malloc(namelen);
    strcpy(full_name, pref_1);
    strcat(full_name, pref_2);

    // copy all but last path segment to full_name

    if (*path == '/')
        path++;                     // skip initial slash in path

    path_ptr = strrchr(path, '/');
    if (path_ptr)
    {
        // copy up to slash after full_name

        path_ptr++;
        int offset = strlen(full_name);
        memcpy(full_name + offset, path, path_ptr - path);
        full_name[offset + path_ptr - path] = 0;        // end of string
    }
    else
    {
        path_ptr = path;
    }
    strcat(full_name, pref_3);
    inf = fopen(full_name, "r");
    if (! inf)
        goto nopref;                    // cannot open preference file

    /*
     * We're not going to build or link in a full XML parser.  Instead, find
     * the path string and quoute. Then, look for "value", "=", quote. Then
     * get string up to quote.
     */

    while ((c = getc(inf)) != EOF)
    {
        char pref_str[PM_STRING_MAX];
        if (c != '"')
            continue;                   // scan up to quote

        // look for quote string quote

        if (! match_string(inf, path_ptr))
            continue;                   // path not found

        if (getc(inf) != '"')
            continue;                   // path not found, keep scanning

        if (! match_string(inf, "value"))
            goto nopref;                // value not found

        if (! match_string(inf, "="))
            goto nopref;                // = not found

        if (! match_string(inf, "\""))
            goto nopref;                // quote not found

        // now read the value up to the close quote

        for (i = 0; i < PM_STRING_MAX; ++i)
        {
            if ((c = getc(inf)) == '"')
                break;

            pref_str[i] = c;
        }
        if (i == PM_STRING_MAX)
            continue;                   // value too long, ignore

        pref_str[i] = 0;                // BUG:  Was pref_str[i] == 0;
        i = pm_find_default_device(pref_str, input);
        if (i != pmNoDevice)
            id = i;

        break;
    }

nopref:

    return id;
}

/*
 * finddefault.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

