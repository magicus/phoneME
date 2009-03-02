/*
 * @(#)QtFramePeer.cc	1.24 06/10/25
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
#include "java_awt_Frame.h"
#include "sun_awt_qt_QtFramePeer.h"
#include "java_awt_event_WindowEvent.h"
#include "QtFramePeer.h"
#include "QtMenuBarPeer.h"
#include "QpFrame.h"
#include "QpMenuBar.h"
#include "QtDisposer.h"

// 6176847
QString *string_with_space = new QString(" ");
// 6176847

/**
 * Constructor for QtFramePeer
 */
QtFramePeer::QtFramePeer (JNIEnv *env,
			  jobject thisObj,
			  QpWidget *frame)
  : QtWindowPeer(env, thisObj, frame)
{
  this->menuBarPeer = NULL;
}

/* As the menu bar is added to the frame widget we need to do 
   the resizing of the menubar ourselves. This function is called 
   when the frame is resized so that the menu bar size is also 
   updated. We set the width of the menu bar to be the new width
   of the window. */
void 
QtFramePeer :: resize(QResizeEvent *evt)
{
  const QSize& s = evt->size();

  /*
   * Ignores Qt's null-size resize events because we are not interested in
   * the menu bar's height given that the frame has not been fully sized.
   */
  if (s.width() <= 1) {
      return;
  }
  
  QtDisposer::Trigger guard;

  jobject thisObj = this->getPeerGlobalRef();
  JNIEnv *env;
  
  if (!thisObj || JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
      return;
  }
  guard.setEnv(env);

  int newHeight;

  if (this->menuBarPeer != NULL) {       
      QpMenuBar *menuBar = (QpMenuBar *)this->menuBarPeer->getWidget();
      newHeight = menuBar->heightForWidth(s.width());
      menuBar->resize(s.width(), newHeight);
      if (newHeight != this->menuBarHeight) {
          /* Update the insets. */
          this->menuBarHeight = newHeight;

          env->CallVoidMethod (thisObj,
                               QtCachedIDs.QtFramePeer_updateMenuBarHeightMID,
                               (jint)newHeight);
      }
  }

  QtWindowPeer::resize(evt);
}

/*
 * Destructor of QtFramePeer
 */
QtFramePeer :: ~QtFramePeer(void)
{
}

QtMenuBarPeer *
QtFramePeer :: getMenuBarPeer(void)
{
    return this->menuBarPeer;
}

void 
QtFramePeer :: setMenuBarPeer(JNIEnv *env, 
                              QtMenuBarPeer *newMenuBarPeer)
{
    QtDisposer::Trigger guard(env);
    
    jobject thisObj = this->getPeerGlobalRef();
    if(!thisObj)
    {
        return;
    }

    QpMenuBar *oldMenuBar = NULL;
    QtMenuBarPeer *oldMenuBarPeer = this->getMenuBarPeer();
    
    if (oldMenuBarPeer != NULL) {
	oldMenuBar = (QpMenuBar *)oldMenuBarPeer->getWidget();
    }

    QPoint point(0, 0);

    if (oldMenuBar != NULL) {       
        /* resize/update parent frame */
        QpFrame *frame = (QpFrame *) this->getWidget();
        QRect frameRect = frame->frameRect();
        frameRect.setHeight(frameRect.height() - oldMenuBar->height());
        frame->setFrameRect(frameRect);
	
        /* remove the menubar widget from this frame */
        oldMenuBar->hide();
        oldMenuBar->reparent(0, 0, point);

        /* 
         * If we aren't going to supply a new menu bar then
         * set the menu bar height to 0 and update the insets.
         */
        if (newMenuBarPeer == NULL) {    
            env->CallVoidMethod (thisObj,
                         QtCachedIDs.QtFramePeer_updateMenuBarHeightMID,
                         (jint)0);
        }
    }
    
    this->menuBarPeer = newMenuBarPeer;
	
    /* If we are supplying a new menu bar then install it. */  
    if (newMenuBarPeer != NULL) {
        QpMenuBar *menuBar = (QpMenuBar *)newMenuBarPeer->getWidget();
	
        if (menuBar == NULL) {    
            env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
            return;
        }
	
        /* Add the menu bar to the top of the frame widget */
        menuBar->reparent(this->getWidget(), 0, point);

        /*
         * Calling show() on menuBar which also ensures that correct keyboard
         * accelerators are set up.
         */
        menuBar->show();

        /* Update the menu bar height as well as the insets. */
        //this->menuBarHeight = newMenuBarPeer->getPreferredHeight();

        // 6182409: Window.pack does not work correctly.
        // Use menuBar->heightForWidth() is more reliable than using
        // menuBarPeer->getPreferredHeight().
        this->menuBarHeight = menuBar->heightForWidth(this->getWidget()->geometry().width());

        env->CallVoidMethod (thisObj,
                             QtCachedIDs.QtFramePeer_updateMenuBarHeightMID,
                            (jint)this->menuBarHeight);
    }

}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFramePeer_initIDs (JNIEnv *env, jclass cls)
{  
    GET_METHOD_ID (QtFramePeer_updateMenuBarHeightMID,
                   "updateMenuBarHeight",
                   "(I)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFramePeer_create (JNIEnv *env, jobject thisObj, 
                                    jobject parent, jboolean isUndecorated)
{  

    QtFramePeer *framePeer;

    QpWidget *parentWidget = 0;

    if(parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
        env->GetIntField (parent,
                        QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
 
    int flags = Qt::WType_TopLevel ;

    if ( isUndecorated == JNI_TRUE ) {
        flags |= (Qt::WStyle_Customize | Qt::WStyle_NoBorder) ;
    }

    QpFrame *frame = new QpFrame(parentWidget, "QtFrame", flags);
    frame->setFrameStyle(QFrame::Panel);

    framePeer = new QtFramePeer(env, thisObj, frame);

    if (framePeer == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }

// Removed the #ifdef Q_WS_X11 around the if-block.  It now applies
// for QWS as well.
//
// 6182409: Window.pack does not work correctly.
// Guesses the borders of a decorated window.
// 6393054
    framePeer->guessBorders(env, thisObj, isUndecorated);

    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFramePeer_setTitleNative (JNIEnv *env, jobject thisObj, 
                                            jstring title)
{ 
  QtDisposer::Trigger guard(env);
    
  QtFramePeer *frame = 
    (QtFramePeer *)QtComponentPeer::getPeerFromJNI(env,
                                                   thisObj);
  if (frame == NULL) {
    return;
  }
  
  QString* titleString = awt_convertToQString(env, title);
  QString* titlePtr;

  // If the title string is empty, we create a string with a space
  // character as some window managers use the string passed to create
  // the qApplication object as the title instead of an empty string.
  // This is not desirable. (fix for bug #6176965).
  if (titleString->length() == 0)     
    titlePtr = string_with_space; // 6176847
  else titlePtr = titleString;

  frame->getWidget()->setCaption(*titlePtr);

  delete titleString;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFramePeer_setMenuBarNative (JNIEnv *env, jobject thisObj, jobject menuBarPeer)
{  
  QtFramePeer *framePeer;
  QtMenuBarPeer *qtmenuBarPeer;
  
  QtDisposer::Trigger guard(env);
  
  framePeer = (QtFramePeer *)QtComponentPeer::getPeerFromJNI(env,
                                                             thisObj);
  if (framePeer == NULL) {
    return;
  }

  if (menuBarPeer == NULL) {
      framePeer->setMenuBarPeer(env, NULL);
  } else {
      qtmenuBarPeer = QtMenuBarPeer::getPeerFromJNI(env, menuBarPeer);
      if (qtmenuBarPeer == NULL) {
	  return;
      }
      
      /* Store the menu bar in the frame's data. This is needed because we need to 
	 layout the menu bar when the frame is resized ourselves. */    
      framePeer->setMenuBarPeer(env, qtmenuBarPeer);
      
  }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFramePeer_setState (JNIEnv *env, jobject thisObj, 
				      jint state)
{
#ifndef QWS
  QtDisposer::Trigger guard(env);
    
  QtFramePeer *framePeer = 
      (QtFramePeer *)QtComponentPeer::getPeerFromJNI(env,
						     thisObj);
  if (framePeer == NULL) {
      return;
  }
  
  QpWidget *frame = framePeer->getWidget();

  if (frame == NULL) {
      return;
  }
  
  if (state == java_awt_Frame_ICONIFIED) {
      frame->showMinimized();
  } else if (state == java_awt_Frame_NORMAL) {
      frame->showNormal();
  }
#endif
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtFramePeer_getState (JNIEnv *env, jobject thisObj) 
{
  jint retVal = java_awt_Frame_NORMAL;

#ifndef QWS
  QtDisposer::Trigger guard(env);
    
  QtFramePeer *framePeer = 
      (QtFramePeer *)QtComponentPeer::getPeerFromJNI(env,
						     thisObj);
  if (framePeer == NULL) {
      return retVal;
  }
  
  QpWidget *frame = framePeer->getWidget();

  if (frame == NULL) {
      return retVal;
  }
  
  if (frame->isMinimized()) {
      retVal = java_awt_Frame_ICONIFIED;
  }

#endif

  return retVal;
}
