/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
static const Key Keys[] = {
    {KEY_POWER, "POWER" }, /* 0 */
    {KEY_SOFT1, "SOFT1" }, /* 1 */
    {KEY_SOFT2, "SOFT2" }, /* 2 */
    {KEY_UP,    "Up"    }, /* 3 */
    {KEY_DOWN,  "Down"  }, /* 4 */
    {KEY_LEFT,  "Left"  }, /* 5 */
    {KEY_RIGHT, "Right" }, /* 6 */
    {KEY_SELECT,"Select"}, /* 7 */
    {KEY_SEND,  "Send"  }, /* 8 */
    {KEY_END,   "End"   }, /* 9 */
    {KEY_CLEAR, "Clear" }, /* 10 */
    {KEY_1,     "1"     }, /* 11 */
    {KEY_2,     "2"     }, /* 12 */
    {KEY_3,     "3"     }, /* 13 */
    {KEY_4,     "4"     }, /* 14 */
    {KEY_5,     "5"     }, /* 15 */
    {KEY_6,     "6"     }, /* 16 */
    {KEY_7,     "7"     }, /* 17 */
    {KEY_8,     "8"     }, /* 18 */
    {KEY_9,     "9"     }, /* 19 */
    {KEY_ASTERISK, "*"  }, /* 20 */
    {KEY_0,     "0"     }, /* 21 */
    {KEY_POUND, "#"     }, /* 22 */
    {KEY_GAMEA, "Calendar" }, /* 23 */
    {KEY_GAMEB, "Addressbook" }, /* 24 */
    {KEY_GAMEC, "Menu" }, /* 25 */
    {KEY_GAMED, "Mail" }, /* 26 */


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
    {KEY_GAME_UP, "SHIFT_UP" }, /* 27 */
    /* GAME KEY_DOWN CustomItem KEY_DOWN */
    {KEY_GAME_DOWN, "SHIFT_DOWN" }, /* 28 */
    /* GAME KEY_LEFT CustomItem KEY_LEFT */
    {KEY_GAME_LEFT, "SHIFT_LEFT" }, /* 29 */
    /* GAME KEY_RIGHT CustomItem KEY_RIGHT */
    {KEY_GAME_RIGHT, "SHIFT_RIGHT" }, /* 30 */    
};

/**
 * Return the key code corresponding to the given abstract
 * game action.
 *
 * @param gameAction game action value
 * IMPL_NOTE:move to share platform
 */
int 
keymap_get_key_code(int gameAction)
{

    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_key_code(%d)\n", gameAction);

    switch (gameAction) {
    case  1: // Canvas.UP
        return KEY_UP;

    case  6: // Canvas.DOWN
	return KEY_DOWN;

    case  2: // Canvas.LEFT
	return KEY_LEFT;

    case  5: // Canvas.RIGHT
	return KEY_RIGHT;

    case  8: // Canvas.FIRE
	return KEY_SELECT;

    case  9: // Canvas.GAME_A
	return KEY_GAMEA;

    case 10: // Canvas.GAME_B
	return KEY_GAMEB;

    case 11: // Canvas.GAME_C
	return KEY_GAMEC;

    case 12: // Canvas.GAME_D
	return KEY_GAMED;

    default: return 0;
    }
}

/**
 * Return the abstract game action corresponding to the
 * given key code.
 *
 * @param keyCode key code value
 * IMPL_NOTE:move to share platform
 */
int 
keymap_get_game_action(int keyCode)
{
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_game_action(%d)\n", keyCode);

    
    switch (keyCode) {
    case KEY_UP:
    case KEY_GAME_UP: // Customitem Game UP
        return 1; // Canvas.UP

    case KEY_DOWN:
    case KEY_GAME_DOWN:// Customitem Game DOWN
        return 6; // Canvas.DOWN

    case KEY_LEFT:
    case KEY_GAME_LEFT:// Customitem Game LEFT
        return 2; // Canvas.LEFT

    case KEY_RIGHT:
    case KEY_GAME_RIGHT:// Customitem Game RIGHT
        return 5; // Canvas.RIGHT

    case KEY_SELECT:
        return 8; // Canvas.FIRE

    case KEY_GAMEA:
    case KEY_1:
        return 9;  // Canvas.GAME_A

    case KEY_GAMEB:
    case KEY_3:
        return 10; // Canvas.GAME_B

    case KEY_GAMEC:
    case KEY_7:
        return 11; // Canvas.GAME_C

    case KEY_GAMED:
    case KEY_9:
        return 12; // Canvas.GAME_D

    default:

	if(keymap_is_invalid_key_code(keyCode)) {
	    //Invalid key code
	    return -1;
	}

	// No game action available for this key
	return 0;
    }
}

/**
 * Return the system key corresponding to the given key
 * code.
 *
 * @param keyCode key code value
 * IMPL_NOTE:move to share platform
 */
int
keymap_get_system_key(int keyCode)
{
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_system_key(%d)\n", keyCode);

    switch (keyCode) {
    case KEY_POWER: return 1;
    case KEY_SEND:  return 2;
    case KEY_END:   return 3;
    case KEY_BACKSPACE:
    case KEY_CLEAR: return 4;
    default:        return 0;
    }
}


/**
 * Return the key string to the given key code.
 *
 * @param keyCode key code value
 *
 * @return C pointer to char or NULL if the keyCode does not
 * correspond to any name.
 * IMPL_NOTE:move to share platform and change to loop through table Keys[]
 */
char *
keymap_get_key_name(int keyCode)
{
    REPORT_CALL_TRACE1(LC_LOWUI, "LF:keymap_get_key_name(%d)\n", keyCode);

    switch (keyCode) {
    case KEY_POWER:    return Keys[0].name;
    case KEY_SEND:     return Keys[8].name;
    case KEY_END:      return Keys[9].name;
    case KEY_CLEAR:    return Keys[10].name;
    case KEY_SOFT1:    return Keys[1].name; 
    case KEY_SOFT2:    return Keys[2].name;
    case KEY_UP:       return Keys[3].name;
    case KEY_DOWN:     return Keys[4].name;
    case KEY_LEFT:     return Keys[5].name;
    case KEY_RIGHT:    return Keys[6].name;
    case KEY_SELECT:   return Keys[7].name;
    case KEY_1:        return Keys[11].name;
    case KEY_2:        return Keys[12].name;
    case KEY_3:        return Keys[13].name;
    case KEY_4:        return Keys[14].name;
    case KEY_5:        return Keys[15].name;
    case KEY_6:        return Keys[16].name;
    case KEY_7:        return Keys[17].name;
    case KEY_8:        return Keys[18].name;
    case KEY_9:        return Keys[19].name;
    case KEY_0:        return Keys[21].name;
    case KEY_ASTERISK: return Keys[20].name;
    case KEY_POUND:    return Keys[22].name;
    case KEY_GAMEA:    return Keys[23].name;
    case KEY_GAMEB:    return Keys[24].name;
    case KEY_GAMEC:    return Keys[25].name;
    case KEY_GAMED:    return Keys[26].name;
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

    // Valid within UNICODE and not 0x0 and 0xffff
    // since they are defined to be invalid
    if ((keyCode <= 0x0) || (keyCode >= 0xFFFF) ) {
	return KNI_TRUE;
    }

    return KNI_FALSE;
}
