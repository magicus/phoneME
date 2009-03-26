/*
 * @(#)color8.c	1.13 06/10/10
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
#include "Xheaders.h"
#include "common.h"

#include "p8.ci"

static void Color8_pixel2RGB(unsigned int pixel,
			     int *r, int *g, int *b) {
    if ((pixel > 255) || (mapped_pixels[pixel] != pixel)) {
	int i = 255;
	while (--i > 0) {
	    if (mapped_pixels[i] == pixel) {
		break;
	    }
	}
	pixel = i;
    }

    *r = color8_cells[pixel].red   >> 8;
    *g = color8_cells[pixel].green >> 8;
    *b = color8_cells[pixel].blue  >> 8;
}

static unsigned int Color8_RGB2Pixel(int r, int g, int b) {
    return color8_cells[find_index(r,g,b)].pixel;
}

static unsigned int Color8_jRGB2Pixel(unsigned int RGB, int gamma_corr) {
    int r = (RGB>>16) & 0xff;
    int g = (RGB>>8)  & 0xff;
    int b = (RGB>>0)  & 0xff;

    return color8_cells[find_index(r,g,b)].pixel;
}

void setupColor8(Colormap cmap, unsigned long pixels[],
		 XVisualInfo *visInfo) {
    int i;

    assert(NATIVE_LOCK_HELD());
    if (visInfo->class == PseudoColor) {
	for (i = 0; i < 256; ++i) {
	    color8_cells[i].pixel = pixels[i];
	}
	XStoreColors(awt_display, awt_cmap, color8_cells, 256);
    } else {
	for (i = 0; i < 256; ++i) {
	    XAllocColor(awt_display, awt_cmap, color8_cells + i);
	}
    }

    pixel2RGB  = Color8_pixel2RGB;
    RGB2Pixel  = Color8_RGB2Pixel;
    jRGB2Pixel = Color8_jRGB2Pixel;
    numColors  = 256;
    grayscale  = 0;
}

