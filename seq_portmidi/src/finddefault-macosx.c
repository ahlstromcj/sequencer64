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
 * \file        finddefault_macosx.c
 *
 *      Provides the find_default_device() implementation, for Mac OSX.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2018-05-13
 * \updates     2018-05-13
 * \license     GNU GPLv2 or above
 *
 *  Roger Dannenberg, June 2008, some fixes and reformatting by Chris Ahlstrom.
 *  However, since this uses Java "prefs", and Sequencer64 already has its
 *  own configuration file, this code will not be used. Kept for any
 *  "cover-your-ass" situations.
 */

#include <stdlib.h>
#include <string.h>

#include "portmidi.h"
#include "pmutil.h"
#include "pminternal.h"
#include "pmmacosxcm.h"
#include "readbinaryplist.h"

/**
 *  Parse preference files, find default device, search devices --
 * This parses the preference file(s) once for input and once for
 * output, which is inefficient but much simpler to manage. Note
 * that using the readbinaryplist.c module, you cannot keep two
 * plist files (user and system) open at once (due to a simple
 * memory management scheme).
 *
 * \param path
 *      The name of the preference we are searching for.
 *
 *  \param input
 *      True if this is an input device.
 *
 *  \param id
 *      Current default device id.
 *
 *  \return
 *      Returns the matching device ID if found, otherwise it returns the
 *      \a id parameter.
 */

PmDeviceID
find_default_device (char * path, int input, PmDeviceID id)
{
    static char * pref_file = "com.apple.java.util.prefs.plist";
    char * pref_str = NULL;
    value_ptr prefs = bplist_read_user_pref(pref_file); /* read device prefs    */
    if (not_nullptr(prefs))
    {
        value_ptr pref_val = value_dict_lookup_using_path(prefs, path);
        if (pref_val)
            pref_str = value_get_asciistring(pref_val);
    }
    if (is_nullptr(pref_str))
    {
        bplist_free_data();                             /* look elsewhere       */
        prefs = bplist_read_system_pref(pref_file);
        if (not_nullptr(prefs))
        {
            value_ptr pref_val = value_dict_lookup_using_path(prefs, path);
            if (not_nullptr(pref_val))
                pref_str = value_get_asciistring(pref_val);
        }
    }
    if (not_nullptr(pref_str))                          /* search devs for match */
    {
        int i = pm_find_default_device(pref_str, input);
        if (i != pmNoDevice)
            id = i;
    }
    if (not_nullptr(prefs))
        bplist_free_data();

    return id;
}

/*
 * finddefault_macosx.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


