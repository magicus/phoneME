/*
 * @(#)img_nodither.h	1.9 06/10/10
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
 * This file contains macro definitions for the Encoding category of
 * the macros used by the generic scaleloop function.
 *
 * This implementation performs no encoding of output pixels at all
 * and is only useful for output pixel formats which consist of 3
 * bytes per pixel containing the red, green and blue components.
 * Since the components are stored explicitly in their own memory
 * accesses, they do not need to be encoded into a single monolithic
 * pixel first.  The value of the "pixel" variable will be undefined
 * during the output stage of the generic scale loop if this file
 * is used.
 */

#define DeclareDitherVars

#define InitDither(cvdata, clrdata, dstTW)			\
    do {} while (0)

#define StartDitherLine(cvdata, dstX1, dstY)			\
    do {} while (0)

#define DitherPixel(dstX, dstY, pixel, red, green, blue) 	\
    do {} while (0)

#define DitherBufComplete(cvdata, dstX1)			\
    do {} while (0)
