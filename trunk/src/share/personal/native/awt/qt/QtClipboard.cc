/*
 * @(#)QtClipboard.cc	1.13 06/10/25
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

#include <string.h>

#include "sun_awt_qt_QtClipboard.h"
#include "awt.h"
#include "QtClipboard.h"
#include "QpClipboard.h"

// 6185915
// This macro is a place holder for implementing the qt-3.3 API
// QClipboard::ownsClipboard() for qt-emb-2.3.2.  It currently
// always returns false since the dataChanged() signal was never
// emitted for qt-emb-2.3.2 and this macro is only invoked in
// the slot method connected to this signal.
// See also QtClipboard::clipboardChanged().
#if (QT_VERSION < 0x030000)
#define OWNS_CLIPBOARD(_cb_) (_cb_ == NULL)
#endif

QtClipboard::QtClipboard(jobject obj) 
{
    this->javaObj = obj;
}

QtClipboard::~QtClipboard() 
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
	return;    
    
    env->DeleteGlobalRef(this->javaObj);
    javaObj = NULL;
}

void QtClipboard::clipboardChanged() 
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
	return;    

    QpClipboard *sysClipboard = QpClipboard::instance();
    QClipboard *clipboard = sysClipboard->getQClipboard();

    // 6185915
    // The Java API QtClipBoard.getContents() always reads through to
    // the native clipboard, so there's no need for updateContents to
    // be called from the qt native layer.  The lostOwnerShip() method,
    // however, needs to be called if the ownership loss came from the
    // native layer, for example, with xterm/selection/Copy on linux.
    //
    // qt-emb-2.3.2 does not have the QClipboard::ownsClipboard() API.
    // We use a macro as a place holder for the time being.

#if (QT_VERSION < 0x030000)
    if (OWNS_CLIPBOARD(clipboard)) {
#else
    if (!clipboard->ownsClipboard()) {
#endif
	env->CallVoidMethod(javaObj,
	                    QtCachedIDs.QtClipboard_lostOwnershipMID);
    }
    /*
    QString str = clipboard->text();

    if (str.isNull()) {
	env->CallVoidMethod(javaObj,
			    QtCachedIDs.QtClipboard_updateContentsMID,
			    NULL);
	env->CallVoidMethod(javaObj,
	                    QtCachedIDs.QtClipboard_lostOwnershipMID);
    } else {
        jstring javaStr = awt_convertToJstring(env, str);
        env->CallVoidMethod(javaObj,
                            QtCachedIDs.QtClipboard_updateContentsMID, 
                            javaStr);
    }
    */
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtClipboard_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (QtClipboard_lostOwnershipMID, "lostOwnership", "()V"); 
    // 6185915
    // The Java API QtClipBoard.getContents() always reads through to
    // the native clipboard, so there's no need for this to be called
    // from the qt native layer.
    //GET_METHOD_ID (QtClipboard_updateContentsMID, "updateContents", "(Ljava/lang/String;)V");
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtClipboard_initClipboard(JNIEnv *env, jobject thisObj)
{
    jobject javaObj = env->NewGlobalRef(thisObj);

    QtClipboard *qtClipboard = new QtClipboard(javaObj);
    QpClipboard *sysClipboard = QpClipboard::instance();

    sysClipboard->connect(SIGNAL( dataChanged() ),
                          qtClipboard, 
                          SLOT ( clipboardChanged() ));

    return (jint)qtClipboard;
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtClipboard_destroyClipboard(JNIEnv *env, jobject thisObj, jint data)
{
    if (data == 0) return;

    QtClipboard *qtClipboard = (QtClipboard *) data;
    QpClipboard *sysClipboard = QpClipboard::instance();
    
    sysClipboard->disconnect(SIGNAL( dataChanged() ),
                             qtClipboard, 
                             SLOT ( clipboardChanged() ));

    QpObject::deleteQObjectLater(qtClipboard) ;
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtClipboard_setNativeClipboard(JNIEnv *env, jobject
					       thisObj, jstring str)
{
    QpClipboard *sysClipboard = QpClipboard::instance();
    QString *qtStr = awt_convertToQString(env, str);

    // 6185915
    // See comments in QtClipboard::clipboardChanged().
#if (QT_VERSION < 0x030000)
    sysClipboard->setText(qtStr);
#else
    sysClipboard->setText(qtStr, QClipboard::Clipboard);
#endif

    delete qtStr;
}


JNIEXPORT jstring JNICALL
Java_sun_awt_qt_QtClipboard_getNativeClipboardContents(JNIEnv *env, jobject
					       thisObj)
{
    jstring text = NULL;
    QpClipboard *sysClipboard = QpClipboard::instance();
#if (QT_VERSION < 0x030000)
    QString qtStr = sysClipboard->text();
#else
    QString qtStr = sysClipboard->text(QClipboard::Clipboard);
#endif
    text = awt_convertToJstring(env, qtStr);

    //delete qtStr;
    return text;
}
