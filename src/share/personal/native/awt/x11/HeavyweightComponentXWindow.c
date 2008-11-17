/*
 * @(#)HeavyweightComponentXWindow.c	1.7 06/10/10
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

#include "java_awt_HeavyweightComponentXWindow.h"
#include "awt.h"

JNIEXPORT jint JNICALL
Java_java_awt_HeavyweightComponentXWindow_create (JNIEnv *env, jobject this, jint parent)
{
	Window window;
	
	if (parent == 0)
	{
		(*env)->ThrowNew (env, XCachedIDs.AWTError, "parent window is not created");
		return 0;
	}
	
	/* Create the X window. We set the size to 1, 1 because setBounds will be called to set the real size.
	   This is necessary to have windows of 0 width or height which we represent as unmapped windows. */
	
	window = XCreateWindow (xawt_display,
						    (Window)parent,
						    0,
						    0,
						    1,
						    1,
						    0,
						    xawt_depth,
						    InputOutput,
						    xawt_visual,
						    0,
						    NULL);
	
	if (window == 0)
	{
		(*env)->ThrowNew (env, XCachedIDs.OutOfMemoryError, NULL);
		return 0;
	}
	
	if (!xawt_addWindow (env, window, this))
	{
		XDestroyWindow (xawt_display, window);
		window = 0;
	}
	
	return (jint)window;
}

JNIEXPORT void JNICALL
Java_java_awt_HeavyweightComponentXWindow_setBackground (JNIEnv *env, jobject this, jobject color)
{
	Window window = GET_X_WINDOW(this);
	
	XSetWindowBackground (xawt_display, window, xawt_getPixel(env, color));
}

JNIEXPORT void JNICALL
Java_java_awt_HeavyweightComponentXWindow_setMouseEventMask (JNIEnv *env, jobject this, jlong mask)
{
	Window window = GET_X_WINDOW(this);
	
	XSelectInput (xawt_display, window, xawt_getXMouseEventMask(mask) | ExposureMask);
	XFlush (xawt_display);
}
