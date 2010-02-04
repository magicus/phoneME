/*
 * @(#)QtToolkitEventHandler.h	1.7 06/10/25
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

#include <qpoint.h>
#include <qevent.h>
#include <qtimer.h>

#include "jni.h"
#include "QtComponentPeer.h"


class QtToolkitEventHandler : public QObject 
{
 public:
    QtToolkitEventHandler();
    bool eventFilter(QObject *obj, QEvent *evt);
    static jlong getCurrentTime(); 
    static jint getModifiers(ButtonState state);
    static void postMouseEvent (JNIEnv *env, jobject thisObj, jint id, jlong
    when, jint modifiers, jint x, jint y, jint clickCount, jboolean
    popupTrigger, void *event); 

 private:
    static QtComponentPeer *lastMousePressPeer;
    static QPoint* lastMousePressPoint;

    static void postKeyEvent (JNIEnv *env,
    QKeyEvent *event, QtComponentPeer *component); 
    static void postMouseButtonEvent (JNIEnv *env, QMouseEvent *event, QtComponentPeer *component, jboolean popupTrigger); 
    static void postMouseCrossingEvent (JNIEnv *env, QEvent *event, QtComponentPeer *component); 
    static void postMouseMovedEvent (JNIEnv *env, QMouseEvent *event, QtComponentPeer *component); 

   /* Macro required by Qt for slot. */ 
   Q_OBJECT
   /* Timer to determine mouse "dwell". */
   QTimer* stylusTimer;
   QMouseEvent* popupEvent;
   QtComponentPeer* popupComponent;
   /* Flag for popup menu. */
   bool onAPopupComp;
   int mouseMoves;

   public slots: 
      void stylusTimerDone();
};
