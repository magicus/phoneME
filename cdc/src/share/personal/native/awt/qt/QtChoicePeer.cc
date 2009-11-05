/*
 * @(#)QtChoicePeer.cc	1.13 06/10/25
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
#include "sun_awt_qt_QtChoicePeer.h"
#include "QtChoicePeer.h"
#include "QpComboBox.h"
#include "QtDisposer.h"

QtChoicePeer::QtChoicePeer( JNIEnv *env,
                            jobject thisObj,
                            QpWidget *item )
  : QtComponentPeer( env, thisObj, item, item )
{
  if ( item != NULL ) {
    item->show();
  }
  
  /* Connect a signal to the menu so we can listen in to selections. */
  item->connect(SIGNAL( activated( int ) ),
                this, SLOT( choiceSelected( int ) ) );

}

QtChoicePeer::~QtChoicePeer()
{
  // trivial destructor
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtChoicePeer_initIDs ( JNIEnv *env, jclass cls )
{
  GET_METHOD_ID (QtChoicePeer_postItemEventMID, "postItemEvent", "(I)V");

  cls = env->FindClass( "java/awt/Choice" );
  if (cls == NULL) {
    return;
  }

  GET_FIELD_ID (java_awt_Choice_selectedIndexFID, "selectedIndex", "I");

  return;
}

void
QtChoicePeer::choiceSelected ( int index )
{
  if (index != -1) {

    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
  
    if (!thisObj || JVM->AttachCurrentThread ( (void **)&env, NULL ) != 0) {
        return;
    }
    guard.setEnv(env);

    jobject target =
      env->GetObjectField( thisObj,
                           QtCachedIDs.QtComponentPeer_targetFID );
    env->SetIntField( target,
                      QtCachedIDs.java_awt_Choice_selectedIndexFID,
                      (jint)index );
    env->CallVoidMethod( thisObj,
                         QtCachedIDs.QtChoicePeer_postItemEventMID,
                         (jint)index );

    env->DeleteLocalRef(target);
  }

  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtChoicePeer_create ( JNIEnv *env, jobject thisObj, 
				       jobject parent )
{
  // Create the label widget & then the peer object
  QpWidget *parentWidget = 0;
  
  if (parent) {
      QtComponentPeer *parentPeer = (QtComponentPeer *)
	  env->GetIntField (parent,
			    QtCachedIDs.QtComponentPeer_dataFID);
      parentWidget = parentPeer->getWidget();
  }
    
  // Uses the new style QComboBox in qt 3.3.
  QpComboBox *combo = new QpComboBox(FALSE, parentWidget);
  QtChoicePeer *peer = new QtChoicePeer( env, thisObj, combo );
  
  // Check for failure
  if ( peer == NULL ) {
    env->ThrowNew( QtCachedIDs.OutOfMemoryErrorClass, NULL );
  }
  return;
}

/*
 * Add an item with text <itemText> at index <index>.
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtChoicePeer_addNative ( JNIEnv *env,
                                         jobject thisObj,
                                         jstring itemText,
                                         jint index)
{
    QtDisposer::Trigger guard(env);
    
  QtChoicePeer *peer =
    (QtChoicePeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );

  if ( peer == NULL ) {
    return;
  }
  
  QString *qItemText = awt_convertToQString(env, itemText);
  // Create a new choice entry using the index and QString
  QpComboBox *widget = (QpComboBox *)peer->getWidget();
  if ( widget != NULL ) {
    widget->insertItem( *qItemText, (int)index );
  }
  delete qItemText;

  return;
}

/*
 * Remove an item.
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtChoicePeer_removeNative ( JNIEnv *env, jobject thisObj, jint index )
{
    QtDisposer::Trigger guard(env);
    
  QtChoicePeer *peer =
    (QtChoicePeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );
  if ( peer == NULL ) {
    return;
  }
  
  QpComboBox *widget = (QpComboBox *)peer->getWidget();
  if ( widget != NULL && index > -1 ) {
    widget->removeItem( (int)index );
  }
  
  return;
}

/*
 * Select an item.
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtChoicePeer_select ( JNIEnv *env, jobject thisObj, jint index )
{
    QtDisposer::Trigger guard(env);
    
  QtChoicePeer *peer =
    (QtChoicePeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );
  if ( peer == NULL ) {
    return;
  }

  QpComboBox *widget = (QpComboBox *)peer->getWidget();
  if ( widget != NULL && index > -1 ) {
    widget->setCurrentItem( (int)index );
  }

  return;
}

