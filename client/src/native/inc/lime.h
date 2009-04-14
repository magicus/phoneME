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


/* OVERVIEW:
 *
 * The Lime client API allows invocation of methods on a running Java
 * virtual machine by a native application. Primitive Java data types and
 * single-dimensional arrays of these types may be passed as
 * parameters (by value) and received as return values.
 *
 * A reference to a Lime function is obtained with NewLimeFunction,
 *
 *  LimeFunction *f = NewLimeFunction("java.lang",
 *                                    "System",
 *                                    "currentTimeMillis");
 *
 * and involed with the call() method, passing the Lime function itself as
 * the first argument.
 *
 *  f->call(f, NULL);
 *
 * The other parameters to call() are:
 *
 *  - a pointer to the location of the return value. If the invoked
 *  method has a void return type, as in this example, NULL may be
 *  given for this parameter
 *
 * - a sequence of parameters to the invoked method. Primitive types
 * are passed as themselves; arrays are passed as two parameters each,
 * these being a pointer to the array and its length. In the above
 * example no paramters were passed, since System.currentTimeMillis()
 * takes none.
 *
 * The implementation of a Lime function on the server side is a
 * static method with public access that takes and returns only
 * primitive types and arrays of these types.
 *
 * The Lime client depends on a server running on a JVM on the same machine.
 *
 */


#ifndef __lime_h
#define __lime_h

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define LIMEEXPORT __declspec(dllexport)
#else
#define LIMEEXPORT
#endif

#ifndef BOOL_DEFINED
typedef enum {
  FALSE = 0,
  TRUE = 1
} bool_t;
#endif

typedef int LimeType;

#define LIME_TYPE_VOID 0
#define LIME_TYPE_BYTE 1
#define LIME_TYPE_SHORT 2
#define LIME_TYPE_INT 3
#define LIME_TYPE_LONG 4
#define LIME_TYPE_CHAR 5
#define LIME_TYPE_BOOLEAN 6
#define LIME_TYPE_FLOAT 7
#define LIME_TYPE_DOUBLE 8
#define LIME_TYPE_STRING 9
#define LIME_TYPE_ARRAY 16

extern const char *LimeTypes[];

#define LIME_COMMAND_PORT_VARIABLE "COMMAND_PORT"
#define LIME_EVENT_PORT_VARIABLE "EVENT_PORT"
#define LIME_TRACE_FLAG "LIME_TRACE"
#define LIME_SYNCH_FLAG "LIME_SYNCH"

#define LIME_LOOKUP 1
#define LIME_COMMAND 2
#define LIME_EVENTQUERY 3

#define LIME_NULL -1

typedef struct __LimeFunction {
    struct InternalLimeData *data;

    /* Calls a Lime function */
    void (*call) (struct __LimeFunction *f, void *result, ...);

    /* Turns tracing on or off for this function */
    void (*trace) (struct __LimeFunction *f, bool_t trace);
} LimeFunction;

/* Starts up the Lime subsystem */
LIMEEXPORT void StartLime(void);

/* Shuts down the Lime subsystem */
LIMEEXPORT void EndLime(void);

/* Checks to see if there is an event waiting to be processed
 *
 * Parameters:
 *   flag: a pointer to an integer which is to be set to 0 if there are no
 *   events to be processed, and 1 if there is at least one event waiting.
 *
 * Returns: 0 if everything went smoothly, otherwise an error code
 */
LIMEEXPORT int LimeCheckEvent(int *flag);

/* Notifies the LIME event subsystem that an event has been processed */
LIMEEXPORT int LimeDecrementEventCount();

/* Acquires a reference to a named Lime function which links to the given Java
 * method */
LIMEEXPORT LimeFunction *NewLimeFunction(const char *packageName,
                              const char *className,
                              const char *methodName);

/* Releases a reference to a Lime function */
LIMEEXPORT void DeleteLimeFunction(LimeFunction *f);

/* Locks the Lime system against access from other threads */
LIMEEXPORT void LimeLock();

/* Unlocks Lime, allowing access from other threads */
LIMEEXPORT void LimeUnlock();

#ifdef __cplusplus
}
#endif

#endif
