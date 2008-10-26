/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * @ingroup highui_lfpport
 *
 * @brief Font-specific porting functions and data structures.
 */

#include <lfpport_error.h>
#include <lfpport_font.h>
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Gets the font type. The bit values of the font attributes are
 * defined in the <i>MIDP Specification</i>.
 * When this function returns successfully, *fontPtr will point to the
 * platform font.
 *
 * @param fontPtr pointer to the font's PlatformFontPtr structure.
 * @param face typeface of the font (not a particular typeface,
 *        but a typeface class, such as <tt>MONOSPACE</i>).
 * @param style any angle, weight, or underlining for the font.
 * @param size relative size of the font (not a particular size,
 *        but a general size, such as <tt>LARGE</tt>).
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_get_font(PlatformFontPtr* fontPtr,
			   int face, int style, int size){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Frees native resources used by the system for font registry
 */
void lfpport_font_finalize(){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
}

#ifdef __cplusplus
} /* extern C */
#endif
