/*
 * @(#)color.c	1.51 06/10/10
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

/*-
 *	Image dithering and rendering code for X11.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "java_awt_Color.h"

#include "color.h"
#include "img_colors.h"
#include "img_util.h"
#include "img_util_md.h"


/* returns the absolute value x */
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define CLIP(val,min,max)	((val < min) ? min : ((val > max) ? max : val))

enum {
	FREE_COLOR = 0,
	LIKELY_COLOR = 1,
	UNAVAILABLE_COLOR = 2,
	ALLOCATED_COLOR = 3
};

int             awt_num_colors;
ColorEntry      awt_Colors[256];

/*
 * Constants to control the filling of the colormap. By default, try to
 * allocate colors in the default colormap until CMAP_ALLOC_DEFAULT colors
 * are being used (by Java and/or other applications). For cases where the
 * default colormap may already have a large number of colors in it, make
 * sure that we ourselves try to add at least CMAP_ALLOC_MIN new colors, even
 * if we need to allocate more than the DEFAULT to do that. Under no
 * circumstances will the colormap be filled to more than CMAP_ALLOC_MAX
 * colors.
 */
#define CMAP_ALLOC_MIN		100	/* minimum number of colors to "add" */
#define CMAP_ALLOC_DEFAULT	200	/* default number of colors in cmap */
#define CMAP_ALLOC_MAX		245	/* maximum number of colors in cmap */

#define LOOKUPSIZE	32	/* per-axis size of dithering lookup table */

#ifdef SOLARIS2
#include <sys/utsname.h>

struct {
	char           *machine;
	int             cubesize;
}
                machinemap[] =
{
	{
		"sun4c", LOOKUPSIZE / 4
	}
	,
	{
		"sun4m", LOOKUPSIZE / 2
	}
	,
	{
		"sun4d", LOOKUPSIZE / 2
	}
	,
	{
		"sun4u", LOOKUPSIZE / 1
	}
	,
};

#define MACHMAPSIZE	(sizeof(machinemap) / sizeof(machinemap[0]))

int
getVirtCubeSize()
{
	struct utsname  name;
	int             i, ret;

	ret = uname(&name);
	if (ret < 0) {
		return LOOKUPSIZE;
	}
	for (i = 0; i < MACHMAPSIZE; i++) {
		if (strcmp(name.machine, machinemap[i].machine) == 0) {
			return machinemap[i].cubesize;
		}
	}

	return LOOKUPSIZE;
}
#else				/* SOLARIS */
#define getVirtCubeSize()	(LOOKUPSIZE)
#endif				/* SOLARIS */

unsigned char   img_clr_tbl[LOOKUPSIZE * LOOKUPSIZE * LOOKUPSIZE];

unsigned char   img_grays[256];
unsigned char   img_bwgamma[256];

uns_ordered_dither_array img_oda_alpha;
sgn_ordered_dither_array img_oda_red;
sgn_ordered_dither_array img_oda_green;
sgn_ordered_dither_array img_oda_blue;

/* the function pointers for doing image manipulation */
awtImageData    theAwtImage;
awtImageData   *awtImage = &theAwtImage;

/* the function for doing color to pixel conversion */
unsigned int             (*AwtColorMatch) (int, int, int);

ImgConvertFcn   DirectImageConvert;
ImgConvertFcn   Dir16IcmOpqUnsImageConvert;
ImgConvertFcn   Dir16IcmTrnUnsImageConvert;
ImgConvertFcn   Dir16IcmOpqSclImageConvert;
ImgConvertFcn   Dir16DcmOpqUnsImageConvert;
ImgConvertFcn   Dir16DcmTrnUnsImageConvert;
ImgConvertFcn   Dir16DcmOpqSclImageConvert;
ImgConvertFcn   Dir32IcmOpqUnsImageConvert;
ImgConvertFcn   Dir32IcmTrnUnsImageConvert;
ImgConvertFcn   Dir32IcmOpqSclImageConvert;
ImgConvertFcn   Dir32DcmOpqUnsImageConvert;
ImgConvertFcn   Dir32DcmTrnUnsImageConvert;
ImgConvertFcn   Dir32DcmOpqSclImageConvert;

ImgConvertFcn   PseudoImageConvert;
ImgConvertFcn   PseudoFSImageConvert;
ImgConvertFcn   FSColorIcmOpqUnsImageConvert;
ImgConvertFcn   FSColorDcmOpqUnsImageConvert;
ImgConvertFcn   OrdColorIcmOpqUnsImageConvert;
ImgConvertFcn   OrdColorDcmOpqUnsImageConvert;

/*
 * Find the best color.
 */
unsigned int
awt_color_matchTC(int r, int g, int b)
{
	r = CLIP(r, 0, 255);
	g = CLIP(g, 0, 255);
	b = CLIP(b, 0, 255);
	return (((r >> awtImage->clrdata.rScale) << awtImage->clrdata.rOff) |
		((g >> awtImage->clrdata.gScale) << awtImage->clrdata.gOff) |
		((b >> awtImage->clrdata.bScale) << awtImage->clrdata.bOff));
}

unsigned int
awt_color_matchGS(int r, int g, int b)
{
	r = CLIP(r, 0, 255);
	g = CLIP(g, 0, 255);
	b = CLIP(b, 0, 255);
	return img_grays[RGBTOGRAY(r, g, b)];
}

unsigned int
awt_color_match(int r, int g, int b)
{
	int             besti = 0;
	int             mindist, i, t, d;
	ColorEntry     *p = awt_Colors;

	r = CLIP(r, 0, 255);
	g = CLIP(g, 0, 255);
	b = CLIP(b, 0, 255);

	/* look for pure gray match */
	if ((r == g) && (g == b)) {
		mindist = 256;
		for (i = 0; i < awt_num_colors; i++, p++)
			if (p->flags == ALLOCATED_COLOR) {
				if (!((p->r == p->g) && (p->g == p->b)))
					continue;
				d = ABS(p->r - r);
				if (d == 0)
					return i;
				if (d < mindist) {
					besti = i;
					mindist = d;
				}
			}
		return besti;
	}
	/* look for non-pure gray match */
	mindist = 256 * 256 * 256;
	for (i = 0; i < awt_num_colors; i++, p++)
		if (p->flags == ALLOCATED_COLOR) {
			t = p->r - r;
			d = t * t;
			if (d >= mindist)
				continue;
			t = p->g - g;
			d += t * t;
			if (d >= mindist)
				continue;
			t = p->b - b;
			d += t * t;
			if (d >= mindist)
				continue;
			if (d == 0)
				return i;
			if (d < mindist) {
				besti = i;
				mindist = d;
			}
		}
	return besti;
}

/*
 * Allocate a color in the X color map and return the pixel. If the "expected
 * pixel" is non-negative then we will only accept the allocation if we get
 * exactly that pixel value. This prevents us from seeing a bunch of
 * ReadWrite pixels allocated by another imaging application and duplicating
 * that set of inaccessible pixels in our precious remaining ReadOnly
 * colormap cells.
 */
static unsigned int
alloc_col(Display * dpy, Colormap cm, int r, int g, int b, int pixel)
{
	XColor          col;

	r = CLIP(r, 0, 255);
	g = CLIP(g, 0, 255);
	b = CLIP(b, 0, 255);

	col.flags = DoRed | DoGreen | DoBlue;
	col.red = (r << 8) | r;
	col.green = (g << 8) | g;
	col.blue = (b << 8) | b;
	if (XAllocColor(dpy, cm, &col)) {
		if (pixel >= 0 && col.pixel != pixel) {
			/*
			 * If we were trying to allocate a shareable
			 * "ReadOnly" color then we would have gotten back
			 * the expected pixel.  If the returned pixel was
			 * different, then the source color that we were
			 * attempting to gain access to must be some other
			 * application's ReadWrite private color.  We free
			 * the returned pixel so that we won't waste precious
			 * colormap entries by duplicating that color in the
			 * as yet unallocated entries.  We return -1 here to
			 * indicate the failure to get the expected pixel.
			 */
			awt_Colors[pixel].flags = UNAVAILABLE_COLOR;
			XFreeColors(dpy, cm, &col.pixel, 1, 0);
			return -1;
		}
		awt_Colors[col.pixel].flags = ALLOCATED_COLOR;
		awt_Colors[col.pixel].r = col.red >> 8;
		awt_Colors[col.pixel].g = col.green >> 8;
		awt_Colors[col.pixel].b = col.blue >> 8;
		return col.pixel;
	}
	return awt_color_match(r, g, b);
}


void awt_pixel_match(unsigned int pixel, int *r, int *g, int *b)
{
  *r = awt_Colors[pixel].r;
  *g = awt_Colors[pixel].g;
  *b = awt_Colors[pixel].b;
}


void
awt_fill_imgcv(ImgConvertFcn ** array, int mask, int value, ImgConvertFcn fcn)
{
	int             i;

	for (i = 0; i < NUM_IMGCV; i++) {
		if ((i & mask) == value) {
			array[i] = fcn;
		}
	}
}

/*
 * called from X11Server_create() in xlib.c
 */
int
awt_allocate_colors()
{
	Display        *dpy;
	unsigned long   plane_masks[1];

	/*
	 * Malloc heap for those variable instead of getting it from stack as
	 * local variable to avoid huge stack usage.
	 */
	unsigned long  *freecolors;
	XColor         *cols;
	unsigned char  *reds, *greens, *blues, *indices;

	Colormap        cm;
	int             i, j, k, cmapsize, nfree, depth, bpp, screen;
	XPixmapFormatValues *pPFV;
	int             numpfv;
	XVisualInfo    *pVI;
	char           *forcemono;
	char           *forcegray;

	char           *alloced = XtMalloc(256 * sizeof(unsigned long)
		  + 256 * sizeof(XColor) + 4 * 256 * sizeof(unsigned char));

	if (!alloced) {
		return 0;
	}

	freecolors = (unsigned long *) alloced;
	cols = (XColor *) (alloced + 256 * sizeof(unsigned long));
	reds = (unsigned char *) (alloced + 256 * sizeof(unsigned long)
		  + 256 * sizeof(XColor) + 0 * 256 * sizeof(unsigned char));
	greens = (unsigned char *) (alloced + 256 * sizeof(unsigned long)
		  + 256 * sizeof(XColor) + 1 * 256 * sizeof(unsigned char));
	blues = (unsigned char *) (alloced + 256 * sizeof(unsigned long)
		  + 256 * sizeof(XColor) + 2 * 256 * sizeof(unsigned char));
	indices = (unsigned char *) (alloced + 256 * sizeof(unsigned long)
		  + 256 * sizeof(XColor) + 3 * 256 * sizeof(unsigned char));

	make_uns_ordered_dither_array(img_oda_alpha, 256);


	forcemono = getenv("FORCEMONO");
	forcegray = getenv("FORCEGRAY");
	if (forcemono && !forcegray)
		forcegray = forcemono;

	/*
	 * Get the colormap and make sure we have the right visual
	 */
	dpy = awt_display;
	screen = awt_screen;
	cm = awt_cmap;
	depth = awt_depth;
	pVI = &awt_visInfo;
	awt_num_colors = awt_visInfo.colormap_size;

	pPFV = XListPixmapFormats(dpy, &numpfv);
	if (pPFV) {
		for (i = 0; i < numpfv; i++) {
			if (pPFV[i].depth == depth) {
				awtImage->wsImageFormat = pPFV[i];
				break;
			}
		}
		XtFree((char *)pPFV);
		pPFV = NULL;
	}
	bpp = awtImage->wsImageFormat.bits_per_pixel;
	if (bpp == 24) {
		bpp = 32;
	}
	awtImage->clrdata.bitsperpixel = bpp;
	awtImage->Depth = depth;


	if ((bpp == 32 || bpp == 16) && pVI->class == TrueColor) {
		AwtColorMatch = awt_color_matchTC;
		awtImage->clrdata.rOff = 0;
		for (i = pVI->red_mask; (i & 1) == 0; i >>= 1) {
			awtImage->clrdata.rOff++;
		}
		awtImage->clrdata.rScale = 0;
		while (i < 0x80) {
			awtImage->clrdata.rScale++;
			i <<= 1;
		}
		awtImage->clrdata.gOff = 0;
		for (i = pVI->green_mask; (i & 1) == 0; i >>= 1) {
			awtImage->clrdata.gOff++;
		}
		awtImage->clrdata.gScale = 0;
		while (i < 0x80) {
			awtImage->clrdata.gScale++;
			i <<= 1;
		}
		awtImage->clrdata.bOff = 0;
		for (i = pVI->blue_mask; (i & 1) == 0; i >>= 1) {
			awtImage->clrdata.bOff++;
		}
		awtImage->clrdata.bScale = 0;
		while (i < 0x80) {
			awtImage->clrdata.bScale++;
			i <<= 1;
		}
		awt_fill_imgcv(awtImage->convert, 0, 0, DirectImageConvert);
		awt_fill_imgcv(awtImage->convert,
			       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
				| IMGCV_ALPHABITS | IMGCV_CMBITS),
			       (IMGCV_UNSCALED | IMGCV_BYTEIN
				| IMGCV_OPAQUE | IMGCV_ICM),
			       (bpp == 32 ? Dir32IcmOpqUnsImageConvert : Dir16IcmOpqUnsImageConvert));
		awt_fill_imgcv(awtImage->convert,
			       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
				| IMGCV_ALPHABITS | IMGCV_CMBITS),
			       (IMGCV_UNSCALED | IMGCV_BYTEIN
				| IMGCV_ALPHA | IMGCV_ICM),
			       (bpp == 32 ? Dir32IcmTrnUnsImageConvert : Dir16IcmTrnUnsImageConvert));
		awt_fill_imgcv(awtImage->convert,
			       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
				| IMGCV_ALPHABITS | IMGCV_CMBITS),
			       (IMGCV_SCALED | IMGCV_BYTEIN
				| IMGCV_OPAQUE | IMGCV_ICM),
			       (bpp == 32 ? Dir32IcmOpqSclImageConvert : Dir16IcmOpqSclImageConvert));
		awt_fill_imgcv(awtImage->convert,
			       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
				| IMGCV_ALPHABITS | IMGCV_CMBITS),
			       (IMGCV_UNSCALED | IMGCV_INTIN
				| IMGCV_OPAQUE | IMGCV_DCM8),
			       (bpp == 32 ? Dir32DcmOpqUnsImageConvert : Dir16DcmOpqUnsImageConvert));
		awt_fill_imgcv(awtImage->convert,
			       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
				| IMGCV_ALPHABITS | IMGCV_CMBITS),
			       (IMGCV_UNSCALED | IMGCV_INTIN
				| IMGCV_ALPHA | IMGCV_DCM8),
			       (bpp == 32 ? Dir32DcmTrnUnsImageConvert : Dir16DcmTrnUnsImageConvert));
		awt_fill_imgcv(awtImage->convert,
			       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
				| IMGCV_ALPHABITS | IMGCV_CMBITS),
			       (IMGCV_SCALED | IMGCV_INTIN
				| IMGCV_OPAQUE | IMGCV_DCM8),
			       (bpp == 32 ? Dir32DcmOpqSclImageConvert : Dir16DcmOpqSclImageConvert));
	} else if (bpp <= 8 && (pVI->class == StaticGray || pVI->class == GrayScale || forcegray)) {
		AwtColorMatch = awt_color_matchGS;

		/* Needed for color lookup by image conversion (24->8) functions */
		RGB2Pixel = awt_color_matchGS;
		pixel2RGB = awt_pixel_match;

		awtImage->clrdata.grayscale = 1;
		awtImage->clrdata.bitsperpixel = 8;
		awt_fill_imgcv(awtImage->convert, 0, 0, PseudoImageConvert);
		if (getenv("NOFSDITHER") == 0) {
			awt_fill_imgcv(awtImage->convert, IMGCV_ORDERBITS, IMGCV_TDLRORDER, PseudoFSImageConvert);
		}
	} else if (bpp <= 8 && (pVI->class == PseudoColor || pVI->class == TrueColor	/* BugTraq ID 4061285 */
				|| pVI->class == StaticColor)) {
		if (pVI->class == TrueColor) {
			awt_num_colors = (1 << pVI->depth);	/* BugTraq ID 4061285 */
		}
		AwtColorMatch = awt_color_match;

		/* Needed for color lookup by image conversion (24->8) functions */
		RGB2Pixel = awt_color_match;
		pixel2RGB = awt_pixel_match;

		awtImage->clrdata.bitsperpixel = 8;
		awt_fill_imgcv(awtImage->convert, 0, 0, PseudoImageConvert);


		if (getenv("NOFSDITHER") == 0) {
			awt_fill_imgcv(awtImage->convert, IMGCV_ORDERBITS, IMGCV_TDLRORDER, PseudoFSImageConvert);
			awt_fill_imgcv(awtImage->convert,
				       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
					| IMGCV_ALPHABITS | IMGCV_ORDERBITS
					| IMGCV_CMBITS),
				       (IMGCV_UNSCALED | IMGCV_BYTEIN
					| IMGCV_OPAQUE | IMGCV_TDLRORDER | IMGCV_ICM), FSColorIcmOpqUnsImageConvert);
			awt_fill_imgcv(awtImage->convert,
				       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS
					| IMGCV_ALPHABITS | IMGCV_ORDERBITS
					| IMGCV_CMBITS),
				       (IMGCV_UNSCALED | IMGCV_INTIN
					| IMGCV_OPAQUE | IMGCV_TDLRORDER | IMGCV_DCM8), FSColorDcmOpqUnsImageConvert);
		}
		awt_fill_imgcv(awtImage->convert,
		       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS | IMGCV_ALPHABITS
			| IMGCV_ORDERBITS | IMGCV_CMBITS),
			       (IMGCV_UNSCALED | IMGCV_BYTEIN | IMGCV_OPAQUE
				| IMGCV_RANDORDER | IMGCV_ICM), OrdColorIcmOpqUnsImageConvert);
		awt_fill_imgcv(awtImage->convert,
		       (IMGCV_SCALEBITS | IMGCV_INSIZEBITS | IMGCV_ALPHABITS
			| IMGCV_ORDERBITS | IMGCV_CMBITS),
			       (IMGCV_UNSCALED | IMGCV_INTIN | IMGCV_OPAQUE
				| IMGCV_RANDORDER | IMGCV_DCM8), OrdColorDcmOpqUnsImageConvert);
	} else {
		XtFree(alloced);
		alloced = NULL;
		return 0;
	}

	if (depth > 8) {
		XtFree(alloced);
		alloced = NULL;
		return 1;
	}
	if (awt_num_colors > 256) {
		XtFree(alloced);
		alloced = NULL;
		return 0;
	}
	/*
	 * Initialize colors array
	 */
	for (i = 0; i < awt_num_colors; i++) {
		cols[i].pixel = i;
	}

	XQueryColors(dpy, cm, cols, awt_num_colors);
	for (i = 0; i < awt_num_colors; i++) {
		awt_Colors[i].r = cols[i].red >> 8;
		awt_Colors[i].g = cols[i].green >> 8;
		awt_Colors[i].b = cols[i].blue >> 8;
		awt_Colors[i].flags = LIKELY_COLOR;
	}

	/*
	 * Determine which colors in the colormap can be allocated and mark
	 * them in the colors array
	 */
	nfree = 0;
	for (i = 128; i > 0; i >>= 1) {
		if (XAllocColorCells(dpy, cm, False, plane_masks, 0, freecolors + nfree, i)) {
			nfree += i;
		}
	}

	for (i = 0; i < nfree; i++) {
		awt_Colors[freecolors[i]].flags = FREE_COLOR;
	}

	XFreeColors(dpy, cm, freecolors, nfree, 0);

	/*
	 * Allocate the colors that are already allocated by other
	 * applications
	 */
	for (i = 0; i < awt_num_colors; i++) {
		if (awt_Colors[i].flags == LIKELY_COLOR) {
			awt_Colors[i].flags = FREE_COLOR;
			alloc_col(dpy, cm, awt_Colors[i].r, awt_Colors[i].g, awt_Colors[i].b, i);
		}
	}

	/*
	 * Allocate more colors, filling the color space evenly.
	 */

	alloc_col(dpy, cm, 255, 255, 255, -1);
	alloc_col(dpy, cm, 255, 0, 0, -1);

	if (awtImage->clrdata.grayscale) {
		int             g;

		if (!forcemono) {
			for (i = 128; i > 0; i >>= 1) {
				for (g = i; g < 256; g += (i * 2)) {
					alloc_col(dpy, cm, g, g, g, -1);
				}
			}
		}
		make_sgn_ordered_dither_array(img_oda_green, -16, 16);

		for (g = 0; g < 256; g++) {
			ColorEntry     *p;
			int             mindist, besti;
			int             d;

			p = awt_Colors;
			mindist = 256;
			besti = 0;
			for (i = 0; i < awt_num_colors; i++, p++) {
				if (forcegray && (p->r != p->g || p->g != p->b))
					continue;
				if (forcemono && p->g != 0 && p->g != 255)
					continue;
				if (p->flags == ALLOCATED_COLOR) {
					d = p->g - g;
					if (d < 0)
						d = -d;
					if (d < mindist) {
						besti = i;
						if (d == 0) {
							break;
						}
						mindist = d;
					}
				}
			}

			img_grays[g] = besti;
		}

		if (forcemono || (depth == 1)) {
			char           *gammastr = getenv("HJGAMMA");
			double          gamma = atof(gammastr ? gammastr : "1.6");

			if (gamma < 0.01)
				gamma = 1.0;
			for (i = 0; i < 256; i++) {
				img_bwgamma[i] = (int) (pow(i / 255.0, gamma) * 255);
			}
		} else {
			for (i = 0; i < 256; i++) {
				img_bwgamma[i] = i;
			}
		}

		XtFree(alloced);
		alloced = NULL;
		return 1;
	}
	alloc_col(dpy, cm, 0, 255, 0, -1);
	alloc_col(dpy, cm, 0, 0, 255, -1);
	alloc_col(dpy, cm, 255, 255, 0, -1);
	alloc_col(dpy, cm, 255, 0, 255, -1);
	alloc_col(dpy, cm, 0, 255, 255, -1);
	alloc_col(dpy, cm, 192, 192, 192, -1);
	alloc_col(dpy, cm, 255, 128, 128, -1);
	alloc_col(dpy, cm, 128, 255, 128, -1);
	alloc_col(dpy, cm, 128, 128, 255, -1);
	alloc_col(dpy, cm, 255, 255, 128, -1);
	alloc_col(dpy, cm, 255, 128, 255, -1);
	alloc_col(dpy, cm, 128, 255, 255, -1);

	j = 0;
	k = 0;
	for (i = 0; i < awt_num_colors; i++) {
		if (awt_Colors[i].flags == ALLOCATED_COLOR) {
			reds[j] = awt_Colors[i].r;
			greens[j] = awt_Colors[i].g;
			blues[j] = awt_Colors[i].b;
			indices[j] = i;
			j++;
		} else if (awt_Colors[i].flags == UNAVAILABLE_COLOR) {
			k++;
		}
	}
	cmapsize = 0;
	if (getenv("CMAPSIZE") != 0) {
		cmapsize = atoi(getenv("CMAPSIZE"));
	}
	if (cmapsize <= 0) {
		cmapsize = CMAP_ALLOC_DEFAULT;
	}
	if (cmapsize < j + k + CMAP_ALLOC_MIN) {
		cmapsize = j + k + CMAP_ALLOC_MIN;
	}
	if (cmapsize > CMAP_ALLOC_MAX) {
		cmapsize = CMAP_ALLOC_MAX;
	}
	if (cmapsize < j) {
		cmapsize = j;
	}
	cmapsize -= k;

	k = 0;
	if (getenv("VIRTCUBESIZE") != 0) {
		k = atoi(getenv("VIRTCUBESIZE"));
	}
	if (k == 0 || (k & (k - 1)) != 0 || k > 32) {
		k = getVirtCubeSize();
	}
	img_makePalette(cmapsize, k, LOOKUPSIZE, 50, 250, j, TRUE, reds, greens, blues, img_clr_tbl);
	for (i = 0; i < cmapsize; i++) {
		indices[i] = alloc_col(dpy, cm, reds[i], greens[i], blues[i], -1);
	}
	for (i = 0; i < sizeof(img_clr_tbl); i++) {
		img_clr_tbl[i] = indices[img_clr_tbl[i]];
	}

	/*
	 * Initialize the per-component ordered dithering arrays Choose a
	 * size based on how far between elements in the virtual cube. Assume
	 * the cube has cuberoot(cmapsize) elements per axis and those
	 * elements are distributed over 256 colors. The calculation should
	 * really divide by (#comp/axis - 1) since the first and last
	 * elements are at the extremes of the 256 levels, but in a practical
	 * sense this formula produces a smaller error array which results in
	 * smoother images that have slightly less color fidelity but much
	 * less dithering noise, especially for grayscale images.
	 */
	i = (int) (256 / pow(cmapsize, 1.0 / 3.0));
	make_sgn_ordered_dither_array(img_oda_red, -i / 2, i / 2);
	make_sgn_ordered_dither_array(img_oda_green, -i / 2, i / 2);
	make_sgn_ordered_dither_array(img_oda_blue, -i / 2, i / 2);

	/*
	 * Flip green horizontally and blue vertically so that the errors
	 * don't line up in the 3 primary components.
	 */
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 4; j++) {
			k = img_oda_green[i][j];
			img_oda_green[i][j] = img_oda_green[i][7 - j];
			img_oda_green[i][7 - j] = k;
			k = img_oda_blue[j][i];
			img_oda_blue[j][i] = img_oda_blue[7 - j][i];
			img_oda_blue[7 - j][i] = k;
		}
	}

	XtFree(alloced);
	alloced = NULL;
	return 1;
}

/** Returns NULL if, and only if, an exception has occurred. */

jobject
awt_getColorModel(JNIEnv * env)
{
	jobject         awt_colormodel = NULL;

	if (awt_visInfo.class == TrueColor) {
		awt_colormodel = (*env)->NewObject(env,
			    MCachedIDs.java_awt_image_DirectColorModelClass,
		  MCachedIDs.java_awt_image_DirectColorModel_constructorMID,
						   awt_visInfo.depth,
						   awt_visInfo.red_mask,
						   awt_visInfo.green_mask,
						   awt_visInfo.blue_mask,
						   0);
	} else {
		jbyteArray      redByteArray, greenByteArray, blueByteArray;
		int             i;
		jbyte           dummyByte[256];

		redByteArray = (*env)->NewByteArray(env, 256);

		if (redByteArray == NULL)
			return NULL;

		greenByteArray = (*env)->NewByteArray(env, 256);

		if (redByteArray == NULL)
			return NULL;

		blueByteArray = (*env)->NewByteArray(env, 256);

		if (redByteArray == NULL)
			return NULL;

		for (i = 0; i < 256; i++) {
			dummyByte[i] = awt_Colors[i].r;
		}
		
		(*env)->SetByteArrayRegion(env, redByteArray, 0, 256, dummyByte);
		

		for (i = 0; i < 256; i++) {
			dummyByte[i] = awt_Colors[i].g;
		}

	        (*env)->SetByteArrayRegion(env, greenByteArray, 0, 256, dummyByte);
		
		for (i = 0; i < 256; i++) {
			dummyByte[i] = awt_Colors[i].b;
		}

		(*env)->SetByteArrayRegion(env, blueByteArray, 0, 256, dummyByte);

		awt_colormodel = (*env)->NewObject(env, MCachedIDs.java_awt_image_IndexColorModelClass,
		   MCachedIDs.java_awt_image_IndexColorModel_constructorMID,
						   awt_visInfo.depth,
						   awt_num_colors,
						   redByteArray,
						   greenByteArray,
						   blueByteArray);
	}

	return awt_colormodel;
}

#define red(v)		(((v) >> 16) & 0xFF)
#define green(v)	(((v) >>  8) & 0xFF)
#define blue(v)		(((v) >>  0) & 0xFF)

jint
awt_getColor(JNIEnv * env, jobject color)
{
	if (color != NULL) {
		jint            col = (*env)->GetIntField(env, color, MCachedIDs.java_awt_Color_pDataFID);

		if (col != 0)
			return col - 1;

		col = (*env)->CallIntMethod(env, color, MCachedIDs.java_awt_Color_getRGBMID);

		if ((*env)->ExceptionCheck(env))
			(*env)->ExceptionDescribe(env);

		col = AwtColorMatch(red(col), green(col), blue(col));
		/*
		 * unhand(color)->pData = col + 1;
		 */
		return col;
	}
	return 0;
}
