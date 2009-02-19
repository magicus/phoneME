/*
 * @(#)QtTextComponentPeer.cc	1.10 06/10/25
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
#include <qt.h>

#include "jni.h"
#include "sun_awt_qt_QtTextComponentPeer.h"
#include "QtTextComponentPeer.h"
#include "QtDisposer.h"

/*
 * Constructor of the class
 * To be used by the TextArea
 */
QtTextComponentPeer :: QtTextComponentPeer (JNIEnv* env, jobject thisObj,
                                            QpWidget* textWidget)
    :QtComponentPeer(env, thisObj, textWidget, textWidget)
{
    if (textWidget) {
	textWidget->show();
    }
}

/*
 * Destructor of the class
 */
QtTextComponentPeer :: ~QtTextComponentPeer(void)
{
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextComponentPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (QtTextComponentPeer_postTextEventMID, "postTextEvent",
                   "()V");
}

  
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextComponentPeer_setEditable (JNIEnv *env, 
                                                 jobject thisObj, 
                                                 jboolean editable)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
     
    if (peer == NULL) 
        return; 
    
    peer->setEditable(editable);
}

JNIEXPORT jstring JNICALL
Java_sun_awt_qt_QtTextComponentPeer_getTextNative (JNIEnv *env, 
                                                   jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);

    jstring text; 
    
    if (peer == NULL)
        return NULL;
    
    text = peer->getTextNative(env);
    return text; 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextComponentPeer_setTextNative (JNIEnv *env, 
                                                   jobject thisObj, 
                                                   jstring text)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (peer == NULL)
        return;
    
    peer->setTextNative(env, text);
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtTextComponentPeer_getSelectionStart (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    jint pos;
    
    if (peer == NULL)
        return 0;
    
    pos = peer->getSelectionStart();
    
    return pos;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtTextComponentPeer_getSelectionEnd (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    jint pos;
    
    if (peer == NULL)
        return 0;
    
    pos = peer->getSelectionEnd();
    
    return pos; 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextComponentPeer_select (JNIEnv *env, jobject thisObj,
                                            jint selStart, jint selEnd)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer == NULL)
        return;
    
    peer->select(env, selStart, selEnd);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextComponentPeer_setCaretPosition (JNIEnv *env, 
                                                      jobject thisObj, 
                                                      jint pos)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer == NULL)
        return;
    
    peer->setCaretPosition(env, pos);
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtTextComponentPeer_getCaretPosition (JNIEnv *env, 
                                                      jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtTextComponentPeer* peer = (QtTextComponentPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    jint pos;
    
    if (peer == NULL)
        return 0;
    
    pos = peer->getCaretPosition();
    
    return pos; 
} 
