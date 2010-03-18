/*
 * @(#)X11Toolkit.c	1.37 06/10/10
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
#include "java_awt_X11Toolkit.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "java_awt_event_WindowEvent.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <X11/keysym.h>

#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x > y) ? x : y)

static int numModalDialogs;
static int modalDialogsCapacity;
static Window *modalDialogs;
static pthread_mutex_t modalDialogsMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t eventMutex = PTHREAD_MUTEX_INITIALIZER;


/* Adds a window with its associated Java object so the object can later be retrieved by
   the event processing loop. This should be called whenever a Window is created. This allows
   us to determine which Java object an event should be sent to when we receive events
   from the X server. */

Boolean
xawt_addWindow (JNIEnv *env, Window window, jobject object)
{

  XPointer obj = (XPointer)(*env)->NewGlobalRef(env, object);

  return (XSaveContext(xawt_display, window, xawt_context, obj) == 0);
}

/* Removes a window and its associated object. This should be called whenever a Window is
   destroyed. */

void
xawt_removeWindow (JNIEnv *env, Window window)
{
  jobject obj;
  
  /* Lock to make sure we aren't processing an event for this window while it gets destroyed. */
    
  pthread_mutex_lock(&eventMutex);

  if(XFindContext(xawt_display, window, xawt_context, (XPointer *)&obj)==0) {

    (*env)->DeleteGlobalRef (env, obj);
    XDeleteContext(xawt_display, window, xawt_context);
    XDestroyWindow(xawt_display, window);
  }
  
  pthread_mutex_unlock(&eventMutex);
}

/* Given a java.awt.Color object this function determines the Pixel that represents the closest
   color. */

Pixel
xawt_getPixel (JNIEnv *env, jobject color)
{
	jint value;
	
	if (color == NULL)
		return xawt_blackPixel;
	
	value = (*env)->GetIntField (env, color, XCachedIDs.Color_valueFID);

	return colorCvtFunc((value&0x00ff0000)>>16, (value&0x0000ff00)>>8, value&0x000000ff);
}

/* Given a Java event mask this method determines the X event mask. */

long
xawt_getXMouseEventMask (jlong mask)
{
	long xmask = NoEventMask;
	
	if ((mask & java_awt_AWTEvent_MOUSE_EVENT_MASK) == java_awt_AWTEvent_MOUSE_EVENT_MASK)
		xmask |= ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask;
		
	if ((mask & java_awt_AWTEvent_MOUSE_MOTION_EVENT_MASK) == java_awt_AWTEvent_MOUSE_MOTION_EVENT_MASK)
		xmask |= PointerMotionMask | ButtonMotionMask;
		
	return xmask;
}

JNIEXPORT void JNICALL
Java_java_awt_X11Toolkit_sync (JNIEnv *env, jobject this)
{
	XFlush(xawt_display);
}

JNIEXPORT void JNICALL
Java_java_awt_X11Toolkit_beep (JNIEnv *env, jobject this)
{
	XBell(xawt_display, 0);
}

JNIEXPORT jint JNICALL
Java_java_awt_X11Toolkit_getScreenResolution (JNIEnv *env, jobject this)
{
	Screen *screen = ScreenOfDisplay(xawt_display, xawt_screen);
	return (jint)WidthOfScreen(screen) / ((WidthMMOfScreen(screen) * 254) / 10);
}

/* Pushes a new modal dialog onto the modal dialog stack. If events go to a window that does
	 not belong to the modal dialog on the top of the stack then we just beep. */

JNIEXPORT void JNICALL
Java_java_awt_X11Toolkit_pushModal (JNIEnv *env, jclass cls, jint xwindow)
{
	int i;
	
	pthread_mutex_lock(&modalDialogsMutex);

	if (xwindow != 0)
	{
		if (numModalDialogs == modalDialogsCapacity)
		{
			Window *oldModalDialogs = modalDialogs;
			modalDialogsCapacity *= 2;
			modalDialogs = malloc(sizeof(Window) * modalDialogsCapacity);
			memcpy(modalDialogs, oldModalDialogs, sizeof(Window) * numModalDialogs);
			free(oldModalDialogs);
		}
		
		/* Check dialog isn't already in stack. */
		
		for (i = 0; i < numModalDialogs; i++)
		{
			if (modalDialogs[i] == (Window)xwindow)
			{
				pthread_mutex_unlock(&modalDialogsMutex);
				return;
			}
		}
		
		modalDialogs[numModalDialogs++] = (Window)xwindow;
	}
	
	pthread_mutex_unlock(&modalDialogsMutex);
}

/* Pops the specified dialog off the modal stack. */

JNIEXPORT void JNICALL
Java_java_awt_X11Toolkit_popModal (JNIEnv *env, jclass cls, jint xwindow)
{
 	int i;
 	
 	if (xwindow != 0)
 	{
 		pthread_mutex_lock(&modalDialogsMutex);
 	
		for (i = 0; i < numModalDialogs; i++)
		{
			if (modalDialogs[i] == (Window)xwindow)
			{
				for (; i < numModalDialogs - 1; i++)
					modalDialogs[i] = modalDialogs[i + 1];
					
				numModalDialogs--;
				break;
			}
		}
		
		pthread_mutex_unlock(&modalDialogsMutex);
	}
}

/* Posts any kind of mouse event (other than button pressed) to the specified object. */

static void
postMouseEvent (JNIEnv *env, jobject object,
				jint id,
				jlong when,
				jint modifiers,
				jint x,
				jint y)
{
	assert (id != java_awt_event_MouseEvent_MOUSE_CLICKED &&
			id != java_awt_event_MouseEvent_MOUSE_RELEASED &&
			id != java_awt_event_MouseEvent_MOUSE_PRESSED);

	(*env)->CallVoidMethod (env, object, XCachedIDs.ComponentXWindow_postMouseEvent,
							id,
							when,
							modifiers,
							x,
							y);
							
	if ((*env)->ExceptionCheck(env))
	{
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
}

/* Posts a key event to the specified object. */

static void
postKeyEvent (JNIEnv *env, jobject object, jint id, jlong when, jint modifiers, jint keyCode, jchar keyChar)
{
	(*env)->CallVoidMethod (env, object, XCachedIDs.WindowXWindow_postKeyEventMID,
	 						id,
	 						when,
	 						modifiers,
	 						keyCode,
	 						keyChar);

	if ((*env)->ExceptionCheck(env))
	{
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
}

/* Posts a window event of type id to the specified object. */

static void
postWindowEvent (JNIEnv *env, jobject object, jint id)
{
	(*env)->CallVoidMethod (env, object, XCachedIDs.WindowXWindow_postWindowEventMID, id);
	
	if ((*env)->ExceptionCheck(env))
	{
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
}

/* Gets the Java keyboard modifiers that correspond to the state specified that is common to many
   X events. */

static jint
getKeyboardModifiers (unsigned int state)
{
	jint modifiers = 0;
	
	if (state & ShiftMask)
		modifiers |= java_awt_event_InputEvent_SHIFT_MASK;
		
	if (state & ControlMask)
		modifiers |= java_awt_event_InputEvent_CTRL_MASK;
		
	if (state & Mod1Mask)
		modifiers |= java_awt_event_InputEvent_ALT_MASK;
		
	/*if (state & Mod2Mask)
		modifiers |= java_awt_event_InputEvent_ALT_MASK;*/
		
	if (state & Mod3Mask)
		modifiers |= java_awt_event_InputEvent_META_MASK;
		
	return modifiers;
}

/* Gets the Java mouse modifiers that correspond to the state specified that is common to many
   X events. */

static jint
getMouseModifiers (unsigned int state)
{
	jint modifiers = 0;
	
	if (state & Button1Mask)
		modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;
		
	if (state & Button2Mask)
		modifiers |= java_awt_event_InputEvent_BUTTON2_MASK;
		
	if (state & Button3Mask)
		modifiers |= java_awt_event_InputEvent_BUTTON3_MASK;
		
	return modifiers;
}

/* Gets the Java mouse button that correspond to the x button number. */

static jint
getMouseButton (unsigned int xbutton)
{
	jint button = 0;
					
	if (xbutton == Button1)
		button = java_awt_event_InputEvent_BUTTON1_MASK;
		
	else if (xbutton == Button2)
		button = java_awt_event_InputEvent_BUTTON2_MASK;
		
	else if (xbutton == Button3)
		button = java_awt_event_InputEvent_BUTTON3_MASK;
		
	return button;
}

/* Called when a button event ocurrs on a window. POsts the required event to the specified object. */

static void
processButtonEvent (JNIEnv *env, jobject object, XButtonEvent *event)
{
	if (event->button <= 3)
	{
		jint modifiers = getMouseButton (event->button) | getKeyboardModifiers (event->state);
		
		(*env)->CallVoidMethod (env, object, XCachedIDs.ComponentXWindow_postMouseButtonEvent,
							(jint)event->type == ButtonPress ? java_awt_event_MouseEvent_MOUSE_PRESSED : java_awt_event_MouseEvent_MOUSE_RELEASED,
							(jlong)event->time,
							modifiers,
							(jint)event->x,
							(jint)event->y);
							
		if ((*env)->ExceptionCheck(env))
		{
			(*env)->ExceptionDescribe(env);
			(*env)->ExceptionClear(env);
		}
	}
}

/* Called when the mouse pointer is moved over a window. Posts the required event to the
   specified object. */

static void
processPointerMovedEvent (JNIEnv *env, jobject object, XPointerMovedEvent *event)
{
	jint mouseModifiers = getMouseModifiers (event->state);

	postMouseEvent (env, object,
					(mouseModifiers == 0) ? java_awt_event_MouseEvent_MOUSE_MOVED : java_awt_event_MouseEvent_MOUSE_DRAGGED,
					event->time,
					mouseModifiers | getKeyboardModifiers (event->state),
					event->x,
					event->y);
}

/* Called when the mouse pointer enters/leaves a window. Posts the required event to the
   specified object. */

static void
processCrossingEvent (JNIEnv *env, jobject object, XCrossingEvent *event)
{
	postMouseEvent (env, object,
					(event->type == EnterNotify) ? java_awt_event_MouseEvent_MOUSE_ENTERED : java_awt_event_MouseEvent_MOUSE_EXITED,
					event->time,
					getMouseModifiers (event->state) | getKeyboardModifiers (event->state),
					event->x,
					event->y);
}

static void
processKeyEvent (JNIEnv *env, jobject object, XKeyEvent *event)
{
	char buffer[1];
	KeySym keySym;
	int length;
	jchar keyChar = 0;
	jlong keyCode;
	Boolean printable;
	jint modifiers;
	
	length = XLookupString(event, buffer, sizeof(buffer), &keySym, NULL);
	
	if (length != 0)
		keyChar = buffer[0];
		
	xawt_getJavaKeyCode (keySym, &keyCode, &printable);
	
	modifiers = getKeyboardModifiers (event->state) | getMouseModifiers (event->state);
	
	postKeyEvent (env, object, (event->type == KeyRelease) ? java_awt_event_KeyEvent_KEY_RELEASED : java_awt_event_KeyEvent_KEY_PRESSED,
				  event->time,
				  modifiers,
				  keyCode,
				  keyChar);
				  
	if (length != 0 && event->type == KeyRelease)
		postKeyEvent (env, object, java_awt_event_KeyEvent_KEY_TYPED,
				  event->time,
				  modifiers,
				  java_awt_event_KeyEvent_VK_UNDEFINED,
				  keyChar);
}

/* An area of a top level window has been exposed. As we only receive these for top level windows
   we can assume the object is a WindowXWindow. */

static void
processExposeEvent (JNIEnv *env, jobject object, XExposeEvent *event)
{
	jint x = event->x, y = event->y, width = event->width, height = event->height;
	Window window = event->window;

	while (XCheckTypedWindowEvent (xawt_display, window, Expose, (XEvent *)event))
	{
		x = min(event->x, x);
		y = min(event->y, y);
		width = max(x + width, event->x + event->width) - x;
		height = max(y + height, event->y + event->height) - y;
	}
	
	(*env)->CallVoidMethod (env, object, XCachedIDs.HeavyweightComponentXWindow_postPaintEventMID,
						    x,
						    y,
						    width,
						    height);

	if ((*env)->ExceptionCheck(env))
	{
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
}

/* Process a reparent event from the X server. This will occur when a top level window is mapped
   and the window manager will reparent it.  We need to determine the border values for the window
   as these are used as the insets in Java. In Java a top level window's size includes the borders
   but in X this is not the case. As we only receive these for top level windows
   we can assume the object is a WindowXWindow.*/

static void
processReparentEvent (JNIEnv *env, jobject object, XReparentEvent *event)
{
	Window window = event->window, parent, root, oldOuter;
	Window *children;
	unsigned int nchildren;
	jint topBorder = 0, leftBorder = 0, rightBorder, bottomBorder;
	int x, y;
	unsigned int width, height, borderWidth, depth;
	unsigned int origWidth, origHeight;
	Boolean gotOrigSize = False;
	Boolean hasOuterWindow;
	jobject component;
	jint componentWidth, componentHeight;
	
	while (XCheckTypedWindowEvent (xawt_display, window, ReparentNotify, (XEvent *)event))
			;
	
	/* Determine the border widths of the window.
	   Find window manager's outer window (i.e. the window that is the child of the root window). */
	
	while (True)
	{
		if (!XQueryTree (xawt_display, window, &root, &parent, &children, &nchildren))
			return;
			
		if (!XGetGeometry (xawt_display, window, &root, &x, &y, &width, &height, &borderWidth, &depth))
			return;
		
		if (!gotOrigSize)
		{
			origWidth = width;
			origHeight = height;
			gotOrigSize = True;
		}
		
		if (children != NULL)
			XFree(children);
		
		if (root == parent)
			break;
			
		leftBorder += x;
		topBorder += y;
		
		window = parent;
	}
	
	rightBorder = width - origWidth - leftBorder;
	bottomBorder = height - origHeight - topBorder;
	hasOuterWindow = (window != event->window);
	
	if (hasOuterWindow)
		XSelectInput (xawt_display, window, StructureNotifyMask);
	
	/* If we had a previous outer window then remove it from the hash table. */
	
	oldOuter = (Window)(*env)->GetIntField (env, object, XCachedIDs.WindowXWindow_outerWindowFID);
	
	if (oldOuter != 0)
	{
		jobject obj;
	
		if(XFindContext(xawt_display, oldOuter, xawt_context, (XPointer *)&obj) == 0)
		{
    		(*env)->DeleteGlobalRef (env, obj);
    		XDeleteContext(xawt_display, oldOuter, xawt_context);
		}
	}
	
	/* Remember window manager's window. */
	
	(*env)->SetIntField (env, object, XCachedIDs.WindowXWindow_outerWindowFID, hasOuterWindow ? (jint)window : (jint)0);
	
	/* Tell the Java object the new border sizes. */
	
	(*env)->CallVoidMethod (env, object, XCachedIDs.WindowXWindow_setBordersMID,
							topBorder,
							leftBorder,
							bottomBorder,
							rightBorder);
							
	/* Associate the window manager's window with this Java object for future ConfigureNotify
	   events when the outer window is moved or resized. */
	
	if (hasOuterWindow && !xawt_addWindow(env, window, object))
		(*env)->ExceptionDescribe(env);
	
	/* Get current size of window in Java. If this does not match the size of the window
	   manager's outer window then resize it to the Java objects size. */
	
	component = (*env)->GetObjectField (env, object, XCachedIDs.ComponentXWindow_componentFID);
	componentWidth = (*env)->GetIntField (env, component, XCachedIDs.Component_widthFID);
	componentHeight = (*env)->GetIntField (env, component, XCachedIDs.Component_heightFID);
	
	if (componentWidth != width || componentHeight != height)
	{
		/* We resize our window and not the outer window as this prevents creeping of the window size. */
	
		XResizeWindow (xawt_display, event->window,
					   max(1, componentWidth - leftBorder - rightBorder),
					   max(1, componentHeight - topBorder - bottomBorder));
	}
}

/* Process a configure event from the X server. As we only receive these for top level windows
   we can assume the object is a WindowXWindow. */

static void
processConfigureEvent (JNIEnv *env, jobject object, XConfigureEvent *event)
{
	Window window = event->window;
	Window outerWindow;
	
	outerWindow = (Window)(*env)->GetIntField (env, object, XCachedIDs.WindowXWindow_outerWindowFID);
	
	/* Only send resize/move events for configure events on the outer window (i.e. the window manager's window). */
	
	if (window == outerWindow)
	{
		jobject component = (*env)->GetObjectField (env, object, XCachedIDs.ComponentXWindow_componentFID);
		
		while (XCheckTypedWindowEvent (xawt_display, window, ConfigureNotify, (XEvent *)event))
			;
	
		if (event->x != (*env)->GetIntField (env, component, XCachedIDs.Component_xFID) ||
			event->y != (*env)->GetIntField (env, component, XCachedIDs.Component_yFID))
		{
			(*env)->SetIntField (env, component, XCachedIDs.Component_xFID, (jint)event->x);
			(*env)->SetIntField (env, component, XCachedIDs.Component_yFID, (jint)event->y);
			(*env)->CallVoidMethod (env, object, XCachedIDs.WindowXWindow_postMovedEventMID);
		}
		
		if (event->width != (*env)->GetIntField (env, component, XCachedIDs.Component_widthFID) ||
			event->height != (*env)->GetIntField (env, component, XCachedIDs.Component_heightFID))
		{
			(*env)->SetIntField (env, component, XCachedIDs.Component_widthFID, (jint)event->width);
			(*env)->SetIntField (env, component, XCachedIDs.Component_heightFID, (jint)event->height);
			(*env)->CallVoidMethod (env, object, XCachedIDs.WindowXWindow_postResizedEventMID);
		}
	}
}

/* Processes an event from a client (probably window manager). Checks if it is a window delete
   message and in which case posts a window closing event to Java. */

static void
processClientMessage (JNIEnv *env, jobject object, XClientMessageEvent *event)
{
	if (event->data.l[0] == XA_WM_DELETE_WINDOW)
	{
		postWindowEvent (env, object, java_awt_event_WindowEvent_WINDOW_CLOSING);
	}
}

/* Processes a single event from the X server and posts the corresponfing event to the specified
   Java object. */

static void
processEvent (JNIEnv *env, XEvent *event, jobject object)
{
	switch (event->type)
	{
		case ButtonPress:
		case ButtonRelease:
			processButtonEvent (env, object, (XButtonEvent *)event);
		break;
		
		case MotionNotify:
			processPointerMovedEvent (env, object, (XPointerMovedEvent *)event);
		break;
		
		case EnterNotify:
		case LeaveNotify:
			processCrossingEvent (env, object, (XCrossingEvent *)event);
		break;
		
		case KeyPress:
		case KeyRelease:
			processKeyEvent (env, object, (XKeyEvent *)event);
		break;
		
		case Expose:
			processExposeEvent (env, object, (XExposeEvent *)event);
		break;
		
		case ReparentNotify:
			processReparentEvent (env, object, (XReparentEvent *)event);
		break;
		
		case MapNotify:
			postWindowEvent (env, object, java_awt_event_WindowEvent_WINDOW_DEICONIFIED);
		break;
		
		case UnmapNotify:
			postWindowEvent (env, object, java_awt_event_WindowEvent_WINDOW_ICONIFIED);
		break;
		
		case FocusIn:
			postWindowEvent (env, object, java_awt_event_WindowEvent_WINDOW_ACTIVATED);
		break;
		
		case FocusOut:
			postWindowEvent (env, object, java_awt_event_WindowEvent_WINDOW_DEACTIVATED);
		break;
		
		case ConfigureNotify:
			processConfigureEvent (env, object, (XConfigureEvent *)event);
		break;
		
		case ClientMessage:
			processClientMessage (env, object, (XClientMessageEvent *)event);
		break;
	}
}

#ifdef CVM_DEBUG

/* Some useful debugging functions. */

/* Calls toString on the object and prints the result to stdout. */

static void
printObject(JNIEnv *env, jobject object)
{
	const char *txt;
	jstring str;
	jclass objectClass = (*env)->FindClass(env, "java/lang/Object");
	jmethodID toStringMID = (*env)->GetMethodID(env, objectClass, "toString", "()Ljava/lang/String;");
	str = (*env)->CallObjectMethod(env, object, toStringMID);
	txt = (*env)->GetStringUTFChars (env, str, NULL);
	printf (txt);
	(*env)->ReleaseStringUTFChars(env, str, txt);
}

/* Prints the window hierachy of the specified window and its children with their
   associated java objects to stdout. */

static void
printWindowHierachy(JNIEnv *env, Window window, int indent)
{
	Window root, parent;
	Window *children;
	unsigned int nchildren;
	XWindowAttributes attrs;
	int i;
	jobject object;
	
	XGetWindowAttributes(xawt_display, window, &attrs);
	
	for (i = 0; i < indent * 4; i++)
		printf(" ");
		
	printf("%s Window %d: (%d,%d,%d,%d)", (attrs.class == InputOnly) ? "InputOnly" : "InputOutput", (int)window, attrs.x, attrs.y, attrs.width, attrs.height);
	
	if (attrs.map_state & IsUnmapped)
		printf ("unmapped ");
		
	else printf("mapped ");
	
	if (attrs.map_state & IsUnviewable)
		printf("unviewable ");
		
	else if (attrs.map_state & IsViewable)
		printf("viewable ");
		
	if (attrs.your_event_mask & ButtonPressMask)
	 	printf ("ButtonPressMask ");
	 	
	if (attrs.your_event_mask & ButtonReleaseMask)
	 	printf ("ButtonReleaseMask ");
	 	
	 printf ("-> ");
	 	
	 if (XFindContext(xawt_display, window, xawt_context, (XPointer *)&object) == 0)
	 {
	 	jobject component;
	 	
	 	component = (*env)->GetObjectField(env, object, XCachedIDs.ComponentXWindow_componentFID);
	 	printObject(env, component);
	 }
		
	printf("\n");

	XQueryTree (xawt_display, window, &root, &parent, &children, &nchildren);
				
	for (i = 0; i < nchildren; i++)
		printWindowHierachy(env, children[i], indent + 1);
	
	if (children != NULL)
		XFree(children);
}

#endif

/* Checks whether an event should be allowed. This is required when modal dialogs are open.
   It checks to make sure the event is to the modal dialog.
   Returns True if the event should be processed as normal or False if it should be ignored. */

static Boolean
checkModalEvent (JNIEnv *env, XEvent *event)
{
	Boolean allowEvent = True;

	pthread_mutex_lock(&modalDialogsMutex);

	/* If there are some modal dialogs open then we must only process the event if it is to
	   the active (last opened) modal dialog that was popped onto the stack. If the event is for
	   any window other than this then we just beep and ignore it. */
	
	if (numModalDialogs > 0 &&
		(event->type == ButtonPress ||
		event->type == ButtonRelease ||
		event->type == KeyPress ||
		event->type == KeyRelease ||
		event->type == MotionNotify ||
		(event->type == ClientMessage && event->xclient.data.l[0] == XA_WM_DELETE_WINDOW)))
	{
		Window window = event->xany.window, root, parent;
		Window *children;
		unsigned int nchildren;
		Window modalDialog = modalDialogs[numModalDialogs - 1];
		jobject object;
		allowEvent = False;
		
		/* Scan tree and see if this window is in the modal dialog. */
		
		while (True)
		{
			if (window == modalDialog)
			{
				allowEvent = True;
				break;
			}
		
			XQueryTree (xawt_display, window, &root, &parent, &children, &nchildren);
			
			if (children != NULL)
				XFree(children);
				
			/* If this is a top level window but not a frame or dialog then we allow the event
			   to be processed because it is most likely a popup of some kind. */
			
			if (parent == root && XFindContext(xawt_display, window, xawt_context, (XPointer *)&object) == 0)
			{
				if ((*env)->IsSameObject(env, (*env)->GetObjectClass(env, object), XCachedIDs.WindowXWindow))
				{
					allowEvent = True;
					break;
				}
			}
				
			if (parent == 0 || parent == root)
				break;
								
			window = parent;
		}
		
		/* If it isn't in the dialog then beep if we have any kind of mouse or keyboard event-> */
		
		if (!allowEvent && event->type != MotionNotify)
			XBell(xawt_display, 0);
	}
	
	pthread_mutex_unlock(&modalDialogsMutex);
	
	return allowEvent;
}

/* The main event loop for processing events from the X server. */

JNIEXPORT void JNICALL
Java_java_awt_X11GraphicsEnvironment_run (JNIEnv *env, jclass cls)
{
	XEvent event;
	jobject object;

	while (True)
	{
		XNextEvent (xawt_display, &event);
		
		/* Lock to make sure the window is not destroyed whilst we process an event. */
		
		pthread_mutex_lock(&eventMutex);
		
		/* Lookup entry for this winodw to determine the Java object to which the event
	   should be posted.
	
	   If there is no entry for this window then the Java object has been disposed
	   so we don't bother to send the event to it. */
		
		if (XFindContext(xawt_display, event.xany.window, xawt_context, (XPointer *)&object) == 0 &&
			checkModalEvent(env, &event))
		{
#ifdef CVM_DEBUG

			/* In debug mode we allow Ctrl-F1 to display the current X window hierachy and the
			   corresponding java component. */
			
			if (event.type == KeyPress)
			{
				XKeyEvent *keyEvent = (XKeyEvent *)&event;
				char buffer[1];
				KeySym keySym;
				
				XLookupString(keyEvent, buffer, sizeof(buffer), &keySym, NULL);
				
				if (keyEvent->state & ControlMask &&
					keySym == XK_F1)
				{
					printWindowHierachy(env, event.xany.window, 0);
				}
			}
		
#endif
		
			assert(object != NULL);
			
			/* Process the event and send to the Java object. */
			
			processEvent (env, &event, object);
		}
		
		pthread_mutex_unlock(&eventMutex);
	}
}
