/*
 * @(#)awt.h	1.13 06/10/25
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
 */

#ifndef _AWT_H_
#define _AWT_H_

#include <jni.h>

#include "qt.h"

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
	jfieldID    QtFontMetrics_ascentFID;
	jfieldID    QtFontMetrics_descentFID;
	jfieldID    QtFontMetrics_heightFID;
	jfieldID    QtFontMetrics_leadingFID;
	jfieldID    QtFontMetrics_maxAdvanceFID;
	jfieldID    QtFontMetrics_maxAscentFID;
	jfieldID    QtFontMetrics_maxDescentFID;
	jfieldID    QtFontMetrics_maxHeightFID;
	jfieldID    QtFontMetrics_widthsFID;

	jfieldID    QtImage_widthFID;
	jfieldID    QtImage_heightFID;
	jfieldID    QtImage_psdFID;

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
	jmethodID   java_lang_Thread_yieldMID;

    jmethodID   DirectColorModel_constructor;
	jmethodID   Point_constructor;
	jmethodID   IndexColorModel_constructor;
	jmethodID   java_awt_image_ColorModel_getRGBMID;

    jmethodID   Window_postMouseButtonEventMID;
    jmethodID   Window_postMouseEventMID;
    jmethodID   Window_postKeyEventMID;
};

extern struct CachedIDs QtCachedIDs;

struct qtimagedesc {
	QPaintDevice *qpd;
	int		count;
	int		width;
	int		height;
	QImage *loadBuffer;    // This is temporary until optimized model comes on board.
	const QBitmap *mask;
};

typedef struct qtimagedesc QtImageDesc;

struct qtgraphdesc {
	int			used;
	int			qid;	
	QPen		*qp;
	QBrush		*qb;
	QFont		*font;	
	int			clip[4];
	int			clipped;
	int 		extraalpha;
	Qt::RasterOp	rasterOp;
	int			currentalpha;   // QtColor doesn't support holding the alpha channel so we augment it.
	jint		blendmode;
};

typedef struct qtgraphdesc QtGraphDesc;

#define JNIVERSION JNI_VERSION_1_2

/* These macros can be used when initializing cached ids stored in the QtCachedIDs structure.
   They should be called from static initializes of classes to get any IDs they use.
   In debug mode, they will check the ID exists and return if not (causing an exception to be thrown).
   In non-debug mode they are optimised for space and performance to assume the ID exists. This
   is because they are probably romized with the classes and once they work it is not necessary
   to check every time that they exist. */
#ifdef __cplusplus

#if CVM_DEBUG

#define FIND_CLASS(name) if ((cls = env->FindClass(name)) == NULL) return;
#define GET_CLASS(id,name) if ((cls = QtCachedIDs.id = (jclass)(env->NewGlobalRef(env->FindClass(name)))) == NULL) return;
#define GET_FIELD_ID(id,name,type) if ((QtCachedIDs.id = env->GetFieldID(cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id,name,type) if ((QtCachedIDs.id = env->GetMethodID(cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id,name,type) if ((QtCachedIDs.id = env->GetStaticMethodID(cls, name, type)) == NULL) return;

#else

#define FIND_CLASS(name) cls = env->FindClass(name);
#define GET_CLASS(id,name) cls = QtCachedIDs.id = (jclass)(env->NewGlobalRef(env->FindClass(name)));
#define GET_FIELD_ID(id,name,type) QtCachedIDs.id = env->GetFieldID(cls, name, type);
#define GET_METHOD_ID(id,name,type) QtCachedIDs.id = env->GetMethodID(cls, name, type);
#define GET_STATIC_METHOD_ID(id,name,type) QtCachedIDs.id = env->GetStaticMethodID(cls, name, type);

#endif

#else

#if CVM_DEBUG

#define FIND_CLASS(name) if ((cls = (*env)->FindClass(env, name)) == NULL) return;
#define GET_CLASS(id,name) if ((cls = QtCachedIDs.id = (jclass)((*env)->NewGlobalRef(env, (*env)->FindClass(env, name)))) == NULL) return;
#define GET_FIELD_ID(id,name,type) if ((QtCachedIDs.id = (*env)->GetFieldID(env, cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id,name,type) if ((QtCachedIDs.id = (*env)->GetMethodID(env, cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id,name,type) if ((QtCachedIDs.id = (*env)->GetStaticMethodID(env, cls, name, type)) == NULL) return;

#else

#define FIND_CLASS(name) cls = (*env)->FindClass(env, name);
#define GET_CLASS(id,name) cls = QtCachedIDs.id = (jclass)((*env)->NewGlobalRef(env, (*env)->FindClass(env, name)));
#define GET_FIELD_ID(id,name,type) QtCachedIDs.id = (*env)->GetFieldID(env, cls, name, type);
#define GET_METHOD_ID(id,name,type) QtCachedIDs.id = (*env)->GetMethodID(env, cls, name, type);
#define GET_STATIC_METHOD_ID(id,name,type) QtCachedIDs.id = (*env)->GetStaticMethodID(env, cls, name, type);

#endif

#endif

#define TRUE 1
#define FALSE 0


#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x > y) ? x : y)


#define SCREEN_X_RESOLUTION	640	
#define SCREEN_Y_RESOLUTION	480	

//#define SCREEN_X_RESOLUTION	300	
//#define SCREEN_Y_RESOLUTION	220

// #define SCREEN_X_RESOLUTION	320	
// #define SCREEN_Y_RESOLUTION	240	

#define NUM_GRAPH_DESCRIPTORS	128
#define NUM_IMAGE_DESCRIPTORS	128

#endif




