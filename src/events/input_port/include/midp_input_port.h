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

#ifndef _MIDP_INPUT_PORT_H_
#define _MIDP_INPUT_PORT_H_

/**
 * @file
 * Utility functions to handle received system signals from 
 * an input devices like keyboard, keypad, mouse etc.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <midpServices.h>
#include <midpEvents.h>
#include <keymap_input.h> 

#define KEYMAP_MD_KEY_HOME       (KEYMAP_KEY_MACHINE_DEP)
#define KEYMAP_MD_KEY_SWITCH_APP (KEYMAP_KEY_MACHINE_DEP - 1)
#define KEYMAP_MD_NEXT_DISPLAY   (KEYMAP_KEY_MACHINE_DEP - 2)


/**
 * Handle received keyboard/keypad signals
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handle_key_port(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent);

/**
 * Handle received pointer signals
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handle_pointer_port(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent);

/**
 * Support repeated key presses for a platforms with no own support for it.
 *
 * One of the possible implementations is timer-based generation of MIDP
 * events for a keys pressed and not released for a certain time interval.
 *
 * @param midpKeyCode MIDP keycode of the pressed/released key
 * @param isPressed true if the key is pressed, false if released 
 */
void handle_repeated_key_port(int midpKeyCode, jboolean isPressed);

/**
 * An input devices can produce bit-based keyboard events. Thus single
 * native event can produce several MIDP ones. This function detects
 * whether are one or more key bits still not converted into MIDP events
 *
 * @return true when pending key exists, false otherwise
 */
jboolean has_pending_key_port();
    
#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _MIDP_INPUT_PORT_H_ */
