/*
 * @(#)img_dcm.h	1.21 06/10/10
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
 * This file contains macro definitions for the Decoding category of
 * the macros used by the generic scaleloop function.
 *
 * This implementation can decode the pixel information associated
 * with any Java DirectColorModel object.  This implemenation will
 * scale the decoded color components to 8-bit quantities if needed.
 * Another file is provided to optimize DCM parsing when the masks
 * are guaranteed to be at least 8-bits wide.  This implementation
 * examines some of the private fields of the DirectColorModel
 * object and decodes the red, green, blue, and possibly alpha values
 * directly rather than calling the getRGB method on the Java object.
 */

/*
 * These definitions vector the standard macro names to the "DCM"
 * versions of those macros only if the "DecodeDeclared" keyword has
 * not yet been defined elsewhere.  The "DecodeDeclared" keyword is
 * also defined here to claim ownership of the primary implementation
 * even though this file does not rely on the definitions in any other
 * files.
 */
#ifndef DecodeDeclared
#define DeclareDecodeVars	DeclareDCMVars
#define PixelDecode		PixelDCMDecode
#define PixelDecodeComplete     PixelDCMDecodeComplete
#define DecodeDeclared
#define InitPixelDecode(CM)	InitPixelDCM(CM)
#endif

#define DeclareDCMVars						\
    int red_mask, green_mask, blue_mask, alpha_mask;		\
    int red_scale, green_scale, blue_scale, alpha_scale;	\
    unsigned int red_off, green_off, blue_off, alpha_off;	\
    int scale = 0;

#define InitPixelDCM(CM)					        \
    do {							        \
	getDCMParams(env, CM,					        \
		     &red_mask, (int*)&red_off, &red_scale,		\
		     &green_mask, (int*)&green_off, &green_scale,	\
		     &blue_mask, (int*)&blue_off, &blue_scale,          \
                     &alpha_mask, (int*)&alpha_off, &alpha_scale);	\
	scale = (red_scale | green_scale | blue_scale			\
		 IfAlpha(| alpha_scale));				\
    } while (0)

#define PixelDCMDecode(CM, pixel, red, green, blue, alpha)		\
    do {								\
	IfAlpha(alpha = ((alpha_mask == 0)				\
			 ? 255						\
			 : ((pixel & alpha_mask) >> alpha_off));)	\
	red = ((pixel & red_mask) >> red_off);				\
	green = ((pixel & green_mask) >> green_off);			\
	blue = ((pixel & blue_mask) >> blue_off);			\
	if (scale) {							\
	    if (red_scale) {						\
		red = red * 255 / (red_scale);				\
	    }								\
	    if (green_scale) {						\
		green = green * 255 / (green_scale);			\
	    }								\
	    if (blue_scale) {						\
		blue = blue * 255 / (blue_scale);			\
	    }								\
	    IfAlpha(if (alpha_scale) {					\
		alpha = alpha * 255 / (alpha_scale);			\
	    })								\
	}								\
    } while (0)

#define PixelDCMDecodeComplete(CM) do { } while (0)






