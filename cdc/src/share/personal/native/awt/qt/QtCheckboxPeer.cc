/*
 * @(#)QtCheckboxPeer.cc	1.11 06/10/25
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
#include "sun_awt_qt_QtCheckboxPeer.h"
#include "QtCheckboxPeer.h"
#include "QpCheckBox.h"
#include "QpRadioButton.h"
#include "QtDisposer.h"

/**
 * Constructor for QtCheckboxPeer
 */
QtCheckboxPeer :: QtCheckboxPeer (JNIEnv *env,
                                  jobject thisObj,
                                  QpWidget* checkboxWidget)
  : QtComponentPeer(env, thisObj, checkboxWidget, checkboxWidget)
{
  this->radio = FALSE;
  if (checkboxWidget) {
    checkboxWidget->show();
  }
  /* Connect a signal so that when the button is clicked it notifies
     the EventDispatchThread with an ActionEvent. */    
  checkboxWidget->connect(SIGNAL(clicked()), this, SLOT(checkboxToggled()));
}

/**
 * Destructor of QtCheckboxPeer
 */
QtCheckboxPeer :: ~QtCheckboxPeer(void)
{
}

bool
QtCheckboxPeer :: isRadio()
{
  return this->radio;
}

void 
QtCheckboxPeer :: setRadio(bool isRadio)
{
  this->radio = isRadio;
}

void
QtCheckboxPeer :: checkboxToggled ()
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0) {
      return;
    }
    guard.setEnv(env);

    QpWidget *button = this->getWidget();

    jboolean state = JNI_FALSE;
    if ( this->isRadio() ){
        state = (jboolean)((QpRadioButton *)button)->isOn();
    }
    else {
        state = (jboolean)((QpCheckBox *)button)->isOn();
    }
        
    env->CallVoidMethod (thisObj, 
                         QtCachedIDs.QtCheckboxPeer_postItemEventMID, state);
    if (env->ExceptionCheck ())
    env->ExceptionDescribe ();

}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxPeer_initIDs (JNIEnv *env, jclass cls)
{  
  GET_METHOD_ID (QtCheckboxPeer_postItemEventMID, "postItemEvent", "(Z)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxPeer_create (JNIEnv *env, jobject thisObj, 
				       jobject parent, jboolean radio)
{
  QtCheckboxPeer *checkboxPeer;
  QpWidget *checkbox;

  // Create the label widget & then the peer object
  QpWidget *parentWidget = 0;

  if(parent) {
    QtComponentPeer *parentPeer = (QtComponentPeer *)
      env->GetIntField (parent,
                        QtCachedIDs.QtComponentPeer_dataFID);
    parentWidget = parentPeer->getWidget();
  }
    
  /* Create the checkbox Widget. This might be the Checkbox or RadioButton */  
  if (radio) {    
    checkbox = new QpRadioButton(parentWidget);
  }    
  else {
    checkbox = new QpCheckBox(parentWidget);
  }    

  if (checkbox == NULL) {
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }      

  checkbox->show();

  checkboxPeer = new QtCheckboxPeer(env, thisObj, checkbox);

  if (checkboxPeer == NULL) {    
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }
     
  /* set the boolean to indicate checkbox or radiobutton widget */
  checkboxPeer->setRadio(radio);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxPeer_setState (JNIEnv *env, 
                                         jobject thisObj, 
                                         jboolean state)
{  
    QtDisposer::Trigger guard(env);
    
  QtCheckboxPeer *checkboxPeer = 
    (QtCheckboxPeer *)QtComponentPeer::getPeerFromJNI(env,
                                                      thisObj);
    
    if (checkboxPeer->isRadio()) {
      (state == JNI_TRUE) ?
        ((QpRadioButton *)checkboxPeer->getWidget())->setChecked(TRUE):
        ((QpRadioButton *)checkboxPeer->getWidget())->setChecked(FALSE);
    }
    else {
      (state == JNI_TRUE) ?
        ((QpCheckBox *)checkboxPeer->getWidget())->setChecked(TRUE):
        ((QpCheckBox *)checkboxPeer->getWidget())->setChecked(FALSE);
    }
}
    
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxPeer_setLabelNative (JNIEnv *env, 
                                               jobject thisObj, 
                                               jstring label)
{  
    QtDisposer::Trigger guard(env);
    
   QtCheckboxPeer *checkboxPeer = 
    (QtCheckboxPeer *)QtComponentPeer::getPeerFromJNI(env,
                                                      thisObj);
    
   if (checkboxPeer == NULL) {    
     env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
     return;
   }

   if (label == NULL) {
       return;
   }
    
    /* Set the label on the label widget inside the button. */
    QString *labelString = awt_convertToQString(env, label);
    if (checkboxPeer->isRadio()) {
        ((QpRadioButton *)checkboxPeer->getWidget())->setText(*labelString);
    }
    else {
        ((QpCheckBox *)checkboxPeer->getWidget())->setText(*labelString);
    }
    delete labelString;
}
