/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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


/*
 * A Java-style wrapper for native threads
 *
 * Use example (asynchronous hello world):
 *
 * void f(void *p) {
 *   char *s = (char *) p;
 *   printf(s);
 * }
 *
 * ...
 *
 * LimeThread t = new LimeThread(f, "Hello World\n");
 * if (t != NULL) {
 *     t->start(t);
 * }
 * DeleteLimeThread(t);
 *
 *
 * A shorthand for this is,
 *
 * LimeThreadRun(f, "Hello World");
 *
 */

#ifndef __limethread_h
#define __limethread_h 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __Thread {
    struct InternalThreadData *data;
    int (*start) (struct __Thread *t);
    void (*clearError) (struct __Thread *s);
    int (*getError) (struct __Thread *s);
} LimeThread;


/* A type for a function to be run in a separate thread */
typedef void (*LimeRunnable) (void *parameter);

/* Create a new thread, using the given LimeRunnable function that is
 * to receive the given parameter */
LimeThread *NewLimeThread(LimeRunnable f, void *parameter);

/** Free the memory used by a thread. Note that if the thread is
 *  running, it will continue to run. */
void DeleteLimeThread(LimeThread *t);

/** Run a LimeRunnable with the given parameter in its own thread */
int LimeThreadRun(LimeRunnable f, void *parameter);

#ifdef __cplusplus
}
#endif

#endif /* __limethread_h */
