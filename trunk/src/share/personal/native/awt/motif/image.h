/*
 * @(#)image.h	1.43 06/10/10
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

#include <X11/Xlib.h>
#include "jni.h"
#include "java_awt_Color.h"
#include "sun_awt_image_ImageRepresentation.h"

extern Drawable image_getIRDrawable(JNIEnv * env, jobject ir);

extern int 
awt_imageDraw(JNIEnv * env, jobject irh, jobject c,
	      Drawable win, GC gc,
	      int xormode,
	      unsigned long xorpixel, unsigned long fgpixel,
	      long dx, long dy,
	      long sx, long sy, long sw, long sh,
	      XRectangle * clip);

extern int 
awt_imageStretch(JNIEnv * env, jobject irh, jobject c,
		 Drawable win, GC gc,
		 int xormode,
		 unsigned long xorpixel, unsigned long fgpixel,
		 long dx1, long dy1, long dx2, long dy2,
		 long sx1, long sy1, long sx2, long sy2,
		 XRectangle * clip);

extern GC       awt_getImageGC(Pixmap pixmap);
