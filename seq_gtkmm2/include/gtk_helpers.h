#ifndef SEQ24_GTK_HELPERS_H
#define SEQ24_GTK_HELPERS_H

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
 * \file          gtk_helpers.h
 *
 *  This module declares/defines some helpful macros or functions.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-05-29
 * \license       GNU GPLv2 or above
 *
 *  Copyright (C) 2015-2023 Chris Ahlstrom <ahlstromcj@gmail.com>
 *
 *  Currently, the only thing defined is the add_tooltip() macro.
 */

/**
 *  This macro defines a tooltip helper, for old versus new GTK handling.
 *  It is used in the mainwnd, options, perfedit, and seqedit classes.
 */

#if GTK_MINOR_VERSION >= 12
#define add_tooltip(obj, text) obj->set_tooltip_text(text);
#else
#define add_tooltip(obj, text) m_tooltips->set_tip(*obj, text);
#endif

#endif   // SEQ24_GTK_HELPERS_H

/*
 * gtk_helpers.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

