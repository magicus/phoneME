/*
 * @(#)Qt2TextAreaPeer.cc	1.53 06/10/25
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
#include "sun_awt_qt_QtTextAreaPeer.h"
#include "Qt2TextAreaPeer.h"
#include "QpMultiLineEdit.h"
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
    textAreaWidget->connect(SIGNAL(textChanged()), this,
                            SLOT(textChanged()));
}

/*
 * Destructor of the class QtTextAreaPeer
 */
QtTextAreaPeer::~QtTextAreaPeer(void) 
{
}

/*
 * Pure virtual functions from the parent class QtTextComponentPeer
 */
void
QtTextAreaPeer::setEditable (jboolean editable)
{
    ((QpMultiLineEdit*) this->getWidget())->setReadOnly(!editable);
}


jstring 
QtTextAreaPeer::getTextNative (JNIEnv* env)
{
    QString textString = ((QpMultiLineEdit *)this->getWidget())->text();
    return awt_convertToJstring(env, textString);
}

void
QtTextAreaPeer::setTextNative (JNIEnv* env, jstring text)
{
    QString* textString = awt_convertToQString(env, text);
    ((QpMultiLineEdit*) this->getWidget())->setText(*textString);
    delete textString;
}


jint 
QtTextAreaPeer::getSelectionStart ()
{
    jint xpos;
    jint ypos;
    jint xdummy;
    jint ydummy;
    
    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) this->getWidget();
    bool retVal = myEdit->getSelectedRegion(&xpos, &ypos, &xdummy, &ydummy);
    if (!retVal) {
        myEdit->getCursorPosition(&xpos, &ypos);
    }

    return myEdit->getPosition(xpos, ypos);
}

jint
QtTextAreaPeer::getSelectionEnd ()
{
    jint xpos;
    jint ypos;
    jint xdummy;
    jint ydummy;

    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) this->getWidget();
    bool retVal = myEdit->getSelectedRegion(&xdummy, &ydummy, &xpos, &ypos);
    if (!retVal) {
        myEdit->getCursorPosition(&xpos, &ypos);
    }
    
    return myEdit->getPosition(xpos, ypos);
}

void
QtTextAreaPeer::select ( JNIEnv *env, jint start, jint end)
{
    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) this->getWidget();

    int total_length = myEdit->length();
    end = (end < total_length) ? end : total_length;
    jint position = start = start < end ? start : end;
    
    int rowfrom, colfrom, rowto, colto;
    myEdit->getRowCol(position, &rowfrom, &colfrom);
    myEdit->getRowCol(end, &rowto, &colto);
    myEdit->setSelection(rowfrom, colfrom, rowto, colto);
}


/*
 * Sets the caret position to the location specified in position.
 */
void
QtTextAreaPeer::setCaretPosition (JNIEnv *env, jint pos)
{
    int lineLength = 0;
    int row = 0;
    int col = 0;

    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) this->getWidget();
    lineLength = myEdit->charsInLine();

    /* Calculate the row and column from the position. */
    myEdit->getRowCol(pos, &row, &col);
    myEdit->setCursorPosition(row, col, FALSE);
}


jint
QtTextAreaPeer::getCaretPosition ()
{
    jint line;
    jint col;
    
    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) this->getWidget();
    myEdit->getCursorPosition(&line, &col);
    
    return myEdit->getPosition(line, col); 
} 

/* End Pure Virtual Functions from the parent class QtTextComponentPeer */

JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtTextAreaPeer_create (JNIEnv *env, jobject thisObj, 
				       jobject parent, jboolean wordwrap)
{
    
    QtTextAreaPeer* textAreaPeer;
    QpWidget* parentWidget = 0;
    QpMultiLineEdit* multiLineEdit;
    
    if (parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
            env->GetIntField (parent,
                              QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
    jobject target = env->GetObjectField 
        (thisObj, QtCachedIDs.QtComponentPeer_targetFID);
    int scrollbarVisibility = (int) env->CallIntMethod(target, QtCachedIDs.java_awt_TextArea_getScrollbarVisibilityMID);
    
    multiLineEdit = new QpMultiLineEdit(parentWidget, 0);
    
    if (multiLineEdit == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    } 

    switch (scrollbarVisibility) {
    case java_awt_TextArea_SCROLLBARS_BOTH:
    {
        multiLineEdit->setTableOptions(Tbl_vScrollBar | 
                                       Tbl_hScrollBar |
                                       Tbl_smoothScrolling);
    }
    break;
    case java_awt_TextArea_SCROLLBARS_VERTICAL_ONLY: 
    {
        if (wordwrap) {
            multiLineEdit->setWrapPolicy(QMultiLineEdit::AtWhiteSpace);
        } else {
	    multiLineEdit->setWrapPolicy(QMultiLineEdit::Anywhere);
        }
        multiLineEdit->setWordWrap(QMultiLineEdit::WidgetWidth);
        multiLineEdit->setTableOptions(Tbl_vScrollBar | 
                                       Tbl_smoothVScrolling);
    }
    break;
    case java_awt_TextArea_SCROLLBARS_HORIZONTAL_ONLY:
        multiLineEdit->setTableOptions(Tbl_hScrollBar |
                                       Tbl_smoothHScrolling);
        break;
    case java_awt_TextArea_SCROLLBARS_NONE:
        if (wordwrap) {
            multiLineEdit->setWrapPolicy(QMultiLineEdit::AtWhiteSpace);
        } else {
	    multiLineEdit->setWrapPolicy(QMultiLineEdit::Anywhere);
        }
        multiLineEdit->setWordWrap(QMultiLineEdit::WidgetWidth);
        multiLineEdit->clearTableOptions(Tbl_autoScrollBars);
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
    
    int lineLength = 0;
    QString* qstringText;
    
    qstringText = awt_convertToQString(env, text);
    if (qstringText == NULL) {
        return;
    }

    /* Get a reference to QMultiLineEdit. */
    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) peer->getWidget();

    lineLength = myEdit->charsInLine();
    int row = 0;
    int col = 0;

    /* Calculate the row and column from the position. */
    myEdit->getRowCol(pos, &row, &col);

    bool visible = myEdit->rowIsVisible(row);

    if (visible) {
        myEdit->setAutoUpdate(FALSE);
    }
    myEdit->insertAt(*qstringText, row , col, FALSE);
    if (visible) {
        myEdit->setAutoUpdate(TRUE);
        myEdit->update();
    }

    /* Now set the caret to the appropriate position. */
    peer->setCaretPosition(env, pos + qstringText->length());

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
    
    /* Get a reference to QMultiLineEdit. */
    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) peer->getWidget();

    int w = myEdit->extraWidth();

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
    
    /* Get a reference to QMultiLineEdit. */
    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) peer->getWidget();

    int h = myEdit->extraHeight();

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

    QpMultiLineEdit* myEdit = (QpMultiLineEdit*) peer->getWidget();

    int total_length = myEdit->length();
    end = (end < total_length) ? end : total_length;
    jint position = start = start < end ? start : end;

    int rowfrom, colfrom, rowto, colto;
    myEdit->getRowCol(position, &rowfrom, &colfrom);
    myEdit->getRowCol(end, &rowto, &colto);
    
    qstringText = awt_convertToQString(env, text);
    if (qstringText == NULL) {
        return;
    }   
    myEdit->setSelection(rowfrom, colfrom, rowto, colto);

    bool visible = myEdit->rowIsVisible(rowfrom) | myEdit->rowIsVisible(rowto);

    if (visible) {
        myEdit->setAutoUpdate(FALSE);
    }
    // myEdit->cut();
    /* call deleteSelectedText instead of cut - cut modifies the contents of
     * the clipboard, which we don't want to do.  But, we don't want to cal
     * deleteSelectedText() if there's nothing selected - the side effect
     * would be that the character to the right of the cursor would be 
     * deleted
     */
    if (!(start == end))
       myEdit->deleteSelectedText();

    myEdit->insert(*qstringText);
    if (visible) {
        myEdit->setAutoUpdate(TRUE);
        myEdit->update();
    }

    delete qstringText;
    peer->textChanged();
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
