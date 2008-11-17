/*
 * @(#)QtRobotHelper.cpp	1.15 06/10/16
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

#include <qconfig.h>
#include <qt.h>
#include <ctype.h>
#include "jni.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_KeyEvent.h"
#include <qnamespace.h>
#include "QpRobot.h"
#include "QtBackEnd.h"
#include "QtApplication.h"
#include <unistd.h>

int redShift, blueShift, greenShift;
int redBits, greenBits, blueBits;
unsigned int redMask, blueMask, greenMask;


extern "C" {

enum StateKey {NoKey, ShiftKey, ControlKey, AltKey};
static int stateKey;
static QWidget *mWidget;

/** Gets the QT key code for the java KeyEvent.VK_ code. */

int
awt_getQtKeyCode (jint keyCode)
{
        int i;
                                                                                
        /*for (i = 0; i < keymapTable.length; i++)*/
        for (i = 0; i < 145; i++)
        {
                if (keymapTable[i].keyCode == keyCode)
                        return keymapTable[i].qtKey;
        }
                                                                                
        return 0;

}

/** Gets the java KeyEvent.VK_ code for the supplied QT key code. */

jint
awt_getJavaKeyCode (int keyCode)
{
        int i;
                                                                                
        /*for (i = 0; i < keymapTable.length; i++)*/
        for (i = 0; i < 145; i++)
        {
                if (keymapTable[i].qtKey == keyCode)
                        return keymapTable[i].keyCode;
        }
                                                                                
        return java_awt_event_KeyEvent_VK_UNDEFINED;

}

/** Gets the unicode character for the supplied java key code and modifiers. */

jchar
awt_getUnicodeChar (jint keyCode, jint modifiers)
{
	jchar c = (jchar)keyCode;

	if ((modifiers & java_awt_event_InputEvent_SHIFT_MASK) == 0)
		c = (jchar)tolower ((int)c);
	
	return c;
}


static QpRobot *qt_robot = NULL;

JNIEXPORT void JNICALL 
Java_java_awt_QtRobotHelper_init(JNIEnv *env,
                                   jclass clazz)
{
    AWT_QT_LOCK; 
    mWidget = qApp->mainWidget();
    if (mWidget != NULL) {
        mWidget->setMouseTracking(TRUE);
    }
    AWT_QT_UNLOCK; 
    if ( qt_robot == NULL ) {
        qt_robot = new QpRobot();
        qt_robot->init();
    }
    stateKey = Qt::NoButton;
}

JNIEXPORT void JNICALL 
Java_java_awt_QtRobotHelper_doMouseActionNative(JNIEnv *env, jobject helper,
					      jint x, jint y, jint buttons,
					      jboolean pressed)
{
    qt_robot->mouseAction(x, y, buttons, (bool)pressed);
}

JNIEXPORT void JNICALL 
Java_java_awt_QtRobotHelper_doKeyActionOnWidget(JNIEnv *env, 
                                                  jobject helper,
                                                  jint jKeyCode, 
                                                  jint widgetType,
                                                  jboolean pressed)
{
}


JNIEXPORT void JNICALL 
Java_java_awt_QtRobotHelper_doKeyActionNative(JNIEnv *env, jobject helper,
					    jint jKeyCode, jboolean pressed)
{
    qt_robot->keyAction(awt_getQtKeyCode(jKeyCode),
                        (int)awt_getUnicodeChar(jKeyCode, 0),
                        (bool)pressed);
}

JNIEXPORT void JNICALL
Java_java_awt_QtRobotHelper_pCreateScreenCapture(JNIEnv *env, jclass cls, jint qtImageDescSrc, jint qtImageDescDst, jint x, jint y, jint w, jint h)
{
    AWT_QT_LOCK;
#ifdef QWS // 6246609
    // On the zaurus in landcape mode, bitblt() does not work correctly.
    // - When we pass a QWidget as the source, it assumes the entire
    //   desktop for some reasons.
    // - In landcape mode the orgin is in the botton-left and bitblt()
    //   uses the source x, y from that point which results in incorrect
    //   image. 
    // So we get the Pixmap for the desktop and use that as the source
    // which works on both Landscape and Potrait mode.
    QPixmap desktopPixmap =
        QPixmap::grabWindow(QApplication::desktop()->winId(),
                             x, y, w, h);
    QPaintDevice *qpds = &desktopPixmap;
#else  // 6246609
    QPaintDevice *qpds = QtImageDescPool[qtImageDescSrc].qpd;
#endif /* QWS */
    QPaintDevice *qpdd = QtImageDescPool[qtImageDescDst].qpd;

    bitBlt(qpdd, 0, 0, qpds, x, y, w, h, Qt::CopyROP);

    //assert(QtImageDescPool[qtImageDescDst].loadBuffer != NULL);
    *(QtImageDescPool[qtImageDescDst].loadBuffer) = *(QPixmap *)qpdd;
    AWT_QT_UNLOCK;
                                                                                
}

                                                                                
JNIEXPORT jint JNICALL
Java_java_awt_QtRobotHelper_pGetScreenPixel(JNIEnv *env, jclass cls, jint qtImageDesc, jint x, jint y)
{
    AWT_QT_LOCK;
        jint pixel = (jint)QtImageDescPool[qtImageDesc].loadBuffer->pixel(x, y);    AWT_QT_UNLOCK;
    return pixel;
}
  
}

