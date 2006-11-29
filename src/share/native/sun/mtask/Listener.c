/*
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * @(#)Listener.c	1.8 06/10/10
 *
 * Native portion of sun.mtask.Listener
 */

#include "jni.h"
#include "javavm/include/clib.h"
#include "javavm/include/globals.h"

/*
 * Set the fd field of a FileDescriptor object
 */
JNIEXPORT void JNICALL
Java_sun_mtask_Listener_setMtaskFd(
    JNIEnv* env,
    jclass  listenerClass,
    jobject fdObj)
{
    jclass fdClass;
    jfieldID fdFid;
    jint fd;

    fd = CVMglobals.commFd;
    
    /* insertion sort in linked list based on thisClass value */
    fdClass = (*env)->GetObjectClass(env, fdObj);
    assert(fdClass != NULL);
    assert(!(*env)->ExceptionOccurred(env));

    fdFid   = (*env)->GetFieldID(env, fdClass, "fd", "I");
    assert(fdFid != NULL);
    assert(!(*env)->ExceptionOccurred(env));

    (*env)->SetIntField(env, fdObj, fdFid, fd);
}

/*
 * Return the client ID of this client
 */
JNIEXPORT jint JNICALL
Java_sun_mtask_Listener_getMtaskClientId(
    JNIEnv* env,
    jclass  listenerClass)
{
    return CVMglobals.clientId;
}

/*
 * Return the port number which the server VM is listening to.
 */
JNIEXPORT jint JNICALL
Java_sun_mtask_Listener_getMtaskServerPort(
    JNIEnv* env,
    jclass  listenerClass)
{
    return CVMglobals.serverPort;
}
