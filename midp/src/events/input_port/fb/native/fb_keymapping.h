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

#ifndef _FB_KEYMAPPING_H_
#define _FB_KEYMAPPING_H_

/**
 * @file
 * Key mappings for received key signals handling
 */

#include <java_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Map MIDP keyss and platform dependent key up/down codes */
typedef struct _KeyMapping {
    int midp_keycode;
    unsigned raw_keydown;
    unsigned raw_keyup;
} KeyMapping;

/** Keyboard info for the ARM Versatile and Integrator boards */
extern KeyMapping versatile_integrator_keys[];

/** Keyboard info for the Sharp Zaurus SL5500 */
extern KeyMapping zaurus_sl5500_keys[];

/**
 * Keypad info for the OMAP 730
 * Key press values shall be the values with one and only one bit set
 * Key release values are not used.
 */
extern KeyMapping omap_730_keys[];

/** Active key mapping to handle received keyboard signals */
extern KeyMapping *mapping;

/**
 * Indicates whether input device generates bitscale value
 * for each new input event, so changed bits are to be analyzed
 * to know what the keys are pressed or released. Otherwise each
 * keycode generaed by input device is treated as a single key
 * press or release
 */
extern jboolean bitscale_mode;

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _FB_KEYMAPPING_H_ */
