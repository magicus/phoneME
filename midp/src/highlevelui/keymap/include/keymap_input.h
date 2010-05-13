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

#ifndef _KEYMAP_INPUT_H_
#define _KEYMAP_INPUT_H_


/**
 * @defgroup highui_keymap Key Maping Porting Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_keymap
 *
 * @brief Porting api for keymap library
 */

#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Actions copied from event handler */
/**
 * @name Input device states
 * what has happened with a key or pointer
 * @{
 */
#define KEYMAP_STATE_PRESSED  1
#define KEYMAP_STATE_RELEASED 2
#define KEYMAP_STATE_REPEATED 3
#define KEYMAP_STATE_DRAGGED  3
/** Special key action for I18N */
#define KEYMAP_STATE_IME      4
/** @} */

/** @name Key Codes
 * numeric values associated with keys
 * @{
 */
typedef enum {
    KEYMAP_KEY_INVALID      = 0,

    KEYMAP_KEY_BACKSPACE    = 8,

    KEYMAP_KEY_POUND        = '#',
    KEYMAP_KEY_ASTERISK     = '*',

    KEYMAP_KEY_0            = '0',
    KEYMAP_KEY_1            = '1',
    KEYMAP_KEY_2            = '2',
    KEYMAP_KEY_3            = '3',
    KEYMAP_KEY_4            = '4',
    KEYMAP_KEY_5            = '5',
    KEYMAP_KEY_6            = '6',
    KEYMAP_KEY_7            = '7',
    KEYMAP_KEY_8            = '8',
    KEYMAP_KEY_9            = '9',
    KEYMAP_KEY_SPACE        = ' ',

    KEYMAP_KEY_UP           = -1,
    KEYMAP_KEY_DOWN         = -2,
    KEYMAP_KEY_LEFT         = -3,
    KEYMAP_KEY_RIGHT        = -4,
    KEYMAP_KEY_SELECT       = -5,

    KEYMAP_KEY_SOFT1        = -6,
    KEYMAP_KEY_SOFT2        = -7,
    KEYMAP_KEY_CLEAR        = -8,

    /* these may not be available to java */
    KEYMAP_KEY_SEND         = -10,
    KEYMAP_KEY_END          = -11,
    KEYMAP_KEY_POWER        = -12, 

    /* The game A B C D */
    KEYMAP_KEY_GAMEA        = -13,
    KEYMAP_KEY_GAMEB        = -14,
    KEYMAP_KEY_GAMEC        = -15,
    KEYMAP_KEY_GAMED        = -16,

    KEYMAP_KEY_GAME_UP      = -17,
    KEYMAP_KEY_GAME_DOWN    = -18,
    KEYMAP_KEY_GAME_LEFT    = -19,
    KEYMAP_KEY_GAME_RIGHT   = -20,

    /* This is generated only when tracing is enabled. Currently only 
     * one type (DEBUG_TRACE1) is supported (dump all stacks)
     * but you can add more DEBUG_TRACE<n> keys in the future.
     *
     * This key is consumed inside DisplayEventListener.process(Event event)
     * and is never passed to application.
     **/
    KEYMAP_KEY_DEBUG_TRACE1 = -21,
    
    KEYMAP_KEY_SCREEN_ROT   = -22,

    /* This key event is generated when platform requests to show
     * virtual keyboard. Is intended to be converted from key event
     * into appropriate MIDP event
     */
    KEYMAP_KEY_VIRT_KEYB    = -23,

    /* This is the last enum. Please shift
     * it if you are adding new values.
     * All values lower than KEY_MACHINE_DEP
     * can be used for associations with platform
     * dependent keys (for example KEYMAP_MD_KEY_HOME).
     **/
    KEYMAP_KEY_MACHINE_DEP  = -24

} KeymapKeyCode;
/** @} */

/**
 * Minimun valid key value.
 */
#define MIN_KEY_VALUE (KEYMAP_KEY_GAME_RIGHT)

/**
 * Auxiliary data type to define association between key codes
 * and key names.
 */
typedef struct {
    KeymapKeyCode keyCode;  /**< numeric key code */
    char *name;             /**< human-readable key name */
} KeymapKey;


/**
 * Return the key code corresponding to the given abstract game action.
 *
 * @param gameAction game action value
 */
extern int keymap_get_key_code(int gameAction);

/**
 * Return the system key corresponding to the given key code.
 * For non-system key codes, 0 is returned.
 *
 * @param keyCode key code value
 */
extern int keymap_get_system_key(int keyCode);


/**
 * Return the abstract game action corresponding to the given key code.
 *
 * @param keyCode key code value
 */
extern int keymap_get_game_action(int keyCode);


/**
 * Return the key string to the given key code.
 *
 * @param keyCode key code value
 *
 * @return C pointer to char or NULL if the keyCode does not
 * correspond to any name.
 */
extern char *keymap_get_key_name(int keyCode);

/**
 * Return whether the keycode given is correct for
 * this platform.
 *
 * @param keyCode key code value
 */
extern jboolean keymap_is_invalid_key_code(int keyCode);


#ifdef __cplusplus
}
#endif

#endif /* _KEYMAP_INPUT_H_ */
