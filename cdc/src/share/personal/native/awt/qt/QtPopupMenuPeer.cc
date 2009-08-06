/*
 * @(#)QtPopupMenuPeer.cc	1.11 06/10/25
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
#include "sun_awt_qt_QtPopupMenuPeer.h"
#include "QtPopupMenuPeer.h"
#include "QpPopupMenu.h"
#include "QtDisposer.h"
#include <qframe.h>

QtPopupMenuPeer::QtPopupMenuPeer(JNIEnv *env, 
                                 jobject thisObj, QpWidget *popupMenu)
                :QtMenuPeer(env, thisObj, popupMenu)
{
    this->label = NULL ;
}

QtPopupMenuPeer::~QtPopupMenuPeer(void)
{
    if ( this->label != NULL ) {
        /* delete the memory for the proxy object, the QLabel contained
         * within the proxy object would be deleted by the QPopupMenu.
         * doing a "this->label->deleteLater()" would result in a double
         * free and would cause a crash.
         */
        delete this->label;
        this->label = NULL;
    }
}

void
QtPopupMenuPeer::removeFromContainer(void) {
    // since QtMenuItemPeer::dispose() calls this method we just want
    // a no-op implementation
}

void 
QtPopupMenuPeer::setContent(QString text, bool setAccelKey, int accelKey) {
    // must be a stand-alone popup menu without a container
    // these popups can't set their own labels, so we must do it
    // like this
    if (!text.isEmpty()) {
        QpPopupMenu *menu = (QpPopupMenu *)this->getWidget();
        if ( this->label == NULL ) 
            this->label = new QpLabel(text,menu);
        if (this->label != NULL) {
            this->label->setFrameStyle(menu->frameStyle() | 
                                       QFrame::PopupPanel);
            this->label->setFont(menu->font());
            this->id = menu->insertItem(label);
            menu->insertSeparator();
        }
    }
}

void 
QtPopupMenuPeer::setEnabled(bool enabled) {
    /* we support a stand-alone popup menu by setting the popup's     */
    /* first menu item as just a label with a NULL widget. But, any   */
    /* use of its id number in a qt method will result in a seg fault.*/
}

void 
QtPopupMenuPeer::setFont(QFont font) {
    QpPopupMenu *menu = (QpPopupMenu *)this->getWidget();
    menu->setFont(font);
    if (this->label != NULL) {
      this->label->setFont(font);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtPopupMenuPeer_create (JNIEnv *env, jobject thisObj)
{
  QpPopupMenu *popupMenu = new QpPopupMenu();
  if (popupMenu == NULL)
  {
    env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  QtPopupMenuPeer *popupMenuPeer = new QtPopupMenuPeer(env, 
                                                       thisObj, 
                                                       popupMenu);
  if (popupMenuPeer == NULL)
  {
    env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }
	
  env->SetIntField (thisObj, QtCachedIDs.QtMenuItemPeer_dataFID, (jint)popupMenuPeer);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtPopupMenuPeer_show (JNIEnv *env, jobject thisObj, jint x, jint y)
{
  QtDisposer::Trigger guard(env);

  QtPopupMenuPeer *popupMenuPeer = (QtPopupMenuPeer *)
      QtMenuItemPeer::getPeerFromJNI (env, thisObj);

  QpPopupMenu *menu = NULL ;
  if (popupMenuPeer == NULL || 
      (menu = (QpPopupMenu *)popupMenuPeer->getWidget()) == NULL) {
    env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
    return;
  }

  /* algorithm to determine the best location of the popup */
  
  int left = (int) x;
  int top = (int) y;

  /* The commented out part below is for putting the popup menu at the
     left of the stylus. It is backed out for now.
  */

//    int width = popupMenuPeer->getWidget()->width();
//    int height = popupMenuPeer->getWidget()->height();
  
//    /* the left part */
//    if (left - width > 0) {
//        left -= width;
//    } else {
//        left = 0;
//    }
  
//    /* the bottom part */
//    int desktopHeight = QApplication::desktop()->height();
//    if (top + height > desktopHeight) {
//        top = desktopHeight - height;
//    }

  menu->popup(QPoint(left, top));
}
