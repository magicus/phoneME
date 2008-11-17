/*
 * @(#)QtMenuPeer.cc	1.10 06/10/25
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
#include "sun_awt_qt_QtMenuPeer.h"
#include "QtMenuPeer.h"
#include "QtMenuBarPeer.h"
#include "QpPopupMenu.h"
#include "QpMenuBar.h"
#include "QtDisposer.h"


/*
 * QtMenuPeer Methods
 */
QtMenuPeer::QtMenuPeer (JNIEnv *env, jobject thisObj, QpWidget *menuWidget)
           :QtMenuItemPeer(env, thisObj, QtMenuItemPeer::Menu,menuWidget)
{
  jobject target = env->GetObjectField(thisObj, 
                                       QtCachedIDs.QtMenuItemPeer_targetFID);
  jboolean isTearOff = JNI_FALSE;
  
  if (env->IsInstanceOf(target, QtCachedIDs.java_awt_MenuClass)) {
      isTearOff = env->CallBooleanMethod(target, 
                       QtCachedIDs.java_awt_Menu_isTearOffMID);
  }

  if ((isTearOff == JNI_TRUE) && (menuWidget)) {
    ((QpPopupMenu *)menuWidget)->insertTearOffHandle();
  }
}

QtMenuPeer::~QtMenuPeer(void) 
{
}

void 
QtMenuPeer::addToContainer(QpWidget *menuContainer) {
    this->menuContainer = menuContainer ;
    QpMenuItemContainer *menuItemContainer = 
        (QpMenuItemContainer *)menuContainer ;
    QpMenuItemContainer::Type type =  menuItemContainer->containerType();
    if ( type == QpMenuItemContainer::MenuBar) {
        QpMenuBar *menuBar = (QpMenuBar *)menuItemContainer ;
        this->id = menuBar->insertItem("", (QpPopupMenu *)this->getWidget());
    }
    else
    if ( type == QpMenuItemContainer::PopupMenu ) {
        QpPopupMenu *menu = (QpPopupMenu *)menuItemContainer;
        this->id = menu->insertItem("", (QpPopupMenu *)this->getWidget());
    }
}

void
QtMenuPeer::removeFromContainer(void) {
    QpMenuItemContainer *menuItemContainer = 
        (QpMenuItemContainer *)this->getContainer() ;
    QpMenuItemContainer::Type type =  menuItemContainer->containerType();
    if ( type == QpMenuItemContainer::MenuBar) {
        QpMenuBar *menuBar = (QpMenuBar *)menuItemContainer;
        menuBar->removeItem(this->id);
    }
    else 
    if ( type == QpMenuItemContainer::PopupMenu ) {
        QtMenuItemPeer::removeFromContainer() ;
    }
}

void 
QtMenuPeer::setContent(QString text, bool setAccelKey, int accelKey) {
    QpMenuItemContainer *menuItemContainer = 
        (QpMenuItemContainer *)this->getContainer() ;
    QpMenuItemContainer::Type type =  menuItemContainer->containerType();
    if ( type == QpMenuItemContainer::MenuBar) {
        QpMenuBar *menuBar = (QpMenuBar *)menuItemContainer;
        if ( setAccelKey ) {
          menuBar->setAccel(accelKey, id) ;
          this->setShortcutSet((accelKey != 0));
        }
        menuBar->changeItem(id, text);
    }
    else 
    if ( type == QpMenuItemContainer::PopupMenu ) {
        QtMenuItemPeer::setContent(text, setAccelKey, accelKey) ;
    }
}

void 
QtMenuPeer::setEnabled(bool enabled) {
    QpMenuItemContainer *menuItemContainer = 
        (QpMenuItemContainer *)this->getContainer() ;
    QpMenuItemContainer::Type type =  menuItemContainer->containerType();
    if ( type == QpMenuItemContainer::MenuBar) {
        QpMenuBar *menuBar = (QpMenuBar *)menuItemContainer;
        menuBar->setItemEnabled(this->id, enabled);
    }
    else 
    if ( type == QpMenuItemContainer::PopupMenu ) {
        QtMenuItemPeer::setEnabled(enabled) ;
    }
}

void
QtMenuPeer::addToList(void *ptr) 
{
  this->list.append((void *const *) ptr); 
}

void
QtMenuPeer::propagateSettings(void)
{
    QListIterator<void *> it(list);
    for ( it.toLast(); it.current(); --it ) {
        QtMenuItemPeer *menuItemPeer = (QtMenuItemPeer *)it.current();
        QpWidget *menuItemWidget = menuItemPeer->getWidget();
        // Note : menuItemWidget is NULL for "Label", "Seperator" and 
        // "Checkbox" menu items, so we dont have to set the font. Setting
        // font on its container (QpPopupMenu) would take the effect of
        // changing the font for the items.
        if ( menuItemWidget != NULL ){
            menuItemWidget->setFont(this->getWidget()->font());
        }
        menuItemPeer->propagateSettings();
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuPeer_create (JNIEnv *env, jobject thisObj)
{
  QpPopupMenu *menu = new QpPopupMenu();
  if (menu == NULL)
  {
    env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  QtMenuPeer *menuPeer = new QtMenuPeer(env, thisObj, menu);
  if (menuPeer == NULL)
  {
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }
 
  env->SetIntField (thisObj, 
                    QtCachedIDs.QtMenuItemPeer_dataFID, 
                    (jint)menuPeer);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuPeer_add (JNIEnv *env, jobject thisObj, jobject peer)
{
  QtDisposer::Trigger guard(env);
    
  QtMenuPeer *menuPeer = (QtMenuPeer *) 
      QtMenuItemPeer::getPeerFromJNI (env, thisObj);
  QtMenuItemPeer *menuItemPeer = 
      QtMenuItemPeer::getPeerFromJNI (env, peer);
	
  if (menuPeer == NULL || menuPeer->getWidget() == NULL || 
      menuItemPeer == NULL)
  {
    env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
    return;
  }
	
  QpPopupMenu *menu = (QpPopupMenu *) menuPeer->getWidget();
  menuItemPeer->addToContainer(menu);
  menuPeer->addToList( (void *) menuItemPeer );
}
