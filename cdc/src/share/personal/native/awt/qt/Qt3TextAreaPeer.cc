/*
 * @(#)Qt3TextAreaPeer.cc	1.19 06/10/25
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
#include <string.h>

#include "jni.h"
#include "java_awt_TextArea.h"
#include "java_awt_event_MouseEvent.h"
#include "sun_awt_qt_QtTextAreaPeer.h"
#include "Qt3TextAreaPeer.h"
#include "QpTextEdit.h"
#include "QtDisposer.h"

/* 
 * Constructor of the class QtTextAreaPeer
 */
QtTextAreaPeer::QtTextAreaPeer(JNIEnv* env, jobject thisObj,
                               QpWidget* textAreaWidget)
    : QtTextComponentPeer(env, thisObj, textAreaWidget) 
{
    if(textAreaWidget) {
        textAreaWidget->show();
    }
    /* Connect slot for signal textChanged() */
    textAreaWidget->connect (SIGNAL(textChanged()), 
                             this, SLOT(textChanged()));

    //6265234
    // Store "viewport" to avoid calling QpTextEdit::viewport from 
    // QtTextAreaPeer::dispose since QtTextAreaPeer::dispose is called when
    // Qt lock accrued and calling QpTextEdit::viewport when Qt is locked 
    // would result in a deadlock.
    this->viewport = ((QpTextEdit *)textAreaWidget)->viewport();
    //6265234
    // Associate the "viewport" to peer, since mouse events happen on the
    // viewport instead of the ListBox
    QtComponentPeer::putPeerForWidget(this->viewport, this);
}

void
QtTextAreaPeer::dispose(JNIEnv *env)
{
  QtComponentPeer::removePeerForWidget(this->viewport);
  QtComponentPeer::dispose(env);
}

/*
 * Destructor of the class QtTextAreaPeer
 */
QtTextAreaPeer::~QtTextAreaPeer()
{
}

/*
 * Pure virtual functions from the parent class QtTextComponentPeer
 */
void
QtTextAreaPeer::setEditable (jboolean editable)
{
    ((QpTextEdit*) this->getWidget())->setReadOnly(!editable);
}


jstring 
QtTextAreaPeer::getTextNative (JNIEnv* env)
{
    QString textString = ((QpTextEdit *)this->getWidget())->text();
    return awt_convertToJstring(env, textString);
}

void
QtTextAreaPeer::setTextNative (JNIEnv* env, jstring text)
{
    QString* textString = awt_convertToQString(env, text);
    ((QpTextEdit*) this->getWidget())->setText(*textString);
    delete textString;
}


jint 
QtTextAreaPeer::getSelectionStart ()
{
    QpTextEdit* edit = (QpTextEdit*) this->getWidget();
    if ( edit->hasSelectedText() ) {
        return edit->getSelectionStartPosition() ;
    }
    return edit->getCursorPosition();
}

jint
QtTextAreaPeer::getSelectionEnd ()
{
    QpTextEdit* edit = (QpTextEdit*) this->getWidget();
    if ( edit->hasSelectedText() ) {
        return edit->getSelectionEndPosition() ;
    }
    return edit->getCursorPosition();
}

void
QtTextAreaPeer::select ( JNIEnv *env, jint start, jint end)
{
    QpTextEdit* edit = (QpTextEdit*) this->getWidget();
    int total_length = edit->length();
    end = (end < total_length) ? end : total_length;
    jint position = start = start < end ? start : end;

    int paraFrom, paraTo, indexFrom, indexTo ;
    paraFrom = edit->positionToParaIndex(position, &indexFrom);
    paraTo   = edit->positionToParaIndex(end, &indexTo);
    edit->setSelection(paraFrom, indexFrom, paraTo, indexTo);
}


/*
 * Sets the caret position to the location specified in position.
 */
void
QtTextAreaPeer::setCaretPosition (JNIEnv *env, jint pos)
{
    int para  = 0;
    int index = 0;

    QpTextEdit* edit = (QpTextEdit*) this->getWidget();
    para = edit->positionToParaIndex(pos, &index);
    edit->setCursorPosition(para, index);
}


jint
QtTextAreaPeer::getCaretPosition ()
{
    int para  = 0;
    int index = 0;
    
    QpTextEdit* edit = (QpTextEdit*) this->getWidget();
    edit->getCursorPosition(&para, &index);
    
    return edit->paraIndexToPosition(para, index); 
} 

/*
 * Slot function for signal textChanged
 */
void
QtTextAreaPeer :: textChanged (void)
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env; 
    
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
      return;   
    guard.setEnv(env);

    env->CallVoidMethod (thisObj, QtCachedIDs.
                         QtTextComponentPeer_postTextEventMID);

    if (env->ExceptionCheck ()) {
        env->ExceptionDescribe ();
    }
    return;      
}

/* End Pure Virtual Functions from the parent class QtTextComponentPeer */

JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtTextAreaPeer_create (JNIEnv *env, jobject thisObj, 
                                       jobject parent, jboolean wordwrap)
{
    
    QtTextAreaPeer* textAreaPeer;
    QpWidget* parentWidget = 0;
    QpTextEdit* multiLineEdit;
    
    if (parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
            env->GetIntField (parent,
                              QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
    jobject target = env->GetObjectField 
        (thisObj, QtCachedIDs.QtComponentPeer_targetFID);
    int scrollbarVisibility = (int) env->CallIntMethod(target, 
        QtCachedIDs.java_awt_TextArea_getScrollbarVisibilityMID);
    
    multiLineEdit = new QpTextEdit(parentWidget);
    
    if (multiLineEdit == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    } 

    switch (scrollbarVisibility) {
    case java_awt_TextArea_SCROLLBARS_BOTH:
        multiLineEdit->setHScrollBarMode(QScrollView::AlwaysOn);
        multiLineEdit->setVScrollBarMode(QScrollView::AlwaysOn);
        break;
    case java_awt_TextArea_SCROLLBARS_VERTICAL_ONLY: 
        if (wordwrap) {
            multiLineEdit->setWrapPolicy(QTextEdit::AtWordBoundary);
        } else {
            multiLineEdit->setWrapPolicy(QTextEdit::Anywhere);
        } 
        multiLineEdit->setWordWrap(QTextEdit::WidgetWidth);
        multiLineEdit->setHScrollBarMode(QScrollView::AlwaysOff);
        multiLineEdit->setVScrollBarMode(QScrollView::AlwaysOn);
        break;
    case java_awt_TextArea_SCROLLBARS_HORIZONTAL_ONLY:
        multiLineEdit->setHScrollBarMode(QScrollView::AlwaysOn);
        multiLineEdit->setVScrollBarMode(QScrollView::AlwaysOff);
        break;
    case java_awt_TextArea_SCROLLBARS_NONE:
        if (wordwrap) {
            multiLineEdit->setWrapPolicy(QTextEdit::AtWordBoundary);
        } else {
            multiLineEdit->setWrapPolicy(QTextEdit::Anywhere);
        } 
        multiLineEdit->setWordWrap(QTextEdit::WidgetWidth);
        multiLineEdit->setHScrollBarMode(QScrollView::AlwaysOff);
        multiLineEdit->setVScrollBarMode(QScrollView::AlwaysOff);
        break;
    }
    
    textAreaPeer = (QtTextAreaPeer*) new QtTextAreaPeer(env,  
                                                        thisObj, 
                                                        multiLineEdit);
    if (textAreaPeer == NULL) {
        env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextAreaPeer_insertNative (JNIEnv *env, 
                                             jobject thisObj, 
                                             jstring text, 
                                             jint pos)
{
    QtDisposer::Trigger guard(env);
    
    QtTextAreaPeer* peer = (QtTextAreaPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (peer == NULL)
        return;
    
    QString* qstringText;
    
    qstringText = awt_convertToQString(env, text);
    if (qstringText == NULL) {
        return;
    }

    /* Get a reference to QpTextEdit. */
    QpTextEdit* edit = (QpTextEdit*) peer->getWidget();
    edit->insertAt(pos, *qstringText) ;
    delete qstringText;
    peer->textChanged();
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtTextAreaPeer_getExtraWidth (JNIEnv *env, 
                                              jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtTextAreaPeer* peer = (QtTextAreaPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (peer == NULL)
        return (jint)0;
    
    /* Get a reference to QTextEdit. */
    QpTextEdit* edit = (QpTextEdit*) peer->getWidget();

    int w = edit->extraWidth();

    return (jint)w;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtTextAreaPeer_getExtraHeight (JNIEnv *env, 
                                               jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtTextAreaPeer* peer = (QtTextAreaPeer* )
        QtComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (peer == NULL)
        return (jint)0;
    
    /* Get a reference to QTextEdit. */
    QpTextEdit* edit = (QpTextEdit*) peer->getWidget();

    int h = edit->extraHeight();

    return (jint)h;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextAreaPeer_replaceRangeNative (JNIEnv *env, 
                                                   jobject thisObj, 
                                                   jstring text, 
                                                   jint start, 
                                                   jint end)
{
    QtDisposer::Trigger guard(env);
    
    QtTextAreaPeer* peer = (QtTextAreaPeer* )
        QtTextComponentPeer::getPeerFromJNI(env, thisObj);
    
    if (peer == NULL)
        return;
    
    QString* qstringText;
    qstringText = awt_convertToQString(env, text);
    if (qstringText == NULL) {
        return;
    }   

    QpTextEdit* edit = (QpTextEdit*) peer->getWidget();
    edit->replaceAt(*qstringText, start, end) ;

    delete qstringText;
    peer->textChanged();
}

