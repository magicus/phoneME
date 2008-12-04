/*
 * @(#)WindowXWindow.c	1.18 06/10/10
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

#include "java_awt_WindowXWindow.h"
#include "awt.h"

JNIEXPORT jint JNICALL
Java_java_awt_WindowXWindow_create (JNIEnv *env, jobject this, jint owner)
{
	Window window;
	XSetWindowAttributes attributes;
	
	/* Make sure window manager does not reparent this window as we are not a Frame or Dialog. */
	
	attributes.override_redirect = True;
	
	window = XCreateWindow (xawt_display,
							xawt_root,
							0,
							0,
							1,
							1,
							0,
							xawt_depth,
							InputOutput,
							xawt_visual,
							CWOverrideRedirect,
							&attributes);
	
	if (window == 0)
	{
		(*env)->ThrowNew (env, XCachedIDs.OutOfMemoryError, NULL);
		return 0;
	}
	
	if (owner != 0)
		XSetTransientForHint (xawt_display, window, (Window)owner);
	
	if (!xawt_addWindow (env, window, this))
	{
		XDestroyWindow (xawt_display, window);
		window = 0;
	}
	
	return (jint)window;
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_setMouseEventMask (JNIEnv *env, jobject this, jlong mask)
{
	Window window = GET_X_WINDOW(this);
	
	XSelectInput (xawt_display, window, xawt_getXMouseEventMask(mask) |
									    ExposureMask |
									    StructureNotifyMask |
									    KeyPressMask |
									    KeyReleaseMask |
									    FocusChangeMask);
	XFlush (xawt_display);
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_setTitle (JNIEnv *env, jobject this, jstring title)
{
	Window window = GET_X_WINDOW(this);
	char *xtitle;
	
	if (title != NULL)
	{
		xtitle = (char *)(*env)->GetStringUTFChars (env, title, NULL);
		XStoreName(xawt_display, window, xtitle);
		(*env)->ReleaseStringUTFChars(env, title, xtitle);
	}
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_toFront (JNIEnv *env, jobject this)
{
	Window window = GET_X_WINDOW(this);
	
	XRaiseWindow (xawt_display, window);
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_toBack (JNIEnv *env, jobject this)
{
	Window window = GET_X_WINDOW(this);
	
	XLowerWindow (xawt_display, window);
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_setIconImageNative (JNIEnv *env, jobject this, jint pixmap)
{
	Window window = GET_X_WINDOW(this);
	XWMHints hints;
	
	hints.flags = IconPixmapHint;
	hints.icon_pixmap = (Pixmap)pixmap;
	
	XSetWMHints(xawt_display, window, &hints);
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_setBoundsNative (JNIEnv *env, jobject this, jint x, jint y, jint width, jint height)
{
	Window window = GET_X_WINDOW(this);
	XSizeHints hints;
	
	hints.flags = PPosition | PSize;
	hints.x = x;
	hints.y = y;
	hints.width = width;
	hints.height = height;
	XSetNormalHints(xawt_display, window, &hints);
	
	XMoveResizeWindow (xawt_display, window, x, y, width, height);
	XFlush (xawt_display);
}

JNIEXPORT void JNICALL
Java_java_awt_WindowXWindow_setSizeHintsNative (JNIEnv *env, jobject this,
												jint minWidth,
												jint minHeight,
												jint maxWidth,
												jint maxHeight)
{
	Window window = GET_X_WINDOW(this);
	XSizeHints hints;
	
	hints.flags = PMinSize | PMaxSize;
	hints.min_width = minWidth;
	hints.min_height = minHeight;
	hints.max_width = maxWidth;
	hints.max_height = maxHeight;
	
	XSetNormalHints(xawt_display, window, &hints);
}
