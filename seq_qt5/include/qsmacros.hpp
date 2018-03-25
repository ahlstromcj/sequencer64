#ifndef SEQ64_QSMACROS_HPP
#define SEQ64_QSMACROS_HPP

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
 * \file          qsmacros.hpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-23
 * \updates       2018-03-25
 * \license       GNU GPLv2 or above
 *
 *  In these macros, the "s" parameter is a QString,
 *  toUtf8() returns a QByteArray, and constData() returns a pointer to the
 *  data inside the QByteArray.

 *  It is important to understand that the QByteArray is temporary. Therefore,
 *  it is "destroyed as the last step in evaluating the full-expression that
 *  (lexically) contains the point where [it was] created".
 *  Concretely, this means that this call is correct:
 *
 *      do_something(qsmacro(string));
 *
 *  But this one is undefined behavior:
 *
 *      const char * s = qsmacro(string);
 *      do_something(s);
 *
 *  To avoid the problem, store the QByteArray in a local variable so that it
 *  lives long enough.
 *
 */

/*
 *  Other potentian macros a la the VLC project.
 *
#define qfu(s)  QString::fromUtf8(s)
#define qfue(s) QString::fromUtf8(s).replace( "&", "&&" )
#define qtr(s)  QString::fromUtf8( vlc_gettext(i) )
#define qtl(s)  ((s).toUtf8().constData())
 *
 */

/*
 *  Returns a "const char *" value.  It uses QString::toLatin1().
 */

#define QS_CHAR_PTR(s)                  ((s).toLatin1().constData())

/*
 *  Returns the first byte of a QString, useful in processing incoming
 *  keystrokes in a simple manner.  Instead of dealing with
 *  QKeyEvent::key(), just grab QKeyEvent::text(), and pass it to this
 *  macro.  This macro also casts to unsigned.
 */

#define QS_TEXT_CHAR(s)                 (unsigned(* QS_CHAR_PTR(s)))

#endif      // SEQ64_QSMACROS_HPP

/*
 * qsmacros.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

