/*
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

#ifndef _Qt_INTERFACE_H
#define _Qt_INTERFACE_H

#include "KeyCodes.h"

typedef enum
{
	EVENT_MOUSE_BUTTON_PRESSED,
	EVENT_MOUSE_BUTTON_RELEASED,
	EVENT_MOUSE_MOVED,
	EVENT_KEY_PRESSED,
	EVENT_KEY_RELEASED
} EventType;

typedef enum
{
	MOUSE_BUTTON_LEFT = 0,
	MOUSE_BUTTON_MIDDLE = 1,
	MOUSE_BUTTON_RIGHT = 2
} MouseButton;

typedef struct _MouseButtonEvent
{
	EventType type;
	MouseButton button;
} MouseButtonEvent;

typedef struct _MouseMoveEvent
{
	EventType type;
	int x, y;
} MouseMoveEvent;

typedef struct _KeyboardEvent
{
	EventType type;
	long keyCode;
	unsigned short keyChar;	/* Unicode key if displayable. */
} KeyboardEvent;

typedef union _Event
{
	EventType	type;
	MouseButtonEvent mouseButton;
	MouseMoveEvent mouseMove;
	KeyboardEvent keyboard;
} Event;

#endif
