/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 */

/**
 * @file
 * Key mapping of QVFb keycodes to MIDP key constants
 */

#include <keymap_input.h>
#include <midp_input_port.h>
#include "qvfb_keymapping.h"

KeyMapping *mapping = qvfb_keys;

 /**
  * IMPL_NOTE: The QVFb keycode is a unicode value consisted of two words.
  *   A functional keys have the meaning code in the higher word, while
  *   the letters and control key like ENTER, BACKSPACE have the meaning
  *   code in a lower word. The mapping consists of a meaning single word
  *   codes only, it is up to key handler routine to interpret them
  *   properly.
  */

KeyMapping qvfb_keys[] = {
    /* The keys with meaning code in a higher word of keycode */
    {KEYMAP_KEY_SOFT1,         0x1030},    /* F1 key */
    {KEYMAP_KEY_SOFT2,         0x1031},    /* F2 key */
    {KEYMAP_KEY_SCREEN_ROT,    0x1032},    /* F3 key */
    {KEYMAP_MD_KEY_SWITCH_APP, 0x1033},    /* F4 key */
    {KEYMAP_MD_NEXT_DISPLAY,   0x1034},    /* F5 key */
    {KEYMAP_KEY_VIRT_KEYB,     0x1035},    /* F6 key */
    {KEYMAP_KEY_GAMEA,         0x1038},    /* F9 key */
    {KEYMAP_KEY_GAMEB,         0x1039},    /* F10 key */
    {KEYMAP_KEY_GAMEC,         0x103a},    /* F11 key */
    {KEYMAP_KEY_GAMED,         0x103b},    /* F12 key */
    {KEYMAP_KEY_LEFT,          0x1012},    /* LEFT key */
    {KEYMAP_KEY_UP,            0x1013},    /* UP key */
    {KEYMAP_KEY_RIGHT,         0x1014},    /* RIGHT key */
    {KEYMAP_KEY_DOWN,          0x1015},    /* DOWN key */
    {KEYMAP_MD_KEY_HOME,       0x1010},    /* HOME key */
    {KEYMAP_MD_KEY_HOME,       0x1036},    /* F7 key */
    {KEYMAP_KEY_END,           0x1011},    /* END key */
    {KEYMAP_KEY_END,           0x1037},    /* F8 key */

    /* The keys with meaning code in a lower word of keycode */
    {KEYMAP_KEY_SELECT,        0x000d},    /* ENTER key */
    {KEYMAP_KEY_BACKSPACE,     0x0008},    /* BACKSPACE key */
    {KEYMAP_KEY_INVALID,       0x0000},    /* end of table */
};
