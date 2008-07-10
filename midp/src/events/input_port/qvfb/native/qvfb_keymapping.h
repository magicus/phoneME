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

#ifndef _QVFB_KEYMAPPING_H_
#define _QVFB_KEYMAPPING_H_

/**
 * @file
 * Key mappings for received key signals handling
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Map MIDP keyss and platform dependent key codes */
typedef struct _KeyMapping {
    int midp_keycode;
    unsigned raw_keycode;
} KeyMapping;

/** Keyboard mapping for QVFb device, usually TTY */
extern KeyMapping qvfb_keys[];

/** Active key mapping to handle received keyboard signals */
extern KeyMapping *mapping;

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _QVFB_KEYMAPPING_H_ */
