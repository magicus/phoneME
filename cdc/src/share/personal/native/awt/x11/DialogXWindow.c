/*
 * @(#)DialogXWindow.c	1.9 06/10/10
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

#include "java_awt_DialogXWindow.h"
#include "awt.h"

JNIEXPORT jint JNICALL
Java_java_awt_DialogXWindow_create (JNIEnv *env, jobject this, jint owner)
{
	Window window;

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
							0,
							NULL);
	
	if (window == 0)
	{
		(*env)->ThrowNew (env, XCachedIDs.OutOfMemoryError, NULL);
		return 0;
	}
	
	if (owner != 0)
		XSetTransientForHint (xawt_display, window, (Window)owner);
	
	XSetWMProtocols (xawt_display, window, &XA_WM_DELETE_WINDOW, 1);
	
	if (!xawt_addWindow (env, window, this))
	{
		XDestroyWindow (xawt_display, window);
		window = 0;
	}
	
	return (jint)window;
}
