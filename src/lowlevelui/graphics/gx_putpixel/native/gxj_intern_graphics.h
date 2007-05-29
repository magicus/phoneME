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

#ifndef _GXJ_INTERN_GRAPHICS_H_
#define _GXJ_INTERN_GRAPHICS_H_


#include <gx_font.h>
#include <gxj_putpixel.h>

#include "gxj_intern_image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DOTTED_SOLID_SIZE 2
#define DOTTED_EMPTY_SIZE 1

#define CLIP_X1 0
#define CLIP_Y1 1
#define CLIP_X2 2
#define CLIP_Y2 3

#define QUADRANT_STATUS_UNDEFINED         0
#define QUADRANT_STATUS_TOTALLY_CLIPPED   0x1
#define QUADRANT_STATUS_PARTIALLY_CLIPPED 0x2
#define QUADRANT_STATUS_UNCLIPPED         0x4
#define QUADRANT_STATUS_FULL_ARC          0x8
#define QUADRANT_STATUS_PARTIAL_ARC       0x10
#define QUADRANT_STATUS_NO_ARC            0x20

#define BITMAP_WIDTH 0
#define BITMAP_HEIGHT 1
#define BITMAP_MUTABLE 2
#define BITMAP_DATA 3


/**
 * Convenient macro for getting both on and off screen buffer.
 */
#define getScreenBuffer(sbuf) \
    ((sbuf == NULL) ? (&gxj_system_screen_buffer) : sbuf)

#ifdef __cplusplus
}
#endif

#endif /* _GXJ_INTERN_GRAPHICS_H_ */
