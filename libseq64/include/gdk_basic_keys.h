#ifndef SEQ64_GDK_BASIC_KEYS_HPP
#define SEQ64_GDK_BASIC_KEYS_HPP

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
 * \updates       2015-09-28
 * \license       GNU GPLv2 or above
 *
 *  This file is provided as a convenience so that we have some reasonable
 *  and universal set of basic PC keys to use in Sequencer64 code.  It
 *  defines keys, key modifiers, and other values universal between
 *  graphical user-interface frameworks.  Almost all of these values are
 *  used by Sequencer64; we've defined only the ones we need.
 */

#ifndef GDK_KEY_Home

#define GDK_KEY_BackSpace           0xff08
#define GDK_KEY_Tab                 0xff09
#define GDK_KEY_Linefeed            0xff0a
#define GDK_KEY_Clear               0xff0b
#define GDK_KEY_Return              0xff0d
#define GDK_KEY_Pause               0xff13
#define GDK_KEY_Scroll_Lock         0xff14
#define GDK_KEY_Sys_Req             0xff15
#define GDK_KEY_Escape              0xff1b
#define GDK_KEY_Delete              0xffff
#define GDK_KEY_Home                0xff50
#define GDK_KEY_Left                0xff51
#define GDK_KEY_Up                  0xff52
#define GDK_KEY_Right               0xff53
#define GDK_KEY_Down                0xff54
#define GDK_KEY_Prior               0xff55
#define GDK_KEY_Page_Up             0xff55
#define GDK_KEY_Next                0xff56
#define GDK_KEY_Page_Down           0xff56
#define GDK_KEY_End                 0xff57
#define GDK_KEY_Begin               0xff58
#define GDK_KEY_Select              0xff60
#define GDK_KEY_Print               0xff61
#define GDK_KEY_Execute             0xff62
#define GDK_KEY_Insert              0xff63
#define GDK_KEY_Undo                0xff65
#define GDK_KEY_Redo                0xff66
#define GDK_KEY_Menu                0xff67
#define GDK_KEY_Find                0xff68
#define GDK_KEY_Cancel              0xff69
#define GDK_KEY_Help                0xff6a
#define GDK_KEY_Break               0xff6b
#define GDK_KEY_Mode_switch         0xff7e
#define GDK_KEY_script_switch       0xff7e
#define GDK_KEY_Num_Lock            0xff7f
#define GDK_KEY_Shift_L             0xffe1
#define GDK_KEY_Shift_R             0xffe2
#define GDK_KEY_Control_L           0xffe3
#define GDK_KEY_Control_R           0xffe4
#define GDK_KEY_Caps_Lock           0xffe5
#define GDK_KEY_Shift_Lock          0xffe6
#define GDK_KEY_Meta_L              0xffe7
#define GDK_KEY_Meta_R              0xffe8
#define GDK_KEY_Alt_L               0xffe9
#define GDK_KEY_Alt_R               0xffea
#define GDK_KEY_Super_L             0xffeb
#define GDK_KEY_Super_R             0xffec
#define GDK_KEY_Hyper_L             0xffed
#define GDK_KEY_Hyper_R             0xffee
#define GDK_KEY_ISO_Lock            0xfe01
#define GDK_KEY_space               0x020
#define GDK_KEY_exclam              0x021
#define GDK_KEY_quotedbl            0x022
#define GDK_KEY_numbersign          0x023
#define GDK_KEY_dollar              0x024
#define GDK_KEY_percent             0x025
#define GDK_KEY_ampersand           0x026
#define GDK_KEY_apostrophe          0x027
#define GDK_KEY_quoteright          0x027
#define GDK_KEY_parenleft           0x028
#define GDK_KEY_parenright          0x029
#define GDK_KEY_asterisk            0x02a
#define GDK_KEY_plus                0x02b
#define GDK_KEY_comma               0x02c
#define GDK_KEY_minus               0x02d
#define GDK_KEY_period              0x02e
#define GDK_KEY_slash               0x02f
#define GDK_KEY_0                   0x030
#define GDK_KEY_1                   0x031
#define GDK_KEY_2                   0x032
#define GDK_KEY_3                   0x033
#define GDK_KEY_4                   0x034
#define GDK_KEY_5                   0x035
#define GDK_KEY_6                   0x036
#define GDK_KEY_7                   0x037
#define GDK_KEY_8                   0x038
#define GDK_KEY_9                   0x039
#define GDK_KEY_colon               0x03a
#define GDK_KEY_semicolon           0x03b
#define GDK_KEY_less                0x03c
#define GDK_KEY_equal               0x03d
#define GDK_KEY_greater             0x03e
#define GDK_KEY_question            0x03f
#define GDK_KEY_at                  0x040
#define GDK_KEY_A                   0x041
#define GDK_KEY_B                   0x042
#define GDK_KEY_C                   0x043
#define GDK_KEY_D                   0x044
#define GDK_KEY_E                   0x045
#define GDK_KEY_F                   0x046
#define GDK_KEY_G                   0x047
#define GDK_KEY_H                   0x048
#define GDK_KEY_I                   0x049
#define GDK_KEY_J                   0x04a
#define GDK_KEY_K                   0x04b
#define GDK_KEY_L                   0x04c
#define GDK_KEY_M                   0x04d
#define GDK_KEY_N                   0x04e
#define GDK_KEY_O                   0x04f
#define GDK_KEY_P                   0x050
#define GDK_KEY_Q                   0x051
#define GDK_KEY_R                   0x052
#define GDK_KEY_S                   0x053
#define GDK_KEY_T                   0x054
#define GDK_KEY_U                   0x055
#define GDK_KEY_V                   0x056
#define GDK_KEY_W                   0x057
#define GDK_KEY_X                   0x058
#define GDK_KEY_Y                   0x059
#define GDK_KEY_Z                   0x05a
#define GDK_KEY_bracketleft         0x05b
#define GDK_KEY_backslash           0x05c
#define GDK_KEY_bracketright        0x05d
#define GDK_KEY_asciicircum         0x05e
#define GDK_KEY_underscore          0x05f
#define GDK_KEY_grave               0x060
#define GDK_KEY_quoteleft           0x060
#define GDK_KEY_a                   0x061
#define GDK_KEY_b                   0x062
#define GDK_KEY_c                   0x063
#define GDK_KEY_d                   0x064
#define GDK_KEY_e                   0x065
#define GDK_KEY_f                   0x066
#define GDK_KEY_g                   0x067
#define GDK_KEY_h                   0x068
#define GDK_KEY_i                   0x069
#define GDK_KEY_j                   0x06a
#define GDK_KEY_k                   0x06b
#define GDK_KEY_l                   0x06c
#define GDK_KEY_m                   0x06d
#define GDK_KEY_n                   0x06e
#define GDK_KEY_o                   0x06f
#define GDK_KEY_p                   0x070
#define GDK_KEY_q                   0x071
#define GDK_KEY_r                   0x072
#define GDK_KEY_s                   0x073
#define GDK_KEY_t                   0x074
#define GDK_KEY_u                   0x075
#define GDK_KEY_v                   0x076
#define GDK_KEY_w                   0x077
#define GDK_KEY_x                   0x078
#define GDK_KEY_y                   0x079
#define GDK_KEY_z                   0x07a
#define GDK_KEY_braceleft           0x07b
#define GDK_KEY_bar                 0x07c
#define GDK_KEY_braceright          0x07d
#define GDK_KEY_asciitilde          0x07e

#define GDK_KEY_igrave              0x0ec

#endif      // GDK_KEY_Home

/**
 * Types of modifiers, essentially copied from gtk-2.0/gdk/gdktypes.h.
 */

typedef enum
{
    GDK_NO_MASK         = 0,
    GDK_SHIFT_MASK      = 1,
    GDK_LOCK_MASK	    = 1 << 1,
    GDK_CONTROL_MASK    = 1 << 2,
    GDK_MOD1_MASK	    = 1 << 3,
    GDK_MOD2_MASK	    = 1 << 4,
    GDK_MOD3_MASK	    = 1 << 5,
    GDK_MOD4_MASK	    = 1 << 6,
    GDK_MOD5_MASK	    = 1 << 7,
    GDK_BUTTON1_MASK    = 1 << 8,
    GDK_BUTTON2_MASK    = 1 << 9,
    GDK_BUTTON3_MASK    = 1 << 10,
    GDK_BUTTON4_MASK    = 1 << 11,
    GDK_BUTTON5_MASK    = 1 << 12,

    /*
    * Bits 13 and 14 are used by XKB, bits 15 to 25 are unused. Bit 29 is
    * used internally.
    */

    GDK_SUPER_MASK      = 1 << 26,
    GDK_HYPER_MASK      = 1 << 27,
    GDK_META_MASK       = 1 << 28,
    GDK_RELEASE_MASK    = 1 << 30   // GDK_MODIFIER_MASK = 0x5c001fff

} gdk_modifier_t;

/**
 * Types of scroll events, essentially copied from gtk-2.0/gdk/gdkevents.h.
 */

typedef enum
{
    GDK_SCROLL_UP,
    GDK_SCROLL_DOWN,
    GDK_SCROLL_LEFT,
    GDK_SCROLL_RIGHT

} gdk_scroll_direction_t;


#endif      // SEQ64_GDK_BASIC_KEYS_HPP

/*
 * gdk_basic_keys.h
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
