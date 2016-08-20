#ifndef SEQ64_GDK_BASIC_KEYS_H
#define SEQ64_GDK_BASIC_KEYS_H

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
 * \file          gdk_basic_keys.h
 *
 *  This module defines a non-Unicode subset of the keys defined by Gtk-2/3.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2016-08-20
 * \license       GNU GPLv2 or above
 *
 *  This file is provided as a convenience so that we have some reasonable
 *  and universal set of basic PC keys to use in Sequencer64 code.  It
 *  defines keys, key modifiers, and other values universal between
 *  graphical user-interface frameworks.  Almost all of these values are
 *  used by Sequencer64; we've defined only the ones we need.
 *
 *  Shamelessly derived from the following files:
 *
 *      /usr/include/gtk-2.0/gdk/gdkkeysyms-compat.h
 *      /usr/include/gtk-2.0/gdk/gdkevents.h
 */

/**
 *  Provides a macro for comparing our types with GDK types, with absolutely
 *  no safety at all.  Use at your own risk.  We do!
 */

#define CAST_EQUIVALENT(x, y)           ((int)(x) == (int)(y))

#define CAST_AND_EQUIVALENT(x, y, z) \
 ((int)(x) == (int)(y) && (int)(x) == (int)(z))

#define CAST_OR_EQUIVALENT(x, y, z) \
 ((int)(x) == (int)(y) || (int)(x) == (int)(z))

#define AND_EQUIVALENT(x, y, z)         ((x) == (y) && (x) == (z))
#define OR_EQUIVALENT(x, y, z)          ((x) == (y) || (x) == (z))

/**
 *  Defines our own names for keystrokes, so that we don't need to rely
 *  on the headers of a particular user-interface framework.
 */

#ifndef SEQ64_Home

#define SEQ64_BackSpace           0xff08
#define SEQ64_Tab                 0xff09
#define SEQ64_Linefeed            0xff0a
#define SEQ64_Clear               0xff0b
#define SEQ64_Return              0xff0d
#define SEQ64_Pause               0xff13
#define SEQ64_Scroll_Lock         0xff14
#define SEQ64_Sys_Req             0xff15
#define SEQ64_Escape              0xff1b
#define SEQ64_Delete              0xffff
#define SEQ64_Home                0xff50
#define SEQ64_Left                0xff51
#define SEQ64_Up                  0xff52
#define SEQ64_Right               0xff53
#define SEQ64_Down                0xff54
#define SEQ64_Prior               0xff55
#define SEQ64_Page_Up             0xff55
#define SEQ64_Next                0xff56
#define SEQ64_Page_Down           0xff56
#define SEQ64_End                 0xff57
#define SEQ64_Begin               0xff58
#define SEQ64_Select              0xff60
#define SEQ64_Print               0xff61
#define SEQ64_Execute             0xff62
#define SEQ64_Insert              0xff63
#define SEQ64_Undo                0xff65
#define SEQ64_Redo                0xff66
#define SEQ64_Menu                0xff67
#define SEQ64_Find                0xff68
#define SEQ64_Cancel              0xff69
#define SEQ64_Help                0xff6a
#define SEQ64_Break               0xff6b
#define SEQ64_Mode_switch         0xff7e
#define SEQ64_script_switch       0xff7e
#define SEQ64_Num_Lock            0xff7f

#define SEQ64_F1                  0xffbe
#define SEQ64_F2                  0xffbf
#define SEQ64_F3                  0xffc0
#define SEQ64_F4                  0xffc1
#define SEQ64_F5                  0xffc2
#define SEQ64_F6                  0xffc3
#define SEQ64_F7                  0xffc4
#define SEQ64_F8                  0xffc5
#define SEQ64_F9                  0xffc6
#define SEQ64_F10                 0xffc7
#define SEQ64_F11                 0xffc8
#define SEQ64_F12                 0xffc9

#define SEQ64_KP_Space            0xff80
#define SEQ64_KP_Tab              0xff89
#define SEQ64_KP_Enter            0xff8d
#define SEQ64_KP_F1               0xff91
#define SEQ64_KP_F2               0xff92
#define SEQ64_KP_F3               0xff93
#define SEQ64_KP_F4               0xff94
#define SEQ64_KP_Home             0xff95
#define SEQ64_KP_Left             0xff96
#define SEQ64_KP_Up               0xff97
#define SEQ64_KP_Right            0xff98
#define SEQ64_KP_Down             0xff99
#define SEQ64_KP_Prior            0xff9a
#define SEQ64_KP_Page_Up          0xff9a
#define SEQ64_KP_Next             0xff9b
#define SEQ64_KP_Page_Down        0xff9b
#define SEQ64_KP_End              0xff9c
#define SEQ64_KP_Begin            0xff9d
#define SEQ64_KP_Insert           0xff9e
#define SEQ64_KP_Delete           0xff9f
#define SEQ64_KP_Equal            0xffbd
#define SEQ64_KP_Multiply         0xffaa
#define SEQ64_KP_Add              0xffab
#define SEQ64_KP_Separator        0xffac
#define SEQ64_KP_Subtract         0xffad
#define SEQ64_KP_Decimal          0xffae
#define SEQ64_KP_Divide           0xffaf
#define SEQ64_KP_0                0xffb0
#define SEQ64_KP_1                0xffb1
#define SEQ64_KP_2                0xffb2
#define SEQ64_KP_3                0xffb3
#define SEQ64_KP_4                0xffb4
#define SEQ64_KP_5                0xffb5
#define SEQ64_KP_6                0xffb6
#define SEQ64_KP_7                0xffb7
#define SEQ64_KP_8                0xffb8
#define SEQ64_KP_9                0xffb9

#define SEQ64_Shift_L             0xffe1
#define SEQ64_Shift_R             0xffe2
#define SEQ64_Control_L           0xffe3
#define SEQ64_Control_R           0xffe4
#define SEQ64_Caps_Lock           0xffe5
#define SEQ64_Shift_Lock          0xffe6
#define SEQ64_Meta_L              0xffe7
#define SEQ64_Meta_R              0xffe8
#define SEQ64_Alt_L               0xffe9
#define SEQ64_Alt_R               0xffea
#define SEQ64_Super_L             0xffeb
#define SEQ64_Super_R             0xffec
#define SEQ64_Hyper_L             0xffed
#define SEQ64_Hyper_R             0xffee
#define SEQ64_ISO_Lock            0xfe01
#define SEQ64_space               0x020
#define SEQ64_exclam              0x021
#define SEQ64_quotedbl            0x022
#define SEQ64_numbersign          0x023
#define SEQ64_dollar              0x024
#define SEQ64_percent             0x025
#define SEQ64_ampersand           0x026
#define SEQ64_apostrophe          0x027
#define SEQ64_quoteright          0x027
#define SEQ64_parenleft           0x028
#define SEQ64_parenright          0x029
#define SEQ64_asterisk            0x02a
#define SEQ64_plus                0x02b
#define SEQ64_comma               0x02c
#define SEQ64_minus               0x02d
#define SEQ64_period              0x02e
#define SEQ64_slash               0x02f
#define SEQ64_0                   0x030
#define SEQ64_1                   0x031
#define SEQ64_2                   0x032
#define SEQ64_3                   0x033
#define SEQ64_4                   0x034
#define SEQ64_5                   0x035
#define SEQ64_6                   0x036
#define SEQ64_7                   0x037
#define SEQ64_8                   0x038
#define SEQ64_9                   0x039
#define SEQ64_colon               0x03a
#define SEQ64_semicolon           0x03b
#define SEQ64_less                0x03c
#define SEQ64_equal               0x03d
#define SEQ64_greater             0x03e
#define SEQ64_question            0x03f
#define SEQ64_at                  0x040
#define SEQ64_A                   0x041
#define SEQ64_B                   0x042
#define SEQ64_C                   0x043
#define SEQ64_D                   0x044
#define SEQ64_E                   0x045
#define SEQ64_F                   0x046
#define SEQ64_G                   0x047
#define SEQ64_H                   0x048
#define SEQ64_I                   0x049
#define SEQ64_J                   0x04a
#define SEQ64_K                   0x04b
#define SEQ64_L                   0x04c
#define SEQ64_M                   0x04d
#define SEQ64_N                   0x04e
#define SEQ64_O                   0x04f
#define SEQ64_P                   0x050
#define SEQ64_Q                   0x051
#define SEQ64_R                   0x052
#define SEQ64_S                   0x053
#define SEQ64_T                   0x054
#define SEQ64_U                   0x055
#define SEQ64_V                   0x056
#define SEQ64_W                   0x057
#define SEQ64_X                   0x058
#define SEQ64_Y                   0x059
#define SEQ64_Z                   0x05a
#define SEQ64_bracketleft         0x05b
#define SEQ64_backslash           0x05c
#define SEQ64_bracketright        0x05d
#define SEQ64_asciicircum         0x05e
#define SEQ64_underscore          0x05f
#define SEQ64_grave               0x060
#define SEQ64_quoteleft           0x060
#define SEQ64_a                   0x061
#define SEQ64_b                   0x062
#define SEQ64_c                   0x063
#define SEQ64_d                   0x064
#define SEQ64_e                   0x065
#define SEQ64_f                   0x066
#define SEQ64_g                   0x067
#define SEQ64_h                   0x068
#define SEQ64_i                   0x069
#define SEQ64_j                   0x06a
#define SEQ64_k                   0x06b
#define SEQ64_l                   0x06c
#define SEQ64_m                   0x06d
#define SEQ64_n                   0x06e
#define SEQ64_o                   0x06f
#define SEQ64_p                   0x070
#define SEQ64_q                   0x071
#define SEQ64_r                   0x072
#define SEQ64_s                   0x073
#define SEQ64_t                   0x074
#define SEQ64_u                   0x075
#define SEQ64_v                   0x076
#define SEQ64_w                   0x077
#define SEQ64_x                   0x078
#define SEQ64_y                   0x079
#define SEQ64_z                   0x07a
#define SEQ64_braceleft           0x07b
#define SEQ64_bar                 0x07c
#define SEQ64_braceright          0x07d
#define SEQ64_asciitilde          0x07e
#define SEQ64_igrave              0x0ec

#endif      // SEQ64_Home

namespace seq64
{

/**
 *  Types of modifiers, essentially copied from gtk-2.0/gdk/gdktypes.h.
 *  We have to tweak the names to avoid redeclaration errors and to
 * "personalize" the values.  We change "GDK" to "SEQ64".
 *
 *  Since we're getting events from, say Gtk-2.4, but using our (matching)
 *  values for comparison, use the CAST_EQUIVALENT() macro to compare them.
 *  Note that we might still end up having to a remapping (e.g. if trying to
 *  get the code to work with the Qt framework).
 */

typedef enum
{
    SEQ64_NO_MASK           = 0,
    SEQ64_SHIFT_MASK        = 1,                // Shift modifier key
    SEQ64_LOCK_MASK	        = 1 << 1,           // Lock (scroll)? modifier key
    SEQ64_CONTROL_MASK      = 1 << 2,           // Ctrl modifier key
    SEQ64_MOD1_MASK	        = 1 << 3,           // Alt modifier key
    SEQ64_MOD2_MASK	        = 1 << 4,           // Num Lock modifier key
    SEQ64_MOD3_MASK	        = 1 << 5,           // Hyper_L (?)
    SEQ64_MOD4_MASK	        = 1 << 6,           // Super/Windoze modifier key
    SEQ64_MOD5_MASK	        = 1 << 7,           // Mode_Switch (?)
    SEQ64_BUTTON1_MASK      = 1 << 8,
    SEQ64_BUTTON2_MASK      = 1 << 9,
    SEQ64_BUTTON3_MASK      = 1 << 10,
    SEQ64_BUTTON4_MASK      = 1 << 11,
    SEQ64_BUTTON5_MASK      = 1 << 12,

    /**
     * Bits 13 and 14 are used by XKB, bits 15 to 25 are unused. Bit 29 is
     * used internally.
     */

    SEQ64_SUPER_MASK        = 1 << 26,
    SEQ64_HYPER_MASK        = 1 << 27,
    SEQ64_META_MASK         = 1 << 28,
    SEQ64_RELEASE_MASK      = 1 << 30,  // GDK_MODIFIER_MASK = 0x5c001fff
    SEQ64_MASK_MAX          = 1 << 31

} seq_modifier_t;

/**
 *  Event types copped from gtk-2.0/gdk/gdkevents.h for use with this
 *  application.  Only the values we need have been grabbed.
 *  We have to tweak the names to avoid redeclaration errors and to
 *  "personalize" the values.  We change "GDK" to "SEQ64", but, for
 *  convenience (to hide errors? :-D), we keep the number the same.
 *
 *  Since we're getting events from, say Gtk-2.4, but using our (matching)
 *  values for comparison, use the CAST_EQUIVALENT() macro to compare them.
 *  Note that we might still end up having to a remapping (e.g. if trying to
 *  get the code to work with the Qt framework).
 */

typedef enum
{
    SEQ64_NOTHING           = -1,
    SEQ64_DELETE		    = 0,
    SEQ64_DESTROY		    = 1,
    SEQ64_EXPOSE		    = 2,
    SEQ64_MOTION_NOTIFY	    = 3,
    SEQ64_BUTTON_PRESS	    = 4,
    SEQ64_2BUTTON_PRESS	    = 5,
    SEQ64_3BUTTON_PRESS	    = 6,
    SEQ64_BUTTON_RELEASE	= 7,
    SEQ64_KEY_PRESS         = 8,
    SEQ64_KEY_RELEASE       = 9,
    SEQ64_SCROLL            = 31,
    SEQ64_EVENT_LAST

} seq_event_type_t;

/**
 * Types of scroll events, essentially copied from gtk-2.0/gdk/gdkevents.h.
 *  We have to tweak the names to avoid redeclaration errors and to
 * "personalize" the values.  We change "SEQ64" to "SEQ64".
 *
 *  Since we're getting events from, say Gtk-2.4, but using our (matching)
 *  values for comparison, use the CAST_EQUIVALENT() macro to compare them.
 *  Note that we might still end up having to a remapping (e.g. if trying to
 *  get the code to work with the Qt framework).
 */

typedef enum
{
    SEQ64_SCROLL_UP,
    SEQ64_SCROLL_DOWN,
    SEQ64_SCROLL_LEFT,
    SEQ64_SCROLL_RIGHT

} seq_scroll_direction_t;

}           // namespace seq64

#endif      // SEQ64_GDK_BASIC_KEYS_H

/*
 * gdk_basic_keys.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

