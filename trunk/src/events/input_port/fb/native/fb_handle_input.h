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
 * Utility functions to handle received system signals
 */

#ifndef _FB_HANDLE_INPUT_H_
#define _FB_HANDLE_INPUT_H_

/**
 * @file
 * Unified input key structure to hold
 * pressed keys state from chek to check
 */

#include "fb_keymapping.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Unified structure for input keys state */
typedef struct _InputKeyState {
    jboolean hasPendingKeySignal;
    unsigned key;
    unsigned changedBits;
    KeyMapping *km;
    int down;
} InputKeyState;

/** Input keys state */
extern InputKeyState keyState; 

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _FB_KEYMAPPING_H_ */
