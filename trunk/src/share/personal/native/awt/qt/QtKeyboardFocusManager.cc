/*
 * @(#)QtKeyboardFocusManager.cc	1.12 06/10/25
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

#include "jni.h"
#include "jlong.h"
#include "java_awt_KeyboardFocusManager.h"
#include "QtComponentPeer.h"
#include "QtApplication.h"
#include "QtDisposer.h"
#include "java_awt_event_FocusEvent.h"

JNIEXPORT void JNICALL
Java_java_awt_KeyboardFocusManager_initIDs (JNIEnv *env, jclass cls)
{
                                                                    
    QtCachedIDs.java_awt_KeyboardFocusManagerClass = (jclass) env->NewGlobalRef
        (cls);

    GET_STATIC_METHOD_ID(java_awt_KeyboardFocusManager_shouldNativelyFocusHeavyweightMID, 
                  "shouldNativelyFocusHeavyweight",
                  "(Ljava/awt/Component;Ljava/awt/Component;ZZJ)I");

    
    GET_STATIC_METHOD_ID(java_awt_KeyboardFocusManager_markClearGlobalFocusOwnerMID, 
                         "markClearGlobalFocusOwner", "()Ljava/awt/Window;");
}

JNIEXPORT void JNICALL
Java_java_awt_KeyboardFocusManager__1clearGlobalFocusOwner(JNIEnv *env, 
                                                           jobject thisObj)
{
    QWidget* focusWidget;
    jobject activeWindow;

    activeWindow = env->CallStaticObjectMethod
        (QtCachedIDs.java_awt_KeyboardFocusManagerClass,
         QtCachedIDs.java_awt_KeyboardFocusManager_markClearGlobalFocusOwnerMID);

    QtDisposer::Trigger guard(env);
    
    focusWidget = qtApp->focusWidget();
    QtComponentPeer *focusPeer = 
	QtComponentPeer::getPeerForWidget(focusWidget);

    if (focusPeer != NULL) {
	jobject focusObj = focusPeer->getPeerGlobalRef();
	env->CallVoidMethod(focusObj,
			    QtCachedIDs.QtComponentPeer_postFocusEventMID,
			    java_awt_event_FocusEvent_FOCUS_LOST,
			    false,
			    (jint)0);
	QpWidget *widget = focusPeer->getWidget();
	if (widget != NULL) {
	    widget->clearFocus();
	}
    }
    return;
}

JNIEXPORT jobject JNICALL
Java_java_awt_KeyboardFocusManager_getNativeFocusOwner(JNIEnv *env,
                                                       jclass thisClass) 
{
    QtDisposer::Trigger guard(env);
    
    QWidget* widget = QpObject::staticGetNativeFocusOwner();
    QtComponentPeer *peer = (QtComponentPeer *)
                QtComponentPeer::getPeerForWidget( widget);
    if (peer == NULL) {
        return NULL;
    }
    jobject peerObj = peer->getPeerGlobalRef();
    jobject returnVal = env->GetObjectField(peerObj, 
                             QtCachedIDs.QtComponentPeer_targetFID);
    return returnVal;
}

JNIEXPORT jobject JNICALL
Java_java_awt_KeyboardFocusManager_getNativeFocusedWindow(JNIEnv *env,
                                                       jclass thisClass) 
{
    QtDisposer::Trigger guard(env);
    
    QWidget* widget = QpObject::staticGetNativeFocusedWindow();
    QtComponentPeer *peer = (QtComponentPeer *)
                QtComponentPeer::getPeerForWidget( widget);
    if (peer == NULL) {
        return NULL;
    }
    jobject peerObj = peer->getPeerGlobalRef();
     jobject returnVal = env->GetObjectField(peerObj, 
                             QtCachedIDs.QtComponentPeer_targetFID); 
    return returnVal;
}
