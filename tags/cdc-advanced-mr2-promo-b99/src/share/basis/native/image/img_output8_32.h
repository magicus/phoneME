/*
 * @(#)img_output8_32.h	1.13 06/10/10
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
 * This file contains macro definitions for the Storing category of
 * the macros used by the generic scaleloop function.
 *
 * This implementation can store 8-bit or 32-bit pixels into an array
 * of either bytes or longs such that the pixel for (srcX, srcY) is
 * stored at index (srcOff + srcY * srcScan + srcX) in the array.
 */

#define DeclareOutputVars				\
    pixptr dstP;					\
    int dst32;

#define InitOutput(cvdata, clrdata, dstX, dstY)			\
    do {							\
	switch (clrdata->bitsperpixel) {			\
	case 8: dst32 = 0; break;				\
	case 32: dst32 = 2; break;				\
	default:						\
	    return SCALEFAILURE;				\
	}							\
	img_check((ScanBytes(cvdata) & ((1 << dst32)-1)) == 0);	\
	dstP.vp = cvdata->outbuf;				\
	dstP.bp += dstY * ScanBytes(cvdata) + (dstX << dst32);	\
    } while (0)

#define PutPixelInc(pixel, red, green, blue)			\
    do {							\
	if (dst32) {						\
	    *dstP.ip++ = pixel;					\
	} else {						\
	    *dstP.bp++ = ((unsigned char) pixel);		\
	}							\
    } while (0)

#define EndOutputRow(cvdata, dstY, dstX1, dstX2)		\
    do {							\
	SendRow(cvdata, dstY, dstX1, dstX2);			\
	dstP.bp += (ScanBytes(cvdata)				\
		    - ((dstX2 - dstX1) << dst32));		\
    } while (0)

#define EndOutputRect(cvdata, dstX1, dstY1, dstX2, dstY2)	\
    SendBuffer(cvdata, dstX1, dstY1, dstX2, dstY2)


