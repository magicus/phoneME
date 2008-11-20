/*
 * @(#)QtScrollPanePeer.cc	1.18 06/10/25
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
#include "java_awt_Adjustable.h"
#include "java_awt_ScrollPane.h"
#include "sun_awt_qt_QtScrollPanePeer.h"
#include "QtScrollPanePeer.h"
#include "QpScrollView.h"
#include "QtDisposer.h"

/*
 * Constructor of class QtScrollPanePeer
 */ 
QtScrollPanePeer::QtScrollPanePeer(JNIEnv *env,
                                   jobject thisObj,
                                   QpWidget *scrollpaneWidget)
    : QtComponentPeer(env, thisObj, scrollpaneWidget,
                      scrollpaneWidget)
{
    hScrollValue = -1;  // 6255265
    vScrollValue = -1;  // 6255265
    if (scrollpaneWidget) {
	scrollpaneWidget->show();
    }

    QpScrollView* view = (QpScrollView*) scrollpaneWidget;

    view->connectHScrollBar(SIGNAL(valueChanged(int)), 
             this, SLOT(hScrollBarScrolled(int)));
    view->connectVScrollBar(SIGNAL(valueChanged(int)), 
             this, SLOT(vScrollBarScrolled(int)));

    jclass cls = env->FindClass ("java/awt/Adjustable");
    horizontal = env->GetStaticIntField(cls, QtCachedIDs.java_awt_Adjustable_horizontalFID);
    vertical   = env->GetStaticIntField(cls, QtCachedIDs.java_awt_Adjustable_verticalFID);

    //6265234
    // Store "viewport" to avoid calling QpScrollView::viewport from 
    // QtScrollPanePeer::dispose since QtScrollPanePeer::dispose is called when
    // Qt lock accrued and calling QpScrollView::viewport when Qt is locked 
    // would result in a deadlock.
    this->viewport = view->viewport();
    //6265234
    //6238579
    // Associate the "viewport" to peer, since mouse events happen on the
    // viewport instead of the ScrollView widget
    QtComponentPeer::putPeerForWidget(this->viewport, this);
    //6238579

   if (env->ExceptionCheck()) {
       env->ExceptionDescribe();
   }
}

/*
 * Destructor of class QtScrollPanePeer
 */
QtScrollPanePeer::~QtScrollPanePeer()
{
}

/* slot functions*/
void
QtScrollPanePeer::hScrollBarScrolled(int value) {
    // If we have the scroll value, then don't post an event.
    if (value != hScrollValue) {   //655265
        QtDisposer::Trigger guard;

        jobject thisObj = this->getPeerGlobalRef();
        JNIEnv *env;
        if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
            return;
        guard.setEnv(env);

        env->CallVoidMethod (thisObj, 
                             QtCachedIDs.QtScrollPanePeer_postAdjustmentEventMID, 
                             horizontal,
                             value);

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
        }
    } else {
        // Reset scrollValue
        hScrollValue = -1;
    }
}

void
QtScrollPanePeer::vScrollBarScrolled(int value) {
    // If we have the scroll value, then don't post an event.
    if (value != vScrollValue) {   //655265
        QtDisposer::Trigger guard;

        jobject thisObj = this->getPeerGlobalRef();
        JNIEnv *env;
        if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
            return;
        guard.setEnv(env);

        env->CallVoidMethod (thisObj, 
                             QtCachedIDs.QtScrollPanePeer_postAdjustmentEventMID, 
                             vertical,
                             value);

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
        }
    } else {
        // Reset scrollValue
        vScrollValue = -1;
    }
}

//6238579
void
QtScrollPanePeer::dispose(JNIEnv *env)
{
  QtComponentPeer::removePeerForWidget(this->viewport);
  QtComponentPeer::dispose(env);
}
//6238579


/*
 * Beginning JNI Functions
 */
JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtScrollPanePeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (QtScrollPanePeer_postAdjustmentEventMID, 
                   "postAdjustableEvent", "(II)V");

    cls = env->FindClass ("java/awt/ScrollPane");
	
    if (cls == NULL)
        return;
    
    GET_FIELD_ID (java_awt_ScrollPane_scrollbarDisplayPolicyFID,
                  "scrollbarDisplayPolicy", "I");

    cls = env->FindClass ("java/awt/Adjustable");
	
    if (cls == NULL)
        return;

    GET_STATIC_FIELD_ID (java_awt_Adjustable_horizontalFID, "HORIZONTAL", "I");
    GET_STATIC_FIELD_ID (java_awt_Adjustable_verticalFID, "VERTICAL", "I");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollPanePeer_create (JNIEnv *env, jobject thisObj, 
					 jobject parent)
{
    QtScrollPanePeer* scrollPanePeer;
    QpWidget* parentWidget = 0;
    jobject target;
    
    if (parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
            env->GetIntField (parent,
                              QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
    target = env->GetObjectField (thisObj, QtCachedIDs.
                                  QtComponentPeer_targetFID);
    jint scrollbarDisplayPolicy =  env->GetIntField (
        target, QtCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID);
    
    QpScrollView* scrollView = new QpScrollView(parentWidget);
    if (scrollView == NULL) {
         env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
         return;
    }

    switch(scrollbarDisplayPolicy) {
    case java_awt_ScrollPane_SCROLLBARS_ALWAYS:
        scrollView->setVScrollBarMode(QScrollView::AlwaysOn);
        scrollView->setHScrollBarMode(QScrollView::AlwaysOn);
        break;
    case java_awt_ScrollPane_SCROLLBARS_NEVER:
        scrollView->setVScrollBarMode(QScrollView::AlwaysOff);
        scrollView->setHScrollBarMode(QScrollView::AlwaysOff);
        break;
    case java_awt_ScrollPane_SCROLLBARS_AS_NEEDED:
    default:
        scrollView->setVScrollBarMode(QScrollView::Auto);
        scrollView->setHScrollBarMode(QScrollView::Auto);
        break;
    }
    scrollView->setHScrollBarTracking(FALSE);
    scrollView->setVScrollBarTracking(FALSE);
    scrollView->show();
    scrollPanePeer = new QtScrollPanePeer(env, thisObj, scrollView);

    if (!scrollPanePeer) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollPanePeer_add (JNIEnv *env, jobject thisObj, jobject peer)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (scrollPanePeer == NULL) {
        return;
    }
    QtComponentPeer* componentPeer = QtComponentPeer::getPeerFromJNI(
        env, peer);
    if(componentPeer == NULL) {
        return; 
    }
  
    QpScrollView *scrollView = (QpScrollView*)scrollPanePeer->getWidget(); 
    scrollView->addWidget(componentPeer->getWidget());
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtScrollPanePeer_calculateHScrollbarHeight (JNIEnv *env, 
                                                            jclass cls)
{
    return (jint)QpScrollView::defaultHScrollBarHeight();
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtScrollPanePeer_calculateVScrollbarWidth (JNIEnv *env, 
                                                           jclass cls)
{
    return (jint)QpScrollView::defaultVScrollBarWidth();
}

/* 6249842 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollPanePeer_updateScrollBarsNative (JNIEnv *env, 
                                                       jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (scrollPanePeer == NULL) {
        return;
    }
  
    QpScrollView *scrollView = (QpScrollView*)scrollPanePeer->getWidget(); 
    scrollView->updateScrollBars();
}

/* 6228838 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollPanePeer_childResizedNative (JNIEnv *env, 
                                                     jobject thisObj, 
                                                     jint    width, 
                                                     jint    height)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);

    if (scrollPanePeer != NULL) {
        ((QpScrollView*)
            scrollPanePeer->getWidget())->childResized((int)width, (int)height);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollPanePeer_setUnitIncrementNative (JNIEnv *env, 
                                                         jobject thisObj, 
                                                         jint orientation, 
                                                         jint increment)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);

    if (scrollPanePeer == NULL) {
        return;
    }

    if (orientation == java_awt_Adjustable_HORIZONTAL) {
        ((QpScrollView*)scrollPanePeer->getWidget())->scrollBy(increment, 0);
    }
    else {
        ((QpScrollView*)scrollPanePeer->getWidget())->scrollBy(0, increment);
    }
}

//6255265
JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtScrollPanePeer_setAdjusterNative (JNIEnv *env, 
                                                    jobject thisObj, 
                                                    jint orientation,
                                                    jint value,
                                                    jint max,
                                                    jint pageSize)
{
    QtDisposer::Trigger guard(env);
    
    int rval = 0;   //6255265
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (scrollPanePeer == NULL) {
        return (jint)rval;   //6255265
    }

    QpScrollView* scrollView = (QpScrollView*) scrollPanePeer->getWidget();
    
    if (orientation == java_awt_Adjustable_HORIZONTAL) {
        scrollPanePeer->hScrollValue = value;
        scrollView->setHScrollBarSteps(0, (int)pageSize);
        scrollView->setHScrollBarRange(((pageSize==max)?(int)max : 0),(int)max);
        rval = scrollView->setHScrollBarValue(value);   //6255265
    }
    else {
        scrollPanePeer->vScrollValue = value;
        scrollView->setVScrollBarSteps(0, (int)pageSize);
        scrollView->setVScrollBarRange(((pageSize==max)?(int)max : 0),(int)max);
        rval = scrollView->setVScrollBarValue(value);   //6255265
    }
    return (jint)rval;
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtScrollPanePeer_scrollbarVisible (JNIEnv *env, 
						   jobject thisObj, 
						   jint orientation) 
{
    QtDisposer::Trigger guard(env);
    
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);

    if (scrollPanePeer == NULL) {
        return JNI_FALSE;
    }

    QpScrollView* scrollView = (QpScrollView*) scrollPanePeer->getWidget();
    bool visible = JNI_FALSE;
    switch (orientation) {
    case java_awt_Adjustable_HORIZONTAL:
        visible = (jboolean) scrollView->isHScrollBarVisible();
        break;
        
    case java_awt_Adjustable_VERTICAL:
        visible = (jboolean) scrollView->isVScrollBarVisible();
        break;
    }
       
    return visible ? JNI_TRUE : JNI_FALSE;
}

/* Gives explict control over scrollpane's scrollbars */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtScrollPanePeer_enableScrollbarsNative(JNIEnv *env,
							jobject thisObj,
							jboolean hBarOn,
							jboolean vBarOn)
{
    QtDisposer::Trigger guard(env);
    
    QtScrollPanePeer* scrollPanePeer = (QtScrollPanePeer*)
        QtComponentPeer::getPeerFromJNI(env, thisObj);

    if (scrollPanePeer == NULL) {
        return;
    }

    QpScrollView* scrollView = (QpScrollView*) scrollPanePeer->getWidget();
    if (hBarOn == JNI_FALSE) {
        scrollView->setHScrollBarMode(QScrollView::AlwaysOff);
    }
    else {
        scrollView->setHScrollBarMode(QScrollView::AlwaysOn);
    }
    if (vBarOn == JNI_FALSE) {
        scrollView->setVScrollBarMode(QScrollView::AlwaysOff);
    }
    else {
        scrollView->setVScrollBarMode(QScrollView::AlwaysOn);
    }
}
/*
 * End JNI Functions
 */ 


