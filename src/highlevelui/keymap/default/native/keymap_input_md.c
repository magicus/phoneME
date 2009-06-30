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

#include <kni.h>
#include <keymap_input.h>
#include <midp_logging.h>

/**
 * @file
 *
 * Platform key mapping and input mode handling functions.
 *
 * This file contains all the platform input related
 * code, including all the key binding functions.
 */


/**
 * Platform specific code to name mapping table.
 */
static const KeymapKey Keys[] = {
    {KEYMAP_KEY_POWER,      "POWER"         }, /* 0 */
    {KEYMAP_KEY_SOFT1,      "SOFT1"         }, /* 1 */
    {KEYMAP_KEY_SOFT2,      "SOFT2"         }, /* 2 */
    {KEYMAP_KEY_UP,         "Up"            }, /* 3 */
    {KEYMAP_KEY_DOWN,       "Down"          }, /* 4 */
    {KEYMAP_KEY_LEFT,       "Left"          }, /* 5 */
    {KEYMAP_KEY_RIGHT,      "Right"         }, /* 6 */
    {KEYMAP_KEY_SELECT,     "Select"        }, /* 7 */
    {KEYMAP_KEY_SEND,       "Send"          }, /* 8 */
    {KEYMAP_KEY_END,        "End"           }, /* 9 */
    {KEYMAP_KEY_CLEAR,      "Clear"         }, /* 10 */
    {KEYMAP_KEY_1,          "1"             }, /* 11 */
    {KEYMAP_KEY_2,          "2"             }, /* 12 */
    {KEYMAP_KEY_3,          "3"             }, /* 13 */
    {KEYMAP_KEY_4,          "4"             }, /* 14 */
    {KEYMAP_KEY_5,          "5"             }, /* 15 */
    {KEYMAP_KEY_6,          "6"             }, /* 16 */
    {KEYMAP_KEY_7,          "7"             }, /* 17 */
    {KEYMAP_KEY_8,          "8"             }, /* 18 */
    {KEYMAP_KEY_9,          "9"             }, /* 19 */
    {KEYMAP_KEY_ASTERISK,   "*"             }, /* 20 */
    {KEYMAP_KEY_0,          "0"             }, /* 21 */
    {KEYMAP_KEY_POUND,      "#"             }, /* 22 */
    {KEYMAP_KEY_GAMEA,      "Calendar"      }, /* 23 */
    {KEYMAP_KEY_GAMEB,      "Addressbook"   }, /* 24 */
    {KEYMAP_KEY_GAMEC,      "Menu"          }, /* 25 */
    {KEYMAP_KEY_GAMED,      "Mail"          }, /* 26 */
    {KEYMAP_KEY_SPACE,      "Space"         }, /* 27 */
    {KEYMAP_KEY_BACKSPACE,  "BackSpace"     }, /* 28 */


    /**
     * These set of keys and the key events available to a 
     * CustomItem for UP, DOWN, LEFT and RIGHT game actions.
     * This is different from what is available on a Canvas.  
     * In this particular case the system has traversal so the system uses 
     * directional keys for traversal.
     * This is the mapping between key codes and UP, DOWN, LEFT and RIGHT 
     * game actions in a CustomItem.
     */

    /* GAME KEY_UP CustomItem KEY_UP */
    {KEYMAP_KEY_GAME_UP,    "SHIFT_UP"      }, /* 29 */
    /* GAME KEY_DOWN CustomItem KEY_DOWN */
    {KEYMAP_KEY_GAME_DOWN,  "SHIFT_DOWN"    }, /* 30 */
    /* GAME KEY_LEFT CustomItem KEY_LEFT */
    {KEYMAP_KEY_GAME_LEFT,  "SHIFT_LEFT"    }, /* 31 */
    /* GAME KEY_RIGHT CustomItem KEY_RIGHT */
    {KEYMAP_KEY_GAME_RIGHT, "SHIFT_RIGHT"   }, /* 32 */    
    /* Indicator of end of table */
    {KEYMAP_KEY_INVALID,    ""              }, /* 33 */
};

/**
 * Return the key code corresponding to the given abstract
 * game action.
 *
 * @param gameAction game action value
 */
int 
keymap_get_key_code(int gameAction)
{

    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_key_code(%d)\n", gameAction);

    switch (gameAction) {
    case  1: /* Canvas.UP */
        return KEYMAP_KEY_UP;

    case  6: /* Canvas.DOWN */
        return KEYMAP_KEY_DOWN;

    case  2: /* Canvas.LEFT */
        return KEYMAP_KEY_LEFT;

    case  5: /* Canvas.RIGHT */
        return KEYMAP_KEY_RIGHT;

    case  8: /* Canvas.FIRE */
        return KEYMAP_KEY_SELECT;

    case  9: /* Canvas.GAME_A */
        return KEYMAP_KEY_GAMEA;

    case 10: /* Canvas.GAME_B */
        return KEYMAP_KEY_GAMEB;

    case 11: /* Canvas.GAME_C */
        return KEYMAP_KEY_GAMEC;

    case 12: /* Canvas.GAME_D */
        return KEYMAP_KEY_GAMED;

    default: return 0;
    }
}

/**
 * Return the abstract game action corresponding to the
 * given key code.
 *
 * @param keyCode key code value
 */
int 
keymap_get_game_action(int keyCode)
{
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_game_action(%d)\n", keyCode);

    
    switch (keyCode) {
    case KEYMAP_KEY_UP:
    case KEYMAP_KEY_GAME_UP: /* Customitem Game UP */
        return 1; /* Canvas.UP */

    case KEYMAP_KEY_DOWN:
    case KEYMAP_KEY_GAME_DOWN:/* Customitem Game DOWN */
        return 6; /* Canvas.DOWN */

    case KEYMAP_KEY_LEFT:
    case KEYMAP_KEY_GAME_LEFT:/* Customitem Game LEFT */
        return 2; /* Canvas.LEFT */

    case KEYMAP_KEY_RIGHT:
    case KEYMAP_KEY_GAME_RIGHT:/* Customitem Game RIGHT */
        return 5; /* Canvas.RIGHT */

    case KEYMAP_KEY_SELECT:
        return 8; /* Canvas.FIRE */

    case KEYMAP_KEY_GAMEA:
    case KEYMAP_KEY_1:
        return 9;  /* Canvas.GAME_A */

    case KEYMAP_KEY_GAMEB:
    case KEYMAP_KEY_3:
        return 10; /* Canvas.GAME_B */

    case KEYMAP_KEY_GAMEC:
    case KEYMAP_KEY_7:
        return 11; /* Canvas.GAME_C */

    case KEYMAP_KEY_GAMED:
    case KEYMAP_KEY_9:
        return 12; /* Canvas.GAME_D */

    default:

        if(keymap_is_invalid_key_code(keyCode)) {
            /* Invalid key code */
            return -1;
        }

        /* No game action available for this key */
        return 0;
    }
}

/**
 * Return the system key corresponding to the given key
 * code.
 *
 * @param keyCode key code value
 */
int
keymap_get_system_key(int keyCode)
{
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_system_key(%d)\n", keyCode);

    switch (keyCode) {
    case KEYMAP_KEY_POWER:  return 1;
    case KEYMAP_KEY_SEND:   return 2;
    case KEYMAP_KEY_END:    return 3;
    case KEYMAP_KEY_BACKSPACE:
    case KEYMAP_KEY_CLEAR:  return 4;
    case KEYMAP_KEY_SELECT: return 5; 
    default:                return 0;
    }
}


/**
 * Return the key string to the given key code.
 *
 * @param keyCode key code value
 *
 * @return C pointer to char or NULL if the keyCode does not
 * correspond to any name.
 */
char *
keymap_get_key_name(int keyCode)
{
    KeymapKey *map;
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_key_name(%d)\n", keyCode);
    for (map = (KeymapKey *)Keys; map->keyCode != KEYMAP_KEY_INVALID; map++) {
        if (map->keyCode == keyCode) {
            return map->name;
        }
    }
    return 0;
}

/**
 * Return whether the keycode given is correct for
 * this platform.
 *
 * @param keyCode key code value
 */
jboolean
keymap_is_invalid_key_code(int keyCode)
{
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_is_invalid_key_code(%d)\n", 
                       keyCode);

    /* 
     * Valid within UNICODE and not 0x0 and 0xffff 
     * since they are defined to be invalid
     */
    if ((keyCode == 0x0) || (keyCode >= 0xFFFF) || (keyCode < MIN_KEY_VALUE)) {
        return KNI_TRUE;
    }

    return KNI_FALSE;
}
