/*
 * @(#)X11GraphicsEnv.c	1.10 06/10/10
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

#include "awt.h"

#include "java_awt_X11GraphicsEnvironment.h"
#include "java_awt_X11GraphicsDevice.h"
#include "java_awt_X11GraphicsConfig.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


/* Global variables. */

struct CachedIDs XCachedIDs;		/* All the cached JNI IDs. */
Display *xawt_display = NULL;		/* The opened X Display. */
Window xawt_root;					/* The root window on the display. */
Visual *xawt_visual = NULL;			/* The default visual used by display. */
Colormap xawt_colormap;				/* The color map used by the display. */
jobject xawt_colormodel;
XContext xawt_context;                          /* Context used to associate object data with windows */
int xawt_screen = 0;				/* The screen number used. */
int xawt_numScreens = 0;                        /* The total number of screens available on this display */
int xawt_depth = 0;					/* The depth of the display. */
Pixel xawt_blackPixel;
Pixel xawt_whitePixel;
Atom XA_WM_DELETE_WINDOW;			/* Atom used to communicate that a window should be closed
									   from the window manager. */

unsigned long (*colorCvtFunc)(int, int, int);    /* Color convererter - translate javaRGB -> nativeRGB */
unsigned long (*colorDeCvtFunc)(int, int);    /* Color DEconvererter - translate javaRGB <- nativeRGB */


#ifdef CVM_DEBUG
JavaVM *jvm;
#endif


JNIEXPORT jintArray JNICALL
Java_java_awt_X11GraphicsDevice_getDepths(JNIEnv *env, jobject this, jint screen)
{
  int numDepths;
  jintArray depthArray;
  int *depths = XListDepths(xawt_display, screen, &numDepths);

  if(numDepths>0) {
    depthArray = (*env)->NewIntArray(env, numDepths);

    (*env)->SetIntArrayRegion(env, depthArray, 0, numDepths, (jint *)depths);

    XFree(depths);
  }
    
  return depthArray;
}


JNIEXPORT jintArray JNICALL
Java_java_awt_X11GraphicsDevice_getVisualIDs(JNIEnv *env, jobject this, jint screen, jint depth)
{
  XVisualInfo xVisTemplate;
  XVisualInfo *xVis;
  int xVisNum, i;

  jintArray visualIDs = NULL;

  memset(&xVisTemplate, '\0', sizeof(XVisualInfo));

  xVisTemplate.screen = (int) screen;
  xVisTemplate.depth = (unsigned int) depth;

  xVis = XGetVisualInfo(xawt_display, VisualScreenMask|VisualDepthMask, &xVisTemplate, &xVisNum);

  if(xVis!=NULL) {
    jint *visuals = calloc(xVisNum, sizeof(jint));

    for(i=0;i<xVisNum;i++)
      visuals[i] = (jint) xVis[i].visualid;

    visualIDs = (*env)->NewIntArray(env, xVisNum);

    (*env)->SetIntArrayRegion(env, visualIDs, 0, xVisNum, visuals);

    free(visuals);

    XFree(xVis);
  }

  return visualIDs;
}



JNIEXPORT jstring JNICALL
Java_java_awt_X11GraphicsDevice_getIDstring(JNIEnv *env, jobject this)
{
  return (*env)->NewStringUTF(env, XDisplayString(xawt_display));

}



JNIEXPORT jint JNICALL
Java_java_awt_X11GraphicsEnvironment_getDefaultScreenNum(JNIEnv *env, jobject this)
{
    return (jint) DefaultScreen(xawt_display);
}



/* ----- Convert 888 -> X11 color ----------- */

static unsigned long DecomposedColor888Cvt(int r, int g, int b) {

  unsigned long c = (b<<16)|(g<<8)|r;

 return c;
}


static unsigned long DecomposedColor565Cvt(int r, int g, int b) {

  unsigned long c = ((r & 0xf8) << 8)|((g & 0xfc) << 3)|(b >> 3);

 return c;
}

static unsigned long DecomposedColor555Cvt(int r, int g, int b) {

  unsigned long c = ((r & 0xf8) << 8)|((g & 0xf8) << 3)|(b >> 3);

 return c;
}


/* --------- DeConvert 888 <- X Color --------------*/

static unsigned long DecomposedColor888DeCvt(int a, int xcolor) {

  unsigned long c = (a<<24)|xcolor;

 return c;
}


static unsigned long DecomposedColor565DeCvt(int a, int xcolor) {

  unsigned long c = (a<<24)|((xcolor & 0xf800) << 16)|((xcolor & 0x07e0) << 8)|(xcolor & 0x001f);

 return c;
}

static unsigned long DecomposedColor555DeCvt(int a, int xcolor) {

  unsigned long c = (a<<24)|((xcolor & 0xf800) << 16)|((xcolor & 0x07c0) << 8)|(xcolor & 0x003e);

 return c;
}



static jobject directColorModel(JNIEnv *env, int depth, int red_mask, int green_mask, int blue_mask) {

  return (*env)->NewObject(env, XCachedIDs.DirectColorModel, XCachedIDs.DirectColorModel_constructor,
			   depth,
			   red_mask,
			   green_mask,
			   blue_mask,
			   0 );

}

/* ---------- figure out which color (de)converter to use -------------*/
/* Note: should at some time go into the configuration/make color model stuff. */

static void setupColorCvt(JNIEnv *env, VisualID visID) {

    XVisualInfo visTemplate;
    XVisualInfo *visInfo;
    int nitems;

    visTemplate.visualid = visID;

    if((visInfo = XGetVisualInfo(xawt_display, VisualIDMask, &visTemplate, &nitems)) == NULL)
      return;

    if(visInfo->class == DirectColor ||
       visInfo->class == TrueColor) {

      if (visInfo->bits_per_rgb == 6) {
	  colorCvtFunc = DecomposedColor565Cvt;
	  colorDeCvtFunc = DecomposedColor565DeCvt;
	  xawt_colormodel = directColorModel(env, visInfo->depth, visInfo->red_mask, visInfo->green_mask, visInfo->blue_mask);
      } else if (visInfo->bits_per_rgb == 5) {
	  colorCvtFunc = DecomposedColor555Cvt;
	  colorDeCvtFunc = DecomposedColor555DeCvt;
	  xawt_colormodel = directColorModel(env, visInfo->depth, visInfo->red_mask, visInfo->green_mask, visInfo->blue_mask);
      } else if (visInfo->bits_per_rgb == 8) {
	  colorCvtFunc = DecomposedColor888Cvt;
	  colorDeCvtFunc = DecomposedColor888DeCvt;
	  xawt_colormodel = directColorModel(env, visInfo->depth, visInfo->red_mask, visInfo->green_mask, visInfo->blue_mask);
      }

    }
    
    else abort();

    XFree(visInfo);

}

JNIEXPORT jobject JNICALL
Java_java_awt_X11GraphicsConfig_getNativeColorModel (JNIEnv *env, jobject this, jint visualID)
{

  /*  if(xawt_colormodel == NULL) */
    setupColorCvt(env, (VisualID)visualID);

  return xawt_colormodel;

}

/* If debug version then provide a debug error handler. */

#ifdef CVM_DEBUG
static int
errorHandler (Display *display, XErrorEvent *error)
{
	char message[100];
	JNIEnv *env;
	
	XGetErrorText (display, error->error_code, message, sizeof(message));
	
	fprintf (stderr, "XError: %s\n", message);
	
	/* Display java stack trace. */
	
	if ((*jvm)->AttachCurrentThread(jvm, (void **)&env, NULL) == 0)
	{
		(*env)->ThrowNew (env, XCachedIDs.AWTError, message);
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	
	return 0;
}

#else

static int
errorHandler (Display *display, XErrorEvent *error)
{
	return 0;
}

#endif

JNIEXPORT void JNICALL
Java_java_awt_X11GraphicsEnvironment_initDisplay(JNIEnv *env, jclass cls)
{
	GET_CLASS(AWTError, "java/awt/AWTError");

	if ((*env)->GetVersion(env) == 0x10001)
	{
		(*env)->ThrowNew (env, XCachedIDs.AWTError, "Requires at least version 1.2 JNI");
		return;
	}

	/* Need JavaVM for debug error handler. */

#ifdef CVM_DEBUG
	(*env)->GetJavaVM(env, &jvm);
#endif

  /* Cache commonly used JNI IDs. */

	FIND_CLASS("java/awt/Component");
	GET_FIELD_ID(Component_xFID, "x", "I");
	GET_FIELD_ID(Component_yFID, "y", "I");
	GET_FIELD_ID(Component_widthFID, "width", "I");
	GET_FIELD_ID(Component_heightFID, "height", "I");
	
	GET_CLASS(WindowXWindow, "java/awt/WindowXWindow");
	GET_CLASS(OutOfMemoryError, "java/lang/OutOfMemoryError");
	GET_CLASS(NullPointerException, "java/lang/NullPointerException");
	
	FIND_CLASS("java/awt/ComponentXWindow");
	GET_FIELD_ID(ComponentXWindow_componentFID, "component", "Ljava/awt/Component;");
	GET_FIELD_ID(ComponentXWindow_windowIDFID, "windowID", "I");
	GET_METHOD_ID(ComponentXWindow_postMouseButtonEvent, "postMouseButtonEvent", "(IJIII)V");
	GET_METHOD_ID(ComponentXWindow_postMouseEvent, "postMouseEvent", "(IJIII)V");
	
	FIND_CLASS("java/awt/HeavyweightComponentXWindow");
	GET_METHOD_ID(HeavyweightComponentXWindow_postPaintEventMID, "postPaintEvent", "(IIII)V");
	
	FIND_CLASS("java/awt/WindowXWindow");
	GET_FIELD_ID(WindowXWindow_outerWindowFID, "outerWindow", "I");
	GET_METHOD_ID(WindowXWindow_postKeyEventMID, "postKeyEvent", "(IJIIC)V");
	GET_METHOD_ID(WindowXWindow_postMovedEventMID, "postMovedEvent", "()V");
	GET_METHOD_ID(WindowXWindow_postResizedEventMID, "postResizedEvent", "()V");
	GET_METHOD_ID(WindowXWindow_postWindowEventMID, "postWindowEvent", "(I)V");
	GET_METHOD_ID(WindowXWindow_setBordersMID, "setBorders", "(IIII)V");
  
	GET_CLASS(Point, "java/awt/Point");
	GET_METHOD_ID(Point_constructor, "<init>", "(II)V");
	
	FIND_CLASS("java/awt/Color");
	GET_FIELD_ID(Color_valueFID, "value", "I");
	
	GET_CLASS(DirectColorModel, "java/awt/image/DirectColorModel");
	GET_METHOD_ID(DirectColorModel_constructor,"<init>","(IIIII)V");

	if (XInitThreads() == 0)
	{
		(*env)->ThrowNew (env, XCachedIDs.AWTError, "Could not initialze xlib for multi-threading");
		return;
	}

	xawt_display = XOpenDisplay(NULL);
	
	if (xawt_display == NULL)
	{
		char message[100];
		strcpy (message, "Could not open display: ");
		strcat (message, XDisplayName(NULL));
		(*env)->ThrowNew (env, XCachedIDs.AWTError, message);
		return;
	}
	
	XA_WM_DELETE_WINDOW = XInternAtom (xawt_display, "WM_DELETE_WINDOW", False);
	
	XSetErrorHandler(errorHandler);

#ifdef CVM_DEBUG

	/* For debugging puposes we synchronize our requests with the server so that any problems are
	   found when they occur and are not buffered. */

	XSynchronize(xawt_display, True);
	
#endif
	
	xawt_screen = DefaultScreen (xawt_display);
	xawt_numScreens = XScreenCount(xawt_display);
	xawt_root = RootWindow (xawt_display, xawt_screen);
	xawt_visual = DefaultVisual (xawt_display, xawt_screen);
	xawt_depth = DefaultDepth (xawt_display, xawt_screen);
	xawt_colormap = XDefaultColormap (xawt_display, xawt_screen);
	xawt_blackPixel = BlackPixel (xawt_display, xawt_screen);
	xawt_whitePixel = WhitePixel (xawt_display, xawt_screen);
	xawt_context = XUniqueContext();
}



JNIEXPORT jint JNICALL
Java_java_awt_X11GraphicsEnvironment_getNumScreens(JNIEnv *env, jobject this)
{
    return (jint) xawt_numScreens;
}


JNIEXPORT jint JNICALL
Java_java_awt_X11GraphicsDevice_getDefaultVisual(JNIEnv *env, jobject this)
{

  return (jint) xawt_visual->visualid;

}


JNIEXPORT jint JNICALL
Java_java_awt_X11GraphicsDevice_getDisplay(JNIEnv *env, jobject this)
{
    return (jint) xawt_display;
}


JNIEXPORT jint JNICALL
Java_java_awt_X11GraphicsConfig_getXResolution(JNIEnv *env, jobject this, jint screen)
{
    return (jint)DisplayWidth(xawt_display, screen);
}


JNIEXPORT jint JNICALL
Java_java_awt_X11GraphicsConfig_getYResolution(JNIEnv *env, jobject this, jint screen)
{
    return (jint)DisplayHeight(xawt_display, screen);
}


JNIEXPORT void JNICALL
Java_java_awt_X11GraphicsConfig_init(JNIEnv *env, jobject this, jint visualNum, jint screen)
{


  /* somewhere like here I have to set the color convertor correctly, match the visualNum, and the depth, etc. */

}
    

    

