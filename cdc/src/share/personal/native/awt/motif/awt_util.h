/*
 * @(#)awt_util.h	1.42 06/10/10
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
#ifndef _AWT_UTIL_H_
#define _AWT_UTIL_H_

Widget          awt_util_createWarningWindow(Widget parent, char *warning);
jboolean        awt_util_show(JNIEnv * env, Widget w);
jboolean        awt_util_hide(JNIEnv * env, Widget w);
void            awt_util_enable(Widget w);
void            awt_util_disable(Widget w);
jboolean        awt_util_reshape(JNIEnv * env, Widget w, jint x, jint y, jint wd, jint h);
void            awt_util_mapChildren(Widget w, void (*func) (Widget, void *),
			                       int applyToSelf, void *data);
int             awt_util_setCursor(Widget w, Cursor c);
Widget          awt_WidgetAtXY(Widget root, Position x, Position y);
int             awt_util_getIMStatusHeight(Widget vw);

#define AWT_MWM_RESIZABLE_DECOR MWM_DECOR_ALL
#define AWT_MWM_RESIZABLE_FUNC  MWM_FUNC_ALL
#define AWT_MWM_NONRESIZABLE_DECOR (MWM_DECOR_BORDER | MWM_DECOR_TITLE |\
		MWM_DECOR_MENU | MWM_DECOR_MINIMIZE)
#define AWT_MWM_NONRESIZABLE_FUNC  (MWM_FUNC_MOVE | MWM_FUNC_CLOSE |\
		MWM_FUNC_MINIMIZE)

void            awt_util_setMinMaxSizeProps(Widget shellW, Boolean set);
void            awt_util_setShellResizable(Widget shellW, Boolean isMapped);
void            awt_util_setShellNotResizable(Widget shellW, long width, long height, Boolean isMapped);
unsigned        awt_util_runningWindowManager();
Cardinal        awt_util_insertCallback(Widget w);
void            awt_util_consumeAllXEvents(Widget widget);
void            awt_util_cleanupBeforeDestroyWidget(Widget widget);

struct DPos {
	long            x;
	long            y;
	int             mapped;
	void           *data;
	void           *peer;
	long            echoC;
};

extern Widget   prevWidget;

#endif				/* _AWT_UTIL_H_ */
