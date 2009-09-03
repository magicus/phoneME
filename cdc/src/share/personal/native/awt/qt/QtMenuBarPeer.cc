/*
 * @(#)QtMenuBarPeer.cc	1.16 06/10/25
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
#include "sun_awt_qt_QtMenuBarPeer.h"
#include "QtMenuBarPeer.h"
#include "QtMenuPeer.h"
#include "QtComponentPeer.h"
#include "QtFramePeer.h"
#include "QpMenuBar.h"
#include "QtDisposer.h"

QtMenuBarPeer::QtMenuBarPeer(JNIEnv *env, 
                             jobject thisObj, 
                             QpWidget *menuBar) : QtMenuComponentPeer(env,
                                                                      thisObj,
                                                                      menuBar)
{
}

QtMenuBarPeer::~QtMenuBarPeer(void) 
{
} 

QtMenuBarPeer *
QtMenuBarPeer::getPeerFromJNI (JNIEnv *env, jobject peer) 
{
    return (QtMenuBarPeer *)QtMenuComponentPeer::getPeerFromJNI(env, 
                             peer, 
                             QtCachedIDs.QtMenuBarPeer_dataFID);
}

void 
QtMenuBarPeer::addToList(void *ptr) 
{
  this->list.append((void *const *) ptr); 
}

void
QtMenuBarPeer::propagateSettings(void)
{

    QListIterator<void *> it(list);
    for ( it.toLast(); it.current(); --it ) {
        QtMenuPeer *menuPeer = (QtMenuPeer *)it.current();
        menuPeer->getWidget()->setFont(this->getWidget()->font());
        menuPeer->propagateSettings();
    } 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuBarPeer_initIDs (JNIEnv *env, jclass cls) 
{
  GET_FIELD_ID (QtMenuBarPeer_dataFID, "data", "I");
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuBarPeer_create (JNIEnv *env, jobject thisObj)
{
  QpMenuBar *menubar = new QpMenuBar();

  if (menubar == NULL) 	{
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  QtMenuBarPeer *menuBarPeer = new QtMenuBarPeer(env, thisObj, menubar);
  menuBarPeer->setPreferredHeight(menubar->height());

  if (menuBarPeer == NULL) 	{
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  env->SetIntField (thisObj, QtCachedIDs.QtMenuBarPeer_dataFID, (jint)menuBarPeer);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuBarPeer_dispose (JNIEnv *env, jobject thisObj) 
{
  QtDisposer::Trigger guard(env);
    
  QtMenuBarPeer *peer = QtMenuBarPeer::getPeerFromJNI (env, thisObj);

  if (peer != 0) {
	  QtFramePeer *parentFramePeer = (QtFramePeer *)
          peer->getWidget()->parentPeer();
    
	  if (parentFramePeer != 0) {
	      parentFramePeer->setMenuBarPeer(env, NULL);
      }

      env->SetIntField (thisObj, QtCachedIDs.QtMenuBarPeer_dataFID, (jint)0);
      
      QtDisposer::postDisposeRequest(peer);
  }
}

JNIEXPORT void JNICALL
// 4678754 : changed the name to addNative() from add() 
Java_sun_awt_qt_QtMenuBarPeer_addNative(JNIEnv *env, 
                                        jobject thisObj, 
                                        jobject peer) 
{
    
    QtDisposer::Trigger guard(env);
    
    QtMenuBarPeer *menuBarPeer = QtMenuBarPeer::getPeerFromJNI(env, thisObj);
    QtMenuPeer *menuPeer = (QtMenuPeer *)QtMenuItemPeer::getPeerFromJNI(env,
                                                                        peer);
	
  if (menuBarPeer == NULL || menuBarPeer->getWidget() == NULL || 
      menuPeer == NULL || menuPeer->getWidget() == NULL)
  {
    env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
    return;
  }
	
  QpMenuBar *menuBar = (QpMenuBar *)menuBarPeer->getWidget();
  menuPeer->addToContainer(menuBar);
  
  menuBarPeer->addToList( (void *) menuPeer );
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuBarPeer_setFont(JNIEnv *env, 
                                      jobject thisObj, 
                                      jobject font)
{    
    QtDisposer::Trigger guard(env);
    
    QtMenuBarPeer *menuBarPeer = QtMenuBarPeer::getPeerFromJNI(env, thisObj);

    jobject fontPeer = 
        env->GetObjectField (font,	QtCachedIDs.java_awt_Font_peerFID);
    QFont *qtfont = (QFont *) 
        env->GetIntField (fontPeer, QtCachedIDs.QtFontPeer_dataFID);

    if (menuBarPeer == NULL)  {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }

    menuBarPeer->setFont(*qtfont);
}


// 4678754 
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuBarPeer_setAsHelpMenu(JNIEnv *env, 
                                            jobject thisObj, 
                                            jobject menuPeerObj) 
{
    QtDisposer::Trigger guard(env);
    
    QtMenuBarPeer *menuBarPeer = 
        QtMenuBarPeer::getPeerFromJNI(env, thisObj);
    QtMenuPeer *menuPeer = 
        (QtMenuPeer *)QtMenuItemPeer::getPeerFromJNI(env, menuPeerObj);
	
    if (menuBarPeer == NULL || menuBarPeer->getWidget() == NULL || 
        menuPeer == NULL || menuPeer->getWidget() == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }
	
    QpMenuBar *menuBar = (QpMenuBar *)menuBarPeer->getWidget();
    menuBar->setAsHelpMenu(menuPeer->getId());
}
// 4678754 
