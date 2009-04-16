/*
 * @(#)img_icm.h	1.19 06/10/04
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
 * This file contains macro definitions for the Decoding category of
 * the macros used by the generic scaleloop function.
 *
 * This implementation can decode the pixel information associated
 * with any Java IndexColorModel object.  This implementation examines
 * some of the private fields of the IndexColorModel object and decodes
 * the red, green, blue, and possibly alpha values directly rather than
 * calling the getRGB method on the Java object.
 */

/*
 * These definitions vector the standard macro names to the "ICM"
 * versions of those macros only if the "DecodeDeclared" keyword has
 * not yet been defined elsewhere.  The "DecodeDeclared" keyword is
 * also defined here to claim ownership of the primary implementation
 * even though this file does not rely on the definitions in any other
 * files.
 */
#ifndef DecodeDeclared
#define DeclareDecodeVars	DeclareICMVars
#define PixelDecode		PixelICMDecode
#define PixelDecodeComplete     PixelICMDecodeComplete
#define DecodeDeclared
#define InitPixelDecode(CM)	InitPixelICM(CM)
#endif

#define DeclareICMVars					\
             int mapsize;				\
    unsigned int *cmrgb = 0 ;

#define InitPixelICM(CM) 						\
    do {								\
	cmrgb = (unsigned int *) getICMParams(env, CM, &mapsize);	\
    } while (0)
#define PixelICMDecodeComplete(CM)				\
    do {							\
	freeICMParams(env, CM, (jint *) cmrgb);		\
	cmrgb = NULL;						\
	mapsize = 0;						\
    } while (0)

#define PixelICMDecode(CM, pixel, red, green, blue, alpha)	\
    do {							\
	VerifyPixelRange(pixel, (unsigned int)mapsize);			\
	pixel = cmrgb[pixel];					\
	IfAlpha(alpha = (pixel >> ALPHASHIFT) & 0xff;)		\
	red = (pixel >> REDSHIFT) & 0xff;			\
	green = (pixel >> GREENSHIFT) & 0xff;			\
	blue = (pixel >> BLUESHIFT) & 0xff;			\
    } while (0)





