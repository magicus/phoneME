/*
 * @(#)GdkGraphics.h	1.13 06/10/10
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

#ifndef _GDKGRAPHICS_H_
#define _GDKGRAPHICS_H_

#include "awt.h"


typedef struct
{
	/* The drawable object used for drawing using Gdk. */

	GdkDrawable *drawable;
	
	/* The graphics context which keeps track of colors etc. */
	
	GdkGC *gc;
	
	/* TRUE if this drawable is a GdkPixmap. Otherwise it is assumed to be a GdkWindow.
	   This is required for disposing where we musrt either call gdk_pixmap_unref or
	   gdk_window_unref. */
	   
	gboolean isPixmap;
	
	/* The current clip rectangle.  Unfortunately a GdkRectangle only has 16bit resolution */
	
	GdkRectangle clipBounds;
	
	/* TRUE if a clip rectangle has been set. */
	
	gboolean clipHasBeenSet;
	
	/* The foreground color. */
	
	GdkColor foreground;
	
	/* TRUE if XOR mode is being used. */
	
	gboolean xorMode;
	
	GdkColor xorColor;
  
        /* Porter-Duff rule for alpha blending */
        int alpha_rule;

        /* Alpha value (between 0 & 1) for alpha blending */
        float extra_alpha;
	
} GdkGraphicsData;

extern gboolean awt_gtk_getColor (JNIEnv *env, jobject color, GdkColor *gdkcolor);

#endif
