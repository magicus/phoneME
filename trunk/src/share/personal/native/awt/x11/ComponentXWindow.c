/*
 * @(#)ComponentXWindow.c	1.19 06/10/10
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

#include "java_awt_ComponentXWindow.h"
#include "java_awt_Cursor.h"
#include "awt.h"
#include <X11/cursorfont.h>


JNIEXPORT jint JNICALL
Java_java_awt_ComponentXWindow_create (JNIEnv *env, jobject this, jint parent)
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
						    0,
						    InputOnly,
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
Java_java_awt_ComponentXWindow_dispose (JNIEnv *env, jobject this)
{
	Window window = GET_X_WINDOW(this);
	
	xawt_removeWindow (env, window);
}

JNIEXPORT void JNICALL
Java_java_awt_ComponentXWindow_mapNative (JNIEnv *env, jobject this)
{
	Window window = GET_X_WINDOW(this);
	
	XMapWindow(xawt_display, window);
	XFlush(xawt_display);
}

JNIEXPORT void JNICALL
Java_java_awt_ComponentXWindow_unmap (JNIEnv *env, jobject this)
{
	Window window = GET_X_WINDOW(this);
	
	XUnmapWindow(xawt_display, window);
	XFlush(xawt_display);
}

JNIEXPORT jobject JNICALL
Java_java_awt_ComponentXWindow_getLocationOnScreen (JNIEnv *env, jobject this)
{
	Window window = GET_X_WINDOW(this);
	Window root, parent, child;
	Window *children;
	unsigned int nchildren;
	int x, y;
	unsigned int width, height, borderWidth, depth;
	
	XGetGeometry (xawt_display, window, &root, &x, &y, &width, &height, &borderWidth, &depth);
	XQueryTree (xawt_display, window, &root, &parent, &children, &nchildren);
	XTranslateCoordinates (xawt_display, parent, root, x, y, &x, &y, &child);
	
	if (children != NULL)
		XFree (children);
	
	return (*env)->NewObject (env, XCachedIDs.Point, XCachedIDs.Point_constructor, (jint)x, (jint)y);
}

JNIEXPORT void JNICALL
Java_java_awt_ComponentXWindow_setMouseEventMask (JNIEnv *env, jobject this, jlong mask)
{
	Window window = GET_X_WINDOW(this);
	
	XSelectInput (xawt_display, window, xawt_getXMouseEventMask(mask));
	XFlush (xawt_display);
}

JNIEXPORT void JNICALL
Java_java_awt_ComponentXWindow_setBoundsNative (JNIEnv *env, jobject this, jint x, jint y, jint width, jint height)
{
	Window window = GET_X_WINDOW(this);
	
	XMoveResizeWindow (xawt_display, window, x, y, width, height);
	XFlush (xawt_display);
}

JNIEXPORT void JNICALL
Java_java_awt_ComponentXWindow_setCursorNative (JNIEnv *env, jobject this, jint type)
{
	Window window = GET_X_WINDOW(this);
	unsigned int cursorType;
	XSetWindowAttributes attrs;
	
	switch (type)
	{
		case java_awt_Cursor_DEFAULT_CURSOR:
		cursorType = XC_left_ptr;
			break;
		case java_awt_Cursor_CROSSHAIR_CURSOR:
		cursorType = XC_crosshair;
			break;
		case java_awt_Cursor_TEXT_CURSOR:
		cursorType = XC_xterm;
			break;
		case java_awt_Cursor_WAIT_CURSOR:
		cursorType = XC_watch;
			break;
		case java_awt_Cursor_SW_RESIZE_CURSOR:
		cursorType = XC_bottom_left_corner;
			break;
		case java_awt_Cursor_NW_RESIZE_CURSOR:
		cursorType = XC_top_left_corner;
			break;
		case java_awt_Cursor_SE_RESIZE_CURSOR:
		cursorType = XC_bottom_right_corner;
			break;
		case java_awt_Cursor_NE_RESIZE_CURSOR:
		cursorType = XC_top_right_corner;
			break;
		case java_awt_Cursor_S_RESIZE_CURSOR:
		cursorType = XC_bottom_side;
			break;
		case java_awt_Cursor_N_RESIZE_CURSOR:
		cursorType = XC_top_side;
			break;
		case java_awt_Cursor_W_RESIZE_CURSOR:
		cursorType = XC_left_side;
			break;
		case java_awt_Cursor_E_RESIZE_CURSOR:
		cursorType = XC_right_side;
			break;
		case java_awt_Cursor_HAND_CURSOR:
		cursorType = XC_hand2;
			break;
		case java_awt_Cursor_MOVE_CURSOR:
		cursorType = XC_fleur;
			break;
    }
    
    attrs.cursor = XCreateFontCursor(xawt_display, cursorType);
	XChangeWindowAttributes(xawt_display, window, CWCursor, &attrs);
}
