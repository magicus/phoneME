/*
 * @(#)img_input8_32.h	1.12 06/10/04
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

/*
 * This file contains macro definitions for the Fetching category of
 * the macros used by the generic scaleloop function.
 *
 * This implementation can load either 8-bit or 32-bit pixels from an
 * array of bytes or longs where the data for pixel (srcX, srcY) is
 * loaded from index (srcOff + srcY * srcScan + srcX) in the array.
 *
 * This file can be used to provide the default implementation of the
 * Fetching macros to handle all input sizes.
 */

#define DeclareInputVars					\
    pixptr srcP;						\
    int src32;

#define InitInput(srcBPP)						\
    do {								\
	switch (srcBPP) {						\
	case 8: src32 = 0; break;					\
	case 32: src32 = 1; break;					\
	default:							\
	    return SCALEFAILURE;					\
	}								\
    } while (0)

#define SetInputRow(pixels, srcOff, srcScan, srcY, srcOY)		\
    do {								\
	srcP.vp = pixels;						\
	if (src32) {							\
	    srcP.ip += srcOff + ((srcY-srcOY) * srcScan);		\
	} else {							\
	    srcP.bp += srcOff + ((srcY-srcOY) * srcScan);		\
	}								\
    } while (0)

#define GetPixelInc()							\
    (src32 ? *srcP.ip++ : ((int) *srcP.bp++))

#define GetPixel(srcX)							\
    (src32 ? srcP.ip[srcX] : ((int) srcP.bp[srcX]))

#define InputPixelInc(X)						\
    do {								\
	if (src32) {							\
	    srcP.ip += X;						\
	} else {							\
	    srcP.bp += X;						\
	}								\
    } while (0)

#define VerifyPixelRange(pixel, mapsize)				\
    do {								\
	if (((unsigned int) pixel) >= mapsize) {			\
	    return SCALEFAILURE;					\
	}								\
    } while (0)
