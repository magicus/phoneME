/*
 * @(#)MWGraphicsEnv.c	1.29 06/10/10
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
/*#include "cursors.h"*/

#include "java_awt_MWGraphicsEnvironment.h"
#include "java_awt_MWGraphicsDevice.h"

/* #include "java_awt_MWGraphicsConfig.h" */

#include "java_awt_AWTEvent.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_KeyEvent.h"

#include "X11Driver.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

HardwareInterface *mwawt_hardwareInterface;

#ifdef CVM_DEBUG
int escape_quits = 1;
#else
int escape_quits = 0;
#endif

/* local functions */

/*TODO
static void processMouseEvent (JNIEnv * env, jobject window);
static void processKeyEvent (JNIEnv * env, jobject window);*/

struct CachedIDs MWCachedIDs;	/* All the cached JNI IDs. */

/* Defines the current set of modifiers (keyboard and mouse) for events. */

static jint eventModifiers = 0;

/* Current position of the mouse. */

static jint mouseX, mouseY;

#ifdef CVM_DEBUG
JavaVM *jvm;
#endif


JNIEXPORT jstring JNICALL
Java_java_awt_MWGraphicsDevice_getIDstring (JNIEnv * env, jobject this)
{
	return (*env)->NewStringUTF (env, "Micro Windows");
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphicsEnvironment_init (JNIEnv * env, jclass cls)
{
	GET_CLASS (AWTError, "java/awt/AWTError");

	if ((*env)->GetVersion (env) == 0x10001)
	{
		(*env)->ThrowNew (env, MWCachedIDs.AWTError, "Requires at least version 1.2 JNI");
		return;
	}

	/* Need JavaVM for debug error handler. */

#ifdef CVM_DEBUG
	(*env)->GetJavaVM (env, &jvm);
#endif

	/* Cache commonly used JNI IDs. */

	GET_CLASS (OutOfMemoryError, "java/lang/OutOfMemoryError");
	GET_CLASS (NullPointerException, "java/lang/NullPointerException");

	FIND_CLASS ("java/lang/Thread");
	GET_STATIC_METHOD_ID (java_lang_Thread_interruptedMID, "interrupted", "()Z");

	GET_CLASS (Point, "java/awt/Point");
	GET_METHOD_ID (Point_constructor, "<init>", "(II)V");

	FIND_CLASS ("java/awt/Color");
	GET_FIELD_ID (Color_valueFID, "value", "I");

	GET_CLASS (DirectColorModel, "java/awt/image/DirectColorModel");
	GET_METHOD_ID (DirectColorModel_constructor, "<init>", "(IIII)V");

	GET_CLASS (IndexColorModel, "java/awt/image/IndexColorModel");
	GET_METHOD_ID (IndexColorModel_constructor, "<init>", "(II[B[B[B)V");

	GET_CLASS (java_awt_image_BufferedImage, "java/awt/image/BufferedImage");
	GET_METHOD_ID (java_awt_image_BufferedImage_constructor, "<init>", "(Lsun/awt/image/BufferedImagePeer;)V");
	GET_FIELD_ID (java_awt_image_BufferedImage_peerFID, "peer", "Lsun/awt/image/BufferedImagePeer;");

	FIND_CLASS ("java/awt/Window");
	GET_METHOD_ID (Window_postMouseButtonEventMID, "postMouseButtonEvent", "(IIIII)V");
	GET_METHOD_ID (Window_postMouseEventMID, "postMouseEvent", "(IIII)V");
	GET_METHOD_ID (Window_postKeyEventMID, "postKeyEvent", "(IIIC)V");

	mwawt_hardwareInterface = &x11HardwareInterface;

	if (mwawt_hardwareInterface->OpenScreen() == NULL)
	{
		(*env)->ThrowNew (env, MWCachedIDs.AWTError, "Could not open screen");
		return;
	}
}


JNIEXPORT void JNICALL
Java_java_awt_MWGraphicsEnvironment_destroy(JNIEnv *env, jobject this)
{
  mwawt_hardwareInterface->Destroy(mwawt_hardwareInterface);
}

static Driver * eventDriver = NULL;
static jobject eventWindow = NULL;

/* Called when a window has been attatched to a GraphicsDevice. This window will then receive events
   sent to that device. As Micro Windows only supports one screen device at the moment we store
   these in global variables. In a multi screen environment these would be more appropriately stored
   in a map of some kind. */

JNIEXPORT void JNICALL
Java_java_awt_MWGraphicsEnvironment_pSetWindow (JNIEnv * env, jobject this, jint psd, jobject window)
{
	eventDriver = (Driver *) psd;
	eventWindow = (*env)->NewGlobalRef (env, window);
}

static void
processMouseButtonEvent (JNIEnv *env, jobject window, MWMouseButtonEvent *event)
{
	jint id;

	/* Update the global eventModifiers. */

	if (event->type == MWEVENT_MOUSE_BUTTON_PRESSED)
	{
		if (event->button == MWMOUSE_BUTTON_LEFT)
			eventModifiers |= java_awt_event_InputEvent_BUTTON1_MASK;

		else if (event->button == MWMOUSE_BUTTON_MIDDLE)
			eventModifiers |= java_awt_event_InputEvent_BUTTON2_MASK;

		else if (event->button == MWMOUSE_BUTTON_RIGHT)
			eventModifiers |= java_awt_event_InputEvent_BUTTON3_MASK;

		id = java_awt_event_MouseEvent_MOUSE_PRESSED;
	}

	else id = java_awt_event_MouseEvent_MOUSE_RELEASED;

	/* Post mouse event to Java. */

	(*env)->CallVoidMethod(env,
						   window,
						   MWCachedIDs.Window_postMouseButtonEventMID,
						   event->button,
						   id,
						   eventModifiers,
						   (jint)mouseX,
						   (jint)mouseY);

	if (event->type == MWEVENT_MOUSE_BUTTON_RELEASED)
	{
		if (event->button == MWMOUSE_BUTTON_LEFT)
			eventModifiers &= ~java_awt_event_InputEvent_BUTTON1_MASK;

		else if (event->button == MWMOUSE_BUTTON_MIDDLE)
			eventModifiers &= ~java_awt_event_InputEvent_BUTTON2_MASK;

		else if (event->button == MWMOUSE_BUTTON_RIGHT)
			eventModifiers &= ~java_awt_event_InputEvent_BUTTON3_MASK;
	}
}

static void
processMouseMoveEvent (JNIEnv *env, jobject window, MWMouseMoveEvent *event)
{
	jint id;

	/* Update the global mouse coordinates. */

	mouseX = event->x;
	mouseY = event->y;

	id = (eventModifiers & (java_awt_event_InputEvent_BUTTON1_MASK | java_awt_event_InputEvent_BUTTON2_MASK | java_awt_event_InputEvent_BUTTON3_MASK)) ? java_awt_event_MouseEvent_MOUSE_DRAGGED : java_awt_event_MouseEvent_MOUSE_MOVED;

	(*env)->CallVoidMethod(env,
						   window,
						   MWCachedIDs.Window_postMouseEventMID,
						   id,
						   eventModifiers,
						   mouseX,
						   mouseY);
}

static void
processKeyEvent (JNIEnv *env, jobject window, MWKeyboardEvent *event)
{
	jint id;

	/* Update event modifiers */

	if (event->type == MWEVENT_KEY_PRESSED)
	{
		if (event->keyCode == VK_SHIFT)
			eventModifiers |= java_awt_event_InputEvent_SHIFT_MASK;

		else if (event->keyCode == VK_CONTROL)
			eventModifiers |= java_awt_event_InputEvent_CTRL_MASK;
	}

	else
	{
		if (event->keyCode == VK_SHIFT)
			eventModifiers &= ~java_awt_event_InputEvent_SHIFT_MASK;

		else if (event->keyCode == VK_CONTROL)
			eventModifiers &= ~java_awt_event_InputEvent_CTRL_MASK;
	}
	
	if (event->keyChar == 0)
		event->keyChar = java_awt_event_KeyEvent_CHAR_UNDEFINED;

	id = (event->type == MWEVENT_KEY_PRESSED) ? java_awt_event_KeyEvent_KEY_PRESSED : java_awt_event_KeyEvent_KEY_RELEASED;

	/* Post key pressed or released event to Java. */

	(*env)->CallVoidMethod(env, window, MWCachedIDs.Window_postKeyEventMID, id, eventModifiers, event->keyCode, event->keyChar);

	/* Post key typed event to Java if this was a printable character and the key was released. */

	if (event->type == MWEVENT_KEY_RELEASED && event->keyChar != java_awt_event_KeyEvent_CHAR_UNDEFINED)
		(*env)->CallVoidMethod(env, window, MWCachedIDs.Window_postKeyEventMID, java_awt_event_KeyEvent_KEY_TYPED, eventModifiers, java_awt_event_KeyEvent_VK_UNDEFINED, event->keyChar);
}

JNIEXPORT void JNICALL
Java_java_awt_MWGraphicsEnvironment_run (JNIEnv * env, jobject this)
{
	while ((*env)->CallStaticBooleanMethod(env, this, MWCachedIDs.java_lang_Thread_interruptedMID) == JNI_FALSE)
	{
		MWEvent event;
		mwawt_hardwareInterface->GetNextEvent(&event);

		if (eventWindow != NULL)
		{
			switch (event.type)
			{
				case MWEVENT_MOUSE_BUTTON_PRESSED:
				case MWEVENT_MOUSE_BUTTON_RELEASED:
					processMouseButtonEvent (env, eventWindow, (MWMouseButtonEvent *)&event);
				break;

				case MWEVENT_MOUSE_MOVED:
					processMouseMoveEvent (env, eventWindow, (MWMouseMoveEvent *)&event);
				break;

				case MWEVENT_KEY_PRESSED:
				case MWEVENT_KEY_RELEASED:
					processKeyEvent (env, eventWindow, (MWKeyboardEvent *)&event);
				break;

				default:

				break;
			}
		}
	}
}
