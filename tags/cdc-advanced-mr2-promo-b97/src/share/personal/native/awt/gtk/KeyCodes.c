/*
 * @(#)KeyCodes.c	1.11 06/10/10
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

/* Maps to/from KeyEvent key codes to Gdk key codes. */


#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "jni.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_KeyEvent.h"

const static jint keyCodesMap[] =
{
	java_awt_event_KeyEvent_VK_ENTER,		GDK_Return,
	java_awt_event_KeyEvent_VK_ENTER,		GDK_KP_Enter,
	java_awt_event_KeyEvent_VK_BACK_SPACE,	GDK_BackSpace,
	java_awt_event_KeyEvent_VK_TAB,			GDK_Tab,
	java_awt_event_KeyEvent_VK_CANCEL,		GDK_Cancel,
	java_awt_event_KeyEvent_VK_CLEAR,		GDK_Clear,
	java_awt_event_KeyEvent_VK_SHIFT,		GDK_Shift_L,
	java_awt_event_KeyEvent_VK_CONTROL,		GDK_Control_L,
	java_awt_event_KeyEvent_VK_ALT,			GDK_Alt_L,
	java_awt_event_KeyEvent_VK_PAUSE,		GDK_Pause,
	java_awt_event_KeyEvent_VK_CAPS_LOCK,	GDK_Caps_Lock,
	java_awt_event_KeyEvent_VK_ESCAPE,		GDK_Escape,
	java_awt_event_KeyEvent_VK_SPACE,		GDK_space,
	java_awt_event_KeyEvent_VK_PAGE_UP,		GDK_Page_Up,
	java_awt_event_KeyEvent_VK_PAGE_DOWN,	GDK_Page_Down,
	java_awt_event_KeyEvent_VK_END,			GDK_End,
	java_awt_event_KeyEvent_VK_HOME,		GDK_Home,
	java_awt_event_KeyEvent_VK_LEFT,		GDK_Left,
	java_awt_event_KeyEvent_VK_UP,			GDK_Up,
	java_awt_event_KeyEvent_VK_RIGHT,		GDK_Right,
	java_awt_event_KeyEvent_VK_DOWN,		GDK_Down,
	java_awt_event_KeyEvent_VK_COMMA,		GDK_comma,
	java_awt_event_KeyEvent_VK_PERIOD,		GDK_period,
	java_awt_event_KeyEvent_VK_SLASH,		GDK_slash,
	java_awt_event_KeyEvent_VK_0,			GDK_0,
	java_awt_event_KeyEvent_VK_1,			GDK_1,
	java_awt_event_KeyEvent_VK_2,			GDK_2,
	java_awt_event_KeyEvent_VK_3,			GDK_3,
	java_awt_event_KeyEvent_VK_4,			GDK_4,
	java_awt_event_KeyEvent_VK_5,			GDK_5,
	java_awt_event_KeyEvent_VK_6,			GDK_6,
	java_awt_event_KeyEvent_VK_7,			GDK_7,
	java_awt_event_KeyEvent_VK_8,			GDK_8,
	java_awt_event_KeyEvent_VK_9,			GDK_9,
	java_awt_event_KeyEvent_VK_SEMICOLON,	GDK_semicolon,
	java_awt_event_KeyEvent_VK_EQUALS,		GDK_equal,
	java_awt_event_KeyEvent_VK_A,			GDK_a,
	java_awt_event_KeyEvent_VK_B,			GDK_b,
	java_awt_event_KeyEvent_VK_C,			GDK_c,
	java_awt_event_KeyEvent_VK_D,			GDK_d,
	java_awt_event_KeyEvent_VK_E,			GDK_e,
	java_awt_event_KeyEvent_VK_F,			GDK_f,
	java_awt_event_KeyEvent_VK_G,			GDK_g,
	java_awt_event_KeyEvent_VK_H,			GDK_h,
	java_awt_event_KeyEvent_VK_I,			GDK_i,
	java_awt_event_KeyEvent_VK_J,			GDK_j,
	java_awt_event_KeyEvent_VK_K,			GDK_k,
	java_awt_event_KeyEvent_VK_L,			GDK_l,
	java_awt_event_KeyEvent_VK_M,			GDK_m,
	java_awt_event_KeyEvent_VK_N,			GDK_n,
	java_awt_event_KeyEvent_VK_O,			GDK_o,
	java_awt_event_KeyEvent_VK_P,			GDK_p,
	java_awt_event_KeyEvent_VK_Q,			GDK_q,
	java_awt_event_KeyEvent_VK_R,			GDK_r,
	java_awt_event_KeyEvent_VK_S,			GDK_s,
	java_awt_event_KeyEvent_VK_T,			GDK_t,
	java_awt_event_KeyEvent_VK_U,			GDK_u,
	java_awt_event_KeyEvent_VK_V,			GDK_v,
	java_awt_event_KeyEvent_VK_W,			GDK_w,
	java_awt_event_KeyEvent_VK_X,			GDK_x,
	java_awt_event_KeyEvent_VK_Y,			GDK_y,
	java_awt_event_KeyEvent_VK_Z,			GDK_z,
	java_awt_event_KeyEvent_VK_A,			GDK_A,
	java_awt_event_KeyEvent_VK_B,			GDK_B,
	java_awt_event_KeyEvent_VK_C,			GDK_C,
	java_awt_event_KeyEvent_VK_D,			GDK_D,
	java_awt_event_KeyEvent_VK_E,			GDK_E,
	java_awt_event_KeyEvent_VK_F,			GDK_F,
	java_awt_event_KeyEvent_VK_G,			GDK_G,
	java_awt_event_KeyEvent_VK_H,			GDK_H,
	java_awt_event_KeyEvent_VK_I,			GDK_I,
	java_awt_event_KeyEvent_VK_J,			GDK_J,
	java_awt_event_KeyEvent_VK_K,			GDK_K,
	java_awt_event_KeyEvent_VK_L,			GDK_L,
	java_awt_event_KeyEvent_VK_M,			GDK_M,
	java_awt_event_KeyEvent_VK_N,			GDK_N,
	java_awt_event_KeyEvent_VK_O,			GDK_O,
	java_awt_event_KeyEvent_VK_P,			GDK_P,
	java_awt_event_KeyEvent_VK_Q,			GDK_Q,
	java_awt_event_KeyEvent_VK_R,			GDK_R,
	java_awt_event_KeyEvent_VK_S,			GDK_S,
	java_awt_event_KeyEvent_VK_T,			GDK_T,
	java_awt_event_KeyEvent_VK_U,			GDK_U,
	java_awt_event_KeyEvent_VK_V,			GDK_V,
	java_awt_event_KeyEvent_VK_W,			GDK_W,
	java_awt_event_KeyEvent_VK_X,			GDK_X,
	java_awt_event_KeyEvent_VK_Y,			GDK_Y,
	java_awt_event_KeyEvent_VK_Z,			GDK_Z,
	java_awt_event_KeyEvent_VK_OPEN_BRACKET,GDK_bracketleft,
	java_awt_event_KeyEvent_VK_BACK_SLASH,	GDK_backslash,
	java_awt_event_KeyEvent_VK_CLOSE_BRACKET,GDK_bracketright,
	java_awt_event_KeyEvent_VK_NUMPAD0,		GDK_KP_0,
	java_awt_event_KeyEvent_VK_NUMPAD1,		GDK_KP_1,
	java_awt_event_KeyEvent_VK_NUMPAD2,		GDK_KP_2,
	java_awt_event_KeyEvent_VK_NUMPAD3,		GDK_KP_3,
	java_awt_event_KeyEvent_VK_NUMPAD4,		GDK_KP_4,
	java_awt_event_KeyEvent_VK_NUMPAD5,		GDK_KP_5,
	java_awt_event_KeyEvent_VK_NUMPAD6,		GDK_KP_6,
	java_awt_event_KeyEvent_VK_NUMPAD7,		GDK_KP_7,
	java_awt_event_KeyEvent_VK_NUMPAD8,		GDK_KP_8,
	java_awt_event_KeyEvent_VK_NUMPAD9,		GDK_KP_9,
	java_awt_event_KeyEvent_VK_MULTIPLY,	GDK_multiply,
	java_awt_event_KeyEvent_VK_ADD,			GDK_KP_Add,
	java_awt_event_KeyEvent_VK_SEPARATER,	GDK_KP_Separator,
	java_awt_event_KeyEvent_VK_SUBTRACT,	GDK_KP_Subtract,
	java_awt_event_KeyEvent_VK_DECIMAL,		GDK_KP_Decimal,
	java_awt_event_KeyEvent_VK_DIVIDE,		GDK_KP_Divide,
	java_awt_event_KeyEvent_VK_F1,			GDK_F1,
	java_awt_event_KeyEvent_VK_F2,			GDK_F2,
	java_awt_event_KeyEvent_VK_F3,			GDK_F3,
	java_awt_event_KeyEvent_VK_F4,			GDK_F4,
	java_awt_event_KeyEvent_VK_F5,			GDK_F5,
	java_awt_event_KeyEvent_VK_F6,			GDK_F6,
	java_awt_event_KeyEvent_VK_F7,			GDK_F7,
	java_awt_event_KeyEvent_VK_F8,			GDK_F8,
	java_awt_event_KeyEvent_VK_F9,			GDK_F9,
	java_awt_event_KeyEvent_VK_F10,			GDK_F10,
	java_awt_event_KeyEvent_VK_F11,			GDK_F11,
	java_awt_event_KeyEvent_VK_F12,			GDK_F12,
	java_awt_event_KeyEvent_VK_DELETE,		GDK_Delete,
	java_awt_event_KeyEvent_VK_NUM_LOCK,	GDK_Num_Lock,
	java_awt_event_KeyEvent_VK_SCROLL_LOCK,	GDK_Scroll_Lock,
	java_awt_event_KeyEvent_VK_PRINTSCREEN,	GDK_3270_PrintScreen,
	java_awt_event_KeyEvent_VK_INSERT,		GDK_Insert,
	java_awt_event_KeyEvent_VK_HELP,		GDK_Help,
	java_awt_event_KeyEvent_VK_META,		GDK_Meta_L,
	java_awt_event_KeyEvent_VK_BACK_QUOTE,	GDK_quoteleft,
	java_awt_event_KeyEvent_VK_QUOTE,		GDK_quoteleft,
	java_awt_event_KeyEvent_VK_FINAL,		GDK_quoteleft,
	java_awt_event_KeyEvent_VK_CONVERT,		0,
	java_awt_event_KeyEvent_VK_NONCONVERT,	0,
	java_awt_event_KeyEvent_VK_ACCEPT,		0,
	java_awt_event_KeyEvent_VK_MODECHANGE,	GDK_Mode_switch,
};

/** Gets the GDK key code for the java KeyEvent.VK_ code. */

guint
awt_gtk_getGdkKeyCode (jint keyCode)
{
	int size = sizeof (keyCodesMap) / sizeof (jint);
	int i;
	
	for (i = 0; i < size; i += 2)
	{
		if (keyCodesMap[i] == keyCode)
			return keyCodesMap[i + 1];
	}
	
	return 0;
}

/** Gets the java KeyEvent.VK_ code for the supplied GDK key code. */

jint
awt_gtk_getJavaKeyCode (guint keyCode)
{
	int size = sizeof (keyCodesMap) / sizeof (jint);
	int i;
	
	for (i = 1; i < size; i += 2)
	{
		if (((guint)keyCodesMap[i]) == keyCode)
			return keyCodesMap[i - 1];
	}
	
	return java_awt_event_KeyEvent_VK_UNDEFINED;
}

/** Gets the unicode character for the supplied java key code and modifiers. */

jchar
awt_gtk_getUnicodeChar (jint keyCode, jint modifiers)
{
	jchar c = (jchar)keyCode;

	/*
	 * Check for action keys - these do not map to Unicode char's
	 *
	 * NOTE: Not all keys that don't map to Unicode char's are
	 *       represented here because Gtk+ 1.2 does not provide
	 *       sufficient information to make that determination
	 *       in all cases.
	 */
	switch (keyCode) // added for bug #4704552
	{
        	case java_awt_event_KeyEvent_VK_HOME:
        	case java_awt_event_KeyEvent_VK_END:
        	case java_awt_event_KeyEvent_VK_PAGE_UP:
        	case java_awt_event_KeyEvent_VK_PAGE_DOWN:
        	case java_awt_event_KeyEvent_VK_UP:
        	case java_awt_event_KeyEvent_VK_DOWN:
        	case java_awt_event_KeyEvent_VK_LEFT:
        	case java_awt_event_KeyEvent_VK_RIGHT:
        	case java_awt_event_KeyEvent_VK_F1:
        	case java_awt_event_KeyEvent_VK_F2:
        	case java_awt_event_KeyEvent_VK_F3:
        	case java_awt_event_KeyEvent_VK_F4:
        	case java_awt_event_KeyEvent_VK_F5:
        	case java_awt_event_KeyEvent_VK_F6:
        	case java_awt_event_KeyEvent_VK_F7:
        	case java_awt_event_KeyEvent_VK_F8:
        	case java_awt_event_KeyEvent_VK_F9:
        	case java_awt_event_KeyEvent_VK_F10:
        	case java_awt_event_KeyEvent_VK_F11:
        	case java_awt_event_KeyEvent_VK_F12:
        	case java_awt_event_KeyEvent_VK_PRINTSCREEN:
        	case java_awt_event_KeyEvent_VK_SCROLL_LOCK:
        	case java_awt_event_KeyEvent_VK_CAPS_LOCK:
        	case java_awt_event_KeyEvent_VK_NUM_LOCK:
        	case java_awt_event_KeyEvent_VK_PAUSE:
        	case java_awt_event_KeyEvent_VK_INSERT:
        	   return java_awt_event_KeyEvent_CHAR_UNDEFINED;
	}

	if ((modifiers & java_awt_event_InputEvent_SHIFT_MASK) == 0)
		c = (jchar)tolower ((int)c);

	return c;
}
