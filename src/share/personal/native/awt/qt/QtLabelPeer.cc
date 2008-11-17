/*
 * @(#)QtLabelPeer.cc	1.10 06/10/25
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
#include "java_awt_Label.h"
#include "sun_awt_qt_QtLabelPeer.h"
#include "QtLabelPeer.h"
#include "QpLabel.h"
#include "QtDisposer.h"

QtLabelPeer::QtLabelPeer (JNIEnv *env,
                          jobject thisObj,           
                          QpWidget *labelWidget )
  : QtComponentPeer( env, thisObj, labelWidget, labelWidget )
{
  if ( labelWidget != 0 ) {
    labelWidget->show();
  }

  return;
}

QtLabelPeer::~QtLabelPeer()
{
  // trivial destructor
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtLabelPeer_create ( JNIEnv *env, jobject thisObj , 
				     jobject parent)
{
  // Create the label widget & then the peer object
  QpWidget *parentWidget = 0;
  
  if (parent) {
      QtComponentPeer *parentPeer = (QtComponentPeer *)
	  env->GetIntField (parent,
			    QtCachedIDs.QtComponentPeer_dataFID);
      parentWidget = parentPeer->getWidget();
  }

  // Get the text for this label
  jobject javaLabel = env->GetObjectField (thisObj,
					   QtCachedIDs.QtComponentPeer_targetFID);
  jstring labelStr = (jstring) env->CallObjectMethod(javaLabel,
						     QtCachedIDs.java_awt_Label_getTextMID);
  
  QString *str = awt_convertToQString( env, labelStr );
  QpLabel *label = new QpLabel( (str == 0 ) ? QString::fromLatin1( "" ) : *str,
                                parentWidget );
  QtLabelPeer *peer = new QtLabelPeer( env, thisObj, label );

  // Check for failure
  if ( peer == 0 ) {
    env->ThrowNew( QtCachedIDs.OutOfMemoryErrorClass, NULL );
  }

  if ( str != 0 ) {
    delete str;
  }
  
  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtLabelPeer_setTextNative ( JNIEnv *env, jobject thisObj, 
					    jstring text ) 
{
    QtDisposer::Trigger guard(env);
    
    QtLabelPeer *peer = (QtLabelPeer *)
	QtComponentPeer::getPeerFromJNI( env, thisObj );
    
    if ( peer == NULL ) {
	return;
    }
    
    QString *textString = awt_convertToQString( env, text );

    QpLabel *label = (QpLabel *) peer->getWidget();
    label->setText( ( textString == NULL) ? QString::fromLatin1( "" )
		    : *textString );
    if (textString != NULL) {
	delete textString;
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtLabelPeer_setAlignmentNative ( JNIEnv *env, 
						 jobject thisObj, 
						 jint alignment ) 
{
    QtDisposer::Trigger guard(env);
    
    QtLabelPeer *peer = (QtLabelPeer *)
	QtComponentPeer::getPeerFromJNI( env, thisObj );
    
    if ( peer == NULL ) {
	return;
    }
    

    QpLabel *label = (QpLabel *) peer->getWidget();
    int qtAlignment = Qt::AlignVCenter | Qt::ExpandTabs;

    switch (alignment) {
    case java_awt_Label_LEFT:
	qtAlignment |= Qt::AlignLeft;
	break;
    case java_awt_Label_CENTER:
	qtAlignment |= Qt::AlignHCenter;
	break;
    case java_awt_Label_RIGHT:
	qtAlignment |= Qt::AlignRight;
	break;
    default: /* invalid alignment value, don't change anything */
    return ;
    }
    
    label->setAlignment( qtAlignment );
}

