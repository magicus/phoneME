/*
 * @(#)PPCRobotHelper.cpp	1.1 07/01/13
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
#include "sun_awt_pocketpc_PPCRobotHelper.h"
#include "PPCRobotHelper.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_KeyEvent.h"


/* Dummy method */
JNIEXPORT void JNICALL 
Java_sun_awt_pocketpc_PPCRobotHelper_init(JNIEnv *env, jclass cls) {
}

/* Dummy method */
JNIEXPORT void JNICALL 
Java_sun_awt_pocketpc_PPCRobotHelper_doMouseActionNative(JNIEnv *env, jobject helper,
    jint x, jint y, jint buttons, jboolean pressed) {
}

/* Dummy method */
JNIEXPORT void JNICALL 
Java_sun_awt_pocketpc_PPCRobotHelper_doKeyActionOnWidget(JNIEnv *env, jobject helper,
    jint jKeyCode, jint widgetType, jboolean pressed) {
}

/* Dummy method */
JNIEXPORT void JNICALL 
Java_sun_awt_pocketpc_PPCRobotHelper_doKeyActionNative(JNIEnv *env, jobject helper,
    jint jKeyCode, jboolean pressed) {
}

/* Dummy method */
JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCRobotHelper_getPixel(JNIEnv *env, jobject helper, jint x, jint y) {
    return 0;
}

/* Dummy method */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCRobotHelper_getPixelsNative(JNIEnv *env, jobject helper,
    jintArray buffer, jint x, jint y, jint w, jint h) {
}
