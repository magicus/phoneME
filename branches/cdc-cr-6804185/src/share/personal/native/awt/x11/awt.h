/*
 * @(#)awt.h	1.24 06/10/10
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

#ifndef _AWT_H_
#define _AWT_H_

#include <jni.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

struct CachedIDs
{
	jclass		OutOfMemoryError;
	jclass		NullPointerException;
	jclass		AWTError;
	jclass 		WindowXWindow;
	jclass		Point;
        jclass          DirectColorModel;

	jfieldID	Component_xFID;
	jfieldID	Component_yFID;
	jfieldID	Component_widthFID;
	jfieldID	Component_heightFID;
	jfieldID	ComponentXWindow_componentFID;
	jfieldID	ComponentXWindow_windowIDFID;
	jfieldID	Color_pixelFID;
	jfieldID	Color_valueFID;
	jfieldID	WindowXWindow_outerWindowFID;
	jfieldID    X11FontMetrics_ascentFID;
	jfieldID    X11FontMetrics_descentFID;
	jfieldID    X11FontMetrics_heightFID;
	jfieldID    X11FontMetrics_leadingFID;
	jfieldID    X11FontMetrics_maxAdvanceFID;
	jfieldID    X11FontMetrics_maxAscentFID;
	jfieldID    X11FontMetrics_maxDescentFID;
	jfieldID    X11FontMetrics_maxHeightFID;
	jfieldID    X11FontMetrics_widthsFID;

        jfieldID    java_awt_image_IndexColorModel_rgbFID;

        jfieldID    java_awt_image_DirectColorModel_red_maskFID;
        jfieldID    java_awt_image_DirectColorModel_red_offsetFID;
        jfieldID    java_awt_image_DirectColorModel_red_scaleFID;
        jfieldID    java_awt_image_DirectColorModel_green_maskFID;
        jfieldID    java_awt_image_DirectColorModel_green_offsetFID;
        jfieldID    java_awt_image_DirectColorModel_green_scaleFID;
        jfieldID    java_awt_image_DirectColorModel_blue_maskFID;
        jfieldID    java_awt_image_DirectColorModel_blue_offsetFID;
        jfieldID    java_awt_image_DirectColorModel_blue_scaleFID;
        jfieldID    java_awt_image_DirectColorModel_alpha_maskFID;
        jfieldID    java_awt_image_DirectColorModel_alpha_offsetFID;
        jfieldID    java_awt_image_DirectColorModel_alpha_scaleFID;


        jmethodID       DirectColorModel_constructor;
	jmethodID	ComponentXWindow_postMouseButtonEvent;
	jmethodID	ComponentXWindow_postMouseEvent;
	jmethodID	HeavyweightComponentXWindow_postPaintEventMID;
	jmethodID	Point_constructor;
	jmethodID	WindowXWindow_postKeyEventMID;
	jmethodID	WindowXWindow_postMovedEventMID;
	jmethodID	WindowXWindow_postResizedEventMID;
	jmethodID	WindowXWindow_postWindowEventMID;
	jmethodID	WindowXWindow_setBordersMID;
};

extern struct CachedIDs XCachedIDs;
extern Display *xawt_display;
extern Window xawt_root;
extern Visual *xawt_visual;
extern Colormap xawt_colormap;
extern jobject xawt_colormodel;
extern XContext xawt_context;
extern int xawt_screen;
extern int xawt_numScreens;
extern int xawt_depth;
extern Atom XA_WM_DELETE_WINDOW;

extern Pixel xawt_blackPixel;
extern Pixel xawt_whitePixel;


/* Gets the X window from a ComponentXWindow which is stored in the xwindow field. */

#define GET_X_WINDOW(obj) ((Window)(*env)->GetIntField(env, obj, XCachedIDs.ComponentXWindow_windowIDFID))

/* These macros can be used when initializing cached ids stored in the XCachedIDs structure.
   They should be called from static initializes of classes to get any IDs they use.
   In debug mode, they will check the ID exists and return if not (causing an exception to be thrown).
   In non-debug mode they are optimised for space and performance to assume the ID exists. This
   is because they are probably romized with the classes and once they work it is not necessary
   to check every time that they exist. */

#if CVM_DEBUG

#define JNIVERSION JNI_VERSION_1_2

#define FIND_CLASS(name) if ((cls = (*env)->FindClass(env, name)) == NULL) return;
#define GET_CLASS(id,name) if ((cls = XCachedIDs.id = (*env)->NewGlobalRef(env, (*env)->FindClass(env, name))) == NULL) return;
#define GET_FIELD_ID(id,name,type) if ((XCachedIDs.id = (*env)->GetFieldID(env, cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id,name,type) if ((XCachedIDs.id = (*env)->GetMethodID(env, cls, name, type)) == NULL) return;

#else

#define FIND_CLASS(name) cls = (*env)->FindClass(env, name);
#define GET_CLASS(id,name) cls = XCachedIDs.id = (*env)->NewGlobalRef(env, (*env)->FindClass(env, name));
#define GET_FIELD_ID(id,name,type) XCachedIDs.id = (*env)->GetFieldID(env, cls, name, type);
#define GET_METHOD_ID(id,name,type) XCachedIDs.id = (*env)->GetMethodID(env, cls, name, type);

#endif


#define TRUE 1
#define FALSE 0

extern Boolean xawt_addWindow (JNIEnv *env, Window window, jobject object);
extern void xawt_removeWindow (JNIEnv *env, Window window);
extern jobject xawt_getWindowObject (Window window);
extern Pixel xawt_getPixel (JNIEnv *env, jobject color);
extern long xawt_getXMouseEventMask (jlong mask);
extern void xawt_getJavaKeyCode(KeySym x11Key, jlong *keycode, Boolean * printable);
extern KeySym xawt_keyKeySym(jlong awtKey);

extern unsigned long (*colorCvtFunc)(int, int, int);
extern unsigned long (*colorDeCvtFunc)(int, int);

#endif
