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

#ifndef _GXJ_INTERN_FONT_BITMAP_H_
#define _GXJ_INTERN_FONT_BITMAP_H_

/* The font bitmap starts with a header containing the font parameters */
#define FONT_WIDTH 0
#define FONT_HEIGHT 1
#define FONT_ASCENT 2
#define FONT_DESCENT 3
#define FONT_LEADING 4
/* the high byte of the whole range of symbols */
/* in other words, high bytes of the firt and the last symbol in a table
   will be equal */
#define FONT_CODE_RANGE_HIGH 5
/* the low byte of the code of the first symbol in the bit map */
/* for inclusive comparison */
#define FONT_CODE_FIRST_LOW 6
/* the high byte of the code of the last symbol in the bit map */
/* for inclusive comparison */
#define FONT_CODE_LAST_LOW 7
#define FONT_DATA 8

typedef unsigned char* pfontbitmap;

/* the 0-th element is the number of bitmap tables;
 * the parameters: width, height, ascent,descent, leading
 * MUST be the same for all bitmap tables pointed to from
 * this array.
 */
extern pfontbitmap FontBitmaps[];

#endif /* _GXJ_INTERN_FONT_BITMAP_H_ */
