/*
 * @(#)QtButtonPeer.cc	1.11 06/10/25
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
#include <qfont.h>
#include "jni.h"
#include "sun_awt_qt_QtButtonPeer.h" 
#include <qstring.h>
#include "QtButtonPeer.h" 
#include "QpPushButton.h"
#include "QtDisposer.h"

/**
 * Constructor for QtButtonPeer
 */
QtButtonPeer :: QtButtonPeer (JNIEnv *env,
                              jobject thisObj,
                              QpWidget* buttonWidget)
  : QtComponentPeer(env, thisObj, buttonWidget, buttonWidget)
{
  if (buttonWidget) {
    buttonWidget->show();
  }
  /* Connect a slot for signal clicked so that when the button is clicked 
     it notifies the EventDispatchThread with an ActionEvent. */
  buttonWidget->connect(SIGNAL(clicked()), this, SLOT(buttonClicked()));      
}

/**
 * Destructor of QtButtonPeer
 */
QtButtonPeer::~QtButtonPeer(void)
{
}


/* Called when the button is clicked. Posts an ActionEvent to the AWT event dispatch
   thread for processing. */

void
QtButtonPeer :: buttonClicked (void)
{
  QtDisposer::Trigger guard;

  jobject thisObj = this->getPeerGlobalRef();
  JNIEnv *env;
  
  if (!thisObj || JVM->AttachCurrentThread((void **) &env, NULL) != 0)
    return;  
  guard.setEnv(env);

  env->CallVoidMethod(thisObj, QtCachedIDs.QtButtonPeer_postActionEventMID);
  
  if (env->ExceptionCheck ())
    env->ExceptionDescribe (); 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtButtonPeer_initIDs (JNIEnv *env, jclass cls)
{  
  GET_METHOD_ID (QtButtonPeer_postActionEventMID, "postActionEvent", "()V"); 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtButtonPeer_create (JNIEnv *env, jobject thisObj, 
				     jobject parent)
{  
  QtButtonPeer *buttonPeer;
  
  QpWidget *button;

  /* creating a frame with shape = NoFrame -> QFrame draws nothing */
  /* QPushButton needs a parent and hence a dummy frame is created */
  //QFrame *frame = new QFrame();
  //frame->setFrameStyle(QFrame::NoFrame);


  // Create the label widget & then the peer object
  QpWidget *parentWidget = 0;

  if(parent) {
    QtComponentPeer *parentPeer = (QtComponentPeer *)
      env->GetIntField (parent,
                        QtCachedIDs.QtComponentPeer_dataFID);
    parentWidget = parentPeer->getWidget();
  }
  /* Create the button with a label object inside it. */   
  button = new QpPushButton(parentWidget);

  if (button == NULL) {
    env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  buttonPeer = new QtButtonPeer(env, thisObj, button);
  if (buttonPeer == NULL) {
    env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  button->show(); // Do we need this ??? TAF
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtButtonPeer_setLabelNative (JNIEnv *env, jobject thisObj, 
                                             jstring label)
{ 
    QtDisposer::Trigger guard(env);
    
  QtButtonPeer *buttonPeer =
    (QtButtonPeer *)QtComponentPeer::getPeerFromJNI(env,
                                                    thisObj);
  
  if (buttonPeer == NULL)
    return;
  
  QString* labelString = awt_convertToQString(env, label);
  ((QpPushButton *)buttonPeer->getWidget())->setText(*labelString);

  delete labelString;
}
