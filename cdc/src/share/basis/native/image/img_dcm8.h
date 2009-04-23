/*
 * @(#)img_dcm8.h	1.16 06/10/04
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
 * with Java DirectColorModel objects where the color masks are
 * guaranteed to be at least 8-bits wide each.  It is slightly more
 * efficient then the generic DCM parsing code since it does not need
 * to store or test component scaling values.  This implementation
 * examines some of the private fields of the DirectColorModel
 * object and decodes the red, green, blue, and possibly alpha values
 * directly rather than calling the getRGB method on the Java object.
 */

/*
 * These definitions vector the standard macro names to the "DCM8"
 * versions of those macros only if the "DecodeDeclared" keyword has
 * not yet been defined elsewhere.  The "DecodeDeclared" keyword is
 * also defined here to claim ownership of the primary implementation
 * even though this file does not rely on the definitions in any other
 * files.
 */
#ifndef DecodeDeclared
#define DeclareDecodeVars	DeclareDCM8Vars
#define InitPixelDecode(CM)	InitPixelDCM8(CM)
#define PixelDecode		PixelDCM8Decode
#define PixelDecodeComplete     PixelDCM8DecodeComplete
#define DecodeDeclared
#endif

#define DeclareDCM8Vars						\
    unsigned int red_off, green_off, blue_off, alpha_off;

#define InitPixelDCM8(CM)                                               \
    do {                                                                \
	int dummy[8];                                                   \
    	getDCMParams(env, CM,						\
		     dummy + 0, (int*)&red_off,   dummy + 1,            \
		     dummy + 2, (int*)&green_off, dummy + 3,            \
		     dummy + 4, (int*)&blue_off,  dummy + 5,            \
                     dummy + 6, (int*)&alpha_off, dummy + 7);           \
    } while (0)

#define PixelDCM8Decode(CM, pixel, red, green, blue, alpha)		\
    do {								\
	IfAlpha(alpha = ((alpha_off < 0)				\
			 ? 255						\
			 : (pixel >> alpha_off) & 0xff);)		\
	red = (pixel >> red_off) & 0xff;				\
	green = (pixel >> green_off) & 0xff;				\
	blue = (pixel >> blue_off) & 0xff;				\
    } while (0)

#define PixelDCM8DecodeComplete(CM) do { } while (0)



