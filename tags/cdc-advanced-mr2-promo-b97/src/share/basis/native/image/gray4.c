/*
 * @(#)gray4.c	1.15 06/10/10
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

extern unsigned short ntsc_gray_value[3][256];
unsigned char img_grays[256];
unsigned char img_bwgamma[256];
extern double pow(double, double);

static XColor gray4_cells[16] = {
    {0,  0x0000, 0x0000, 0x0000, DoRed | DoGreen | DoBlue, 0},
    {1,  0x1111, 0x1111, 0x1111, DoRed | DoGreen | DoBlue, 0},
    {2,  0x2222, 0x2222, 0x2222, DoRed | DoGreen | DoBlue, 0},
    {3,  0x3333, 0x3333, 0x3333, DoRed | DoGreen | DoBlue, 0},
    {4,  0x4444, 0x4444, 0x4444, DoRed | DoGreen | DoBlue, 0},
    {5,  0x5555, 0x5555, 0x5555, DoRed | DoGreen | DoBlue, 0},
    {6,  0x6666, 0x6666, 0x6666, DoRed | DoGreen | DoBlue, 0},
    {7,  0x7777, 0x7777, 0x7777, DoRed | DoGreen | DoBlue, 0},
    {8,  0x8888, 0x8888, 0x8888, DoRed | DoGreen | DoBlue, 0},
    {9,  0x9999, 0x9999, 0x9999, DoRed | DoGreen | DoBlue, 0},
    {10, 0xaaaa, 0xaaaa, 0xaaaa, DoRed | DoGreen | DoBlue, 0},
    {11, 0xbbbb, 0xbbbb, 0xbbbb, DoRed | DoGreen | DoBlue, 0},
    {12, 0xcccc, 0xcccc, 0xcccc, DoRed | DoGreen | DoBlue, 0},
    {13, 0xdddd, 0xdddd, 0xdddd, DoRed | DoGreen | DoBlue, 0},
    {14, 0xeeee, 0xeeee, 0xeeee, DoRed | DoGreen | DoBlue, 0},
    {15, 0xffff, 0xffff, 0xffff, DoRed | DoGreen | DoBlue, 0},
};

static void Gray4_pixel2RGB(unsigned int pixel, int *r, int *g, int *b) {
    if ((pixel > 15) || (mapped_pixels[pixel] != pixel)) {
	int i = 15;
	while (--i > 0) {
	    if (mapped_pixels[i] == pixel) {
		break;
	    }
	}
	pixel = i;
    }

    *r = gray4_cells[pixel].red   >> 8;
    *g = gray4_cells[pixel].green >> 8;
    *b = gray4_cells[pixel].blue  >> 8;
}

static unsigned int Gray4_RGB2Pixel(int r, int g, int b) {
    unsigned int gray = ntsc_gray_value[0][r & 0xff]
	              + ntsc_gray_value[1][g & 0xff]
	              + ntsc_gray_value[2][b & 0xff];

    return gray4_cells[gray >> 12].pixel;
}

static unsigned int Gray4_jRGB2Pixel(unsigned int RGB, int gamma_corr) {
    unsigned int gray = ntsc_gray_value[0][(RGB >> 16) & 0xff]
	              + ntsc_gray_value[1][(RGB >> 8)  & 0xff]
	              + ntsc_gray_value[2][(RGB >> 0)  & 0xff];

    /* this one is gamma corrected, because it is not used by image code */
    if (gamma_corr) {
	gray = img_bwgamma[gray >> 8];
	return gray4_cells[gray >> 4].pixel;
    } else {
	return gray4_cells[gray >> 12].pixel;
    }
}

void setupGray4(Colormap cmap, unsigned long pixels[],
		 XVisualInfo *visInfo) {
    int i;

    assert(NATIVE_LOCK_HELD());
    if ((visInfo->class == PseudoColor) || (visInfo->class == GrayScale)) {
	for (i = 0; i < 16; ++i) {
	    gray4_cells[i].pixel = pixels[i];
	}
	XStoreColors(awt_display, awt_cmap, gray4_cells, 16);
    } else {
	for (i = 0; i < 16; ++i) {
	    XAllocColor(awt_display, awt_cmap, gray4_cells + i);
	}
    }

    for (i = 0; i < 256; ++i) {
	img_bwgamma[i] = (int)(255 * pow(i/255.0, 0.6));
	img_grays[i] = gray4_cells[i >> 4].pixel;
    }

    pixel2RGB  = Gray4_pixel2RGB;
    RGB2Pixel  = Gray4_RGB2Pixel;
    jRGB2Pixel = Gray4_jRGB2Pixel;
    numColors  = 16;
    grayscale  = 1;
}
