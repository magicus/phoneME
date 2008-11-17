/*
 * @(#)QtMenuItemPeer.cc	1.15 06/10/25
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

#include <jni.h>
#include "sun_awt_qt_QtMenuItemPeer.h"
#include "KeyCodes.h"
#include "QtComponentPeer.h"
#include "QtMenuItemPeer.h"
#include <qnamespace.h>
#include "QpPopupMenu.h"
#include "QtDisposer.h"

QtMenuItemPeer::QtMenuItemPeer(JNIEnv *env, 
                               jobject thisObj, 
                               QtMenuItemPeer::ItemType type,
                               QpWidget *menuItemWidget) :
    QtMenuComponentPeer(env, thisObj, menuItemWidget) {
    this->type = type;
    this->hasShortcutBeenSet = FALSE;
}

QtMenuItemPeer::~QtMenuItemPeer(void) 
{
}

QtMenuItemPeer *
QtMenuItemPeer::getPeerFromJNI (JNIEnv *env, jobject peer) 
{
    return (QtMenuItemPeer *)QtMenuComponentPeer::getPeerFromJNI(env, 
                             peer, 
                             QtCachedIDs.QtMenuItemPeer_dataFID);
}
void
QtMenuItemPeer::propagateSettings(void)
{
}

void 
QtMenuItemPeer::addToContainer(QpWidget *menuContainer) {
    this->menuContainer = menuContainer ;
    QpPopupMenu *menu = (QpPopupMenu *)menuContainer;
    switch (this->type) {
    case Separator :
        this->id = menu->insertSeparator();
        break ;
    case Label :
        this->id = menu->insertItem("");
        menu->connectItem(id, this, SLOT(menuItemSelected()));
        break ;
    default :
        break;
    }
}

void 
QtMenuItemPeer::removeFromContainer(void) {
    QpPopupMenu *menu = (QpPopupMenu *)this->getContainer() ;
    menu->removeItem(this->id);
}

void 
QtMenuItemPeer::setContent(QString text, bool setAccelKey, int accelKey) {
    QpPopupMenu *menu = (QpPopupMenu *)this->getContainer() ;
    if ( setAccelKey ) {
        menu->setAccel(accelKey, this->id) ;
        this->setShortcutSet((accelKey != 0));
    }
    menu->changeItem(this->id, text);
}

void 
QtMenuItemPeer::setEnabled(bool enabled) {
    QpPopupMenu *menu = (QpPopupMenu *) this->getContainer();
    menu->setItemEnabled(this->id, enabled);
}

void 
QtMenuItemPeer::setFont(QFont font) {
    switch (this->type) {
    case Separator :
    case Label :
    case CheckBox :
        /* for the above types we dont support setting of Font. As per
         * the java spec, a java.awt.MenuItem.setFont() can fail silently.
         * If there is a need then we can use QCustomMenuItem() which 
         * can render the text in a different font
         */
        break;
    case Menu :
    case Widget:
        QtMenuComponentPeer::setFont(font);
        break;
    default :
        break;
    }
}

/* Called when the menu item is selected. Post an event to the
   AWT event dispatch thread for processing. */
void
QtMenuItemPeer::menuItemSelected ()
{
  QtDisposer::Trigger guard;

  jobject thisObj = this->getPeerGlobalRef();
  JNIEnv *env;
	
  if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
    return;
  guard.setEnv(env);

  env->CallVoidMethod (thisObj, 
                       QtCachedIDs.QtMenuItemPeer_postActionEventMID);
	
  if (env->ExceptionCheck())
    env->ExceptionDescribe ();
}

QpWidget *
QtMenuItemPeer::getContainer() {
    return this->menuContainer;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuItemPeer_initIDs (JNIEnv *env, jclass cls)
{
  GET_FIELD_ID (QtMenuItemPeer_targetFID, "target", "Ljava/awt/MenuItem;");
  GET_FIELD_ID (QtMenuItemPeer_dataFID, "data", "I");
  GET_FIELD_ID (QtMenuItemPeer_menuBarFID, "menuBar", "Lsun/awt/qt/QtMenuBarPeer;");
  GET_METHOD_ID (QtMenuItemPeer_postActionEventMID, "postActionEvent", "()V");
	
  cls = env->FindClass ("java/awt/MenuItem");
  if (cls == NULL)
    return;
		
  GET_FIELD_ID (java_awt_MenuItem_shortcutFID, "shortcut", "Ljava/awt/MenuShortcut;");
	
  cls = env->FindClass ("java/awt/MenuShortcut");
	
  if (cls == NULL)
    return;
		
  GET_FIELD_ID (java_awt_MenuShortcut_keyFID, "key", "I");
  GET_FIELD_ID (java_awt_MenuShortcut_usesShiftFID, "usesShift", "Z");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuItemPeer_create (JNIEnv *env, jobject thisObj, jboolean isSeparator)
{
    QtMenuItemPeer::ItemType type = 
        (isSeparator) ? QtMenuItemPeer::Separator : QtMenuItemPeer::Label ;
    QtMenuItemPeer *menuItemPeer = new QtMenuItemPeer(env, thisObj, type) ;
 
    if (menuItemPeer == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }

    env->SetIntField (thisObj, 
                      QtCachedIDs.QtMenuItemPeer_dataFID, (jint)menuItemPeer);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuItemPeer_dispose (JNIEnv *env, jobject thisObj)
{
  QtDisposer::Trigger guard(env);
    
  QtMenuItemPeer *peer = QtMenuItemPeer::getPeerFromJNI(env, 
                                                        thisObj);
  
  if (peer != 0) {
    env->SetIntField (thisObj, QtCachedIDs.QtMenuItemPeer_dataFID, (jint)0);    
    // remove the menu item from the container
    peer->removeFromContainer();
    QtDisposer::postDisposeRequest (peer);
  }	

}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuItemPeer_setLabelNative (JNIEnv *env, jobject thisObj, 
                                               jstring label)
{
    QtDisposer::Trigger guard(env);
    
    QtMenuItemPeer *menuItemPeer = QtMenuItemPeer::getPeerFromJNI(env, 
                                                                thisObj);	
    if (menuItemPeer == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }

    QString *str = awt_convertToQString(env, label);

    // Obtain shortcut information
    jobject target = 
        env->GetObjectField (thisObj, 
                             QtCachedIDs.QtMenuItemPeer_targetFID);
    jobject shortcut = 
        env->GetObjectField (target,
                             QtCachedIDs.java_awt_MenuItem_shortcutFID);


    int accelKey = 0 ;
    int setAccelKey = FALSE ; // Indicates if key accelerator should be 
                              // (re)set for the menu ite,
    if  (shortcut != NULL) {
        jint key = env->GetIntField (shortcut, 
                        QtCachedIDs.java_awt_MenuShortcut_keyFID);
        accelKey = awt_qt_getQtKeyCode(key) + Qt::CTRL ;
        if ( env->GetBooleanField(shortcut, 
             QtCachedIDs.java_awt_MenuShortcut_usesShiftFID) == JNI_TRUE) {
            accelKey += Qt::SHIFT ;
        }
        setAccelKey = TRUE ;
    }
    else 
    if (menuItemPeer->isShortcutSet()) {
        // no shortcut specified, but one was specified previously, so we
        // need to reset the accelerator key
        setAccelKey = TRUE ;
    }

    menuItemPeer->setContent(*str, setAccelKey, accelKey) ;
    delete(str);
 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuItemPeer_setEnabled (JNIEnv *env, jobject thisObj, jboolean enabled)
{
    QtDisposer::Trigger guard(env);
    
    QtMenuItemPeer *menuItemPeer = QtMenuItemPeer::getPeerFromJNI(env, 
                                                                  thisObj);	
    
    if (menuItemPeer == NULL){
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }

    menuItemPeer->setEnabled((bool)enabled);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtMenuItemPeer_setFont(JNIEnv *env, 
                                       jobject thisObj, 
                                       jobject font)
{
    QtDisposer::Trigger guard(env);
    
    QtMenuItemPeer *menuItemPeer = QtMenuItemPeer::getPeerFromJNI(env, 
                                                                  thisObj);	

    jobject fontPeer = 
        env->GetObjectField (font,	QtCachedIDs.java_awt_Font_peerFID);
    QFont *qtfont = (QFont *) 
        env->GetIntField (fontPeer, QtCachedIDs.QtFontPeer_dataFID);

    if (menuItemPeer == NULL)  {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }

    menuItemPeer->setFont(*qtfont);
}

int
QtMenuItemPeer::getId() 
{
  return this->id;
}

QtMenuItemPeer::ItemType
QtMenuItemPeer::getType() 
{
  return this->type;
}

bool
QtMenuItemPeer::isShortcutSet()
{
  return this->hasShortcutBeenSet;
}

void
QtMenuItemPeer::setShortcutSet(bool val)
{
  this->hasShortcutBeenSet = val;
}
