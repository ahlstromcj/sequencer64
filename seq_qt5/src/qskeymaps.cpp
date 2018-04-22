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
 * \file          qskeymaps.cpp
 *
 *  Provides functions to convert between the Gtkmm 2.4 and Qt 5
 *  implementations of keystroke events.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-24
 * \updates       2018-04-22
 * \license       GNU GPLv2 or above
 *
 *  Gtkmm 2.4 and Qt 5 handle keystrokes in a somewhat different manner.
 *
 *  The Sequencer64 keystroke configuration sections in the "rc" file currently
 *  rely on the Gdk (Gtkmm) keystroke handling.  The Sequencer64 keystroke
 *  handling has the following features:
 *
 *      -#  The conventional ASCII character set is handled as norma, with
 *          values in the range of ' ' (32 or 0x20) up to '~' (126 or 0x7e).
 *      -#  Most special characters and keystroke modifiers are detected and
 *          are in the range of 0xFF00 and up.
 *      -#  Keys that might have nominally the same function are nonetheless
 *          distinguished.  For example, all the keys on the keypad section of
 *          the keyboard have unique unsigned code numbers.  For each key, the
 *          number is different between having Num-Lock on or not.  All
 *          numbers, even those for the ASCII characters '0' to '9' and '/',
 *          '*', '+', '-', and '.', are in the range of 0xff00 and up, and
 *          not in the range 0x2a to to 0x39.
 *
 *  For Qt 5 keystrokes, this level of distinguishing the characters from their
 *  keyboard keys is not done.  But Qt 5 allow two ways to retrieve key events:
 *  QKeyEvent::key() and QKeyEvent::text().  For ASCII keys, these functions
 *  yield the same value, the ASCII value of the character/keystroke.  For
 *  special keys, however, QKeyEvent::key() returns number from 0x01000000 on
 *  up, and QKeyEvent::text() yields only 0x00.  Also, the key() function
 *  doesn't distinguish between upper and lower case keys, so the text()
 *  function must be used.
 *
 *  So we have some issues:
 *
 *      -   The Gtkmm 2.4 version of Sequencer64 has access to all keycodes
 *          available from a user's keyboard.  (Many keyboards do not provide
 *          all of the keys that are possible.)  The Qt 5 version disregards
 *          whether the left or right versions of Shift, Alt, Ctrl, Super, are
 *          selected, or whether the keypad versions of the numbers and
 *          arithmetic operators are selected... all the codes are the same.
 *          Therefore, the Qt 5 version can access only a subset of the
 *          keycodes configured by the Gtkmm 2.4 version in the "rc" file.
 *      -   To allow both versions to use the same configuration files,
 *          the Qt 5 version must remap the key()/text() combinations
 *          to something sensible in the Gtkmm 2.4 configuration.
 *      -   We have to detect the range of a keystroke (i.e. 0x1000000 or
 *          above) and decide whether to use key() or text().
 *
 *  So, when generating a keystroke configuration in the  "rc" file from the Qt
 *  5 version (not yet implemented), we have to remap the Qt 5 keys to the
 *  Gtkmm 2.4 keys.  And the same thing needs to be done in the Qt 5 version
 *  when processing key-press and key-release events.  And we need to decide if
 *  a keystroke is normal or special.
 */

#include <map>                          /* std::map                         */

#include "qskeymaps.hpp"                /* free functions, seq64 namespace  */

/*
 *  Do not document a namespace, it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Provides a simple structrue to hold the name of a special key, its Gdk
 *  numerical value (0xFF00 to 0xFFFF), and its Qt numerical value (0x1000000
 *  and above).
 */

typedef struct
{
    std::string gks_key_name;       /**< The djinned up name of the key.    */
    unsigned gks_gdk_key_value;     /**< The key's value in Gtkmm 2.4.      */
    unsigned gks_qt_key_value;      /**< The key's value in Qt 5.           */

} q_key_spec_t;

/**
 *  Provides a data type to convert from incoming Qt 5 key() and text() values
 *  into the corresponding Gdk/Gtkmm key value.
 *
 *  It can also be used to convert from Gtkmm 2.4 to Qt 5, but this conversion
 *  is slower and generally not necessary, as Sequencer64 uses the Gtkmm key
 *  set as the canonical key set, both in processing and in storage in the
 *  "rc" configuration file.
 */

typedef std::map<unsigned, q_key_spec_t> QtGtkKeyMap;

/**
 *  The initializer list for the QtGtkKeyMap structure.  Requires C++11 and
 *  above in order to compile.
 */

#if __cplusplus >= 201103L                  /* C++11                        */

static QtGtkKeyMap sg_key_map =
{
    { 0x1000000, { "Escape",       0xff1b, 0x1000000 } },   // see "ESC" below
    { 0x1000003, { "Backspace",    0xff08, 0x1000003 } },
    { 0x1000004, { "Return",       0xff0d, 0x1000004 } },
    { 0x1000005, { "KP_Enter",     0xff8d, 0x1000005 } },
    { 0x1000006, { "Insert",       0xff63, 0x1000006 } },
    { 0x1000006, { "KP_Insert",    0xff9e, 0x1000006 } },
    { 0x1000007, { "KP_Delete",    0xff9f, 0x1000007 } },
    { 0x1000007, { "Delete",       0xffff, 0x1000007 } },
    { 0x1000008, { "Pause",        0xff13, 0x1000008 } },
    { 0x1000009, { "Print Scrn",   0xff61, 0x1000009 } },
    { 0x1000010, { "Home",         0xff50, 0x1000010 } },
    { 0x1000011, { "End",          0xff51, 0x1000011 } },
    { 0x1000011, { "KP_End",       0xff9c, 0x1000011 } },
    { 0x1000012, { "Left",         0xff51, 0x1000012 } },
    { 0x1000013, { "Up",           0xff52, 0x1000013 } },
    { 0x1000014, { "Right",        0xff53, 0x1000014 } },
    { 0x1000014, { "KP_Right",     0xff98, 0x1000014 } },
    { 0x1000015, { "Down",         0xff54, 0x1000015 } },
    { 0x1000015, { "KP_Down",      0xff99, 0x1000015 } },
    { 0x1000016, { "Page Up",      0xff55, 0x1000016 } },
    { 0x1000017, { "Page Down",    0xff56, 0x1000017 } },
    { 0x1000020, { "Shift",        0xffe1, 0x1000020 } },
    { 0x1000020, { "Shift",        0xffe2, 0x1000020 } },
    { 0x1000021, { "Control",      0xffe3, 0x1000021 } },
    { 0x1000022, { "Super",        0xffeb, 0x1000022 } },
    { 0x1000023, { "Alt",          0xffe9, 0x1000023 } },
    { 0x1000025, { "Num_Lock",     0xff7f, 0x1000025 } },
    { 0x1000026, { "Scroll_Lock",  0xff14, 0x1000026 } },
    { 0x1000030, { "F1",           0xffbe, 0x1000030 } },
    { 0x1000031, { "F2",           0xffbf, 0x1000031 } },
    { 0x1000032, { "F3",           0xffc0, 0x1000032 } },
    { 0x1000033, { "F4",           0xffc1, 0x1000033 } },
    { 0x1000034, { "F5",           0xffc2, 0x1000034 } },
    { 0x1000035, { "F6",           0xffc3, 0x1000035 } },
    { 0x1000036, { "F7",           0xffc4, 0x1000036 } },
    { 0x1000037, { "F8",           0xffc5, 0x1000037 } },
    { 0x1000038, { "F9",           0xffc6, 0x1000038 } },
    { 0x1000039, { "F10",          0xffc7, 0x1000039 } },
    { 0x100003a, { "F11",          0xffc8, 0x100003a } },
    { 0x100003b, { "F12",          0xffc9, 0x100003b } },
    { 0x1000055, { "Menu",         0xff67, 0x1000055 } }
};

#else

typedef struct
{
    unsigned keycode;
    q_key_spec_t keyspec;

} keycode_spec_t;

static keycode_spec_t sg_key_map_spec [] =
{
    { 0x1000000, { "Escape",       0xff1b, 0x1000000 } },   // see "ESC" below
    { 0x1000003, { "Backspace",    0xff08, 0x1000003 } },
    { 0x1000004, { "Return",       0xff0d, 0x1000004 } },
    { 0x1000005, { "KP_Enter",     0xff8d, 0x1000005 } },
    { 0x1000006, { "Insert",       0xff63, 0x1000006 } },
    { 0x1000006, { "KP_Insert",    0xff9e, 0x1000006 } },
    { 0x1000007, { "KP_Delete",    0xff9f, 0x1000007 } },
    { 0x1000007, { "Delete",       0xffff, 0x1000007 } },
    { 0x1000008, { "Pause",        0xff13, 0x1000008 } },
    { 0x1000009, { "Print Scrn",   0xff61, 0x1000009 } },
    { 0x1000010, { "Home",         0xff50, 0x1000010 } },
    { 0x1000011, { "End",          0xff51, 0x1000011 } },
    { 0x1000011, { "KP_End",       0xff9c, 0x1000011 } },
    { 0x1000012, { "Left",         0xff51, 0x1000012 } },
    { 0x1000013, { "Up",           0xff52, 0x1000013 } },
    { 0x1000014, { "Right",        0xff53, 0x1000014 } },
    { 0x1000014, { "KP_Right",     0xff98, 0x1000014 } },
    { 0x1000015, { "Down",         0xff54, 0x1000015 } },
    { 0x1000015, { "KP_Down",      0xff99, 0x1000015 } },
    { 0x1000016, { "Page Up",      0xff55, 0x1000016 } },
    { 0x1000017, { "Page Down",    0xff56, 0x1000017 } },
    { 0x1000020, { "Shift",        0xffe1, 0x1000020 } },
    { 0x1000020, { "Shift",        0xffe2, 0x1000020 } },
    { 0x1000021, { "Control",      0xffe3, 0x1000021 } },
    { 0x1000022, { "Super",        0xffeb, 0x1000022 } },
    { 0x1000023, { "Alt",          0xffe9, 0x1000023 } },
    { 0x1000025, { "Num_Lock",     0xff7f, 0x1000025 } },
    { 0x1000026, { "Scroll_Lock",  0xff14, 0x1000026 } },
    { 0x1000030, { "F1",           0xffbe, 0x1000030 } },
    { 0x1000031, { "F2",           0xffbf, 0x1000031 } },
    { 0x1000032, { "F3",           0xffc0, 0x1000032 } },
    { 0x1000033, { "F4",           0xffc1, 0x1000033 } },
    { 0x1000034, { "F5",           0xffc2, 0x1000034 } },
    { 0x1000035, { "F6",           0xffc3, 0x1000035 } },
    { 0x1000036, { "F7",           0xffc4, 0x1000036 } },
    { 0x1000037, { "F8",           0xffc5, 0x1000037 } },
    { 0x1000038, { "F9",           0xffc6, 0x1000038 } },
    { 0x1000039, { "F10",          0xffc7, 0x1000039 } },
    { 0x100003a, { "F11",          0xffc8, 0x100003a } },
    { 0x100003b, { "F12",          0xffc9, 0x100003b } },
    { 0x1000055, { "Menu",         0xff67, 0x1000055 } },
    { 0x0,       { "0",            0x0,    0x0 } }
};

static QtGtkKeyMap sg_key_map;

void
initialize_key_map ()               // where can we call this?
{
    keycode_spec_t * ksptr = &sg_key_map_spec[0];
    for (;;)
    {
        if (ksptr->keycode != 0)
        {
            std::pair<unsigned, q_key_spec_t> p =
                std::make_pair<unsigned, q_key_spec_t>
                (
                    ksptr->keycode, ksptr->keyspec
                );
            (void) sg_key_map.insert(p);
        }
        else
            break;
    }

}

#endif  // __cplusplus >= 201103L

/**
 *  This is meant to accept all key-codes.  However, unless the key-code is
 *  0x1000000 or above, the key-code is returned unaltered, as a normal ASCII
 *  keystroke.
 *
 * ESC Exception:
 *
 *      Although the Esc key does yield key() == 0x1000000, as set up in the
 *      list above, it also yields text() == 0x1b.  We still need to map it
 *      to Gtkmm's 0xff1b.
 *
 * \param qtkey
 *      The Qt 5 key-code, as provided to the Qt keyPress() callback via
 *      QKeyEvent::key().  This value does not distinguish between lower-case
 *      and upper-case characters.
 *
 * \param qttext
 *      The Qt 5 text-code, as provided to the Qt keyPress() callback via
 *      the first byte of QKeyEvent::text(), using the QS_TEXT_CHAR() macro.
 *      For special characters (e.g. function keys, keypad with Num Lock off,
 *      Shift, Alt, Ctrl, Super, and Menu), this value is 0.
 *
 * \return
 *      If the qttext parameter is greater than 0, it is returned unaltered.
 *      If the qtkey parameter is less than 0x1000000 (this should not
 *      happen), it is returned unaltered.  Otherwise, the keycode is looked
 *      up in the sg_key_map, and, if found, the corresponding Gdk keycode is
 *      returned.  If not found, then 0 is returned.  This would indicate an
 *      error of some kind.
 */

unsigned
qt_map_to_gdk (unsigned qtkey, unsigned qttext)
{
    if (qttext > 0 && qttext != 0x1b)
    {
        return qttext;
    }
    else
    {
        if (qtkey >= 0x1000000)
        {
            QtGtkKeyMap::const_iterator qi = sg_key_map.find(qtkey);
            if (qi != sg_key_map.end())
                return qi->second.gks_gdk_key_value;
            else
                return 0;
        }
        else
            return qtkey;
    }
}

/**
 *  Looks up the name of the key/text combination.  Useful mainly for
 *  debugging, as the names are all based on Gdk in Sequencer64.
 *
 *  Later, we will incorporate this function in
 *
 * \param qtkey
 *      The Qt 5 key-code, as provided to the Qt keyPress() callback via
 *      QKeyEvent::key().  This value does not distinguish between lower-case
 *      and upper-case characters.
 *
 * \param qttext
 *      The Qt 5 text-code, as provided to the Qt keyPress() callback via
 *      the first byte of QKeyEvent::text(), using the QS_TEXT_CHAR() macro.
 *      For special characters (e.g. function keys, keypad with Num Lock off,
 *      Shift, Alt, Ctrl, Super, and Menu), this value is 0.
 *
 * \return
 */

std::string
qt_key_name (unsigned qtkey, unsigned qttext)
{
    if (qttext > 0)
    {
        if (qttext == 0x20)
        {
            return std::string("Space");
        }
        else
        {
            char temp[2];
            temp[0] = char(qttext);
            temp[1] = 0;
            return std::string(temp);
        }
    }
    else
    {
        if (qtkey >= 0x1000000)
        {
            QtGtkKeyMap::const_iterator gi = sg_key_map.find(qtkey);
            if (gi != sg_key_map.end())
                return gi->second.gks_key_name;
            else
                return std::string("");
        }
        else
            return std::string("");
    }
}

/**
 *  The inverse of qt_map_to_gdk().  Slower, due to a brute force search.
 *
 * \param gdkkeycode
 *      The Gdk key-code to look up.
 *
 * \return
 *      If the Gdk maps to a special character, this function returns the
 *      corresponding Qt 5 key-code (0x1000000 and above) or, if a normal
 *      character, the Qt 5 text-code (in the ASCII) range.
 */

unsigned
gdk_map_to_qt (unsigned gdkkeycode)
{
    unsigned result = 0;
    if (gdkkeycode >= 0xFF00)
    {
        for
        (
            QtGtkKeyMap::const_iterator gi = sg_key_map.begin();
            gi != sg_key_map.end(); ++gi
        )
        {
            if (gi->second.gks_gdk_key_value == gdkkeycode)
            {
                result = gi->second.gks_qt_key_value;
                break;
            }
        }
        if (result == 0)                /* the value was not found in list  */
            result = gdkkeycode;
    }
    else
        result = gdkkeycode;

    return result;
}

}               // namespace seq64

/*
 * qskeymaps.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */



