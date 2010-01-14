/*
 * @(#)QtDialogPeer.cc	1.18 06/10/25
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
#include "sun_awt_qt_QtDialogPeer.h"
#include "QtDialogPeer.h"
#include "QpFrame.h"
#include "QtDisposer.h"
#include <qframe.h>

// 6176847
extern QString *string_with_space;
// 6176847

QtDialogPeer::QtDialogPeer( JNIEnv *env,
                            jobject thisObj,
                            QpWidget *w )
  : QtWindowPeer( env, thisObj, w )
{
    /*
     if ( w != NULL ) {
         w->show();
     }
    */
}

QtDialogPeer::~QtDialogPeer() 
{
  // trivial destructor
}

void
QtDialogPeer::setTitle( QString titleStr )
{
  title = titleStr;
  return;
}

void
QtDialogPeer::setModal( bool yesNo )
{
  isModal = yesNo;
  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtDialogPeer_create (JNIEnv *env, jobject thisObj, 
                                     jobject parent, jboolean isUndecorated)
{
  QpWidget *parentWidget = 0;

  if ( parent != 0 ) {
    QtComponentPeer *parentPeer = (QtComponentPeer *)
      env->GetIntField (parent,
                        QtCachedIDs.QtComponentPeer_dataFID);
    parentWidget = parentPeer->getWidget();
  }
  
  jobject target = env->GetObjectField(thisObj, 
                                       QtCachedIDs.QtComponentPeer_targetFID );
  jboolean modal = env->GetBooleanField(target, 
                                        QtCachedIDs.java_awt_Dialog_modalFID);

  QpFrame *windowFrame;
  int flags = (modal == JNI_TRUE) ? Qt::WType_Modal : Qt::WType_TopLevel;

  if ( isUndecorated == JNI_TRUE ) {
      flags |= (Qt::WStyle_Customize | Qt::WStyle_NoBorder) ;
  }

  windowFrame = new QpFrame(parentWidget, "QtDialog", flags);

  windowFrame->setFrameStyle( QFrame::Panel | QFrame::Plain );
  windowFrame->setLineWidth( 0 );
  QtDialogPeer *peer = new QtDialogPeer( env, thisObj, windowFrame );
  
  // Check for failure
  if ( peer == 0 ) {
    env->ThrowNew( QtCachedIDs.OutOfMemoryErrorClass, NULL );
  }
  
// Removed the #ifdef Q_WS_X11 around the if-block.  It now applies
// for QWS as well.
//
// 6182409: Window.pack does not work correctly.
// Guesses the borders of a decorated window.
// 6393054
  peer->guessBorders(env, thisObj, isUndecorated);

  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtDialogPeer_setTitleNative (JNIEnv *env, jobject thisObj, 
                                             jstring title)
{
    QtDisposer::Trigger guard(env);
    
  QtDialogPeer *peer =
    (QtDialogPeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );
  if ( peer == NULL ) {
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

  peer->setTitle(*titleString);

  peer->getWidget()->setCaption( *titlePtr );

  delete titleString;
  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtDialogPeer_setModal (JNIEnv *env, jobject thisObj, jboolean modal)
{
    QtDisposer::Trigger guard(env);
    
  QtDialogPeer *peer =
    (QtDialogPeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );
  if ( peer == NULL ) {
    return;
  }

  peer->setModal( modal == JNI_TRUE );
  return;
}
