/*
 * @(#)x_key_utils.c	1.10 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 *
 */
#define XK_KATAKANA

#include "Xheaders.h"
#include "java_awt_event_KeyEvent.h"


#define  FALSE   0
#define  TRUE    1

typedef char Boolean;

typedef struct KEYMAP_ENTRY {
    long awtKey;
    KeySym x11Key;
    Boolean printable;
} KeymapEntry;

KeymapEntry keymapTable[] = {
    { java_awt_event_KeyEvent_VK_A, XK_a, TRUE },
    { java_awt_event_KeyEvent_VK_B, XK_b, TRUE },
    { java_awt_event_KeyEvent_VK_C, XK_c, TRUE },
    { java_awt_event_KeyEvent_VK_D, XK_d, TRUE },
    { java_awt_event_KeyEvent_VK_E, XK_e, TRUE },
    { java_awt_event_KeyEvent_VK_F, XK_f, TRUE },
    { java_awt_event_KeyEvent_VK_G, XK_g, TRUE },
    { java_awt_event_KeyEvent_VK_H, XK_h, TRUE },
    { java_awt_event_KeyEvent_VK_I, XK_i, TRUE },
    { java_awt_event_KeyEvent_VK_J, XK_j, TRUE },
    { java_awt_event_KeyEvent_VK_K, XK_k, TRUE },
    { java_awt_event_KeyEvent_VK_L, XK_l, TRUE },
    { java_awt_event_KeyEvent_VK_M, XK_m, TRUE },
    { java_awt_event_KeyEvent_VK_N, XK_n, TRUE },
    { java_awt_event_KeyEvent_VK_O, XK_o, TRUE },
    { java_awt_event_KeyEvent_VK_P, XK_p, TRUE },
    { java_awt_event_KeyEvent_VK_Q, XK_q, TRUE },
    { java_awt_event_KeyEvent_VK_R, XK_r, TRUE },
    { java_awt_event_KeyEvent_VK_S, XK_s, TRUE },
    { java_awt_event_KeyEvent_VK_T, XK_t, TRUE },
    { java_awt_event_KeyEvent_VK_U, XK_u, TRUE },
    { java_awt_event_KeyEvent_VK_V, XK_v, TRUE },
    { java_awt_event_KeyEvent_VK_W, XK_w, TRUE },
    { java_awt_event_KeyEvent_VK_X, XK_x, TRUE },
    { java_awt_event_KeyEvent_VK_Y, XK_y, TRUE },
    { java_awt_event_KeyEvent_VK_Z, XK_z, TRUE },

    { java_awt_event_KeyEvent_VK_ENTER, XK_Return, TRUE },
    { java_awt_event_KeyEvent_VK_ENTER, XK_KP_Enter, TRUE },
    { java_awt_event_KeyEvent_VK_ENTER, XK_Linefeed, TRUE },

    { java_awt_event_KeyEvent_VK_BACK_SPACE, XK_BackSpace, TRUE },
    { java_awt_event_KeyEvent_VK_TAB, XK_Tab, TRUE },
    { java_awt_event_KeyEvent_VK_CANCEL, XK_Cancel, FALSE },
    { java_awt_event_KeyEvent_VK_CLEAR, XK_Clear, FALSE },
    { java_awt_event_KeyEvent_VK_SHIFT, XK_Shift_L, FALSE },
    { java_awt_event_KeyEvent_VK_SHIFT, XK_Shift_R, FALSE }, 
    { java_awt_event_KeyEvent_VK_CONTROL, XK_Control_L, FALSE },
    { java_awt_event_KeyEvent_VK_CONTROL, XK_Control_R, FALSE }, 
    { java_awt_event_KeyEvent_VK_ALT, XK_Alt_L, FALSE },
    { java_awt_event_KeyEvent_VK_ALT, XK_Alt_R, FALSE },
    { java_awt_event_KeyEvent_VK_META, XK_Meta_L, FALSE },
    { java_awt_event_KeyEvent_VK_META, XK_Meta_R, FALSE }, 
    { java_awt_event_KeyEvent_VK_PAUSE, XK_Pause, FALSE },
    { java_awt_event_KeyEvent_VK_CAPS_LOCK, XK_Caps_Lock, FALSE },
    { java_awt_event_KeyEvent_VK_ESCAPE, XK_Escape, TRUE },
    { java_awt_event_KeyEvent_VK_SPACE, XK_space, TRUE },

    { java_awt_event_KeyEvent_VK_PAGE_UP, XK_Page_Up, FALSE },
    { java_awt_event_KeyEvent_VK_PAGE_UP, XK_R9, FALSE },
    { java_awt_event_KeyEvent_VK_PAGE_UP, XK_Prior, FALSE },
    { java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_Page_Down, FALSE },
    { java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_R15, FALSE },
    { java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_Next, FALSE },
    { java_awt_event_KeyEvent_VK_END, XK_R13, FALSE },
    { java_awt_event_KeyEvent_VK_END, XK_End, FALSE },
    { java_awt_event_KeyEvent_VK_HOME, XK_Home, FALSE },
    { java_awt_event_KeyEvent_VK_HOME, XK_R7, FALSE },

    { java_awt_event_KeyEvent_VK_LEFT, XK_Left, FALSE },
    { java_awt_event_KeyEvent_VK_UP, XK_Up, FALSE },
    { java_awt_event_KeyEvent_VK_RIGHT, XK_Right, FALSE },
    { java_awt_event_KeyEvent_VK_DOWN, XK_Down, FALSE },
    { java_awt_event_KeyEvent_VK_INSERT, XK_Insert, FALSE },
    { java_awt_event_KeyEvent_VK_HELP, XK_Help, FALSE },

    { java_awt_event_KeyEvent_VK_0, XK_0, TRUE },
    { java_awt_event_KeyEvent_VK_1, XK_1, TRUE },
    { java_awt_event_KeyEvent_VK_2, XK_2, TRUE },
    { java_awt_event_KeyEvent_VK_3, XK_3, TRUE },
    { java_awt_event_KeyEvent_VK_4, XK_4, TRUE },
    { java_awt_event_KeyEvent_VK_5, XK_5, TRUE },
    { java_awt_event_KeyEvent_VK_6, XK_6, TRUE },
    { java_awt_event_KeyEvent_VK_7, XK_7, TRUE },
    { java_awt_event_KeyEvent_VK_8, XK_8, TRUE },
    { java_awt_event_KeyEvent_VK_9, XK_9, TRUE },

    { java_awt_event_KeyEvent_VK_EQUALS, XK_equal, TRUE },
    { java_awt_event_KeyEvent_VK_BACK_SLASH, XK_backslash, TRUE },
    { java_awt_event_KeyEvent_VK_BACK_QUOTE, XK_grave, TRUE },
    { java_awt_event_KeyEvent_VK_OPEN_BRACKET, XK_bracketleft, TRUE },
    { java_awt_event_KeyEvent_VK_CLOSE_BRACKET, XK_bracketright, TRUE },
    { java_awt_event_KeyEvent_VK_SEMICOLON, XK_semicolon, TRUE },
    { java_awt_event_KeyEvent_VK_QUOTE, XK_apostrophe, TRUE },
    { java_awt_event_KeyEvent_VK_COMMA, XK_comma, TRUE },
    { java_awt_event_KeyEvent_VK_PERIOD, XK_period, TRUE },
    { java_awt_event_KeyEvent_VK_SLASH, XK_slash, TRUE },

    { java_awt_event_KeyEvent_VK_NUMPAD0, XK_KP_0, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD1, XK_KP_1, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD2, XK_KP_2, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD3, XK_KP_3, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD4, XK_KP_4, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD5, XK_KP_5, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD6, XK_KP_6, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD7, XK_KP_7, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD8, XK_KP_8, TRUE },
    { java_awt_event_KeyEvent_VK_NUMPAD9, XK_KP_9, TRUE },
    { java_awt_event_KeyEvent_VK_MULTIPLY, XK_KP_Multiply, TRUE },
    { java_awt_event_KeyEvent_VK_ADD, XK_KP_Add, TRUE },
    { java_awt_event_KeyEvent_VK_SUBTRACT, XK_KP_Subtract, TRUE },
    { java_awt_event_KeyEvent_VK_DECIMAL, XK_KP_Decimal, TRUE },
    { java_awt_event_KeyEvent_VK_DIVIDE, XK_KP_Divide, TRUE },
    { java_awt_event_KeyEvent_VK_EQUALS, XK_KP_Equal, TRUE },
    { java_awt_event_KeyEvent_VK_INSERT, XK_KP_Insert, FALSE },
    { java_awt_event_KeyEvent_VK_ENTER, XK_KP_Enter, FALSE },

    { java_awt_event_KeyEvent_VK_F1, XK_F1, FALSE },
    { java_awt_event_KeyEvent_VK_F2, XK_F2, FALSE },
    { java_awt_event_KeyEvent_VK_F3, XK_F3, FALSE },
    { java_awt_event_KeyEvent_VK_F4, XK_F4, FALSE },
    { java_awt_event_KeyEvent_VK_F5, XK_F5, FALSE },
    { java_awt_event_KeyEvent_VK_F6, XK_F6, FALSE },
    { java_awt_event_KeyEvent_VK_F7, XK_F7, FALSE },
    { java_awt_event_KeyEvent_VK_F8, XK_F8, FALSE },
    { java_awt_event_KeyEvent_VK_F9, XK_F9, FALSE },
    { java_awt_event_KeyEvent_VK_F10, XK_F10, FALSE },
    { java_awt_event_KeyEvent_VK_F11, XK_F11, FALSE },
    { java_awt_event_KeyEvent_VK_F12, XK_F12, FALSE },

    { java_awt_event_KeyEvent_VK_DELETE, XK_Delete, TRUE },
    { java_awt_event_KeyEvent_VK_DELETE, XK_KP_Delete, TRUE },

    { java_awt_event_KeyEvent_VK_NUM_LOCK, XK_Num_Lock, FALSE },
    { java_awt_event_KeyEvent_VK_SCROLL_LOCK, XK_Scroll_Lock, FALSE },
    { java_awt_event_KeyEvent_VK_PRINTSCREEN, XK_Print, FALSE },

    { java_awt_event_KeyEvent_VK_ACCEPT, XK_Execute, FALSE },
    { java_awt_event_KeyEvent_VK_CONVERT, XK_Henkan, FALSE },
    { java_awt_event_KeyEvent_VK_NONCONVERT, XK_Muhenkan, FALSE },
    { java_awt_event_KeyEvent_VK_MODECHANGE, XK_Henkan_Mode, FALSE },
    { java_awt_event_KeyEvent_VK_KANA, XK_Katakana, FALSE },
    { java_awt_event_KeyEvent_VK_KANA, XK_kana_switch, FALSE },
    { java_awt_event_KeyEvent_VK_KANJI, XK_Kanji, FALSE },

    { 0, 0, 0 }
};

static void keysymToAWTKeyCode(KeySym x11Key, long *keycode, Boolean *printable) 
{
    int i;

    for (i = 0; keymapTable[i].awtKey != 0; i++) {
        if (keymapTable[i].x11Key == x11Key) {
            *keycode = keymapTable[i].awtKey;
            *printable = keymapTable[i].printable;
            return;
        }
    }

    *keycode = 0;
    *printable = FALSE;
/*
    printf("#### No key mapping found : (X11)%x#### \n", x11Key);
    */
}

int LookupKeycode(KeySym sym)
{
    long keycode;
    char printable;
    keysymToAWTKeyCode(sym, &keycode, &printable);

    return keycode;
}

