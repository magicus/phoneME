/*
 * @(#)awt.h	1.19 06/10/10
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
#include "GraphicsEngine.h"
#include "HardwareInterface.h"

struct CachedIDs
{
	jclass	    OutOfMemoryError;
        jclass	    NullPointerException;
	jclass	    AWTError;
        jclass      DirectColorModel;
        jclass      IndexColorModel;
	jclass	    Point;

	jclass      java_awt_image_BufferedImage;

	jfieldID    Color_pixelFID;
	jfieldID    Color_valueFID;
	jfieldID    MWFontMetrics_ascentFID;
	jfieldID    MWFontMetrics_descentFID;
	jfieldID    MWFontMetrics_heightFID;
	jfieldID    MWFontMetrics_leadingFID;
	jfieldID    MWFontMetrics_maxAdvanceFID;
	jfieldID    MWFontMetrics_maxAscentFID;
	jfieldID    MWFontMetrics_maxDescentFID;
	jfieldID    MWFontMetrics_maxHeightFID;
	jfieldID    MWFontMetrics_widthsFID;

	jfieldID    MWImage_widthFID;
	jfieldID    MWImage_heightFID;

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

	jfieldID    java_awt_image_BufferedImage_peerFID;

	jmethodID   java_awt_image_BufferedImage_constructor;
	jmethodID   java_lang_Thread_interruptedMID;

        jmethodID   DirectColorModel_constructor;
	jmethodID   Point_constructor;
	jmethodID	IndexColorModel_constructor;
	jmethodID 	java_awt_image_ColorModel_getRGBMID;

        jmethodID   Window_postMouseButtonEventMID;
        jmethodID   Window_postMouseEventMID;
        jmethodID	Window_postKeyEventMID;
};

extern struct CachedIDs MWCachedIDs;
extern HardwareInterface *mwawt_hardwareInterface;


#define JNIVERSION JNI_VERSION_1_2

/* These macros can be used when initializing cached ids stored in the MWCachedIDs structure.
   They should be called from static initializes of classes to get any IDs they use.
   In debug mode, they will check the ID exists and return if not (causing an exception to be thrown).
   In non-debug mode they are optimised for space and performance to assume the ID exists. This
   is because they are probably romized with the classes and once they work it is not necessary
   to check every time that they exist. */

#if CVM_DEBUG

#define FIND_CLASS(name) if ((cls = (*env)->FindClass(env, name)) == NULL) return;
#define GET_CLASS(id,name) if ((cls = MWCachedIDs.id = (*env)->NewGlobalRef(env, (*env)->FindClass(env, name))) == NULL) return;
#define GET_FIELD_ID(id,name,type) if ((MWCachedIDs.id = (*env)->GetFieldID(env, cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id,name,type) if ((MWCachedIDs.id = (*env)->GetMethodID(env, cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id,name,type) if ((MWCachedIDs.id = (*env)->GetStaticMethodID(env, cls, name, type)) == NULL) return;

#else

#define FIND_CLASS(name) cls = (*env)->FindClass(env, name);
#define GET_CLASS(id,name) cls = MWCachedIDs.id = (*env)->NewGlobalRef(env, (*env)->FindClass(env, name));
#define GET_FIELD_ID(id,name,type) MWCachedIDs.id = (*env)->GetFieldID(env, cls, name, type);
#define GET_METHOD_ID(id,name,type) MWCachedIDs.id = (*env)->GetMethodID(env, cls, name, type);
#define GET_STATIC_METHOD_ID(id,name,type) MWCachedIDs.id = (*env)->GetStaticMethodID(env, cls, name, type);

#endif


#define TRUE 1
#define FALSE 0


#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x > y) ? x : y)

#endif




