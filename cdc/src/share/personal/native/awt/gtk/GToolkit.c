/*
 * @(#)GToolkit.c	1.35 06/10/10
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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "jni.h"
#include "sun_awt_gtk_GToolkit.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_PaintEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "KeyCodes.h"
#include "awt.h"
#include "GComponentPeer.h"


JavaVM *JVM;
struct CachedIDs GCachedIDs;

static void handleEvent (GdkEvent * event);

/* Called when System.exit is invoked. We need to shut down the main gtk event loop in a nice
   way by calling gtk_main_quit. */

JNIEXPORT void JNICALL
Java_sun_awt_gtk_ShutdownHook_gtkMainQuit (JNIEnv * env, jobject this)
{
	awt_gtk_threadsEnter ();
	gtk_main_quit ();
	gdk_flush ();
	awt_gtk_threadsLeave ();
}

/* Static initializer which caches JNI IDs and initialises the Gtk/Gdk libraries before use. */

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GToolkit_initIDs (JNIEnv * env, jclass cls)
{
	int argc = 0;

	/* Cache reference to Java VM. */

	if ((*env)->GetJavaVM (env, &JVM) != 0)
		return;

	cls = (*env)->FindClass (env, "java/awt/Insets");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_InsetsClass = (*env)->NewGlobalRef (env, cls);
	GET_METHOD_ID (java_awt_Insets_constructorMID, "<init>", "(IIII)V");

	cls = (*env)->FindClass (env, "java/awt/AWTError");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_AWTErrorClass = (*env)->NewGlobalRef (env, cls);

	cls = (*env)->FindClass (env, "java/awt/Color");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_ColorClass = (*env)->NewGlobalRef (env, cls);
	GET_METHOD_ID (java_awt_Color_constructorMID, "<init>", "(III)V");
	GET_FIELD_ID (java_awt_Color_valueFID, "value", "I");

	cls = (*env)->FindClass (env, "java/awt/Component");

	if (cls == NULL)
		return;

	GET_FIELD_ID (java_awt_Component_peerFID, "peer", "Lsun/awt/peer/ComponentPeer;");
	GET_FIELD_ID (java_awt_Component_backgroundFID, "background", "Ljava/awt/Color;");
	GET_FIELD_ID (java_awt_Component_foregroundFID, "foreground", "Ljava/awt/Color;");
	GET_FIELD_ID (java_awt_Component_xFID, "x", "I");
	GET_FIELD_ID (java_awt_Component_yFID, "y", "I");
	GET_FIELD_ID (java_awt_Component_widthFID, "width", "I");
	GET_FIELD_ID (java_awt_Component_heightFID, "height", "I");
	GET_FIELD_ID (java_awt_Component_eventMaskFID, "eventMask", "J");
	GET_FIELD_ID (java_awt_Component_mouseMotionListenerFID, "mouseMotionListener",
				  "Ljava/awt/event/MouseMotionListener;");
	GET_FIELD_ID (java_awt_Component_mouseListenerFID, "mouseListener", "Ljava/awt/event/MouseListener;");
	GET_FIELD_ID (java_awt_Component_fontFID, "font", "Ljava/awt/Font;");
	GET_METHOD_ID (java_awt_Component_getBackgroundMID, "getBackground", "()Ljava/awt/Color;");
	GET_METHOD_ID (java_awt_Component_hideMID, "hide", "()V");

	cls = (*env)->FindClass (env, "java/awt/MenuComponent");

	if (cls == NULL)
		return;

	GET_FIELD_ID (java_awt_MenuComponent_peerFID, "peer", "Lsun/awt/peer/MenuComponentPeer;");

	cls = (*env)->FindClass (env, "java/awt/Point");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_PointClass = (*env)->NewGlobalRef (env, cls);

	GET_METHOD_ID (java_awt_Point_constructorMID, "<init>", "(II)V");

	cls = (*env)->FindClass (env, "java/awt/image/DirectColorModel");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_image_DirectColorModelClass = (*env)->NewGlobalRef (env, cls);
	GET_METHOD_ID (java_awt_image_DirectColorModel_constructorMID, "<init>", "(IIII)V");
	GET_FIELD_ID (java_awt_image_DirectColorModel_red_maskFID, "red_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_red_offsetFID, "red_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_red_scaleFID, "red_scale", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_green_maskFID, "green_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_green_offsetFID, "green_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_green_scaleFID, "green_scale", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_blue_maskFID, "blue_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_blue_offsetFID, "blue_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_blue_scaleFID, "blue_scale", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_maskFID, "alpha_mask", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_offsetFID, "alpha_offset", "I");
	GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_scaleFID, "alpha_scale", "I");

	cls = (*env)->FindClass (env, "java/awt/image/IndexColorModel");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_image_IndexColorModelClass = (*env)->NewGlobalRef (env, cls);
	GET_METHOD_ID (java_awt_image_IndexColorModel_constructorMID, "<init>", "(II[B[B[B)V");
	GET_FIELD_ID (java_awt_image_IndexColorModel_rgbFID, "rgb", "[I");

	cls = (*env)->FindClass (env, "java/awt/image/ImageConsumer");

	if (cls == NULL)
		return;

	GET_METHOD_ID (java_awt_image_ImageConsumer_setColorModelMID, "setColorModel",
				   "(Ljava/awt/image/ColorModel;)V");
	GET_METHOD_ID (java_awt_image_ImageConsumer_setHintsMID, "setHints", "(I)V");
	GET_METHOD_ID (java_awt_image_ImageConsumer_setPixelsMID, "setPixels",
				   "(IIIILjava/awt/image/ColorModel;[BII)V");
	GET_METHOD_ID (java_awt_image_ImageConsumer_setPixels2MID, "setPixels",
				   "(IIIILjava/awt/image/ColorModel;[III)V");

	cls = (*env)->FindClass (env, "sun/awt/image/OffScreenImageSource");

	if (cls == NULL)
		return;

	GET_FIELD_ID (sun_awt_image_OffScreenImageSource_baseIRFID, "baseIR",
				  "Lsun/awt/image/ImageRepresentation;");
	GET_FIELD_ID (sun_awt_image_OffScreenImageSource_theConsumerFID, "theConsumer",
				  "Ljava/awt/image/ImageConsumer;");

	cls = (*env)->FindClass (env, "java/lang/NullPointerException");

	if (cls == NULL)
		return;

	GCachedIDs.NullPointerExceptionClass = (*env)->NewGlobalRef (env, cls);

	cls = (*env)->FindClass (env, "java/lang/OutOfMemoryError");

	if (cls == NULL)
		return;

	GCachedIDs.OutOfMemoryErrorClass = (*env)->NewGlobalRef (env, cls);

	cls = (*env)->FindClass (env, "java/lang/ArrayIndexOutOfBoundsException");

	if (cls == NULL)
		return;

	GCachedIDs.ArrayIndexOutOfBoundsExceptionClass = (*env)->NewGlobalRef (env, cls);

	cls = (*env)->FindClass (env, "java/lang/IllegalArgumentException");

	if (cls == NULL)
		return;

	GCachedIDs.IllegalArgumentExceptionClass = (*env)->NewGlobalRef (env, cls);

	cls = (*env)->FindClass (env, "java/awt/Dimension");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_DimensionClass = (*env)->NewGlobalRef (env, cls);
	GET_METHOD_ID (java_awt_Dimension_constructorMID, "<init>", "(II)V");

	cls = (*env)->FindClass (env, "java/awt/Rectangle");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_RectangleClass = (*env)->NewGlobalRef (env, cls);
	GET_FIELD_ID (java_awt_Rectangle_xFID, "x", "I");
	GET_FIELD_ID (java_awt_Rectangle_yFID, "y", "I");
	GET_FIELD_ID (java_awt_Rectangle_widthFID, "width", "I");
	GET_FIELD_ID (java_awt_Rectangle_heightFID, "height", "I");
	GET_METHOD_ID (java_awt_Rectangle_constructorMID, "<init>", "(IIII)V");

	cls = (*env)->FindClass (env, "java/awt/Font");

	if (cls == NULL)
		return;

	GET_FIELD_ID (java_awt_Font_peerFID, "peer", "Lsun/awt/peer/FontPeer;");

	cls = (*env)->FindClass (env, "sun/awt/gtk/GFontPeer");

	if (cls == NULL)
		return;

	GCachedIDs.GFontPeerClass = (*env)->NewGlobalRef (env, cls);
	/*
	GET_FIELD_ID (GFontPeer_ascentFID, "ascent", "I");
	GET_FIELD_ID (GFontPeer_descentFID, "descent", "I");
	GET_FIELD_ID (GFontPeer_dataFID, "data", "I");
	 */
	GET_STATIC_METHOD_ID (GFontPeer_getFontMID, "getFont", "(I)Ljava/awt/Font;");

	cls = (*env)->FindClass (env, "sun/awt/gtk/GPlatformFont");
	
	if (cls == NULL)
		return;
	
	GET_FIELD_ID (GPlatformFont_ascentFID, "ascent", "I");
	GET_FIELD_ID (GPlatformFont_descentFID, "descent", "I");
	
	cls = (*env)->FindClass (env, "java/awt/image/BufferedImage");

	if (cls == NULL)
		return;

	GCachedIDs.java_awt_image_BufferedImageClass = (*env)->NewGlobalRef (env, cls);
	GET_FIELD_ID (java_awt_image_BufferedImage_peerFID, "peer", "Lsun/awt/image/BufferedImagePeer;");
	GET_METHOD_ID (java_awt_image_BufferedImage_constructorMID, "<init>", "(Lsun/awt/image/BufferedImagePeer;)V");

	/* Make gdk thread safe. */

	g_thread_init (NULL);

	/* Initialize the Gtk+ library. */

	if (!gtk_init_check (&argc, NULL))
	{
		(*env)->ThrowNew (env, GCachedIDs.java_awt_AWTErrorClass, "Could not initialize Gtk+ library");
		return;
	}

	/* Make all warnings, errors to be fatal for debugging purposes. */

	g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);

	/* Override event handler so that we can post low level events (eg mouse and keyboard) to
	   the Java event queue for processing by the event dispatch thread. */

	gdk_event_handler_set ((GdkEventFunc) handleEvent, NULL, NULL);
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GToolkit_getScreenWidth (JNIEnv * env, jobject this)
{
	jint width;

	awt_gtk_threadsEnter ();
	width = gdk_screen_width ();
	awt_gtk_threadsLeave ();

	return width;
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GToolkit_getScreenHeight (JNIEnv * env, jobject this)
{
	jint height;

	awt_gtk_threadsEnter ();
	height = gdk_screen_height ();
	awt_gtk_threadsLeave ();

	return height;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GToolkit_beep (JNIEnv * env, jobject this)
{
	awt_gtk_threadsEnter ();
	gdk_beep ();
	awt_gtk_threadsLeave ();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GToolkit_sync (JNIEnv * env, jobject this)
{
	awt_gtk_threadsEnter ();
	XFlush(GDK_DISPLAY());
	awt_gtk_threadsLeave ();
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GToolkit_createBufferedImage (JNIEnv * env, jclass cls, jobject peer)
{
	return (*env)->NewObject (env,
							  GCachedIDs.java_awt_image_BufferedImageClass,
							  GCachedIDs.java_awt_image_BufferedImage_constructorMID,
							  peer);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GToolkit_getBufferedImagePeer (JNIEnv * env, jclass cls, jobject bufferedImage)
{
	return (*env)->GetObjectField (env, bufferedImage, GCachedIDs.java_awt_image_BufferedImage_peerFID);
}

/*
 * Class:     sun_awt_gtk_GToolkit
 * Method:    run
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gtk_GToolkit_run (JNIEnv * env, jobject this)
{
	/* Run the gtk event dispatching loop. */
	gdk_threads_enter ();
	gtk_main ();
	gdk_threads_leave ();
}

jobject
awt_gtk_getColorModel (JNIEnv * env)
{
	static jobject colorModel = NULL;
	GdkColormap *colormap;
	GdkVisual *visual;

	awt_gtk_threadsEnter ();

	if (colorModel == NULL)
	{
		colormap = gdk_colormap_get_system ();
		visual = gdk_colormap_get_visual (colormap);

		/* Create an IndexColorModel if we have an array of colors in the colormap. */

		if (colormap->colors != NULL)
		{
			jbyteArray redArray, greenArray, blueArray;
			int numColors = visual->colormap_size;
			int i;
			jbyte dummy;

			redArray = (*env)->NewByteArray (env, numColors);

			if (redArray == NULL)
				return NULL;

			greenArray = (*env)->NewByteArray (env, numColors);

			if (greenArray == NULL)
				return NULL;

			blueArray = (*env)->NewByteArray (env, numColors);

			if (blueArray == NULL)
				return NULL;

			for (i = 0; i < numColors; i++)
			{
				dummy = (jbyte) ((((gulong) colormap->colors[i].red) * 255) / 65535);
				(*env)->SetByteArrayRegion (env, redArray, i, 1, &dummy);

				dummy = (jbyte) ((((gulong) colormap->colors[i].green) * 255) / 65535);
				(*env)->SetByteArrayRegion (env, greenArray, i, 1, &dummy);

				dummy = (jbyte) ((((gulong) colormap->colors[i].blue) * 255) / 65535);
				(*env)->SetByteArrayRegion (env, blueArray, i, 1, &dummy);
			}

			colorModel = (*env)->NewObject (env, GCachedIDs.java_awt_image_IndexColorModelClass,
											GCachedIDs.java_awt_image_IndexColorModel_constructorMID,
											(jint) visual->depth,
											(jint) numColors, redArray, greenArray, blueArray);
		}

		else					/* Otherwise create a DirectColorModel. */
		{
			colorModel = (*env)->NewObject (env, GCachedIDs.java_awt_image_DirectColorModelClass,
											GCachedIDs.java_awt_image_DirectColorModel_constructorMID,
											(jint) visual->depth,
											(jint) visual->red_mask,
											(jint) visual->green_mask, (jint) visual->blue_mask);
		}

		colorModel = (*env)->NewGlobalRef (env, colorModel);
	}

	awt_gtk_threadsLeave ();

	return colorModel;
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GToolkit_getColorModel (JNIEnv * env, jobject this)
{
	return awt_gtk_getColorModel (env);
}

JNIEXPORT void JNICALL
Java_java_awt_image_ColorModel_deletepData (JNIEnv * env, jobject this)
{
}

/* Gets the java.awt.event.Input event modifier mask for the specified state from a GdkEvent. */

static jint
getModifiers (guint state)
{
	jint modifiers = 0;

	if (state & GDK_SHIFT_MASK)
		modifiers |= java_awt_event_InputEvent_SHIFT_MASK;

	if (state & GDK_CONTROL_MASK)
		modifiers |= java_awt_event_InputEvent_CTRL_MASK;

	if (state & GDK_MOD1_MASK)
		modifiers |= java_awt_event_InputEvent_ALT_MASK;

	if (state & GDK_MOD2_MASK)
		modifiers |= java_awt_event_InputEvent_META_MASK;

	/*if (state & GDK_MOD3_MASK)
	   modifiers |= java_awt_event_InputEvent_ALT_GRAPH_MASK;
	 */
	if (state & GDK_BUTTON1_MASK)
		modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;

	if (state & GDK_BUTTON2_MASK)
		modifiers |= java_awt_event_InputEvent_BUTTON2_MASK;

	if (state & GDK_BUTTON3_MASK)
		modifiers |= java_awt_event_InputEvent_BUTTON3_MASK;

	return modifiers;
}

/* Utility function to post mouse events to the AWT event dispatch thread. */

static void
postMouseEvent (JNIEnv * env,
				jobject this,
				jint id, jlong when, jint modifiers, jint x, jint y, jint clickCount, jboolean popupTrigger, void *event)
{
	awt_gtk_callbackEnter ();

	(*env)->CallVoidMethod (env, this, GCachedIDs.GComponentPeer_postMouseEventMID, id, when, modifiers, x, y,
							clickCount, popupTrigger, (jint)event);

	awt_gtk_callbackLeave ();

	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}


/* Gtk Callback function that posts mouse events to the target component when the mouse is moved. */

static void
postMouseMovedEvent (JNIEnv * env, GdkEventMotion * event, GComponentPeerData * data)
{
	jobject this = data->peerGlobalRef;
	jobject target;
	jlong eventMask;
	jobject mouseMotionListener;

	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);

	/* Make sure target is interested in these kinds of events before sending one. Only send
	   events if the eventMask specifies these kinds of events or a MouseMotionListener has been
	   registered with the component. */

	eventMask = (*env)->GetLongField (env, target, GCachedIDs.java_awt_Component_eventMaskFID);
	mouseMotionListener =
		(*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_mouseMotionListenerFID);

	/*if (((eventMask & java_awt_AWTEvent_MOUSE_MOTION_EVENT_MASK) != 0) || mouseMotionListener != NULL) */
	postMouseEvent (env, this,
					(event->
					 state & GDK_BUTTON1_MASK) ? java_awt_event_MouseEvent_MOUSE_DRAGGED :
					java_awt_event_MouseEvent_MOUSE_MOVED, event->time, getModifiers (event->state), event->x,
					event->y, 0, JNI_FALSE, event);
}

/* Gtk Callback function that posts mouse events to the target component when the mouse has enetered the component. */

static void
postMouseCrossingEvent (JNIEnv * env, GdkEventCrossing * event, GComponentPeerData * data)
{
	jobject this = data->peerGlobalRef;
	jobject target;
	jlong eventMask;
	jobject mouseListener;
	static GtkWidget *lastEntry = NULL;
	GtkWidget *widget = (GtkWidget *) ((GdkEventAny *) event)->window->user_data;

	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);

	/* Make sure target is interested in these kinds of events before sending one. Only send
	   events if the eventMask specifies these kinds of events or a MouseMotionListener has been
	   registered with the component. */

	eventMask = (*env)->GetLongField (env, target, GCachedIDs.java_awt_Component_eventMaskFID);
	mouseListener = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_mouseListenerFID);

	/* for bug #4684382 - to handle extra exit/enter generated by
	 * buttonrelease
	 * Matching exit to previous enter:  If exit is not matched by
	 * enter to same widget, throw away exit event.  If enter event
	 * is duplicate to last entered widget, throw away enter event.
	 */
	if (event->type == GDK_ENTER_NOTIFY) {
	  if (lastEntry == widget) {
	    /* throw away this event, since already in this widget */
	    return;
	  }
	  lastEntry = widget;
	} else {	/* GDK_EXIT_NOTIFY */
	  if (lastEntry != widget) {
	    /* throw away this exit; it has no matching enter */
	    return;
	  } else
	    lastEntry = NULL;	/* exit matched enter, clear lastEntry */
	}

	/*if (((eventMask & java_awt_AWTEvent_MOUSE_EVENT_MASK) != 0) || mouseListener != NULL) */
	postMouseEvent (env,
					this,
					(event->type == GDK_ENTER_NOTIFY) ?
					java_awt_event_MouseEvent_MOUSE_ENTERED :
					java_awt_event_MouseEvent_MOUSE_EXITED,
					event->time, getModifiers (event->state), event->x, event->y, 0, JNI_FALSE, event);
}

/* Gtk Callback function that posts mouse events to the target component when a mouse button
   has been released on the component. */

#define CLICK_PROXIMITY 5

static jobject  lastButtonPressPeer = NULL;

/* Should be called from a thread-safe call */
void GToolkit_destroyStaticLocals(jobject peerRef)
{
    if(peerRef == lastButtonPressPeer)
        lastButtonPressPeer = NULL;

}

static void
postMouseButtonEvent (JNIEnv * env, GdkEventButton * event, GComponentPeerData * data)
{
	jobject this = data->peerGlobalRef;
	jobject target;
	jlong eventMask;
	jobject mouseListener;
	jint modifiers = getModifiers (event->state);
	static jint lastX = -1;
	static jint lastY = -1;

	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);

	/* Make sure target is interested in these kinds of events before sending one. Only send
	   events if the eventMask specifies these kinds of events or a MouseMotionListener has been
	   registered with the component. */

	eventMask = (*env)->GetLongField (env, target, GCachedIDs.java_awt_Component_eventMaskFID);
	mouseListener = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_mouseListenerFID);

	/*if (((eventMask & java_awt_AWTEvent_MOUSE_EVENT_MASK) != 0) || mouseListener != NULL) */
	{
		jint id;
		jboolean popupTrigger;

		switch (event->button)
		{
			case 1:
				modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;
				break;

			case 2:
				modifiers |= java_awt_event_InputEvent_BUTTON2_MASK;
				break;

			case 3:
				modifiers |= java_awt_event_InputEvent_BUTTON3_MASK;
				break;

			default:
				break;
		}

		if (event->type == GDK_BUTTON_PRESS)
		{
			if (event->time - data->lastClickTime < 200)
				data->clickCount++;

			else
				data->clickCount = 1;

			data->lastClickTime = event->time;

			id = java_awt_event_MouseEvent_MOUSE_PRESSED;
			popupTrigger = (event->button == 3 && data->clickCount == 1) ? JNI_TRUE : JNI_FALSE;
			lastButtonPressPeer = this;
			lastX = (jint) event->x;
			lastY = (jint) event->y;
		}

		else
		{
		  jint x, y;
		  jobject new_this = this;
		  gboolean isSame = FALSE;

		  /* only post click event if the button was released
		   * in the same proximity as it was pressed.
		   * if the lastButtonPressPeer is different from the
		   * buttonRelease peer, then still send a buttonrelease
		   * to the lastButtonPressPeer, according to spec.
		   */
		  if (lastButtonPressPeer &&
		    (lastButtonPressPeer != this)) {
		    /* if peers don't match, still inform orig widget */
		    new_this = lastButtonPressPeer;
		  } else if (lastButtonPressPeer == this) {
		    /* same peer - but in case of lightweight widgets,
		     * check for proximity as well
		     */
		    isSame = TRUE;
		  }
		  postMouseEvent (env, new_this,
				  java_awt_event_MouseEvent_MOUSE_RELEASED,
				  event->time, modifiers, event->x, event->y,
				  data->clickCount, JNI_FALSE, event);
		  /* checking for proximity */
		  if (lastX >= 0) {
		    x = (gint) event->x;
		    y = (gint) event->y;
		    if (isSame &&
		        ((x >= (lastX - CLICK_PROXIMITY)) &&
		         (x <= (lastX + CLICK_PROXIMITY))) &&
		        ((y >= (lastY - CLICK_PROXIMITY)) &&
		         (y <= (lastY + CLICK_PROXIMITY)))) {
		      /* then send click */
		      id = java_awt_event_MouseEvent_MOUSE_CLICKED;
		      popupTrigger = JNI_FALSE;
		    } else return;

		  } else return;
		}

		postMouseEvent (env,
						this, id, event->time, modifiers, event->x, event->y, data->clickCount, popupTrigger, event);
	}
}

static void
postKeyEvent (JNIEnv * env, GdkEventKey * event, GComponentPeerData * data)
{
	jobject this = data->peerGlobalRef;
	jint javaCode = awt_gtk_getJavaKeyCode (event->keyval);
	jint modifiers = getModifiers (event->state);

	(*env)->CallVoidMethod (env, this, GCachedIDs.GComponentPeer_postKeyEventMID,
							(event->type ==
							 GDK_KEY_PRESS) ? java_awt_event_KeyEvent_KEY_PRESSED :
							java_awt_event_KeyEvent_KEY_RELEASED, (jlong) event->time, modifiers, javaCode,
							awt_gtk_getUnicodeChar (javaCode, modifiers), (jint)event);

	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);

	if (event->length > 0)
		(*env)->CallVoidMethod (env, this, GCachedIDs.GComponentPeer_postKeyEventMID,
								java_awt_event_KeyEvent_KEY_TYPED,
								(jlong) event->time,
								modifiers, java_awt_event_KeyEvent_VK_UNDEFINED, event->string[0]);
}

/* This is the event handler that processes events. Before passing events son to Gtk for processing we
   also pass low level events on to Java by posting them to Java event queue. */

static void
handleEvent (GdkEvent * event)
{
	GtkWidget *widget, *grabWidget;
	gboolean allowGtkToProcessEvent = TRUE;

	/* For some reason we sometimes get property events without the window being filled in. This then
	   causes Gtk to crash when we try to copy it. The solution is to ignore these events completely
	   and let Gtk do normal processing on them. */

	if (event->any.window == NULL)
	{
		gtk_main_do_event (event);
		return;
	}

	/*  Gtk has some strange modal behaviour. To get around this we make sure that mouse and
	    keyboard events that are not ancestors of the current grab widget are completely ignored.
	    If a menu currently has the grab focus then we allow it to process mouse and keyboard
	    events. If a modal dialog has the grab focus the we only allow mouse and keyboard events
	    to be sent to it if they are to the a component in the dialog or the dialog itself. */

	grabWidget = gtk_grab_get_current();

	if (grabWidget != NULL)
	{
		/* If the widget is a menu then allow Gtk to process the events as normal.
		   We don't allow Java to even hear about the mouse and keyboard events in this case.
		   This is what J2SE does so mimicked that behaviour here. */

		if (GTK_IS_MENU(grabWidget))
		{
			gtk_main_do_event(event);
			return;
		}

		/* The widget with the grab must be a modal dialog.
		   Only allow events to be processed if the event is for a widget that is a child of the modal
		   dialog. Otherwise we ignore the event completely. This fixes bugs in Gtk and also makes us
		   more consistent with J2SE's behaviour. */

		if (GTK_IS_WINDOW(grabWidget) &&
		   (event->type == GDK_KEY_PRESS ||
		 event->type == GDK_KEY_RELEASE ||
		 event->type == GDK_BUTTON_PRESS ||
		 event->type == GDK_BUTTON_RELEASE ||
		 event->type == GDK_2BUTTON_PRESS ||
		 event->type == GDK_3BUTTON_PRESS ||
		 event->type == GDK_MOTION_NOTIFY ||
		 event->type == GDK_ENTER_NOTIFY ||
		 event->type == GDK_LEAVE_NOTIFY))
		{
			widget = gtk_get_event_widget (event);

			if (widget != grabWidget && !gtk_widget_is_ancestor (widget, grabWidget))
			{
				/* Don't beep for mouse moves! */

				if (event->type != GDK_MOTION_NOTIFY &&
					event->type != GDK_ENTER_NOTIFY &&
					event->type != GDK_LEAVE_NOTIFY)
				{
					/* Beep to give the user feedback that their keyboard or mouse event has been ignored. */

					gdk_beep();
				}
				return;
			}
		}
	}

	event = gdk_event_copy (event);

	/* We intercept all low level events so that we can post them into the Java event queue. */

	awt_gtk_callbackEnter ();

	/* We set the high bit of the send_event field to indicate that the event has been processed by
	   Java and has been put back in the queue. Obviously, we don't want to send the event to Java
	   again as this would cause infinitie recursion. */


	/* Check if event has been processed by java yet. If it hasn't then we need to send it to Java
	   for processing and prevent Gtk from seeing it now. Once Java has processed it (and has not
	   consumed it) we can put it back in the queue for Gtk to process. */

	if ((event->any.send_event & (1 << 7)) == 0) /* Event not sent to Java yet. */
	{
		/* Determine the widget from the event. */

		widget = gtk_get_event_widget (event);

		if (widget != NULL)
		{
			/* Get the peer data for this widget. */

			GComponentPeerData *data = awt_gtk_getDataFromWidget (widget);

			if (data != NULL)
			{
				JNIEnv *env;

				if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) == 0)
				{
					switch (event->type)
					{
						case GDK_KEY_PRESS:
						case GDK_KEY_RELEASE:
						{
							/* Determine the widget that currently has focus in the window. If no widget
							   has focus then send the events to the window. This is necessary because,
							   for some reason, the key events come in at the widget currently under the
								mouse which is definately not what we want!. */

							GtkWindow *window = (GtkWindow *)gtk_widget_get_ancestor (widget, GTK_TYPE_WINDOW);

							if (window != NULL)
							{
									if (window->focus_widget != NULL)
									{
										data = awt_gtk_getDataFromWidget (window->focus_widget);
									}

									else if (((GtkWidget *)window) != widget)
										data = awt_gtk_getDataFromWidget ((GtkWidget *)window);
							}

							postKeyEvent (env, (GdkEventKey *) event, data);
							allowGtkToProcessEvent = FALSE;
						}
						break;

						case GDK_BUTTON_PRESS:
						case GDK_BUTTON_RELEASE:
							postMouseButtonEvent (env, (GdkEventButton *) event, data);
							allowGtkToProcessEvent = FALSE;
							break;

						case GDK_2BUTTON_PRESS:
						case GDK_3BUTTON_PRESS:

							/* Don't allow Gtk to process these events. We manage our own click
							   counting system as Java needs to be able to handle more than triple
							   clicks. After Java has finished processing the event we will create
							   these events ourselves to ensure native widgets can use them.
							   See GComponentPeer.postMouseEventToGtk. */

							allowGtkToProcessEvent = FALSE;
							break;

						case GDK_MOTION_NOTIFY:
							postMouseMovedEvent (env, (GdkEventMotion *) event, data);
							allowGtkToProcessEvent = FALSE;
							break;

						case GDK_ENTER_NOTIFY:
						case GDK_LEAVE_NOTIFY:

							/* Only post mouse enter / exit events if this event was for for
							   root widget for the java component. This prevents multiple
							   enter exit events for components that consist of more than one
							   widget. */

							if ((gtk_object_get_data (GTK_OBJECT (widget), "peerData") != NULL)
							    && (((GdkEventCrossing *)event)->mode == GDK_CROSSING_NORMAL))
							{
								postMouseCrossingEvent (env, (GdkEventCrossing *) event, data);
								allowGtkToProcessEvent = FALSE;
							}
							break;

						default:
							break;
					}
				}
			}
		}
	}

	else
	{
		/* The event has already been processed by Java. We clear the high bit of the send_event
		   field so that this is not modified when Gtk processes it (we used this to indicate the
		   event had been processed by Java and then put back in the queue). */

		event->any.send_event &= ~(1 << 7);
	}

	/* If we did not post this event to Java for processing first then we can pass this event on
	   as normal to Gtk. */

	if (allowGtkToProcessEvent)
	{
		gtk_main_do_event (event);
		gdk_event_free (event);
	}

	awt_gtk_callbackLeave ();
}
