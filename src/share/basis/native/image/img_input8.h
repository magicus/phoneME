/*
 * @(#)img_input8.h	1.12 06/10/10
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
 *
 */

/*
 * This file contains macro definitions for the Fetching category of
 * the macros used by the generic scaleloop function.
 *
 * This implementation can load 8-bit pixels from an array of bytes
 * where the data for pixel (srcX, srcY) is loaded from index
 * (srcOff + srcY * srcScan + srcX) in the array.
 */

#define DeclareInputVars					\
    pixptr srcP;

#define InitInput(srcBPP)						\
    img_check(srcBPP == 8)

#define SetInputRow(pixels, srcOff, srcScan, srcY, srcOY)		\
    srcP.vp = pixels;							\
    srcP.bp += srcOff + ((srcY-srcOY) * srcScan)

#define GetPixelInc()							\
    ((int) *srcP.bp++)

#define GetPixel(srcX)							\
    ((int) srcP.bp[srcX])

#define InputPixelInc(X)						\
    srcP.bp += X

#define VerifyPixelRange(pixel, mapsize)				\
    do {								\
	img_check(((unsigned int) pixel) <= 255);			\
	img_check(mapsize >= 256);					\
    } while (0)
