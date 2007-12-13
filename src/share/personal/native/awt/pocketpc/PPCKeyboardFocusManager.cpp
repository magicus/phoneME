/*
 * @(#)PPCKeyboardFocusManager.cc	1.1 07/01/13
 *
 * Copyright  2007-2007 Davy Preuveneers. All Rights Reserved.
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
 */

#include "jni.h"
#include "jlong.h"
#include "java_awt_KeyboardFocusManager.h"
#include "PPCComponentPeer.h"
#include "java_awt_event_FocusEvent.h"

/* Dummy method */
JNIEXPORT void JNICALL 
Java_java_awt_KeyboardFocusManager_initIDs (JNIEnv *env, jclass cls) {
}

/* Dummy method */
JNIEXPORT void JNICALL 
Java_java_awt_KeyboardFocusManager__1clearGlobalFocusOwner(JNIEnv *env, jobject thisObj) {
}

/* Dummy method */
JNIEXPORT jobject JNICALL
Java_java_awt_KeyboardFocusManager_getNativeFocusOwner(JNIEnv *env, jclass thisClass) {
    return 0;
}

/* Dummy method */
JNIEXPORT jobject JNICALL
Java_java_awt_KeyboardFocusManager_getNativeFocusedWindow(JNIEnv *env, jclass thisClass) {
    return 0;
}
