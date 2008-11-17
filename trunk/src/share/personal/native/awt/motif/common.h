/*
 * @(#)common.h	1.26 06/10/10
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
#ifndef _COMMON_H_INCLUDED
#define _COMMON_H_INCLUDED
#include <jni.h>
#include <assert.h>

#include "Xheaders.h"

#define CHECK_EXCEPTION(msg)                                       \
do {                                                               \
    if ((*env)->ExceptionOccurred(env)) {                          \
	fprintf(stderr, "Pending exception, spot %s\n", #msg);     \
	(*env)->ExceptionDescribe(env);                            \
	fflush(stderr);                                            \
    }                                                              \
} while (0)


extern int nlc;
extern jobject native_lock;
extern int check_locking_thread(jobject);


#ifdef NDEBUG
#define threadHoldsLock(arg) 1
#else
#define threadHoldsLock(arg) check_locking_thread(arg)
#endif

#define NATIVE_LOCK_HELD() threadHoldsLock(native_lock)
#define NATIVE_LOCK(env)   (*env)->MonitorEnter(env, native_lock), ++nlc
#define NATIVE_UNLOCK(env)                  \
do {                                        \
    --nlc;                                  \
    assert(nlc >= 0);                       \
    (*env)->MonitorExit(env, native_lock);  \
} while (0)

#define NATIVE_WAIT(env, tm)                                            \
do {                                                                    \
    int old_nlc;						        \
    assert(NATIVE_LOCK_HELD());                                         \
    old_nlc = nlc;                                                      \
    nlc = 0;                                                            \
    (*env)->CallVoidMethod(env, native_lock, lock_waitMID, (jlong)tm);  \
    nlc = old_nlc;                                                      \
} while (0)

#define NATIVE_NOTIFY(env)                                     \
do {                                                           \
    assert(NATIVE_LOCK_HELD());                                \
    (*env)->CallVoidMethod(env, native_lock, lock_notifyMID);  \
} while (0)

#define NATIVE_NOTIFY_ALL(env)                                    \
do {                                                              \
    assert(NATIVE_LOCK_HELD());                                   \
    (*env)->CallVoidMethod(env, native_lock, lock_notifyAllMID);  \
} while (0)

/* constants */
extern int sun_awt_gfX_ScreenImpl_OP_COPY;
extern int sun_awt_gfX_ScreenImpl_OP_XOR;
extern int sun_awt_gfX_ScreenImpl_OP_CLEAR;
extern int sun_awt_gfX_FontImpl_PLAIN;
extern int sun_awt_gfX_FontImpl_BOLD;
extern int sun_awt_gfX_FontImpl_ITALIC;


extern Display  *awt_display;
extern Window   awt_root;
extern Visual   *awt_visual;
extern Colormap awt_cmap;
extern int      awt_depth;



/*
extern Display *display;
extern Window   root_drawable;
extern GC       root_gc;
extern int      screen_depth;
extern Visual  *root_visual;
extern Colormap root_colormap;
*/


typedef void (*pixelToRGBConverter)(unsigned int, int *, int *, int *);
typedef unsigned int (*rgbToPixelConverter)(int, int, int);
typedef unsigned int (*javaRGBToPixelConverter)(unsigned int, int);

extern pixelToRGBConverter     pixel2RGB;
extern rgbToPixelConverter     RGB2Pixel;
extern javaRGBToPixelConverter jRGB2Pixel;
extern unsigned long mapped_pixels[];
extern int numColors, grayscale;

/* extern XFontStruct *loadFont(Display * display, char *name, int pointSize); */
extern void SetupGC(JNIEnv*, GC, jobject, jobject, jobject, Region *); 
extern XPoint *MakePolygonPoints(jint, jint, jint*, jint*, jint*, int);
extern void DrawRoundRect(Drawable, GC, int, int, int, int, int, int);
extern void FillRoundRect(Drawable, GC, int, int, int, int, int, int);
extern void Translate(JNIEnv*, jobject, jint*, jint*);
extern unsigned long gfXGetPixel(JNIEnv *env, jobject fg, 
				 jobject xor, int gamma_correct);
extern int setupImageDecode();
extern jboolean isIndexColorModel(JNIEnv *env, jobject obj);
extern jboolean isDirectColorModel(JNIEnv *env, jobject obj);
extern jint *getICMParams(JNIEnv *env, jobject obj, int *mapsize);
extern void releaseICMParams(JNIEnv *env, jobject obj, int *rgb);
extern void getGCMParams(JNIEnv *env, jobject obj,
			 int *, int *, int *, int *, int *, int *,
			 int *, int *, int *, int *, int *, int *);
extern jobject getColorModel(JNIEnv *env);


typedef struct awtFontList {
    char *xlfd;
    int index_length;
    int load;
    char *charset_name;
    XFontStruct *xfont;
} awtFontList;

struct FontData {
    int charset_num;
    awtFontList *flist;
    XFontSet xfs; 	/* for TextField & TextArea */
    XFontStruct *xfont;	/* Latin1 font */
};


struct GSCachedIDs {
    jfieldID  screenWidthFID;
    jfieldID  screenHeightFID;
    jfieldID  defaultScreenFID;
    jfieldID  colorModelFID;
    jfieldID  inputHandlerFID;
    jfieldID  cursorXFID;
    jfieldID  cursorYFID;
    jfieldID  cursorHiddenFID;

    jmethodID handleKeyInputMID;
    jmethodID handleMouseInputMID;
    jmethodID handleExposeEventMID;

    jint      NONE;
    jint      PRESSED;
    jint      RELEASED;
    jint      ACTION;
    jint      TYPED;

    jclass    java_awt_Dimension;
    jfieldID  java_awt_Dimension_widthFID;
    jfieldID  java_awt_Dimension_heightFID;

    jclass    java_awt_Point;
    jfieldID  java_awt_Point_xFID;
    jfieldID  java_awt_Point_yFID;

    jclass    java_awt_Rectangle;
    jfieldID  java_awt_Rectangle_xFID;
    jfieldID  java_awt_Rectangle_yFID;
    jfieldID  java_awt_Rectangle_widthFID;
    jfieldID  java_awt_Rectangle_heightFID;





};


struct IRCachedIDs
{
    jclass       ImageRepresentation;

    jmethodID    reconstructMID;
    jfieldID     pDataFID;
    jfieldID     srcWFID;
    jfieldID     srcHFID;
    jfieldID     widthFID;
    jfieldID     heightFID;
    jfieldID     hintsFID;
    jfieldID     availinfoFID;
    jfieldID     offscreenFID;
    jfieldID     newbitsFID;

    jclass       ImageObserver;
    jint     WIDTHFID;
    jint     HEIGHTFID;
    jint     SOMEBITSFID;
    jint     FRAMEBITSFID;
    jint     ALLBITSFID;

    jclass       ImageConsumer;
    jmethodID    setColorModelMID;
    jmethodID    setHintsMID;
    jmethodID    setIntPixelsMID;
    jmethodID    setBytePixelsMID;

    jint     TOPDOWNLEFTRIGHTFID;
    jint     COMPLETESCANLINESFID;
    jint     SINGLEPASSFID;

    jclass       java_awt_Rectangle;
    jfieldID     java_awt_Rectangle_xFID;
    jfieldID     java_awt_Rectangle_yFID;
    jfieldID     java_awt_Rectangle_widthFID;
    jfieldID     java_awt_Rectangle_heightFID;


    jclass       OffscreenImageSource;
    jfieldID     baseIRFID;
    jfieldID     theConsumerFID;


    jclass       DirectColorModel;
    jmethodID    DirectColorModel_constructor;

    jclass       IndexColorModel;
    jmethodID    IndexColorModel_constructor;


};


extern struct FontData *awtJNI_GetFontData(JNIEnv *, jobject, char **);
extern struct FontData *GetFontData(JNIEnv * env, jobject font, const char **errmsg);

#endif

