/*
 * @(#)QtScrollbarPeer.cc	1.14 06/10/25
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
#include "java_awt_Scrollbar.h"
#include "java_awt_event_AdjustmentEvent.h"
#include "sun_awt_qt_QtScrollbarPeer.h"
#include "QtScrollbarPeer.h"
#include "QpScrollBar.h"
#include "QtDisposer.h"

/*
 * Contructor of class QtScrollbarPeer
 */
QtScrollbarPeer::QtScrollbarPeer(JNIEnv* env, 
                                 jobject thisObj,
                                 QpWidget* scrollbarWidget)
    :QtComponentPeer(env, thisObj, scrollbarWidget, 
                     scrollbarWidget)
{
    if (scrollbarWidget) {
        scrollbarWidget->show();
    }
    scrollbarWidget->connect (SIGNAL(valueChanged(int)), this,
                              SLOT(handleValueChanged(int)));
}


/*
 * Destructor of class QtScrollbarPeer
 */
QtScrollbarPeer::~QtScrollbarPeer(void) 
{
}


/*
 * Slot Function to handle SIGNAL valueChanged(int)
 */
void
QtScrollbarPeer::handleValueChanged(int value) 
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
        return;
    guard.setEnv(env);
    
    jobject target = env->GetObjectField(thisObj, 
                                         QtCachedIDs.QtComponentPeer_targetFID);
    jint oldValue = env->GetIntField(target, 
                                     QtCachedIDs.java_awt_Scrollbar_valueFID);

    QtScrollbarPeer* peer = (QtScrollbarPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer == NULL) {
	env->DeleteLocalRef(target);
        return;
    }
    
    QpScrollBar* scrollBarWidget = (QpScrollBar*)peer->getWidget();
    if (oldValue != value) {
        jint moveType;
        jint moveValue = value - oldValue;
        jboolean increment = JNI_TRUE;
        
        if(moveValue < 0) {
            moveValue *= -1;
            increment = JNI_FALSE;
        }

        if(moveValue == (jint)scrollBarWidget->pageStep()) // modified for bug #4724630 
            moveType = increment == JNI_TRUE ? 
                java_awt_event_AdjustmentEvent_BLOCK_INCREMENT : 
            java_awt_event_AdjustmentEvent_BLOCK_DECREMENT;
        else if(moveValue == (jint)scrollBarWidget->lineStep()) // modified for bug #4724630 
            moveType = increment == JNI_TRUE ? 
                java_awt_event_AdjustmentEvent_UNIT_INCREMENT : 
            java_awt_event_AdjustmentEvent_UNIT_DECREMENT;
        else
            moveType = java_awt_event_AdjustmentEvent_TRACK;
        
        env->SetIntField (target, QtCachedIDs.java_awt_Scrollbar_valueFID, 
                          value);
        env->CallVoidMethod (thisObj, QtCachedIDs.
                             QtScrollbarPeer_postAdjustmentEvent, moveType, 
                             value);
  	
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
        }
    }

    env->DeleteLocalRef(target);
}


/*
 * Beginning JNI Functions
 */ 

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollbarPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (QtScrollbarPeer_postAdjustmentEvent, "postAdjustmentEvent", 
                   "(II)V");
    
    cls = env->FindClass ("java/awt/Scrollbar");
    
    if (cls == NULL) 
        return; 
    
    GET_FIELD_ID (java_awt_Scrollbar_valueFID, "value", "I");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollbarPeer_createScrollbar (JNIEnv *env, jobject thisObj, 
                                                 jobject parent, 
						 jint orientation)
{
    QtScrollbarPeer* scrollbarPeer;
    QpWidget* parentWidget = 0;
    QpScrollBar* scrollbar;    

    if (parent) {
        QtComponentPeer* parentPeer = (QtComponentPeer* )
            env->GetIntField (parent,
			    QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }

    int type = (orientation == java_awt_Scrollbar_HORIZONTAL) ?
        QScrollBar::Horizontal : QScrollBar::Vertical;
    scrollbar = new QpScrollBar(type, parentWidget);
    if (scrollbar == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }
    scrollbar->setTracking(FALSE);
    scrollbar->show();
    scrollbarPeer = new QtScrollbarPeer(env, thisObj, scrollbar);
    if (scrollbarPeer == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    }
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtScrollbarPeer_isFocusTraversableNative (JNIEnv *env, jobject thisObj)
{
#ifdef QT_KEYPAD_MODE
    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif /* QT_KEYPAD_MODE */
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollbarPeer_setValues (JNIEnv *env, 
                                           jobject thisObj,
                                           jint value,
                                           jint visible,
                                           jint minimum,
                                           jint maximum)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollbarPeer* scrollbarPeer = (QtScrollbarPeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (scrollbarPeer == NULL) {
        return;
    }

    QpScrollBar *scrollbar = (QpScrollBar*)scrollbarPeer->getWidget();
    scrollbar->setMinValue(minimum);
    scrollbar->setMaxValue(maximum-visible);
    scrollbar->setValue(value);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollbarPeer_setLineIncrement (JNIEnv *env, 
                                                  jobject thisObj, 
                                                  jint increment)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollbarPeer* scrollbarPeer = (QtScrollbarPeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (scrollbarPeer == NULL) {
        return;
    }
    
    ((QpScrollBar*)scrollbarPeer->getWidget())->setLineStep(increment);    
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollbarPeer_setPageIncrement (JNIEnv *env, 
                                                  jobject thisObj, 
                                                  jint increment)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollbarPeer* scrollbarPeer = (QtScrollbarPeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (scrollbarPeer == NULL) {
        return;
    }
    
    ((QpScrollBar*)scrollbarPeer->getWidget())->setPageStep(increment);         
}
