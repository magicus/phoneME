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
 *
 * Utility functions to read key data from input devices.
 */

#ifndef _FB_READ_KEY_H_
#define _FB_READ_KEY_H_

/**
 * @file
 * Functions to read key events from platform devices
 */

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Function to read key events from a platform input device 
 * into global structure storing keys state from check to check
 */
typedef jboolean (*fReadKeyEvent)(void);

/** 
 * Update input keys state from OMAP730 keypad device 
 * @return true if data was read successfully
 */
extern jboolean read_omap730_key_event();

/** 
 * Update input keys state reading single char from keyboard device
 * @return true if data was read successfully
 */
extern jboolean read_char_key_event();

#ifdef DIRECTFB
/** 
 * Update input keys state from DirectFB keyboard device 
 * @return true if data was read successfully
 */
extern jboolean read_directfb_key_event();
#endif

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _FB_READ_KEY_H_ */
