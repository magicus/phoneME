/*
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

#ifndef __JAVACALL_KEYPRESS_H_
#define __JAVACALL_KEYPRESS_H_

/**
 * @file javacall_keypress.h
 * @ingroup Input
 * @brief Javacall interfaces for key press
 */

#ifdef __cplusplus
extern "C" {
#endif
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup NotificationKeypress Notification API for Key Press 
 * @ingroup Input
 *
 * The following callback function must be called by the platform when a key press event occurs. 
 *
 * @{
 */        
 
/**
 * @enum javacall_keypress_type
 * @brief Key press type
 */
typedef enum {
   /** Key pressed */
   JAVACALL_KEYPRESSED  = 1,
   /** Key released */
   JAVACALL_KEYRELEASED = 2,
   /** Key repeated */
   JAVACALL_KEYREPEATED = 3,
   /** Key typed - for internal use */
   JAVACALL_KEYTYPED = 4
} javacall_keypress_type;

/**
 * @enum javacall_key
 * @brief Key code value
 */
typedef enum {
    /** Backspace */
    JAVACALL_KEY_BACKSPACE = 8,

    /** Pound(#) */
    JAVACALL_KEY_POUND      = '#',
    /** Asterisk(*) */
    JAVACALL_KEY_ASTERISK   = '*',

    /** Key 0 */
    JAVACALL_KEY_0          = '0',
    /** Key 1 */
    JAVACALL_KEY_1          = '1',
    /** Key 2 */
    JAVACALL_KEY_2          = '2',
    /** Key 3 */
    JAVACALL_KEY_3          = '3',
    /** Key 4 */
    JAVACALL_KEY_4          = '4',
    /** Key 5 */
    JAVACALL_KEY_5          = '5',
    /** Key 6 */
    JAVACALL_KEY_6          = '6',
    /** Key 7 */
    JAVACALL_KEY_7          = '7',
    /** Key 8 */
    JAVACALL_KEY_8          = '8',
    /** Key 9 */
    JAVACALL_KEY_9          = '9',

    /** Up key */
    JAVACALL_KEY_UP         = -1,
    /** Down key */
    JAVACALL_KEY_DOWN       = -2,
    /** Left key */
    JAVACALL_KEY_LEFT       = -3,
    /** Right key */
    JAVACALL_KEY_RIGHT      = -4,
    /** Select key */
    JAVACALL_KEY_SELECT     = -5,

    /** Soft 1 key */
    JAVACALL_KEY_SOFT1      = -6,
    /** Soft 2 key */
    JAVACALL_KEY_SOFT2      = -7,
    /** Clear key */
    JAVACALL_KEY_CLEAR      = -8,

    /** Send key */
    JAVACALL_KEY_SEND       = -10,

    /** Game A key */
    JAVACALL_KEY_GAMEA      = -13,
    /** Game B key */
    JAVACALL_KEY_GAMEB      = -14,
    /** Game C key */
    JAVACALL_KEY_GAMEC      = -15,
    /** Game D key */
    JAVACALL_KEY_GAMED      = -16,

    /** Game Up key */
    JAVACALL_KEY_GAME_UP    = -17,
    /** Game Down key */
    JAVACALL_KEY_GAME_DOWN  = -18,
    /** Game Left key */
    JAVACALL_KEY_GAME_LEFT  = -19,
    /** Game Right key */
    JAVACALL_KEY_GAME_RIGHT = -20
} javacall_key;

/**
 * The notification function to be called by platform for keypress 
 * occurences.
 * The platfrom will invoke the call back in platform context for
 * each key press, key release and key repeat occurence 
 * @param key the key that was pressed
 * @param type <tt>JAVACALL_KEYPRESSED</tt> when key was pressed
 *             <tt>JAVACALL_KEYRELEASED</tt> when key was released
 *             <tt>JAVACALL_KEYREPEATED</tt> when to be called repeatedly
 *             by platform during the duration that the key was held
 */
void javanotify_key_event(javacall_key key, javacall_keypress_type type);
    
/** @} */    
    
#ifdef __cplusplus
}
#endif

#endif 

