/*
 *   
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
#define PRESSED  1
#define RELEASED 2
#define REPEATED 3
#define DRAGGED  3
/** Special key action for I18N */
#define IME      4
/** @} */

/** @name Key Codes
 * numeric values associated with keys
 * @{
 */
typedef enum {
    KEY_INVALID = 0,

    KEY_BACKSPACE = 8,

    KEY_POUND    = '#',
    KEY_ASTERISK = '*',

    KEY_0        = '0',
    KEY_1        = '1',
    KEY_2        = '2',
    KEY_3        = '3',
    KEY_4        = '4',
    KEY_5        = '5',
    KEY_6        = '6',
    KEY_7        = '7',
    KEY_8        = '8',
    KEY_9        = '9',

    KEY_UP       = -1,
    KEY_DOWN     = -2,
    KEY_LEFT     = -3,
    KEY_RIGHT    = -4,
    KEY_SELECT   = -5,

    KEY_SOFT1    = -6,
    KEY_SOFT2    = -7,
    KEY_CLEAR    = -8,

    /* these may not be available to java */
    KEY_SEND     = -10,
    KEY_END      = -11,
    KEY_POWER    = -12, 

    /* The game A B C D */
    KEY_GAMEA    = -13,
    KEY_GAMEB    = -14,
    KEY_GAMEC    = -15,
    KEY_GAMED    = -16,

    KEY_GAME_UP       = -17,
    KEY_GAME_DOWN     = -18,
    KEY_GAME_LEFT     = -19,
    KEY_GAME_RIGHT    = -20,

    /* This is generated only when tracing is enabled. Currently only 
     * one type (DEBUG_TRACE1) is supported (dump all stacks)
     * but you can add more DEBUG_TRACE<n> keys in the future.
     *
     * This key is consumed inside DisplayEventListener.process(Event event)
     * and is never passed to application.
     **/
    KEY_DEBUG_TRACE1  = -21,
    KEY_SCREEN_ROT = -22,

    /* This is the last enum. Please shift
     * it if you are adding new values.
     * All values lower than KEY_MACHINE_DEP
     * can be used for associations with platform
     * dependent keys (for example MD_KEY_HOME).
     **/
    KEY_MACHINE_DEP   = -23

} KeyType;
/** @} */

/**
 * Auxiliary data type to define association between key codes
 * and button names.
 */
typedef struct {
    KeyType button; /**< numeric key code */
    char *name;     /**< human-readable button name */
} Key;


/**
 * Return the key code corresponding to the given abstract game action.
 *
 * @param gameAction game action value
 */
extern int  keymap_get_key_code(int gameAction);

/**
 * Return the system key corresponding to the given key code.
 * For non-system key codes, 0 is returned.
 *
 * @param keyCode key code value
 */
extern int  keymap_get_system_key(int keyCode);


/**
 * Return the abstract game action corresponding to the given key code.
 *
 * @param keyCode key code value
 */
extern int  keymap_get_game_action(int keyCode);


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
