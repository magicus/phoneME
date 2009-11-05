/*
 * @(#)ThreadLocking.h	1.7 06/10/10
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

#ifndef _THREAD_LOCKING_H_
#define _THREAD_LOCKING_H_

/*#define AWT_GTK_THREAD_LOCKING_DEBUG*/

/* These function allow for thread syncronisation in gtk. Because Gtk does not have
   recursive thread locking (i.e. if a thread calls gdk_threads_enter() twice in a row it will
   block indefinately on the second attempt) we implement this ourselves using a lock count. */

#ifndef AWT_GTK_THREAD_LOCKING_DEBUG

extern void awt_gtk_threadsEnter(void);
extern void awt_gtk_threadsLeave(void);
extern void awt_gtk_callbackEnter(void);
extern void awt_gtk_callbackLeave(void);

#else

extern void awt_gtk_threadsEnterDebug(const char *fileName, int lineNum);
extern void awt_gtk_threadsLeaveDebug(void);
extern void awt_gtk_callbackEnter(void);
extern void awt_gtk_callbackLeave(void);

#define awt_gtk_threadsEnter() awt_gtk_threadsEnterDebug(__FILE__, __LINE__)
#define awt_gtk_threadsLeave() awt_gtk_threadsLeaveDebug()

#endif

#endif
