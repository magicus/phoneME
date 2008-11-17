/*
 * @(#)QtRobotHelper.cc	1.12 06/10/16
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
#include "QtEvent.h"
#include <qnamespace.h>
#include "QtComponentPeer.h"
#include "QpRobot.h"
#include "QtApplication.h"

/* there is a similar table in the runtime, but I have
 * duplicated it here to remove the linkage dependency */

#define SIZEOF_KEYMAP   134

typedef struct {
  jint javaKey;
  jint qtKey;
} keyMapType;
int redShift, blueShift, greenShift;
int redBits, greenBits, blueBits;
unsigned int redMask, blueMask, greenMask;

extern "C" {

// On x86 platforms with qt-3.3.3, QKeyEvent has a key value of Key_BackTab
// for Shift+Tab.  We will map this to Java's VK_TAB with Shift modifier set.

static const keyMapType myKeymapTable[SIZEOF_KEYMAP] =
{
  { java_awt_event_KeyEvent_VK_ENTER,		        Qt::Key_Return },
  { java_awt_event_KeyEvent_VK_ENTER,		        Qt::Key_Enter },
  { java_awt_event_KeyEvent_VK_BACK_SPACE,	        Qt::Key_Backspace },
  { java_awt_event_KeyEvent_VK_TAB,			Qt::Key_Tab },
  { java_awt_event_KeyEvent_VK_TAB,			Qt::Key_BackTab },
  { java_awt_event_KeyEvent_VK_CANCEL,		        0 },
  { java_awt_event_KeyEvent_VK_CLEAR,	              	0 },
  { java_awt_event_KeyEvent_VK_SHIFT,		        Qt::Key_Shift },
  { java_awt_event_KeyEvent_VK_CONTROL,		        Qt::Key_Control },
  { java_awt_event_KeyEvent_VK_ALT,			Qt::Key_Alt },
  { java_awt_event_KeyEvent_VK_PAUSE,		        Qt::Key_Pause },
  { java_awt_event_KeyEvent_VK_CAPS_LOCK,	        Qt::Key_CapsLock },
  { java_awt_event_KeyEvent_VK_ESCAPE,		        Qt::Key_Escape },
  { java_awt_event_KeyEvent_VK_SPACE,		        Qt::Key_Space },
  { java_awt_event_KeyEvent_VK_PAGE_UP,		        Qt::Key_PageUp },
  { java_awt_event_KeyEvent_VK_PAGE_DOWN,	        Qt::Key_PageDown },
  { java_awt_event_KeyEvent_VK_END,			Qt::Key_End },
  { java_awt_event_KeyEvent_VK_HOME,		        Qt::Key_Home },
  { java_awt_event_KeyEvent_VK_LEFT,		        Qt::Key_Left },
  { java_awt_event_KeyEvent_VK_UP,			Qt::Key_Up },
  { java_awt_event_KeyEvent_VK_RIGHT,		        Qt::Key_Right },
  { java_awt_event_KeyEvent_VK_DOWN,		        Qt::Key_Down },
  { java_awt_event_KeyEvent_VK_COMMA,		        Qt::Key_Comma },
  { java_awt_event_KeyEvent_VK_PERIOD,		        Qt::Key_Period },
  { java_awt_event_KeyEvent_VK_SLASH,	 	        Qt::Key_Slash },
  { java_awt_event_KeyEvent_VK_0,			Qt::Key_0 },
  { java_awt_event_KeyEvent_VK_1,			Qt::Key_1 },
  { java_awt_event_KeyEvent_VK_2,			Qt::Key_2 },
  { java_awt_event_KeyEvent_VK_3,			Qt::Key_3 },
  { java_awt_event_KeyEvent_VK_4,			Qt::Key_4 },
  { java_awt_event_KeyEvent_VK_5,			Qt::Key_5 },
  { java_awt_event_KeyEvent_VK_6,			Qt::Key_6 },
  { java_awt_event_KeyEvent_VK_7,			Qt::Key_7 },
  { java_awt_event_KeyEvent_VK_8,			Qt::Key_8 },
  { java_awt_event_KeyEvent_VK_9,			Qt::Key_9 },
  { java_awt_event_KeyEvent_VK_SEMICOLON,	        Qt::Key_Semicolon },
  { java_awt_event_KeyEvent_VK_EQUALS,		        Qt::Key_Equal },
  { java_awt_event_KeyEvent_VK_A,			Qt::Key_A },
  { java_awt_event_KeyEvent_VK_B,			Qt::Key_B },
  { java_awt_event_KeyEvent_VK_C,			Qt::Key_C },
  { java_awt_event_KeyEvent_VK_D,			Qt::Key_D },
  { java_awt_event_KeyEvent_VK_E,			Qt::Key_E },
  { java_awt_event_KeyEvent_VK_F,			Qt::Key_F },
  { java_awt_event_KeyEvent_VK_G,			Qt::Key_G },
  { java_awt_event_KeyEvent_VK_H,			Qt::Key_H },
  { java_awt_event_KeyEvent_VK_I,			Qt::Key_I },
  { java_awt_event_KeyEvent_VK_J,			Qt::Key_J },
  { java_awt_event_KeyEvent_VK_K,			Qt::Key_K },
  { java_awt_event_KeyEvent_VK_L,			Qt::Key_L },
  { java_awt_event_KeyEvent_VK_M,			Qt::Key_M },
  { java_awt_event_KeyEvent_VK_N,			Qt::Key_N },
  { java_awt_event_KeyEvent_VK_O,			Qt::Key_O },
  { java_awt_event_KeyEvent_VK_P,			Qt::Key_P },
  { java_awt_event_KeyEvent_VK_Q,			Qt::Key_Q },
  { java_awt_event_KeyEvent_VK_R,			Qt::Key_R },
  { java_awt_event_KeyEvent_VK_S,			Qt::Key_S },
  { java_awt_event_KeyEvent_VK_T,			Qt::Key_T },
  { java_awt_event_KeyEvent_VK_U,			Qt::Key_U },
  { java_awt_event_KeyEvent_VK_V,			Qt::Key_V },
  { java_awt_event_KeyEvent_VK_W,			Qt::Key_W },
  { java_awt_event_KeyEvent_VK_X,			Qt::Key_X },
  { java_awt_event_KeyEvent_VK_Y,			Qt::Key_Y },
  { java_awt_event_KeyEvent_VK_Z,			Qt::Key_Z },
  { java_awt_event_KeyEvent_VK_A,			Qt::Key_A },
  { java_awt_event_KeyEvent_VK_B,			Qt::Key_B },
  { java_awt_event_KeyEvent_VK_C,			Qt::Key_C },
  { java_awt_event_KeyEvent_VK_D,			Qt::Key_D },
  { java_awt_event_KeyEvent_VK_E,			Qt::Key_E },
  { java_awt_event_KeyEvent_VK_F,			Qt::Key_F },
  { java_awt_event_KeyEvent_VK_G,			Qt::Key_G },
  { java_awt_event_KeyEvent_VK_H,			Qt::Key_H },
  { java_awt_event_KeyEvent_VK_I,			Qt::Key_I },
  { java_awt_event_KeyEvent_VK_J,			Qt::Key_J },
  { java_awt_event_KeyEvent_VK_K,			Qt::Key_K },
  { java_awt_event_KeyEvent_VK_L,			Qt::Key_L },
  { java_awt_event_KeyEvent_VK_M,			Qt::Key_M },
  { java_awt_event_KeyEvent_VK_N,			Qt::Key_N },
  { java_awt_event_KeyEvent_VK_O,			Qt::Key_O },
  { java_awt_event_KeyEvent_VK_P,			Qt::Key_P },
  { java_awt_event_KeyEvent_VK_Q,			Qt::Key_Q },
  { java_awt_event_KeyEvent_VK_R,			Qt::Key_R },
  { java_awt_event_KeyEvent_VK_S,			Qt::Key_S },
  { java_awt_event_KeyEvent_VK_T,			Qt::Key_T },
  { java_awt_event_KeyEvent_VK_U,			Qt::Key_U },
  { java_awt_event_KeyEvent_VK_V,			Qt::Key_V },
  { java_awt_event_KeyEvent_VK_W,			Qt::Key_W },
  { java_awt_event_KeyEvent_VK_X,			Qt::Key_X },
  { java_awt_event_KeyEvent_VK_Y,			Qt::Key_Y },
  { java_awt_event_KeyEvent_VK_Z,			Qt::Key_Z },
  { java_awt_event_KeyEvent_VK_OPEN_BRACKET,            Qt::Key_BracketLeft },
  { java_awt_event_KeyEvent_VK_BACK_SLASH,	        Qt::Key_Backslash },
  { java_awt_event_KeyEvent_VK_CLOSE_BRACKET,           Qt::Key_BracketRight },
  { java_awt_event_KeyEvent_VK_NUMPAD0,		        Qt::Key_0 },
  { java_awt_event_KeyEvent_VK_NUMPAD1,		        Qt::Key_1 },
  { java_awt_event_KeyEvent_VK_NUMPAD2,		        Qt::Key_2 },
  { java_awt_event_KeyEvent_VK_NUMPAD3,		        Qt::Key_3 },
  { java_awt_event_KeyEvent_VK_NUMPAD4,		        Qt::Key_4 },
  { java_awt_event_KeyEvent_VK_NUMPAD5,		        Qt::Key_5 },
  { java_awt_event_KeyEvent_VK_NUMPAD6,		        Qt::Key_6 },
  { java_awt_event_KeyEvent_VK_NUMPAD7,		        Qt::Key_7 },
  { java_awt_event_KeyEvent_VK_NUMPAD8,		        Qt::Key_8 },
  { java_awt_event_KeyEvent_VK_NUMPAD9,		        Qt::Key_9 },
  { java_awt_event_KeyEvent_VK_MULTIPLY,	        Qt::Key_Asterisk },
  { java_awt_event_KeyEvent_VK_ADD,			Qt::Key_Plus },
  { java_awt_event_KeyEvent_VK_SEPARATER,	        0 },
  { java_awt_event_KeyEvent_VK_SUBTRACT,	        Qt::Key_Minus },
  { java_awt_event_KeyEvent_VK_DECIMAL,		        Qt::Key_Period },
  { java_awt_event_KeyEvent_VK_DIVIDE,		        Qt::Key_Slash },
  { java_awt_event_KeyEvent_VK_F1,			Qt::Key_F1 },
  { java_awt_event_KeyEvent_VK_F2,			Qt::Key_F2 },
  { java_awt_event_KeyEvent_VK_F3,			Qt::Key_F3 },
  { java_awt_event_KeyEvent_VK_F4,			Qt::Key_F4 },
  { java_awt_event_KeyEvent_VK_F5,			Qt::Key_F5 },
  { java_awt_event_KeyEvent_VK_F6,			Qt::Key_F6 },
  { java_awt_event_KeyEvent_VK_F7,			Qt::Key_F7 },
  { java_awt_event_KeyEvent_VK_F8,			Qt::Key_F8 },
  { java_awt_event_KeyEvent_VK_F9,			Qt::Key_F9 },
  { java_awt_event_KeyEvent_VK_F10,			Qt::Key_F10 },
  { java_awt_event_KeyEvent_VK_F11,			Qt::Key_F11 },
  { java_awt_event_KeyEvent_VK_F12,			Qt::Key_F12 },
  { java_awt_event_KeyEvent_VK_DELETE,		        Qt::Key_Delete },
  { java_awt_event_KeyEvent_VK_NUM_LOCK,	        Qt::Key_NumLock },
  { java_awt_event_KeyEvent_VK_SCROLL_LOCK,	        Qt::Key_ScrollLock },
  { java_awt_event_KeyEvent_VK_PRINTSCREEN,	        Qt::Key_Print },
  { java_awt_event_KeyEvent_VK_INSERT,		        Qt::Key_Insert },
  { java_awt_event_KeyEvent_VK_HELP,		        Qt::Key_Help },
  { java_awt_event_KeyEvent_VK_META,		        Qt::Key_Meta },
  { java_awt_event_KeyEvent_VK_BACK_QUOTE,	        Qt::Key_QuoteLeft },
  { java_awt_event_KeyEvent_VK_QUOTE,		        Qt::Key_QuoteLeft },
  { java_awt_event_KeyEvent_VK_FINAL,		        Qt::Key_QuoteLeft },
  { java_awt_event_KeyEvent_VK_CONVERT,		        0 },
  { java_awt_event_KeyEvent_VK_NONCONVERT,	        0 },
  { java_awt_event_KeyEvent_VK_ACCEPT,	 	        0 },
  { java_awt_event_KeyEvent_VK_MODECHANGE,	        0 }
};

/** Gets the QT key code for the java KeyEvent.VK_ code. */

int
awt_qt_getQtKeyCode (jint keyCode)
{
	int i;
	
	for (i = 0; i < SIZEOF_KEYMAP; i++)
	{
		if (myKeymapTable[i].javaKey == keyCode)
			return myKeymapTable[i].qtKey;
	}
	
	return 0;
}

/** Gets the java KeyEvent.VK_ code for the supplied QT key code. */

jint
awt_qt_getJavaKeyCode (int keyCode)
{
	int i;
	
	for (i = 0; i < SIZEOF_KEYMAP; i++)
	{
		if (myKeymapTable[i].qtKey == keyCode)
			return myKeymapTable[i].javaKey;
	}
	
	return java_awt_event_KeyEvent_VK_UNDEFINED;
}

/** Gets the unicode character for the supplied java key code and modifiers. */

jchar
awt_qt_getUnicodeChar (jint keyCode, jint modifiers)
{
	jchar c = (jchar)keyCode;

	if ((modifiers & java_awt_event_InputEvent_SHIFT_MASK) == 0)
		c = (jchar)tolower ((int)c);
	
	return c;
}


static int computeMasks();

static QpRobot *qt_robot = NULL;

JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtRobotHelper_init(JNIEnv *env,
                                   jclass clazz)
{

    if ( qt_robot == NULL ) {
        qt_robot = new QpRobot();
        qt_robot->init();
    }
    computeMasks();
}

JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtRobotHelper_doMouseActionNative(JNIEnv *env, jobject helper,
					      jint x, jint y, jint buttons,
					      jboolean pressed)
{
    qt_robot->mouseAction(x, y, buttons, (bool)pressed);
}

JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtRobotHelper_doKeyActionOnWidget(JNIEnv *env, 
                                                  jobject helper,
                                                  jint jKeyCode, 
                                                  jint widgetType,
                                                  jboolean pressed)
{
}


JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtRobotHelper_doKeyActionNative(JNIEnv *env, jobject helper,
					    jint jKeyCode, jboolean pressed)
{
    qt_robot->keyAction(awt_qt_getQtKeyCode(jKeyCode),
                        (int)awt_qt_getUnicodeChar(jKeyCode, 0),
                        (bool)pressed);
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtRobotHelper_getPixel(JNIEnv *env,
					 jobject helper,
					 jint x, jint y)
{

    AWT_QT_LOCK;
    QPixmap pm =
	QPixmap::grabWindow(QApplication::desktop()->winId());
    QImage image = pm.convertToImage();
    jint rv = ((jint)image.pixel(x, y) | 0xFF000000);
    AWT_QT_UNLOCK;
    return rv;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtRobotHelper_getPixelsNative(JNIEnv *env, jobject helper,
					  jintArray buffer,
					  jint x, jint y, jint w, jint h)
{
    jint *jarray;
    int ix, iy;
    unsigned int pixel;

    AWT_QT_LOCK;
    QPixmap pm =
	QPixmap::grabWindow(QApplication::desktop()->winId(), 
			    x, y, w, h);
    QImage image = pm.convertToImage();

    jarray = env->GetIntArrayElements(buffer, JNI_FALSE);
    for (iy=0; iy < h; iy++) {
        for (ix=0; ix < w; ix++) {
            pixel = image.pixel(ix, iy);
	    jarray[iy*w+ix] = pixel | 0xFF000000;
        }
    }
    AWT_QT_UNLOCK;
    env->ReleaseIntArrayElements(buffer, jarray, 0);
}


static int computeMasks() 
{
    redShift = greenShift= blueShift = redBits = greenBits = blueBits = 0;
    redMask = 0x00ff0000;
    greenMask = 0x0000ff00;
    blueMask = 0x000000ff;
    
    if (!(redMask && greenMask && blueMask)) {
        return 0;
    }
    while (!(redMask & 0x1)) {
        redMask >>= 1;
        redShift++;
    }
    while (redMask & 0x01) {
        redMask >>= 1;
        redBits++;
    }
    
    while (!(greenMask & 0x1)) {
        greenMask >>= 1;
        greenShift++;
    }
    while (greenMask & 0x01) {
        greenMask >>= 1;
        greenBits++;
    }
    while (!(blueMask & 0x1)) {
        blueMask >>= 1;
        blueShift++;
    }
    
    while (blueMask & 0x01) {
        blueMask >>= 1;
        blueBits++;
    }
    
    return 1;

}
}

