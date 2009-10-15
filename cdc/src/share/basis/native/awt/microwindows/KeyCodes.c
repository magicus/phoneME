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

#include "awt.h"
#include "java_awt_event_KeyEvent.h"

typedef struct  {
	MWKEY mwKey;
	jint awtKey;
} KeymapEntry;

static KeymapEntry keymapTable [] =
{
	{MWKEY_UNKNOWN,			java_awt_event_KeyEvent_VK_UNDEFINED},
	{MWKEY_BACKSPACE,		java_awt_event_KeyEvent_VK_BACK_SPACE},
	{MWKEY_TAB,				java_awt_event_KeyEvent_VK_TAB},
	{MWKEY_ENTER,			java_awt_event_KeyEvent_VK_ENTER},
	{MWKEY_ESCAPE,			java_awt_event_KeyEvent_VK_ESCAPE},

	/* ASCII mappings. Micro windows doesn't have key codes for these so we assume there is
	   a keyboard with a separate key for all of these. Really micro windows should offer
	   its own keycodes for these instead of using ASCII. */

	{' ',					java_awt_event_KeyEvent_VK_SPACE},
	{'!',					java_awt_event_KeyEvent_VK_EXCLAMATION_MARK},
	{'"',					java_awt_event_KeyEvent_VK_QUOTEDBL},
	{'#',					java_awt_event_KeyEvent_VK_NUMBER_SIGN},
	{'$',					java_awt_event_KeyEvent_VK_DOLLAR},
	{'&',					java_awt_event_KeyEvent_VK_AMPERSAND},
	{'\'',					java_awt_event_KeyEvent_VK_QUOTE},
	{'(',					java_awt_event_KeyEvent_VK_OPEN_BRACKET},
	{')',					java_awt_event_KeyEvent_VK_CLOSE_BRACKET},
	{'*',					java_awt_event_KeyEvent_VK_ASTERISK},
	{'+',					java_awt_event_KeyEvent_VK_PLUS},
	{',',					java_awt_event_KeyEvent_VK_COMMA},
	{'-',					java_awt_event_KeyEvent_VK_MINUS},
	{'.',					java_awt_event_KeyEvent_VK_PERIOD},
	{'/',					java_awt_event_KeyEvent_VK_SLASH},
	{'\\',					java_awt_event_KeyEvent_VK_BACK_SLASH},
	{'^',					java_awt_event_KeyEvent_VK_CIRCUMFLEX},
	{'_',					java_awt_event_KeyEvent_VK_UNDERSCORE},
	{'`',					java_awt_event_KeyEvent_VK_BACK_QUOTE},

	/* Following keysyms are mapped to private use portion of Unicode-16*/
	/* arrows + home/end pad*/
	{MWKEY_LEFT,			java_awt_event_KeyEvent_VK_LEFT},
	{MWKEY_RIGHT,			java_awt_event_KeyEvent_VK_RIGHT},
	{MWKEY_UP,			java_awt_event_KeyEvent_VK_UP},
	{MWKEY_DOWN,			java_awt_event_KeyEvent_VK_DOWN},
	{MWKEY_INSERT,			java_awt_event_KeyEvent_VK_INSERT},
	{MWKEY_DELETE,			java_awt_event_KeyEvent_VK_DELETE},
	{MWKEY_HOME,			java_awt_event_KeyEvent_VK_HOME},
	{MWKEY_END,			java_awt_event_KeyEvent_VK_END},
	{MWKEY_PAGEUP,			java_awt_event_KeyEvent_VK_PAGE_UP},
	{MWKEY_PAGEDOWN,			java_awt_event_KeyEvent_VK_PAGE_DOWN},

	/* Numeric keypad*/
	{MWKEY_KP0,			java_awt_event_KeyEvent_VK_NUMPAD0},
	{MWKEY_KP1,			java_awt_event_KeyEvent_VK_NUMPAD1},
	{MWKEY_KP2,			java_awt_event_KeyEvent_VK_NUMPAD2},
	{MWKEY_KP3,			java_awt_event_KeyEvent_VK_NUMPAD3},
	{MWKEY_KP4,			java_awt_event_KeyEvent_VK_NUMPAD4},
	{MWKEY_KP5,			java_awt_event_KeyEvent_VK_NUMPAD5},
	{MWKEY_KP6,			java_awt_event_KeyEvent_VK_NUMPAD6},
	{MWKEY_KP7,			java_awt_event_KeyEvent_VK_NUMPAD7},
	{MWKEY_KP8,			java_awt_event_KeyEvent_VK_NUMPAD8},
	{MWKEY_KP9,			java_awt_event_KeyEvent_VK_NUMPAD9},
	{MWKEY_KP_PERIOD,			java_awt_event_KeyEvent_VK_PERIOD},
	{MWKEY_KP_DIVIDE,			java_awt_event_KeyEvent_VK_DIVIDE},
	{MWKEY_KP_MULTIPLY,			java_awt_event_KeyEvent_VK_MULTIPLY},
	{MWKEY_KP_MINUS,			java_awt_event_KeyEvent_VK_MINUS},
	{MWKEY_KP_PLUS,			java_awt_event_KeyEvent_VK_PLUS},
	{MWKEY_KP_ENTER,		java_awt_event_KeyEvent_VK_ENTER},
	{MWKEY_KP_EQUALS,			java_awt_event_KeyEvent_VK_EQUALS},

	/* Function keys */
	{MWKEY_F1,			java_awt_event_KeyEvent_VK_F1},
	{MWKEY_F2,			java_awt_event_KeyEvent_VK_F2},
	{MWKEY_F3,			java_awt_event_KeyEvent_VK_F3},
	{MWKEY_F4,			java_awt_event_KeyEvent_VK_F4},
	{MWKEY_F5,			java_awt_event_KeyEvent_VK_F5},
	{MWKEY_F6,			java_awt_event_KeyEvent_VK_F6},
	{MWKEY_F7,			java_awt_event_KeyEvent_VK_F7},
	{MWKEY_F8,			java_awt_event_KeyEvent_VK_F8},
	{MWKEY_F9,			java_awt_event_KeyEvent_VK_F9},
	{MWKEY_F10,			java_awt_event_KeyEvent_VK_F10},
	{MWKEY_F11,			java_awt_event_KeyEvent_VK_F11},
	{MWKEY_F12,			java_awt_event_KeyEvent_VK_F12},

	/* Key state modifier keys*/
	{MWKEY_NUMLOCK,			java_awt_event_KeyEvent_VK_NUM_LOCK},
	{MWKEY_CAPSLOCK	,			java_awt_event_KeyEvent_VK_CAPS_LOCK},
	{MWKEY_SCROLLOCK,			java_awt_event_KeyEvent_VK_SCROLL_LOCK},
	{MWKEY_LSHIFT,			java_awt_event_KeyEvent_VK_SHIFT},
	{MWKEY_RSHIFT,			java_awt_event_KeyEvent_VK_SHIFT},
	{MWKEY_LCTRL,			java_awt_event_KeyEvent_VK_CONTROL},
	{MWKEY_RCTRL,			java_awt_event_KeyEvent_VK_CONTROL},
	{MWKEY_LALT,			java_awt_event_KeyEvent_VK_ALT},
	{MWKEY_RALT,				java_awt_event_KeyEvent_VK_ALT},
	{MWKEY_LMETA,			java_awt_event_KeyEvent_VK_META},
	{MWKEY_RMETA,			java_awt_event_KeyEvent_VK_META},
	{MWKEY_ALTGR,			java_awt_event_KeyEvent_VK_ALT_GRAPH},

	/* Misc function keys*/
	{MWKEY_PRINT,			java_awt_event_KeyEvent_VK_PRINTSCREEN},
/*	{MWKEY_SYSREQ,			java_awt_event_KeyEvent_VK_},
	{MWKEY_PAUSE,			java_awt_event_KeyEvent_VK_},*/
};

/* Given a Microwindows key code this function converts it into a Java key code as
   defined in KeyEvent. */

jint
mwawt_getJavaKeyCode(MWKEY mwKey)
{
	int i;

	/* Microwindows uses 32 to 126 as ASCII key mappings. We assume that
	   lower case and upper case characters are from the same key on the keyboard
	   but for other keys (e.g. !"£$) we can't be sure so we just assume there is a separate
	   key for these. */

	if (mwKey >= 32 && mwKey <= 126)
	{
		/* Lower case and uppercase characters use the same key. */

		if (mwKey >= 'a' && mwKey <= 'z')
			mwKey -= ('a' - 'A');

		/* Java's virtual key codes are the same as ASCII for these characters so
		   we can just use them directly. */

		if ((mwKey >= 'A' && mwKey <= 'Z') || (mwKey >= '0' && mwKey <= '9'))
			return mwKey;
	}

	for (i = 0; i < sizeof(keymapTable) / sizeof(KeymapEntry); i++)
	{
		if (keymapTable[i].mwKey == mwKey)
			return keymapTable[i].awtKey;
	}

	return java_awt_event_KeyEvent_VK_UNDEFINED;
}
