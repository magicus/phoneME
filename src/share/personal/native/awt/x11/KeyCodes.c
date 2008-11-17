/*
 * @(#)KeyCodes.c	1.9 06/10/10
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

#include "awt.h"
#include "java_awt_event_KeyEvent.h"
#include <X11/keysym.h>

typedef struct  {
    long awtKey;
    KeySym x11Key;
    Boolean printable;
} KeymapEntry;

static KeymapEntry keymapTable[] =
{
    {java_awt_event_KeyEvent_VK_A, XK_a, TRUE},
    {java_awt_event_KeyEvent_VK_B, XK_b, TRUE},
    {java_awt_event_KeyEvent_VK_C, XK_c, TRUE},
    {java_awt_event_KeyEvent_VK_D, XK_d, TRUE},
    {java_awt_event_KeyEvent_VK_E, XK_e, TRUE},
    {java_awt_event_KeyEvent_VK_F, XK_f, TRUE},
    {java_awt_event_KeyEvent_VK_G, XK_g, TRUE},
    {java_awt_event_KeyEvent_VK_H, XK_h, TRUE},
    {java_awt_event_KeyEvent_VK_I, XK_i, TRUE},
    {java_awt_event_KeyEvent_VK_J, XK_j, TRUE},
    {java_awt_event_KeyEvent_VK_K, XK_k, TRUE},
    {java_awt_event_KeyEvent_VK_L, XK_l, TRUE},
    {java_awt_event_KeyEvent_VK_M, XK_m, TRUE},
    {java_awt_event_KeyEvent_VK_N, XK_n, TRUE},
    {java_awt_event_KeyEvent_VK_O, XK_o, TRUE},
    {java_awt_event_KeyEvent_VK_P, XK_p, TRUE},
    {java_awt_event_KeyEvent_VK_Q, XK_q, TRUE},
    {java_awt_event_KeyEvent_VK_R, XK_r, TRUE},
    {java_awt_event_KeyEvent_VK_S, XK_s, TRUE},
    {java_awt_event_KeyEvent_VK_T, XK_t, TRUE},
    {java_awt_event_KeyEvent_VK_U, XK_u, TRUE},
    {java_awt_event_KeyEvent_VK_V, XK_v, TRUE},
    {java_awt_event_KeyEvent_VK_W, XK_w, TRUE},
    {java_awt_event_KeyEvent_VK_X, XK_x, TRUE},
    {java_awt_event_KeyEvent_VK_Y, XK_y, TRUE},
    {java_awt_event_KeyEvent_VK_Z, XK_z, TRUE},

    {java_awt_event_KeyEvent_VK_ENTER, XK_Return, TRUE},
    {java_awt_event_KeyEvent_VK_ENTER, XK_KP_Enter, TRUE},
    {java_awt_event_KeyEvent_VK_ENTER, XK_Linefeed, TRUE},

    {java_awt_event_KeyEvent_VK_BACK_SPACE, XK_BackSpace, TRUE},
    {java_awt_event_KeyEvent_VK_TAB, XK_Tab, TRUE},
    {java_awt_event_KeyEvent_VK_CANCEL, XK_Cancel, FALSE},
    {java_awt_event_KeyEvent_VK_CLEAR, XK_Clear, FALSE},
    {java_awt_event_KeyEvent_VK_SHIFT, XK_Shift_L, FALSE},
    {java_awt_event_KeyEvent_VK_SHIFT, XK_Shift_R, FALSE},
    {java_awt_event_KeyEvent_VK_CONTROL, XK_Control_L, FALSE},
    {java_awt_event_KeyEvent_VK_CONTROL, XK_Control_R, FALSE},
    {java_awt_event_KeyEvent_VK_ALT, XK_Alt_L, FALSE},
    {java_awt_event_KeyEvent_VK_ALT, XK_Alt_R, FALSE},
    {java_awt_event_KeyEvent_VK_META, XK_Meta_L, FALSE},
    {java_awt_event_KeyEvent_VK_META, XK_Meta_R, FALSE},
    {java_awt_event_KeyEvent_VK_PAUSE, XK_Pause, FALSE},
    {java_awt_event_KeyEvent_VK_CAPS_LOCK, XK_Caps_Lock, FALSE},
    {java_awt_event_KeyEvent_VK_ESCAPE, XK_Escape, TRUE},
    {java_awt_event_KeyEvent_VK_SPACE, XK_space, TRUE},

    {java_awt_event_KeyEvent_VK_PAGE_UP, XK_Page_Up, FALSE},
    {java_awt_event_KeyEvent_VK_PAGE_UP, XK_R9, FALSE},
    {java_awt_event_KeyEvent_VK_PAGE_UP, XK_Prior, FALSE},
    {java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_Page_Down, FALSE},
    {java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_R15, FALSE},
    {java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_Next, FALSE},
    {java_awt_event_KeyEvent_VK_END, XK_R13, FALSE},
    {java_awt_event_KeyEvent_VK_END, XK_End, FALSE},
    {java_awt_event_KeyEvent_VK_HOME, XK_Home, FALSE},
    {java_awt_event_KeyEvent_VK_HOME, XK_R7, FALSE},

    {java_awt_event_KeyEvent_VK_LEFT, XK_Left, FALSE},
    {java_awt_event_KeyEvent_VK_UP, XK_Up, FALSE},
    {java_awt_event_KeyEvent_VK_RIGHT, XK_Right, FALSE},
    {java_awt_event_KeyEvent_VK_DOWN, XK_Down, FALSE},
    {java_awt_event_KeyEvent_VK_INSERT, XK_Insert, FALSE},
    {java_awt_event_KeyEvent_VK_HELP, XK_Help, FALSE},

    {java_awt_event_KeyEvent_VK_KP_UP, XK_KP_Up, FALSE},
    {java_awt_event_KeyEvent_VK_KP_DOWN, XK_KP_Down, FALSE},
    {java_awt_event_KeyEvent_VK_KP_RIGHT, XK_KP_Right, FALSE},
    {java_awt_event_KeyEvent_VK_KP_LEFT, XK_KP_Left, FALSE},

    {java_awt_event_KeyEvent_VK_0, XK_0, TRUE},
    {java_awt_event_KeyEvent_VK_1, XK_1, TRUE},
    {java_awt_event_KeyEvent_VK_2, XK_2, TRUE},
    {java_awt_event_KeyEvent_VK_3, XK_3, TRUE},
    {java_awt_event_KeyEvent_VK_4, XK_4, TRUE},
    {java_awt_event_KeyEvent_VK_5, XK_5, TRUE},
    {java_awt_event_KeyEvent_VK_6, XK_6, TRUE},
    {java_awt_event_KeyEvent_VK_7, XK_7, TRUE},
    {java_awt_event_KeyEvent_VK_8, XK_8, TRUE},
    {java_awt_event_KeyEvent_VK_9, XK_9, TRUE},

    {java_awt_event_KeyEvent_VK_EQUALS, XK_equal, TRUE},
    {java_awt_event_KeyEvent_VK_BACK_SLASH, XK_backslash, TRUE},
    {java_awt_event_KeyEvent_VK_BACK_QUOTE, XK_grave, TRUE},
    {java_awt_event_KeyEvent_VK_OPEN_BRACKET, XK_bracketleft, TRUE},
    {java_awt_event_KeyEvent_VK_CLOSE_BRACKET, XK_bracketright, TRUE},
    {java_awt_event_KeyEvent_VK_SEMICOLON, XK_semicolon, TRUE},
    {java_awt_event_KeyEvent_VK_QUOTE, XK_apostrophe, TRUE},
    {java_awt_event_KeyEvent_VK_COMMA, XK_comma, TRUE},
    {java_awt_event_KeyEvent_VK_MINUS, XK_minus, TRUE},
    {java_awt_event_KeyEvent_VK_PERIOD, XK_period, TRUE},
    {java_awt_event_KeyEvent_VK_SLASH, XK_slash, TRUE},

    {java_awt_event_KeyEvent_VK_NUMPAD0, XK_KP_0, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD1, XK_KP_1, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD2, XK_KP_2, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD3, XK_KP_3, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD4, XK_KP_4, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD5, XK_KP_5, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD6, XK_KP_6, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD7, XK_KP_7, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD8, XK_KP_8, TRUE},
    {java_awt_event_KeyEvent_VK_NUMPAD9, XK_KP_9, TRUE},
    {java_awt_event_KeyEvent_VK_MULTIPLY, XK_KP_Multiply, TRUE},
    {java_awt_event_KeyEvent_VK_ADD, XK_KP_Add, TRUE},
    {java_awt_event_KeyEvent_VK_SUBTRACT, XK_KP_Subtract, TRUE},
    {java_awt_event_KeyEvent_VK_DECIMAL, XK_KP_Decimal, TRUE},
    {java_awt_event_KeyEvent_VK_DIVIDE, XK_KP_Divide, TRUE},
    {java_awt_event_KeyEvent_VK_EQUALS, XK_KP_Equal, TRUE},
    {java_awt_event_KeyEvent_VK_INSERT, XK_KP_Insert, FALSE},

    {java_awt_event_KeyEvent_VK_F1, XK_F1, FALSE},
    {java_awt_event_KeyEvent_VK_F2, XK_F2, FALSE},
    {java_awt_event_KeyEvent_VK_F3, XK_F3, FALSE},
    {java_awt_event_KeyEvent_VK_F4, XK_F4, FALSE},
    {java_awt_event_KeyEvent_VK_F5, XK_F5, FALSE},
    {java_awt_event_KeyEvent_VK_F6, XK_F6, FALSE},
    {java_awt_event_KeyEvent_VK_F7, XK_F7, FALSE},
    {java_awt_event_KeyEvent_VK_F8, XK_F8, FALSE},
    {java_awt_event_KeyEvent_VK_F9, XK_F9, FALSE},
    {java_awt_event_KeyEvent_VK_F10, XK_F10, FALSE},

    {java_awt_event_KeyEvent_VK_DELETE, XK_Delete, TRUE},
    {java_awt_event_KeyEvent_VK_DELETE, XK_KP_Delete, TRUE},

    {java_awt_event_KeyEvent_VK_NUM_LOCK, XK_Num_Lock, FALSE},
    {java_awt_event_KeyEvent_VK_SCROLL_LOCK, XK_Scroll_Lock, FALSE},
    {java_awt_event_KeyEvent_VK_PRINTSCREEN, XK_Print, FALSE},

    /* X11 keysym names for input method related keys don't always match keytop engravings
     or Java virtual key names, so we here only map constants that we've found on real keyboards.
    */

    {java_awt_event_KeyEvent_VK_ACCEPT, XK_Execute, FALSE}, /* Type 5c Japanese keyboard: kakutei */
    {java_awt_event_KeyEvent_VK_CONVERT, XK_Kanji, FALSE}, /* Type 5c Japanese keyboard: henkan */
    {java_awt_event_KeyEvent_VK_INPUT_METHOD_ON_OFF, XK_Henkan_Mode, FALSE}, /* Type 5c Japanese keyboard: nihongo */
    /* VK_KANA_LOCK is handled separately because it generates the same keysym as ALT_GRAPH
     in spite of its different behavior. */

    { java_awt_event_KeyEvent_VK_F11, XK_F11, FALSE },
    { java_awt_event_KeyEvent_VK_F12, XK_F12, FALSE },
    { java_awt_event_KeyEvent_VK_AGAIN, XK_L2, FALSE },
    { java_awt_event_KeyEvent_VK_UNDO, XK_L4, FALSE },
    { java_awt_event_KeyEvent_VK_COPY, XK_L6, FALSE },
    { java_awt_event_KeyEvent_VK_PASTE, XK_L8, FALSE },
    { java_awt_event_KeyEvent_VK_CUT, XK_L10, FALSE },
    { java_awt_event_KeyEvent_VK_FIND, XK_L9, FALSE },
    { java_awt_event_KeyEvent_VK_PROPS, XK_L3, FALSE },
    { java_awt_event_KeyEvent_VK_STOP, XK_L1, FALSE },

    { java_awt_event_KeyEvent_VK_COMPOSE, XK_Multi_key, FALSE },
    { java_awt_event_KeyEvent_VK_ALT_GRAPH, XK_Mode_switch, FALSE },

    {0, 0, 0}
};

static Boolean
keyboardHasKanaLockKey()
{
    static Boolean haveResult = FALSE;
    static Boolean result = FALSE;

    int minKeyCode, maxKeyCode, keySymsPerKeyCode;
    KeySym *keySyms, keySym;
    int i;
    int kanaCount = 0;
    
    /* Solaris doesn't let you swap keyboards without rebooting,
     so there's no need to check for the kana lock key more than once. */

    if (haveResult) {
       return result;
    }
    
    /* There's no direct way to determine whether the keyboard has
     a kana lock key. From available keyboard mapping tables, it looks
     like only keyboards with the kana lock key can produce keysyms
     for kana characters. So, as an indirect test, we check for those. */

    XDisplayKeycodes(xawt_display, &minKeyCode, &maxKeyCode);
    keySyms = XGetKeyboardMapping(xawt_display, minKeyCode, maxKeyCode - minKeyCode + 1, &keySymsPerKeyCode);
    for (i = 0; i < (maxKeyCode - minKeyCode + 1) * keySymsPerKeyCode; i++) {
        keySym = *keySyms++;
        if ((keySym & 0xff00) == 0x0400) {
            kanaCount++;
        }
    }
    XFree(keySyms);

    /* use a (somewhat arbitrary) minimum so we don't get confused by a stray function key */
    result = kanaCount > 10;
    haveResult = TRUE;
    return result;
}

/* Determines the Java key code for an X11 KeySym. */

void
xawt_getJavaKeyCode(KeySym x11Key, jlong *keycode, Boolean * printable)
{
    int i;

    /* Solaris uses XK_Mode_switch for both the non-locking AltGraph
     and the locking Kana key, but we want to keep them separate for
     KeyEvent. */

    /* TODO: This is removed because it causes crashing when alt graph is pressed.
       need to fix this.
       
       if (x11Key == XK_Mode_switch && keyboardHasKanaLockKey()) {
        *keycode = java_awt_event_KeyEvent_VK_KANA_LOCK;
        *printable = FALSE;
        return;
    }*/

    for (i = 0; keymapTable[i].awtKey != 0; i++) {
        if (keymapTable[i].x11Key == x11Key) {
            *keycode = keymapTable[i].awtKey;
            *printable = keymapTable[i].printable;
            return;
        }
    }

    *keycode = 0;
    *printable = FALSE;
}

KeySym
xawt_keyKeySym(jlong awtKey)
{
    int i;

    if (awtKey == java_awt_event_KeyEvent_VK_KANA_LOCK && keyboardHasKanaLockKey()) {
        return XK_Mode_switch;
    }

    for (i = 0; keymapTable[i].awtKey != 0; i++) {
        if (keymapTable[i].awtKey == awtKey) {
            return keymapTable[i].x11Key;
        }
    }
    
    return 0;
}



