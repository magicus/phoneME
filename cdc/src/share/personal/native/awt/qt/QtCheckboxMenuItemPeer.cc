/*
 * @(#)QtCheckboxMenuItemPeer.cc	1.12 06/10/25
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
#include "sun_awt_qt_QtCheckboxMenuItemPeer.h"
#include "QtCheckboxMenuItemPeer.h"
#include "QpPopupMenu.h"
#include "QtDisposer.h"

void 
QtCheckboxMenuItemPeer::addToContainer(QpWidget *menuContainer) {
    QpPopupMenu *menu = (QpPopupMenu *)menuContainer;
    switch (this->type) {
    case CheckBox :
        this->id = menu->insertItem("");
        menu->setItemChecked(this->id, TRUE);
        menu->connectItem(this->id, 
                          ((QtCheckboxMenuItemPeer *) this), 
                          SLOT(handleActivated(int)));
        break ;
    default :
        break;
    }

    // let the super class do its own processing.
    QtMenuItemPeer::addToContainer(menuContainer);
}

/*
 * Contructor of the class QtCheckboxMenuItemPeer
 */
QtCheckboxMenuItemPeer :: QtCheckboxMenuItemPeer(JNIEnv* env, jobject thisObj)
    : QtMenuItemPeer(env, thisObj, QtMenuItemPeer::CheckBox)
{

 /*
  QObject::connect(checkboxMenuItem, SIGNAL(activated(int)), this, 
                   SLOT(handleActivated(int)));
  */
}

/*
 * Destructor of the class QtCheckboxMenuItemPeer
 */
QtCheckboxMenuItemPeer::~QtCheckboxMenuItemPeer(void) 
{
}

/*
 * Private Slot for signal activated(int)
 */
void
QtCheckboxMenuItemPeer::handleActivated (int thisId) 
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    jobject target;

    if (!thisObj || JVM->AttachCurrentThread((void **) &env, NULL) != 0)
        return;  
    guard.setEnv(env);

    QpPopupMenu *menu = (QpPopupMenu *) this->getContainer();
    int itemId = this->getId();

    // Toggles the checkbox menu item state.
    jboolean state =
        (menu->isItemChecked(thisId) == TRUE) ? JNI_FALSE : JNI_TRUE;

    menu->setItemChecked(itemId, state);

    target = env->GetObjectField(thisObj, 
                                 QtCachedIDs.QtMenuItemPeer_targetFID);
    env->SetBooleanField(target, 
                         QtCachedIDs.java_awt_CheckboxMenuItem_stateFID,
                         state);
    env->CallVoidMethod(thisObj, QtCachedIDs.
                        QtCheckboxMenuItemPeer_postItemEventMID, state);
  
    env->DeleteLocalRef(target);

    if (env->ExceptionCheck ()) {
        env->ExceptionDescribe (); 
    }

} 


/*
 * Beginning JNI functions implementation
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxMenuItemPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (QtCheckboxMenuItemPeer_postItemEventMID, "postItemEvent", 
                   "(Z)V");
    cls = env->FindClass ("java/awt/CheckboxMenuItem");
    if (cls == NULL)
        return;
    GET_FIELD_ID (java_awt_CheckboxMenuItem_stateFID, "state", "Z")
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxMenuItemPeer_create (JNIEnv *env, jobject thisObj)
{
  QtCheckboxMenuItemPeer* checkboxMenuItemPeer;
    
  checkboxMenuItemPeer = new QtCheckboxMenuItemPeer(env, thisObj);
  
  if (checkboxMenuItemPeer == NULL)
  {
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }    

  env->SetIntField (thisObj, 
                    QtCachedIDs.QtMenuItemPeer_dataFID, 
                    (jint)checkboxMenuItemPeer);
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCheckboxMenuItemPeer_setState (JNIEnv *env, 
                                                 jobject thisObj, 
                                                 jboolean state)
{
    QtDisposer::Trigger guard(env);
    
    QtCheckboxMenuItemPeer* checkboxMenuItemPeer = (QtCheckboxMenuItemPeer*) 
        QtMenuItemPeer::getPeerFromJNI(env, thisObj);

    QpPopupMenu *menu = (QpPopupMenu *)
        checkboxMenuItemPeer->getContainer();
    int id = checkboxMenuItemPeer->getId();
    /* printf("setState: id -> %d   state -> %d\n", id, state); */
    menu->setItemChecked(id, state);
}
/*
 * End JNI functions implementation
 */
