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

#ifndef _LFPPORT_FONT_H_
#define _LFPPORT_FONT_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Font-specific porting functions and data structures.
 */

#include <lfpport_error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STYLE_PLAIN        0 /**< Font style defined by MIDP Spec */
#define STYLE_BOLD         1 /**< Font style defined by MIDP Spec */
#define STYLE_ITALIC       2 /**< Font style defined by MIDP Spec */
#define STYLE_UNDERLINED   4 /**< Font style defined by MIDP Spec */

#define SIZE_SMALL         8 /**< Font size defined by MIDP Spec */
#define SIZE_MEDIUM        0 /**< Font size defined by MIDP Spec */
#define SIZE_LARGE        16 /**< Font size defined by MIDP Spec */

#define FACE_SYSTEM        0 /**< Font face type defined by MIDP Spec */
#define FACE_MONOSPACE    32 /**< Font face type defined by MIDP Spec */
#define FACE_PROPORTIONAL 64 /**< Font face type defined by MIDP Spec */

/**
 * 32-bit identifier for (or pointer to) a platform font instance
 */
typedef void*	PlatformFontPtr;

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
			   int face, int style, int size);

/**
 * Frees native resources used by the system for font registry
 */
void lfpport_font_finalize();

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _LFPPORT_FONT_H_ */
