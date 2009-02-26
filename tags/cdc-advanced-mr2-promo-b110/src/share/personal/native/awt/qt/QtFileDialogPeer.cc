/*
 * @(#)QtFileDialogPeer.cc	1.19 06/10/25
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
#include "sun_awt_qt_QtFileDialogPeer.h"
#include "QtFileDialogPeer.h"
#include <unistd.h>
#include <qapplication.h>
#include "QpFileDialog.h"
#include "QtDisposer.h"

QtFileDialogPeer::QtFileDialogPeer( JNIEnv *env,
                                    jobject thisObj,
                                    QpWidget *w )
  : QtDialogPeer( env, thisObj, w )
{
    w->connect(SIGNAL(fileSelected(QString,QString)),
               this, SLOT(handleFileSelected(QString,QString)));
    w->connect(SIGNAL(disposed()),
               this, SLOT(handleDisposed()));

    // 6393054: Set warning height and update window
    ((QpFileDialog*)w)->setWarningLabelHeight(w->warningLabelHeight());
    w->resizeWarningLabel();
}

QtFileDialogPeer::~QtFileDialogPeer() 
{
  // trivial destructor
}

void
QtFileDialogPeer::handleFileSelected(QString dirName, QString fileName) {
    
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    
    if (!thisObj || JVM->AttachCurrentThread( (void **)&env, NULL ) != 0) {
        return;
    }
    guard.setEnv(env);

    jobject target = env->GetObjectField(thisObj,
                          QtCachedIDs.QtComponentPeer_targetFID );
    
    jstring dir = NULL, file = NULL;
    // Turn our extracted components into Java Strings
    file = env->NewStringUTF( fileName.utf8() );
    if ( file == NULL ) {
        env->ExceptionDescribe();
    }
    dir = env->NewStringUTF( dirName.utf8() );
    if ( dir == NULL ) {
        env->ExceptionDescribe();
    }
    env->SetObjectField( target,
                         QtCachedIDs.java_awt_FileDialog_fileFID, file );
    env->SetObjectField( target,QtCachedIDs.java_awt_FileDialog_dirFID, dir );

  // Fix for 4750407. Let the Java level know we're dismissed
  //env->CallVoidMethod( target, QtCachedIDs.java_awt_Dialog_hideMID );
  // 5108404 : qt could be holding the "qt-library" lock when the slot
  // function is called. We could endup in a deadlock if we called
  // Dialog.hide(), so we call QtWindowPeer.hideLater() which will invoke
  // Dialog.hide() in the event queue thread
  env->CallVoidMethod( thisObj, QtCachedIDs.QtWindowPeer_hideLaterMID );
  // 5108404
}

void
QtFileDialogPeer::handleDisposed(void) {

    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
                                                                               
    if (!thisObj || JVM->AttachCurrentThread( (void **)&env, NULL ) != 0) {
        return;
    }
    guard.setEnv(env);

    jobject target = env->GetObjectField(thisObj,
                          QtCachedIDs.QtComponentPeer_targetFID );
    env->SetObjectField( target, QtCachedIDs.java_awt_FileDialog_fileFID,
                         NULL );
    env->SetObjectField( target, QtCachedIDs.java_awt_FileDialog_dirFID,
                         NULL );
                                                                               
  // Fix for 4750407. Let the Java level know we're dismissed
  //env->CallVoidMethod( target, QtCachedIDs.java_awt_Dialog_hideMID );
  // 5108404 (See okSelected() for details)
  env->CallVoidMethod( thisObj, QtCachedIDs.QtWindowPeer_hideLaterMID );
  // 5108404
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFileDialogPeer_initIDs (JNIEnv *env, jclass cls)
{
  cls = env->FindClass( "java/awt/FileDialog" );
  if (cls == NULL) {
    printf("Trouble findClass java/awt/FileDialog.\n");
    return;
  }

  GET_FIELD_ID (java_awt_FileDialog_fileFID, "file", "Ljava/lang/String;");
  GET_FIELD_ID (java_awt_FileDialog_dirFID, "dir", "Ljava/lang/String;");

  // This is done within QtToolkit.cc.
  /*
  cls = env->FindClass( "java/awt/Dialog" );
  if (cls == NULL) {
    printf("Trouble findClass java/awt/Dialog.\n");
    return;
  }

  GET_METHOD_ID (java_awt_Dialog_hideMID, "hide", "()V" );
  */
  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFileDialogPeer_create (JNIEnv *env, jobject thisObj, 
                                         jobject parent, jboolean isUndecorated)
{
  // Note that the parent parameter is always null.
  //  QpFileDialog* qdialog = new QpFileDialog(NULL);
  QpFileDialog* qdialog;
  int           flags = Qt::WType_Modal;

  if ( isUndecorated == JNI_TRUE ) {
      flags |= (Qt::WStyle_Customize | Qt::WStyle_NoBorder);
  }
  qdialog = new QpFileDialog(NULL, NULL, flags);

  // Check for failure
  if ( qdialog == 0 ) {
    env->ThrowNew( QtCachedIDs.OutOfMemoryErrorClass, NULL );
    return;
  }

  QtFileDialogPeer *peer = new QtFileDialogPeer( env, thisObj, qdialog );

  // Check for failure
  if ( peer == 0 ) {
    env->ThrowNew( QtCachedIDs.OutOfMemoryErrorClass, NULL );
    return;
  }

  QRect qrect = qdialog->geometry();


#ifdef QWS 
   /*
   * The right way to compute Insets for a top level Window should be
   * after the window is shown using QWidget.frameGeometry() and 
   * QWidget.geometry(), but on the Zaurus both are the same after show()
   * so we use the following method to compute the Insets. These insets
   * can be used for both java.awt.Frame and java.awt.Dialog, since 
   * both have the same decoration.
   *
   * This code was taken from QtWindowPeer.cc
   */

    int currentLeft, currentRight, currentTop, currentBottom;
    if (qdialog->testWFlags(Qt::WStyle_Customize | Qt::WStyle_NoBorder)) { 
        /* undecorated frame, set insets to zero*/
        currentTop = currentLeft = currentBottom = currentRight = 0;
    } else {
        /* calculate */
        QWidget *desktop = QApplication::desktop();
        QWSDecoration &decoration = QApplication::qwsDecoration();
        QRegion region = decoration.region(desktop, desktop->geometry());
        QRect desktopWithDecorBounds = region.boundingRect();
        QRect desktopBounds = desktop->geometry() ;

        /* Note :- desktopBounds.x and y are always greater than
         * desktopWithDecorBounds.x and y respectively 
         */
        currentTop  = desktopBounds.y() - desktopWithDecorBounds.y() ;
        currentLeft = desktopBounds.x() - desktopWithDecorBounds.x() ;
        currentBottom = desktopWithDecorBounds.height() - desktopBounds.height() - currentTop ;
        currentRight = desktopWithDecorBounds.width() - desktopBounds.width() - currentLeft; 
    }

    int  w = qrect.width() + currentLeft + currentRight;
    int  h = qrect.height() + currentTop + currentBottom;
#endif

    jobject target = env->GetObjectField (thisObj,
                                        QtCachedIDs.QtComponentPeer_targetFID);

    env->SetIntField (target,
                    QtCachedIDs.java_awt_Component_widthFID,
#ifdef QWS
                    (jint) w);
#else
                    (jint) qrect.width());
#endif

    env->SetIntField (target,
                    QtCachedIDs.java_awt_Component_heightFID,
#ifdef QWS
                    (jint) h);
#else
                    (jint) qrect.height());
#endif

#ifdef QWS
    /* Update the Inset data in the java land */
    env->CallVoidMethod(thisObj, QtCachedIDs.QtWindowPeer_updateInsetsMID,
                        currentTop, currentLeft, currentBottom, currentRight);

    /* now that the frame is shown, set bounds with proper inset values */
    env->CallVoidMethod (thisObj,
                         QtCachedIDs.QtWindowPeer_restoreBoundsMID);

    if (env->ExceptionCheck())
        env->ExceptionDescribe();
#endif

    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFileDialogPeer_setFileNative (JNIEnv *env, jobject thisObj, jstring file)
{
  QtDisposer::Trigger guard(env);

  QtFileDialogPeer *peer =
    (QtFileDialogPeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );

  if ( peer == NULL ) {
    return;
  }

  QString *name = awt_convertToQString(env, file );

  ((QpFileDialog *)peer->getWidget())->setFileName(*name);

  delete name;

  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFileDialogPeer_setDirectoryNative (JNIEnv *env, jobject thisObj, jstring dir)
{
  QtDisposer::Trigger guard(env);

  QtFileDialogPeer *peer =
    (QtFileDialogPeer *)QtComponentPeer::getPeerFromJNI( env, thisObj );

  if ( peer == NULL ) {
    return;
  }

  QString *name = awt_convertToQString(env, dir );

  ((QpFileDialog *)peer->getWidget())->setDirectoryName(*name);

  delete name;

  return;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFileDialogPeer_showNative (JNIEnv *env, jobject thisObj)
{
  QtDisposer::Trigger guard(env);

  QtFileDialogPeer *peer =
    (QtFileDialogPeer *)QtComponentPeer::getPeerFromJNI (env, thisObj);

  if (peer == NULL) return;

  QpFileDialog *fdialog = (QpFileDialog *)peer->getWidget();
  fdialog->populateFileList();
  fdialog->show();
}
