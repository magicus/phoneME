/*
 * @(#)ThreadLocking.cpp	1.6 06/10/10
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
#include "jni.h"
#include "awt.h"
#include "ThreadLocking.h"

static jobject pocketpcLock = NULL;

void
awt_pocketpc_threadsInit(JNIEnv *env) 
{
    jclass objectClass;
    jmethodID cid;
    jobject temp;
    
    objectClass = env->FindClass("java/lang/Object");
    if (objectClass == NULL) {
	/* an exception will be thrown by FindClass */
	return;
    }

    cid = env->GetMethodID(objectClass, "<init>", "()V");
    if (cid == NULL) {
	/* an exception will be thrown by GetMethodID */
	return;
    }
    
    temp = env->NewObject(objectClass, cid);
    pocketpcLock = env->NewGlobalRef(temp);
    env->DeleteLocalRef(temp);
    env->DeleteLocalRef(objectClass);

    /* if pocketpcLock is still NULL, an exception should have been thrown */
}

void
awt_pocketpc_threadsDestroy(JNIEnv *env)
{
    env->DeleteGlobalRef(pocketpcLock);
}

void
awt_pocketpc_threadsEnter(JNIEnv *env)
{
    env->MonitorEnter(pocketpcLock);
}

void
awt_pocketpc_threadsLeave(JNIEnv *env)
{
    env->MonitorExit(pocketpcLock);
}
