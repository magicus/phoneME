/*
 * @(#)canvas.c	1.139 06/10/10
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
#include "awt_p.h"
#define XK_KATAKANA
#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <ctype.h>
#include "java_awt_Frame.h"
#include "java_awt_Component.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "java_awt_event_FocusEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_InputEvent.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_X11InputMethod.h"
#include "color.h"
#include "canvas.h"
#include "awt_util.h"

/*
 * Two Sun keys are not defined in standard keysym.h Key F11, F12 on Sun
 * keyboard somehow have KeySym SunF36, SunF37 Need to map these two KeySyms
 * to AWT keys F11, F12 so that pressing F11, F12 will generate F11, F12
 * events (Bug #4011219, #1264603)
 */
#define XK_SUNF36 0x1005FF10
#define XK_SUNF37 0x1005FF11

int             awt_override_key = -1;

int             awt_multiclick_smudge = 4;

Widget          prevWidget = 0;	/* for bug fix 4017222 */

extern void     awt_setWidgetGravity(Widget w, int gravity);
extern Boolean  scrollBugWorkAround;
extern jobject  currentX11InputMethodInstance;
extern Boolean  awt_x11inputmethod_lookupString(XEvent *, KeySym *);


void
awt_post_java_key_event(JNIEnv * env, jobject peer, jint id,
		   jlong when, jint keycode, jchar keychar, jint modifiers);

void
awt_post_java_focus_event(JNIEnv * env, jobject peer, jint id, jboolean isTemp);

void
awt_post_java_mouse_event(JNIEnv * env, jobject peer, jint id,
			  jlong when, jint modifiers,
		    jint x, jint y, jint clickcount, jboolean popuptrigger);


typedef struct KEYMAP_ENTRY {
	long            awtKey;
	KeySym          x11Key;
	Boolean         printable;
}               KeymapEntry;

const KeymapEntry keymapTable[] = {
	{java_awt_event_KeyEvent_VK_A, XK_a, TRUE},
	{java_awt_event_KeyEvent_VK_B, XK_b, TRUE},
	{java_awt_event_KeyEvent_VK_C, XK_c, TRUE},
	{java_awt_event_KeyEvent_VK_D, XK_d, TRUE},
	{java_awt_event_KeyEvent_VK_E, XK_e, TRUE},
	{java_awt_event_KeyEvent_VK_F, XK_f, TRUE},
	{java_awt_event_KeyEvent_VK_G, XK_g, TRUE},
	{java_awt_event_KeyEvent_VK_H, XK_h, TRUE},
	{java_awt_event_KeyEvent_VK_I, XK_i, TRUE},
	{java_awt_event_KeyEvent_VK_J, XK_j, TRUE},
	{java_awt_event_KeyEvent_VK_K, XK_k, TRUE},
	{java_awt_event_KeyEvent_VK_L, XK_l, TRUE},
	{java_awt_event_KeyEvent_VK_M, XK_m, TRUE},
	{java_awt_event_KeyEvent_VK_N, XK_n, TRUE},
	{java_awt_event_KeyEvent_VK_O, XK_o, TRUE},
	{java_awt_event_KeyEvent_VK_P, XK_p, TRUE},
	{java_awt_event_KeyEvent_VK_Q, XK_q, TRUE},
	{java_awt_event_KeyEvent_VK_R, XK_r, TRUE},
	{java_awt_event_KeyEvent_VK_S, XK_s, TRUE},
	{java_awt_event_KeyEvent_VK_T, XK_t, TRUE},
	{java_awt_event_KeyEvent_VK_U, XK_u, TRUE},
	{java_awt_event_KeyEvent_VK_V, XK_v, TRUE},
	{java_awt_event_KeyEvent_VK_W, XK_w, TRUE},
	{java_awt_event_KeyEvent_VK_X, XK_x, TRUE},
	{java_awt_event_KeyEvent_VK_Y, XK_y, TRUE},
	{java_awt_event_KeyEvent_VK_Z, XK_z, TRUE},

	{java_awt_event_KeyEvent_VK_ENTER, XK_Return, FALSE},
	{java_awt_event_KeyEvent_VK_ENTER, XK_KP_Enter, FALSE},
	{java_awt_event_KeyEvent_VK_ENTER, XK_Linefeed, FALSE},

	{java_awt_event_KeyEvent_VK_BACK_SPACE, XK_BackSpace, FALSE},
	{java_awt_event_KeyEvent_VK_TAB, XK_Tab, FALSE},
	{java_awt_event_KeyEvent_VK_CANCEL, XK_Cancel, FALSE},
	{java_awt_event_KeyEvent_VK_CLEAR, XK_Clear, FALSE},
	{java_awt_event_KeyEvent_VK_SHIFT, XK_Shift_L, FALSE},
	{java_awt_event_KeyEvent_VK_SHIFT, XK_Shift_R, FALSE},
	{java_awt_event_KeyEvent_VK_CONTROL, XK_Control_L, FALSE},
	{java_awt_event_KeyEvent_VK_CONTROL, XK_Control_R, FALSE},
	{java_awt_event_KeyEvent_VK_ALT, XK_Alt_L, FALSE},
	{java_awt_event_KeyEvent_VK_ALT, XK_Alt_R, FALSE},
	{java_awt_event_KeyEvent_VK_META, XK_Meta_L, FALSE},
	{java_awt_event_KeyEvent_VK_META, XK_Meta_R, FALSE},
	{java_awt_event_KeyEvent_VK_PAUSE, XK_Pause, FALSE},
	{java_awt_event_KeyEvent_VK_CAPS_LOCK, XK_Caps_Lock, FALSE},
	{java_awt_event_KeyEvent_VK_ESCAPE, XK_Escape, FALSE},
	{java_awt_event_KeyEvent_VK_SPACE, XK_space, TRUE},

	{java_awt_event_KeyEvent_VK_PAGE_UP, XK_Page_Up, FALSE},
	{java_awt_event_KeyEvent_VK_PAGE_UP, XK_R9, FALSE},
	{java_awt_event_KeyEvent_VK_PAGE_UP, XK_Prior, FALSE},
	{java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_Page_Down, FALSE},
	{java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_R15, FALSE},
	{java_awt_event_KeyEvent_VK_PAGE_DOWN, XK_Next, FALSE},
	{java_awt_event_KeyEvent_VK_END, XK_R13, FALSE},
	{java_awt_event_KeyEvent_VK_END, XK_End, FALSE},
	{java_awt_event_KeyEvent_VK_HOME, XK_Home, FALSE},
	{java_awt_event_KeyEvent_VK_HOME, XK_R7, FALSE},

	{java_awt_event_KeyEvent_VK_LEFT, XK_Left, FALSE},
	{java_awt_event_KeyEvent_VK_UP, XK_Up, FALSE},
	{java_awt_event_KeyEvent_VK_RIGHT, XK_Right, FALSE},
	{java_awt_event_KeyEvent_VK_DOWN, XK_Down, FALSE},
	{java_awt_event_KeyEvent_VK_INSERT, XK_Insert, FALSE},
	{java_awt_event_KeyEvent_VK_HELP, XK_Help, FALSE},

	{java_awt_event_KeyEvent_VK_0, XK_0, TRUE},
	{java_awt_event_KeyEvent_VK_1, XK_1, TRUE},
	{java_awt_event_KeyEvent_VK_2, XK_2, TRUE},
	{java_awt_event_KeyEvent_VK_3, XK_3, TRUE},
	{java_awt_event_KeyEvent_VK_4, XK_4, TRUE},
	{java_awt_event_KeyEvent_VK_5, XK_5, TRUE},
	{java_awt_event_KeyEvent_VK_6, XK_6, TRUE},
	{java_awt_event_KeyEvent_VK_7, XK_7, TRUE},
	{java_awt_event_KeyEvent_VK_8, XK_8, TRUE},
	{java_awt_event_KeyEvent_VK_9, XK_9, TRUE},

	{java_awt_event_KeyEvent_VK_EQUALS, XK_equal, TRUE},
	{java_awt_event_KeyEvent_VK_BACK_SLASH, XK_backslash, TRUE},
	{java_awt_event_KeyEvent_VK_BACK_QUOTE, XK_grave, TRUE},
	{java_awt_event_KeyEvent_VK_OPEN_BRACKET, XK_bracketleft, TRUE},
	{java_awt_event_KeyEvent_VK_CLOSE_BRACKET, XK_bracketright, TRUE},
	{java_awt_event_KeyEvent_VK_SEMICOLON, XK_semicolon, TRUE},
	{java_awt_event_KeyEvent_VK_QUOTE, XK_apostrophe, TRUE},
	{java_awt_event_KeyEvent_VK_COMMA, XK_comma, TRUE},
	{java_awt_event_KeyEvent_VK_PERIOD, XK_period, TRUE},
	{java_awt_event_KeyEvent_VK_SLASH, XK_slash, TRUE},

	{java_awt_event_KeyEvent_VK_NUMPAD0, XK_KP_0, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD1, XK_KP_1, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD2, XK_KP_2, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD3, XK_KP_3, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD4, XK_KP_4, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD5, XK_KP_5, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD6, XK_KP_6, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD7, XK_KP_7, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD8, XK_KP_8, TRUE},
	{java_awt_event_KeyEvent_VK_NUMPAD9, XK_KP_9, TRUE},
	{java_awt_event_KeyEvent_VK_MULTIPLY, XK_KP_Multiply, TRUE},
	{java_awt_event_KeyEvent_VK_ADD, XK_KP_Add, TRUE},
	{java_awt_event_KeyEvent_VK_SUBTRACT, XK_KP_Subtract, TRUE},
	{java_awt_event_KeyEvent_VK_DECIMAL, XK_KP_Decimal, TRUE},
	{java_awt_event_KeyEvent_VK_DIVIDE, XK_KP_Divide, TRUE},
	{java_awt_event_KeyEvent_VK_EQUALS, XK_KP_Equal, TRUE},
	{java_awt_event_KeyEvent_VK_INSERT, XK_KP_Insert, FALSE},
	{java_awt_event_KeyEvent_VK_ENTER, XK_KP_Enter, FALSE},

	{java_awt_event_KeyEvent_VK_F1, XK_F1, FALSE},
	{java_awt_event_KeyEvent_VK_F2, XK_F2, FALSE},
	{java_awt_event_KeyEvent_VK_F3, XK_F3, FALSE},
	{java_awt_event_KeyEvent_VK_F4, XK_F4, FALSE},
	{java_awt_event_KeyEvent_VK_F5, XK_F5, FALSE},
	{java_awt_event_KeyEvent_VK_F6, XK_F6, FALSE},
	{java_awt_event_KeyEvent_VK_F7, XK_F7, FALSE},
	{java_awt_event_KeyEvent_VK_F8, XK_F8, FALSE},
	{java_awt_event_KeyEvent_VK_F9, XK_F9, FALSE},
	{java_awt_event_KeyEvent_VK_F10, XK_F10, FALSE},
	{java_awt_event_KeyEvent_VK_F11, XK_F11, FALSE},
	{java_awt_event_KeyEvent_VK_F12, XK_F12, FALSE},

	{java_awt_event_KeyEvent_VK_DELETE, XK_Delete, FALSE},
	{java_awt_event_KeyEvent_VK_DELETE, XK_KP_Delete, FALSE},

	{java_awt_event_KeyEvent_VK_NUM_LOCK, XK_Num_Lock, FALSE},
	{java_awt_event_KeyEvent_VK_SCROLL_LOCK, XK_Scroll_Lock, FALSE},
	{java_awt_event_KeyEvent_VK_PRINTSCREEN, XK_Print, FALSE},

	{java_awt_event_KeyEvent_VK_ACCEPT, XK_Execute, FALSE},
	{java_awt_event_KeyEvent_VK_CONVERT, XK_Henkan, FALSE},
	{java_awt_event_KeyEvent_VK_NONCONVERT, XK_Muhenkan, FALSE},
	{java_awt_event_KeyEvent_VK_MODECHANGE, XK_Henkan_Mode, FALSE},
	{java_awt_event_KeyEvent_VK_KANA, XK_Katakana, FALSE},
	{java_awt_event_KeyEvent_VK_KANA, XK_kana_switch, FALSE},
	{java_awt_event_KeyEvent_VK_KANJI, XK_Kanji, FALSE},

	{java_awt_event_KeyEvent_VK_F11, XK_SUNF36, FALSE},
	{java_awt_event_KeyEvent_VK_F12, XK_SUNF37, FALSE},

	{0, 0, 0}
};

static long
keysymToAWTKeyCode(KeySym x11Key, Boolean * printable)
{
	int             i;

	*printable = FALSE;

	for (i = 0; keymapTable[i].awtKey != 0; i++) {
		if (keymapTable[i].x11Key == x11Key) {
			*printable = keymapTable[i].printable;
			return keymapTable[i].awtKey;
		}
	}

	return 0;
}

/* TODO: check the need of this function */
#if 0 
static          KeySym
getX11KeySym(long awtKey)
{
	int             i;

	for (i = 0; keymapTable[i].awtKey != 0; i++) {
		if (keymapTable[i].awtKey == awtKey) {
			return keymapTable[i].x11Key;
		}
	}

	return 0;
}
#endif 


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define INTERSECTS(r1_x1,r1_x2,r1_y1,r1_y2,r2_x1,r2_x2,r2_y1,r2_y2) \
        !((r2_x2 <= r1_x1) ||\
          (r2_y2 <= r1_y1) ||\
          (r2_x1 >= r1_x2) ||\
          (r2_y1 >= r1_y2))

typedef struct COLLAPSE_INFO {
	Window          win;
	DamageRect     *r;
}
                CollapseInfo;

static void
expandDamageRect(DamageRect * drect, XEvent * xev)
{
	int             x1 = xev->xexpose.x;
	int             y1 = xev->xexpose.y;
	int             x2 = xev->xexpose.width;
	int             y2 = xev->xexpose.height;

	drect->x1 = MIN(x1, drect->x1);
	drect->y1 = MIN(y1, drect->y1);
	drect->x2 = MAX(x2, drect->x2);
	drect->y2 = MAX(y2, drect->y2);
}

static void
handleExposeEvent(JNIEnv * env, Widget w, jobject peer, XEvent * event)
{
	struct ComponentData *componentData;

	if (peer == NULL)
		return;

	if (event->xexpose.send_event) {
		(*env)->CallVoidMethod(env, peer, MCachedIDs.MComponentPeer_handleRepaintMID,
				       event->xexpose.x,
				       event->xexpose.y,
				       event->xexpose.width,
				       event->xexpose.height);
		return;
	}

	componentData = (struct ComponentData *) (*env)->GetIntField(env, peer, MCachedIDs.MComponentPeer_pDataFID);

	if (componentData->repaintPending == 0) {
		componentData->exposeRect.x1 = event->xexpose.x;
		componentData->exposeRect.y1 = event->xexpose.y;
		componentData->exposeRect.x2 = event->xexpose.width;
		componentData->exposeRect.y2 = event->xexpose.height;

		componentData->repaintPending = 1;

	} else {
		expandDamageRect(&(componentData->exposeRect), event);
	}

	/*
	 * Only post Expose/Repaint if we know others arn't following
	 * directly in the queue.
	 */
	if (event->xexpose.count == 0) {

		componentData->repaintPending = 0;

		(*env)->CallVoidMethod(env, peer, MCachedIDs.MComponentPeer_handleExposeMID,
				       componentData->exposeRect.x1,
				       componentData->exposeRect.y1,
				       componentData->exposeRect.x2,
				       componentData->exposeRect.y2);
	}
}

/*
 * The AWT provides an isTemporary() method on focus events to be able to
 * determine the difference between focus moving between components and a
 * temporary focus change caused by window de-activation or menu postings.
 * Unfortunately the X11 focus model does not make this distinction easy to
 * determine, so we must track extra state here to determine the difference.
 */
static Widget   activated_shell = NULL;
static Widget   deactivated_shell = NULL;

Widget
getAncestorShell(Widget w)
{
	Widget          p = w;

	while (!(XtIsSubclass(p, shellWidgetClass))) {
		p = XtParent(p);
	}
	return p;
}

void
awt_setActivatedShell(Widget w)
{
	activated_shell = w;
}

void
awt_setDeactivatedShell(Widget w)
{
	deactivated_shell = w;
}


int
getModifiers(unsigned int state)
{
	int             modifiers = 0;

	if (state & ShiftMask) {
		modifiers |= java_awt_event_InputEvent_SHIFT_MASK;
	}
	if (state & ControlMask) {
		modifiers |= java_awt_event_InputEvent_CTRL_MASK;
	}
	if (state & awt_MetaMask) {
		modifiers |= java_awt_event_InputEvent_META_MASK;
	}
	if (state & awt_AltMask) {
		modifiers |= java_awt_event_InputEvent_ALT_MASK;
	}
	if (state & Button1Mask) {
		modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;
	}
	if ((state & Button2Mask) != 0) {
		modifiers |= java_awt_event_InputEvent_ALT_MASK;
		modifiers |= java_awt_event_InputEvent_BUTTON2_MASK;
	}
	if (state & Button3Mask) {
		modifiers |= java_awt_event_InputEvent_META_MASK;
		modifiers |= java_awt_event_InputEvent_BUTTON3_MASK;
	}
	return modifiers;
}

static void
handleKeyEvent(JNIEnv * env, jint keyEventId, XEvent * event, jobject peer)
{
	int             modifiers = getModifiers(event->xkey.state);
	KeySym          keysym = 0;
	long            keycode = 0;
	Modifiers       mods = 0;
	Boolean         printable = FALSE;


	if (event->type == KeyPress) {
		switch (keysym) {
		case XK_Shift_L:
		case XK_Shift_R:
		case XK_Shift_Lock:
			modifiers |= java_awt_event_InputEvent_SHIFT_MASK;
			break;
		case XK_Control_L:
		case XK_Control_R:
			modifiers |= java_awt_event_InputEvent_CTRL_MASK;
			break;
		case XK_Alt_L:
		case XK_Alt_R:
			modifiers |= java_awt_event_InputEvent_ALT_MASK;
			break;
		case XK_Meta_L:
		case XK_Meta_R:
			modifiers |= java_awt_event_InputEvent_META_MASK;
			break;
		}
	}

	keycode = event->xkey.keycode & 0xFFFF;
       
	XtTranslateKeycode(event->xkey.display, (KeyCode) keycode,
			   event->xkey.state, &mods, &keysym);

	keysym &= 0xFFFF;

        keycode = keysymToAWTKeyCode(keysym, &printable);

	awt_post_java_key_event(env, peer, keyEventId,
				event->xkey.time,
		                keycode,
				keysym,
				modifiers);


	if ((keyEventId == java_awt_event_KeyEvent_KEY_PRESSED) && printable) {
		awt_post_java_key_event(env, peer, java_awt_event_KeyEvent_KEY_TYPED,
					event->xkey.time,
					/* keycode, */ java_awt_event_KeyEvent_VK_UNDEFINED,
					keysym,
					modifiers);
	}
}

/* TODO: check the need of this function */
#if 0 
static void
translateXY(Widget w, int *xp, int *yp)
{
	Position        wx, wy;

	XtVaGetValues(w, XmNx, &wx, XmNy, &wy, NULL);
	*xp += wx;
	*yp += wy;
}
#endif

#define ABS(x) ((x) < 0 ? -(x) : (x))

/*
 * Part fix for bug id 4017222. Return the root widget of the Widget
 * parameter.
 */
Widget
getRootWidget(Widget w)
{
	if (!w)
		return NULL;

	if (XtParent(w))
		return getRootWidget(XtParent(w));
	else
		return w;
}

void
awt_canvas_handleEvent(Widget w, XtPointer client_data, XEvent * event, Boolean * cont)
{
	static int      clickCount = 1;
	static jobject  lastPeer = NULL;
	static Time     lastTime = 0;
	static int      lastx = 0;
	static int      lasty = 0;
	int             x, y;
	int             modifiers = 0;
	static int      rbutton = 0;
	Boolean         popupTrigger;
	JNIEnv         *env;

	/* partial workaround for Bug 4041235, part 1 */
	jobject         peer = (jobject) client_data;

	if ((*JVM)->AttachCurrentThread(JVM, (void **) &env, NULL) != 0)
		return;

	/*
	 * Any event handlers which take peer instance pointers as
	 * client_data should check to ensure the widget has not been marked
	 * as destroyed as a result of a dispose() call on the peer (which
	 * can result in the peer instance pointer already haven been gc'd by
	 * the time this event is processed)
	 */
	if (w->core.being_destroyed) {
		return;
	}
	switch (event->type) {
	case SelectionClear:
	case SelectionNotify:
	case SelectionRequest:
		break;
	case GraphicsExpose:
	case Expose:
		handleExposeEvent(env, w, peer, event);
		break;
	case FocusIn:
		awt_post_java_focus_event(env, peer,
				     java_awt_event_FocusEvent_FOCUS_GAINED,
					  FALSE);
		break;
	case FocusOut:
		awt_post_java_focus_event(env, peer,
				       java_awt_event_FocusEvent_FOCUS_LOST,
					  FALSE);
		break;
	case ButtonPress:
		x = event->xbutton.x;
		y = event->xbutton.y;

		if (lastPeer == peer) {
			/* check for multiple clicks */
			if (((event->xbutton.time - lastTime) <= awt_multiclick_time) &&
			    (ABS(lastx - x) < awt_multiclick_smudge && ABS(lasty - y) < awt_multiclick_smudge)) {
				clickCount++;
			} else {
				clickCount = 1;
			}
			lastTime = event->xbutton.time;
		} else {
			clickCount = 1;
			lastPeer = peer;
			lastTime = event->xbutton.time;
			lastx = x;
			lasty = y;
		}
		/*
		 * NOTE: Button2Mask and Button3Mask don't get set on
		 * ButtonPress
		 */
		/*
		 * events properly so we set them here so getModifiers will
		 * do
		 */
		/* the right thing. */
		switch (event->xbutton.button) {
			/*
			 * Added case for Button1Mask since it wasn't getting
			 * set either for ButtonPress events.
			 * 
			 * The state field represents the state of the pointer
			 * buttons and modifier keys just *prior* to the
			 * event which is why it is never set on ButtonPress.
			 */
		case 1:
			event->xbutton.state |= Button1Mask;
			break;
		case 2:
			event->xbutton.state |= Button2Mask;
			break;
		case 3:
			event->xbutton.state |= Button3Mask;
			break;
		default:
			break;
		}
		modifiers = getModifiers(event->xbutton.state);

		/*
		 * (4168006) Find out out how many buttons we have If this is
		 * a two button system Right == 2 If this is a three button
		 * system Right == 3
		 */
		if (rbutton == 0) {
			unsigned char   map[5];

			rbutton = XGetPointerMapping(awt_display, map, 3);
		}
		popupTrigger = (event->xbutton.button == rbutton || event->xbutton.button > 2) ? True : False;

		awt_post_java_mouse_event(env, peer,
				    java_awt_event_MouseEvent_MOUSE_PRESSED,
					  event->xbutton.time,
				 modifiers, x, y, clickCount, popupTrigger);

		break;
	case ButtonRelease:
		prevWidget = 0;
		x = event->xbutton.x;
		y = event->xbutton.y;
		modifiers = getModifiers(event->xbutton.state);

		awt_post_java_mouse_event(env, peer,
				   java_awt_event_MouseEvent_MOUSE_RELEASED,
		   event->xbutton.time, modifiers, x, y, clickCount, FALSE);

		if (lastPeer == peer) {

			awt_post_java_mouse_event(env, peer,
				    java_awt_event_MouseEvent_MOUSE_CLICKED,
						  event->xbutton.time, modifiers, x, y, clickCount, FALSE);
		}
		break;
	case MotionNotify:
		x = event->xmotion.x;
		y = event->xmotion.y;

		modifiers = getModifiers(event->xmotion.state);

		if (event->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
			awt_post_java_mouse_event(env, peer,
				    java_awt_event_MouseEvent_MOUSE_DRAGGED,
						  event->xmotion.time, modifiers, x, y, clickCount, FALSE);

		} else {

			awt_post_java_mouse_event(env, peer,
				      java_awt_event_MouseEvent_MOUSE_MOVED,
						  event->xmotion.time, modifiers, x, y, clickCount, FALSE);
		}

		break;
	case KeyPress:
		clickCount = 0;
		lastTime = 0;
		lastPeer = NULL;
		handleKeyEvent(env, java_awt_event_KeyEvent_KEY_PRESSED, event, peer);
		break;
	case KeyRelease:
		clickCount = 0;
		lastTime = 0;
		lastPeer = NULL;
		handleKeyEvent(env, java_awt_event_KeyEvent_KEY_RELEASED, event, peer);
		break;
	case EnterNotify:
	  /*
		if (event->xcrossing.mode != NotifyNormal ||
		    (event->xcrossing.detail == NotifyVirtual && !XtIsSubclass(w, xmScrolledWindowWidgetClass))) {
			return;
		}

		*/

		clickCount = 0;
		lastTime = 0;
		lastPeer = NULL;

		awt_post_java_mouse_event(env, peer,
				    java_awt_event_MouseEvent_MOUSE_ENTERED,
					  event->xcrossing.time,
					  0, event->xcrossing.x, event->xcrossing.y, clickCount, FALSE);

		awt_post_java_focus_event(env, peer,
				     java_awt_event_FocusEvent_FOCUS_GAINED,
					  TRUE);

		break;

	case LeaveNotify:
	  /*
		if (event->xcrossing.mode != NotifyNormal ||
		    (event->xcrossing.detail == NotifyVirtual && !XtIsSubclass(w, xmScrolledWindowWidgetClass))) {
			return;
		}

		*/

		clickCount = 0;
		lastTime = 0;
		lastPeer = NULL;

		awt_post_java_mouse_event(env, peer,
				     java_awt_event_MouseEvent_MOUSE_EXITED,
					  event->xcrossing.time,
					  0, event->xcrossing.x, event->xcrossing.y, clickCount, FALSE);

		awt_post_java_focus_event(env, peer,
				       java_awt_event_FocusEvent_FOCUS_LOST,
					  TRUE);

		break;

	default:
		break;
	}
}

/*
 * Creates a new drawing area widget to use as a canvas. Returns the new
 * widget or NULL if, and only if, an exception has occurred.
 */

Widget
awt_canvas_create(JNIEnv * env, XtPointer this,
		  Widget parent,
		  char *base, long width, long height, Boolean parentIsFrame, struct FrameData * frameData)
{
	Widget          newCanvas;
	Arg             args[20];
	int             argc;
	char            name[128];

	if (parent == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return NULL;
	}
	strcpy(name, base);

	argc = 0;
	XtSetArg(args[argc], XmNspacing, 0);
	argc++;

	XtSetArg(args[argc], XmNwidth, (XtArgVal) width);
	argc++;
	XtSetArg(args[argc], XmNheight, (XtArgVal) height);
	argc++;

	XtSetArg(args[argc], XmNresizePolicy, XmRESIZE_NONE); argc++;

	XtSetArg(args[argc], XmNmarginHeight, 0);
	argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0);
	argc++;
	XtSetArg(args[argc], XmNuserData, (XtPointer) this);
	argc++;

	strcat(name, "canvas:");

	newCanvas = XmCreateDrawingArea(parent, name, args, argc);

	/*
	 * XtOverrideTranslations(newCanvas,
	 * XtParseTranslationTable("<KeyDown>:DrawingAreaInput()"));
	 */

	XtSetSensitive(newCanvas, True);

	XtAddEventHandler(newCanvas, ExposureMask |
			  KeyPressMask | KeyReleaseMask |
			  ButtonPressMask | ButtonReleaseMask |
			  PointerMotionMask |
			  FocusChangeMask |
			  EnterWindowMask | LeaveWindowMask,
			  True, awt_canvas_handleEvent, this);

	XtManageChild(newCanvas);

	AWT_UNLOCK();

	return newCanvas;
}

struct MoveRecord {
	long            dx;
	long            dy;
};

void
moveWidget(Widget w, void *data)
{
	struct MoveRecord *rec = (struct MoveRecord *) data;

	if (XtIsRealized(w) && XmIsRowColumn(w)) {
		w->core.x -= rec->dx;
		w->core.y -= rec->dy;
	}
}


void
awt_post_java_key_event(JNIEnv * env, jobject peer, jint id,
		    jlong when, jint keycode, jchar keychar, jint modifiers)
{
	jobject         target = (*env)->GetObjectField(env, peer, MCachedIDs.MComponentPeer_targetFID);
	jobject         keyEvent;


	keyEvent = (*env)->NewObject(env,
				     MCachedIDs.java_awt_event_KeyEventClass,
			  MCachedIDs.java_awt_event_KeyEvent_constructorMID,
				     target,
				     id,
				     when,
				     modifiers,
				     keycode,
				     keychar);

	if (keyEvent == NULL) {
		(*env)->ExceptionDescribe(env);
		return;
	}
	(*env)->CallVoidMethod(env, peer, MCachedIDs.MComponentPeer_postEventMID, keyEvent);

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);
}

void
awt_post_java_focus_event(JNIEnv * env, jobject peer, jint id, jboolean isTemp)
{
	jobject         target = (*env)->GetObjectField(env, peer, MCachedIDs.MComponentPeer_targetFID);
	jobject         focusEvent;

	focusEvent = (*env)->NewObject(env,
				  MCachedIDs.java_awt_event_FocusEventClass,
			MCachedIDs.java_awt_event_FocusEvent_constructorMID,
				       target,
				       id,
				       isTemp);

	if (focusEvent == NULL) {
		(*env)->ExceptionDescribe(env);
		return;
	}
	(*env)->CallVoidMethod(env, peer, MCachedIDs.MComponentPeer_postEventMID, focusEvent);

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);
}

void
awt_post_java_mouse_event(JNIEnv * env, jobject peer, jint id,
			  jlong when, jint modifiers, jint x, jint y, jint clickcount, jboolean popuptrigger)
{
	jobject         target = (*env)->GetObjectField(env, peer, MCachedIDs.MComponentPeer_targetFID);
	jobject         mouseEvent;

	mouseEvent = (*env)->NewObject(env,
				  MCachedIDs.java_awt_event_MouseEventClass,
			MCachedIDs.java_awt_event_MouseEvent_constructorMID,
				       target,
				       id,
				       when,
				       modifiers,
				       x,
				       y,
				       clickcount,
				       popuptrigger);

	if (mouseEvent == NULL) {
		(*env)->ExceptionDescribe(env);
		return;
	}
	(*env)->CallVoidMethod(env, peer, MCachedIDs.MComponentPeer_postEventMID, mouseEvent);

	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionDescribe(env);

}
