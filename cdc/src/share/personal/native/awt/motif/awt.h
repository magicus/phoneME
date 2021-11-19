/*
 * @(#)awt.h	1.46 06/10/10
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

/*
 * Common AWT definitions
 */

#ifndef _AWT_
#define _AWT_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include "jni.h"
#include "awt_p.h"

extern JavaVM  *JVM;

#define TIMEOUT_INFINITY 0

#ifdef NETSCAPE
#include "prmon.h"

extern PRMonitor *_pr_rusty_lock;
extern void     PR_XWait(int ms);
extern void     PR_XNotify(void);
extern void     PR_XNotifyAll(void);

#define AWT_LOCK() PR_XLock()

#define AWT_UNLOCK() PR_XUnlock()

#define AWT_FLUSH_UNLOCK() awt_output_flush(); PR_XUnlock()

#define AWT_WAIT(tm) PR_XWait(tm)

#define AWT_NOTIFY() PR_XNotify()

#define AWT_NOTIFY_ALL() PR_XNotifyAll()

#else

#define AWT_LOCK()

#define AWT_UNLOCK()

/* #define AWT_FLUSH_UNLOCK() awt_output_flush(env) */
#define AWT_FLUSH_UNLOCK()

#define AWT_WAIT(tm)

#define AWT_NOTIFY()

#define AWT_NOTIFY_ALL()

#endif				/* NETSCAPE */

extern void    *awt_lock;
extern Display *awt_display;
extern Visual  *awt_visual;
extern GC       awt_maskgc;
extern Window   awt_root;
extern long     awt_screen;
extern int      awt_depth;
extern Colormap awt_cmap;
extern XVisualInfo awt_visInfo;
extern int      awt_whitepixel;
extern int      awt_blackpixel;
extern int      awt_multiclick_time;
extern int      awt_multiclick_smudge;
extern int      awt_MetaMask;
extern int      awt_AltMask;
extern int      awt_NumLockMask;
extern Cursor   awt_scrollCursor;

extern Boolean  awt_isSelectionOwner(JNIEnv * env, char *sel_str);
extern void     awt_notifySelectionLost(JNIEnv * env, char *sel_str);

#endif				/* _AWT_ */
