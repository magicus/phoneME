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

#ifndef _GXJPORT_TEXT_H
#define _GXJPORT_TEXT_H


/**
 * @defgroup lowui_gx Common Putpixel And Platform Graphics External Interface
 * @ingroup lowui
 */

/**
 * @file
 * @ingroup lowui_gx
 *
 * @brief Porting api for graphics library
 */

#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

int gxjport_draw_chars(int pixel,const jshort *clip, void *dst, int dotted,
                       int face, int style, int size,
                       int x, int y, int anchor,
                       const jchar *chararray, int n);

int gxjport_get_font_info(int face, int style, int size,
                          int *ascent, int *descent, int *leading);

int gxjport_get_chars_width(int face, int style, int size,
                            const jchar *charArray, int n);

#ifdef __cplusplus
}
#endif

#endif /* _GXJPORT_TEXT_H */
