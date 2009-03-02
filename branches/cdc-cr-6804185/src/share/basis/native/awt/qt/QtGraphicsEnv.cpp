/*
 * @(#)QtGraphicsEnv.cpp	1.9 06/10/25
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

#include "QtBackEnd.h"
#include "QtScreenFactory.h"

#include "java_awt_GraphicsEnvironment.h"
#include "java_awt_QtGraphicsEnvironment.h"
#include "java_awt_QtGraphicsDevice.h"

#include "java_awt_AWTEvent.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_KeyEvent.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

#ifdef CVM_DEBUG
int escape_quits = 1;
#else
int escape_quits = 0;
#endif

struct CachedIDs QtCachedIDs;	 /* All the cached JNI IDs. */

/* Defines the current set of modifiers (keyboard and mouse) for events. */

static jint eventModifiers = 0;

/* Current position of the mouse. */

static jint mouseX, mouseY;

static JavaVM *theJVM;

JNIEXPORT jobject JNICALL
Java_java_awt_GraphicsEnvironment_getMouseLocation(JNIEnv *env, jobject self) 
{
    jobject mouseLocation =  env->NewObject(QtCachedIDs.Point, QtCachedIDs.Point_constructor, mouseX, mouseY);
    return mouseLocation;
}

JNIEXPORT jstring JNICALL
Java_java_awt_QtGraphicsDevice_getIDstring (JNIEnv * env, jobject self)
{
	return env->NewStringUTF ("Qt Renderbased");
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphicsEnvironment_init (JNIEnv * env, jclass cls)
{
	GET_CLASS (AWTError, "java/awt/AWTError");

	if (env->GetVersion () == 0x10001)
	{
		env->ThrowNew (QtCachedIDs.AWTError, "Requires at least version 1.2 JNI");
		return;
	}

	/* Need JavaVM for debug error handler. */

	env->GetJavaVM (&theJVM);

	/* Cache commonly used JNI IDs. */

	GET_CLASS (OutOfMemoryError, "java/lang/OutOfMemoryError");
	GET_CLASS (NullPointerException, "java/lang/NullPointerException");

	FIND_CLASS ("java/lang/Thread");
	GET_STATIC_METHOD_ID (java_lang_Thread_interruptedMID, "interrupted", "()Z");
	GET_STATIC_METHOD_ID (java_lang_Thread_yieldMID, "yield", "()V");

	GET_CLASS (Point, "java/awt/Point");
	GET_METHOD_ID (Point_constructor, "<init>", "(II)V");

	FIND_CLASS ("java/awt/Color");
	GET_FIELD_ID (Color_valueFID, "value", "I");

	GET_CLASS (DirectColorModel, "java/awt/image/DirectColorModel");
	GET_METHOD_ID (DirectColorModel_constructor, "<init>", "(IIIII)V");

	GET_CLASS (IndexColorModel, "java/awt/image/IndexColorModel");
	GET_METHOD_ID (IndexColorModel_constructor, "<init>", "(II[B[B[B)V");

	GET_CLASS (java_awt_image_BufferedImage, "java/awt/image/BufferedImage");
	GET_METHOD_ID (java_awt_image_BufferedImage_constructor, "<init>", "(Lsun/awt/image/BufferedImagePeer;)V");
	GET_FIELD_ID (java_awt_image_BufferedImage_peerFID, "peer", "Lsun/awt/image/BufferedImagePeer;");

	FIND_CLASS ("java/awt/Window");
	GET_METHOD_ID (Window_postMouseButtonEventMID, "postMouseButtonEvent", "(IIIII)V");
	GET_METHOD_ID (Window_postMouseEventMID, "postMouseEvent", "(IIII)V");
	GET_METHOD_ID (Window_postKeyEventMID, "postKeyEvent", "(IIIC)V");
}


#ifdef AWT_QT_DEBUG
#include "QtDebugBackEnd.h"
static class QtDebugBackEnd *QTDB;
#endif /* AWT_QT_DEBUG */

JNIEXPORT jint JNICALL
Java_java_awt_QtGraphicsEnvironment_initfb (JNIEnv * env, jclass cls)
{
    int status = 0;

    QtScreen *screen = QtScreenFactory::getScreen();
    if ( screen == NULL ) {
        env->ThrowNew (QtCachedIDs.AWTError, "Could not open screen");
        return -1;
	}

#ifdef AWT_QT_DEBUG
    if ( QTDB == NULL ) {
        QTDB = new QtDebugBackEnd(env);
    }
#endif /* AWT_QT_DEBUG */

	return (jint)status;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphicsEnvironment_destroy(JNIEnv *env, jobject self)
{
}


static jobject eventWindow = NULL;

JNIEXPORT void JNICALL
Java_java_awt_QtGraphicsEnvironment_pSetWindow (JNIEnv * env, jobject self, jint qtImageDesc, jobject window)
{
	eventWindow = env->NewGlobalRef (window);
}


void
dispatchMouseButtonEvent (MouseButtonEvent *event)
{
	if(eventWindow==NULL) return;
	
	jint evid;
	JNIEnv *env;

	/* Update the global eventModifiers. */

	if (event->type == EVENT_MOUSE_BUTTON_PRESSED)
	{
		if (event->button == MOUSE_BUTTON_LEFT)
			eventModifiers |= java_awt_event_InputEvent_BUTTON1_MASK;

		else if (event->button == MOUSE_BUTTON_MIDDLE)
			eventModifiers |= java_awt_event_InputEvent_BUTTON2_MASK;

		else if (event->button == MOUSE_BUTTON_RIGHT)
			eventModifiers |= java_awt_event_InputEvent_BUTTON3_MASK;

		evid = java_awt_event_MouseEvent_MOUSE_PRESSED;
	}

	else evid = java_awt_event_MouseEvent_MOUSE_RELEASED;

	/* Post mouse event to Java. */

	theJVM->GetEnv( (void **)&env, JNI_VERSION_1_2);
	env->CallVoidMethod( eventWindow,
		QtCachedIDs.Window_postMouseButtonEventMID,
		event->button,
		evid,
		eventModifiers,
		(jint)mouseX,
		(jint)mouseY);

	if (event->type == EVENT_MOUSE_BUTTON_RELEASED)
	{
		if (event->button == MOUSE_BUTTON_LEFT)
			eventModifiers &= ~java_awt_event_InputEvent_BUTTON1_MASK;

		else if (event->button == MOUSE_BUTTON_MIDDLE)
			eventModifiers &= ~java_awt_event_InputEvent_BUTTON2_MASK;

		else if (event->button == MOUSE_BUTTON_RIGHT)
			eventModifiers &= ~java_awt_event_InputEvent_BUTTON3_MASK;
	}
}


void
dispatchMouseMoveEvent (MouseMoveEvent *event)
{
	if(eventWindow==NULL) return;

	jint evid;
	JNIEnv *env;

	/* Update the global mouse coordinates. */

	mouseX = event->x;
	mouseY = event->y;

	evid = (eventModifiers & (java_awt_event_InputEvent_BUTTON1_MASK | java_awt_event_InputEvent_BUTTON2_MASK | java_awt_event_InputEvent_BUTTON3_MASK)) ? java_awt_event_MouseEvent_MOUSE_DRAGGED : java_awt_event_MouseEvent_MOUSE_MOVED;

	theJVM->GetEnv((void **)&env, JNI_VERSION_1_2);
	env->CallVoidMethod( eventWindow,
		QtCachedIDs.Window_postMouseEventMID,
		evid,
		eventModifiers,
		mouseX,
		mouseY);
}


void
dispatchKeyEvent (KeyboardEvent *event)
{
	if(eventWindow==NULL) return;

	jint evid;
	JNIEnv *env;

	/* Update event modifiers */

	if (event->type == EVENT_KEY_PRESSED)
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

	/* Make sure that '\n' is used for Enter key. This prevents against `\r`
	   being used on the underlying platform. */
	if (event->keyCode == VK_ENTER)
		event->keyChar = '\n';

	evid = (event->type == EVENT_KEY_PRESSED) ? java_awt_event_KeyEvent_KEY_PRESSED : java_awt_event_KeyEvent_KEY_RELEASED;

	theJVM->GetEnv((void **)&env, JNI_VERSION_1_2);
	/* Post key pressed or released event to Java. */

	env->CallVoidMethod(eventWindow, QtCachedIDs.Window_postKeyEventMID, evid, eventModifiers, event->keyCode, event->keyChar);

	/* Post key typed event to Java if self was a printable character and the key was released. */

	if (event->type == EVENT_KEY_PRESSED && event->keyChar != java_awt_event_KeyEvent_CHAR_UNDEFINED)
		env->CallVoidMethod(eventWindow, QtCachedIDs.Window_postKeyEventMID, java_awt_event_KeyEvent_KEY_TYPED, eventModifiers, java_awt_event_KeyEvent_VK_UNDEFINED, event->keyChar);
}
