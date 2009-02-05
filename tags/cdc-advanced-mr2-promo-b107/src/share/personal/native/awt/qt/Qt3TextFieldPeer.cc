/*
 * @(#)Qt3TextFieldPeer.cc	1.16 06/10/25
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
#include "sun_awt_qt_QtTextFieldPeer.h" 
#include "Qt3TextFieldPeer.h" 
#include "QpLineEdit.h"
#include "QtDisposer.h"

/**
 * Constructor for class TextFieldPeer
 */
QtTextFieldPeer :: QtTextFieldPeer(JNIEnv* env, 
                                   jobject thisObj,
                                   QpWidget* textWidget)
  : QtTextComponentPeer(env, thisObj, textWidget)
{  
  if (textWidget) {
    textWidget->show();
  }

  textWidget->connect (SIGNAL(returnPressed()), this,
                       SLOT(keyPressed()));

  textWidget->connect (SIGNAL(textChanged(const QString&)), this,
                       SLOT(newText(const QString&)));
}

/**
 * Destructor for class TextFieldPeer
 */
QtTextFieldPeer :: ~QtTextFieldPeer(void)
{
}

bool
QtTextFieldPeer :: eventFilter( QObject *orig, QEvent *event ) 
{
  static int dWidth = 0, dHeight = 0;
  if ( dWidth == 0 && dHeight == 0 ) {
    QWidget* desktop = QApplication::desktop();
    dWidth = desktop->width();
    dHeight = desktop->height();
  }
  // If this is a paint event, which could indicate a move,
  // and our copy of the QString appears to be more recent
  // than the actual text in the QLineEdit widget, update it.
  if ( event->type() == QEvent::Paint ) {
      QpLineEdit* edit = (QpLineEdit *)this->getWidget();
      if ( edit->needsRefresh() ) {
          edit->setText( edit->getString() );
      }
  }
  return QtComponentPeer::eventFilter(orig, event);
}
/* *********************************************** */
/* PURE VIRTUAL FUNCTIONS FROM QtTextComponentPeer */
/* *********************************************** */
void
QtTextFieldPeer :: setEditable(jboolean editable)
{	
    ((QpLineEdit *)this->getWidget())->setReadOnly(!editable);
}

jstring
QtTextFieldPeer :: getTextNative(JNIEnv* env)
{
    // Use the QString from our copy of the widget's text.
    QString textString = ((QpLineEdit *)this->getWidget())->getString();
    return awt_convertToJstring(env, textString);
}

void
QtTextFieldPeer :: setTextNative(JNIEnv* env, jstring text)
{
    QString* textString = awt_convertToQString(env, text); 
    QpLineEdit* edit = (QpLineEdit *)this->getWidget();
    edit->setText(*textString);
    delete textString;
}

int 
QtTextFieldPeer :: getSelectionStart()
{
    QpLineEdit *textField = (QpLineEdit *)this->getWidget();
    if ( textField->hasSelectedText() ) {
        return textField->selectionStart() ;
    }
    // if no text has been selected, return the caret postion as in J2SE
    return textField->cursorPosition() ;
}

int 
QtTextFieldPeer :: getSelectionEnd()
{
    QpLineEdit *textField = (QpLineEdit *)this->getWidget();
    if ( textField->hasSelectedText() ) {
        QString selText = textField->selectedText() ;
      return textField->selectionStart()+selText.length() ;
    }
    // if no text has been selected, return the caret postion as in J2SE
    return textField->cursorPosition() ;
}

void
QtTextFieldPeer :: select(JNIEnv* env, jint selStart, jint selEnd)
{
  ((QpLineEdit *)this->getWidget())->setSelection(selStart, 
                                                  selEnd-selStart);
}

void
QtTextFieldPeer :: setCaretPosition(JNIEnv* env, jint pos)
{
  ((QpLineEdit *)this->getWidget())-> setCursorPosition(pos);
}

int
QtTextFieldPeer :: getCaretPosition()
{
  return ((QpLineEdit *)this->getWidget())->cursorPosition();
}

/* ***************************************************** */
/* END OF PURE VIRTUAL FUNCTIONS FROM QtTexComponentPeer */
/* ***************************************************** */

/** 
 * Slot functions
 */
bool
QtTextFieldPeer :: keyPressed (void)
{  
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env; 
    
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
      return TRUE;   
    guard.setEnv(env);

    env->CallVoidMethod (thisObj, 
                         QtCachedIDs.QtTextFieldPeer_postActionEventMID);

    if (env->ExceptionCheck ())
    env->ExceptionDescribe ();
    
    return TRUE;      
}

void
QtTextFieldPeer :: newText(const QString& nT)
{
  QtDisposer::Trigger guard;

  jobject thisObj = this->getPeerGlobalRef();
  JNIEnv *env;

  if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
    return;
  guard.setEnv(env);

  // Track the text input change
  QpLineEdit* edit = (QpLineEdit *)this->getWidget();
  edit->setString( nT );
  edit->setDirty( FALSE );
 
  env->CallVoidMethod (thisObj, 
                       QtCachedIDs.QtTextComponentPeer_postTextEventMID);  
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextFieldPeer_initIDs (JNIEnv *env, jclass cls)
{  
   GET_METHOD_ID (QtTextFieldPeer_postActionEventMID, "postActionEvent", "()V");  
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextFieldPeer_create (JNIEnv *env, jobject thisObj, 
				       jobject parent)
{  
  QtTextFieldPeer *textfieldPeer;
  QpLineEdit *textField;

  QpWidget *parentWidget = 0;

  if(parent) {
    QtComponentPeer *parentPeer = (QtComponentPeer *)
      env->GetIntField (parent,
                        QtCachedIDs.QtComponentPeer_dataFID);
    parentWidget = parentPeer->getWidget();
  }
    
  textField = new QpLineEdit(parentWidget);
  
  if (textField == NULL) {    
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }

  textfieldPeer = new QtTextFieldPeer(env, thisObj, textField);
  if (textfieldPeer == NULL) {    
    env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    return;
  }
  textField->show();  
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtTextFieldPeer_setEchoChar (JNIEnv *env, jobject thisObj, jchar c)
{  
    QtDisposer::Trigger guard(env);
    
  QtTextFieldPeer *textfieldPeer =
    (QtTextFieldPeer *)QtComponentPeer::getPeerFromJNI(env,
                                                       thisObj);
    
    if (textfieldPeer == NULL)
      return;
    
    /* Qt does not supoport setting the actual character displayed in a text field.
       Qt supports 1) NoEcho - do not display anything OR 
       2) Password - display asterisks
       if c == 0, NoEcho mode is used & if any other char - password mode is used
       So in our case we just set the mode to echo password if the echo
       character is set non zero.
    */
    if (c != 0) {
      ((QpLineEdit *)textfieldPeer->getWidget())->setEchoMode(QLineEdit::Password);
    } else {
      ((QpLineEdit *)textfieldPeer->getWidget())->setEchoMode(QLineEdit::Normal);  
    }    
}

