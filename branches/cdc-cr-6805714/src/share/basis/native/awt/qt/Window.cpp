/*
 * @(#)Window.cpp	1.11 05/05/02
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

#include "awt.h"

#include <stdio.h>

#include <qcolor.h>   //6221221
#include <qlabel.h>   //6221221
#include <qsizepolicy.h>   //6221221

#include "QtApplication.h" //6229858
#include "QtBackEnd.h"   //6221221

#include "java_awt_Window.h"

#include "QtScreenFactory.h"
                                                                                
// 6229858 wait until the frame is shown before we show the qt widget
JNIEXPORT void JNICALL
Java_java_awt_Window_pShow(JNIEnv *env, jobject self)
{
    AWT_QT_LOCK;
    QtScreenFactory::getScreen()->showWindow();
    AWT_QT_UNLOCK;
}
// 6229858

JNIEXPORT void JNICALL
Java_java_awt_Window_pChangeCursor(JNIEnv *env, jobject self, jint cursorType)
{
    QtScreenFactory::getScreen()->setMouseCursor(cursorType);
}

//6221221
JNIEXPORT jint JNICALL
Java_java_awt_Window_pSetWarningString(JNIEnv * env, jobject self, jstring warningString)
{
	const char *chars;
	jsize length;
	jboolean isCopy;
    jint height = 0;   // Height of label.

    QtScreen *screen = QtScreenFactory::getScreen();
    QtWindow *screenWindow = screen->window();

    // Create the warning label
    chars = env->GetStringUTFChars (warningString, &isCopy);
    if (chars == NULL) {
        return height;
    }
    length = env->GetStringUTFLength (warningString);
    
    QLabel * warningStringLabel = new QLabel(QString::fromUtf8(chars, length), screenWindow);
    warningStringLabel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
    warningStringLabel->setAlignment(Qt::AlignHCenter);

    // Set the color to slightly darker then the frame.
    QColor color = screenWindow->backgroundColor();
    QColor darkColor = color.dark(125);
    warningStringLabel->setBackgroundColor(darkColor);

    height = warningStringLabel->height();
    warningStringLabel->setGeometry(0, 
                                    screenWindow->height() - height, 
                                    screenWindow->width(), 
                                    height);
    warningStringLabel->show();
    return height;
}
