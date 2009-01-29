/*
 * @(#)QtListPeer.cc	1.14 06/10/25
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
#include "sun_awt_qt_QtListPeer.h"
#include "QtListPeer.h"
#include <qlistbox.h>
#include "QpListBox.h"
#include "QtDisposer.h"

/*
 * Constructor of the class QtListPeer
 */
QtListPeer::QtListPeer(JNIEnv* env, jobject thisObj, QpWidget* listWidget)
    : QtComponentPeer(env, thisObj, listWidget, listWidget) 
{
    if (listWidget) {
        listWidget->show();
    }
    this->itemStateList = QArray<int>(8);
    this->itemStateList.fill(0, -1);
    this->flag = TRUE;

    listWidget->connect(SIGNAL(clicked(QListBoxItem *)), this,
                       SLOT(handleClicked(QListBoxItem *)));
    listWidget->connect(SIGNAL(selectionChanged()), this,
                       SLOT(handleSelectionChanged()));
    listWidget->connect(SIGNAL(selected(QListBoxItem *)), this,
                       SLOT(handleSelected(QListBoxItem *)));

    //6265234
    // Store "viewport" to avoid calling QpListBox::viewport from 
    // QtListPeer::dispose since QtListPeer::dispose is called when
    // Qt lock accrued and calling QpListBox::viewport when Qt is locked 
    // would result in a deadlock.
    this->viewport = ((QpListBox *)listWidget)->viewport();
    //6265234
    // Associate the "viewport" to peer, since mouse events happen on the
    // viewport instead of the ListBox
    QtComponentPeer::putPeerForWidget(this->viewport, this);
}


/*
 * Destructor of the class QtListPeer
 */
QtListPeer::~QtListPeer() 
{
}

void
QtListPeer::dispose(JNIEnv *env)
{
  QtComponentPeer::removePeerForWidget(this->viewport);
  QtComponentPeer::dispose(env);
}

void 
QtListPeer::updateItemStateList(int index, int state) 
{
    if (index < 0) 
        return;
    /* Please suggest if you have better solution. Both 
       index and array size can not be negative. */
    if (index >= (int)this->itemStateList.size()) {
        this->itemStateList.resize(index+5);
    }
    (this->itemStateList)[index] = state; 
}

int
QtListPeer::getItemState(int index) 
{
    return (this->itemStateList)[index];
}

void 
QtListPeer::removeFromItemStateList(int index)
{
    (this->itemStateList)[index] = -1;
}

void 
QtListPeer::clearItemStateList() 
{
    (this->itemStateList).resize(0);
}


/*
 * Beginning Slot Functions
 */
void
QtListPeer::handleSelectionChanged()
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;  
    
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
      return;   
    guard.setEnv(env);

    for (int i=0; i< (int)this->itemStateList.size(); i++) {
        /* This item was previously selected */
        if (this->itemStateList[i] == 1) {
            if (((QpListBox*) this->getWidget())->isSelected(i) == FALSE) {
                env->CallVoidMethod (thisObj,
                                     QtCachedIDs.QtListPeer_postItemEventMID, 
                                     i, JNI_FALSE, JNI_FALSE);
                this->updateItemStateList(i, 0);
            }
        }
        /* This item was previously not selected */
        else if (this->itemStateList[i] == 0) {
            if (((QpListBox*) this->getWidget())->isSelected(i) == TRUE) {
                env->CallVoidMethod (thisObj,
                                     QtCachedIDs.QtListPeer_postItemEventMID, 
                                     i, JNI_TRUE, JNI_FALSE);
                this->updateItemStateList(i, 1);
            }     
        }
    }
    if (env->ExceptionCheck ()) { 
        env->ExceptionDescribe (); 
    } 
    flag = FALSE;
}

void
QtListPeer::handleClicked(QListBoxItem *item)
{
    if (this->flag == FALSE) {
        flag = TRUE;
        return;
    }

    int index = ((QpListBox*) this->getWidget())->index(item);
    if (index == -1) { /* item is not in the list */
        return;
    }

    /* This item was previously selected */
    if (this->itemStateList[index] == 1) {
        ((QpListBox*) this->getWidget())->clearSelection();
        this->updateItemStateList(index, 0);
        flag = TRUE;
        return;
    }
}

void
QtListPeer::handleSelected(QListBoxItem *item)
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;  
 
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
        return;   
    guard.setEnv(env);

    int index = ((QpListBox*) this->getWidget())->index(item);
    if (index == -1) { /* item is not in the list */
        return;
    }
    env->CallVoidMethod (thisObj,
                         QtCachedIDs.QtListPeer_postItemEventMID, 
                         index, JNI_TRUE, JNI_TRUE);
    
    if (env->ExceptionCheck ()) { 
        env->ExceptionDescribe (); 
    } 
}

/*
 * End Slot Functions
 */


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (QtListPeer_postItemEventMID, "postItemEvent", "(IZZ)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_create (JNIEnv *env, jobject thisObj, 
				   jobject parent)
{
    QtListPeer* listPeer;
    QpListBox* listBox;
    QpWidget* parentWidget = 0;
    
    if (parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
            env->GetIntField (parent,
                              QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
  
    listBox = new QpListBox(parentWidget);
    if (listBox == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }
    listBox->show();
    listPeer = new QtListPeer(env, thisObj, listBox);
    if (listPeer == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }
    /* initialize the QArray here */
}

JNIEXPORT jintArray JNICALL
Java_sun_awt_qt_QtListPeer_getSelectedIndexes (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);

    if (listPeer == NULL) {
        return NULL;
    }

    jintArray selected;
    int numSelected = 0;
    int numRows = 0;

    QpListBox* listBox = (QpListBox*)(listPeer->getWidget());
    numRows = listBox->numRows(); //or count() instead???
    for (int i=0; i<=numRows; i++) {
        if (listBox->isSelected(i) == TRUE) {
            ++numSelected;
            listPeer->updateItemStateList(i, 1); 
        }
        else {
            listPeer->updateItemStateList(i, 0);
        }
    }
    /* Create an int array to store the selected indices. */
    selected = env->NewIntArray (numSelected);
    
    if (selected == NULL)
        return NULL;
    
    int *buf = new int[numSelected];
    int j = 0;
    for (int i=0; i<=numRows; i++) {
        if (listBox->isSelected(i) == TRUE) {
            buf[j++] = i ;
        }
    }
    env->SetIntArrayRegion (selected, 0, numSelected, buf);

    return selected;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_addNative (JNIEnv *env, jobject thisObj, 
                                      jstring itemText, jint index)
{   
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);

    if (listPeer == NULL) {
        return;
    }

    QString* qtItemText = NULL;
    qtItemText = awt_convertToQString(env, itemText);
    if (qtItemText == NULL) {
        return;
    }
    
    QpListBox* listBox = (QpListBox*)(listPeer->getWidget());
    /* if index=-1 , then insert the item at the end of the list */
    if (index == -1) {
        index = listBox->count() + 1;
    }
    listBox->insertItem(*qtItemText, index);
    listPeer->updateItemStateList(index, 0); 
    delete qtItemText;
}
    
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_delItems (JNIEnv *env, jobject thisObj, jint start, jint end)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return;
    }

    for(int i=start; i<=end; i++) {
        ((QpListBox*)(listPeer->getWidget()))->removeItem(i);
        listPeer->removeFromItemStateList(i); 
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_removeAll (JNIEnv *env, jobject thisObj)
{  
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return;
    }

    ((QpListBox*)(listPeer->getWidget()))->clear();
    listPeer->clearItemStateList(); 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_select (JNIEnv *env, jobject thisObj, jint index)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return;
    }

    ((QpListBox*)(listPeer->getWidget()))->setSelected(index, TRUE);
    listPeer->updateItemStateList(index, 1); 
}
    
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_deselect (JNIEnv *env, jobject thisObj, jint index)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return;
    }
   
    ((QpListBox*)(listPeer->getWidget()))->setSelected(index, FALSE);
    listPeer->updateItemStateList(index, 0); 
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_makeVisible (JNIEnv *env, jobject thisObj, jint index)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return;
    }
   
    ((QpListBox*)(listPeer->getWidget()))->setCurrentItem(index);
    ((QpListBox*)(listPeer->getWidget()))->ensureCurrentVisible();
    listPeer->updateItemStateList(index, 1);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtListPeer_setMultipleMode (JNIEnv *env, jobject thisObj, 
                                            jboolean multipleMode)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return;
    }
   
    if (multipleMode == JNI_TRUE) {
        ((QpListBox*)(listPeer->getWidget()))->setSelectionMode(
                                               QListBox::Multi);
    }
    else {
        ((QpListBox*)(listPeer->getWidget()))->setSelectionMode(
                                               QListBox::Single);
    }
}
    
JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtListPeer_preferredWidth (JNIEnv *env, jobject thisObj, jint rows)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return 0;
    }
   
    return ((QpListBox*)(listPeer->getWidget()))->preferredWidth(rows);
}
    
JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtListPeer_preferredHeight (JNIEnv *env, jobject thisObj, jint rows)
{
    QtDisposer::Trigger guard(env);
    
    QtListPeer* listPeer = (QtListPeer*) QtComponentPeer::getPeerFromJNI(
        env,thisObj);
    
    if (listPeer == NULL) {
        return 0;
    }
   
    // See also wproxy/QpWidgetFactory.cc wrt "case CreateListBox" which
    // calls the qt api QListBox::setVariableHeight(FALSE) to force the
    // the QListBox to have uniform height for each list item.

    return ((QpListBox*)(listPeer->getWidget()))->preferredHeight(rows);
}
    
