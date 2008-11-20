/*
 * @(#)QtBackEnd.cpp	1.15 06/10/25
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

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <time.h>

#include "java_awt_QtGraphicsEnvironment.h"
#include "java_awt_QtDefaultGraphicsConfiguration.h"
#include "java_awt_Cursor.h"
#include "java_awt_AlphaComposite.h"

#include "qt.h"

#define __QT_BACKEND_CPP__

#include "QtApplication.h"
#include "QtBackEnd.h"
#include "QtScreenFactory.h"

/* temporarily for now...FIXME */
extern void dispatchMouseButtonEvent(MouseButtonEvent *qte);
extern void dispatchMouseMoveEvent(MouseMoveEvent *qte);
extern void dispatchKeyEvent(KeyboardEvent *qte);

static long getKeyCode(int qtKey);
/* the contents of the env variables is "<x>,<y>-<width>x>height>" */
#define PBP_SCREEN_BOUNDS_ENV "PBP_SCREEN_BOUNDS"

#ifdef QWS
#define PBP_FRAME_WFLAGS (Qt::WStyle_Customize|Qt::WStyle_NoBorder)
#else /* QWS */
#define PBP_FRAME_WFLAGS (Qt::WRepaintNoErase|Qt::WStyle_Customize|Qt::WStyle_NoBorder)
#endif /* QWS */

#ifdef QT_AWT_STATIC_POOL
QtImageDesc QtImageDescPool[NUM_IMAGE_DESCRIPTORS];
QtGraphDesc QtGraphDescPool[NUM_GRAPH_DESCRIPTORS];
#else
QtImageDesc *QtImageDescPool = NULL;
QtGraphDesc *QtGraphDescPool = NULL;
#endif
int NumGraphDescriptors = NUM_GRAPH_DESCRIPTORS;
int NumImageDescriptors = NUM_IMAGE_DESCRIPTORS;

QtScreen::QtScreen() {
    this->m_bounds_restricted = FALSE;
#ifndef QT_AWT_STATIC_POOL
    if ( QtImageDescPool == NULL ) {
        QtImageDescPool = (QtImageDesc *) calloc(sizeof(QtImageDesc), 
                                                 NumImageDescriptors);
        assert(QtImageDescPool != NULL);
    }
    if ( QtGraphDescPool == NULL ) {
        QtGraphDescPool = (QtGraphDesc *) calloc(sizeof(QtGraphDesc), 
                                                 NumGraphDescriptors);
        assert(QtGraphDescPool != NULL);
    }
#endif
   
}

bool
QtScreen::init()
{
    if ( qtApp != NULL ) 
        return TRUE; // already initialized

    /* 
     * We need to create an array of size 3 because the QApplication assigns a
     * NULL pointer to the argv[argc] position after it processes and removes
     * all the Qt command line options from the argv vector.  This is a Qt bug
     * to write past the end of the argv vector. Qt also keeps a refernece 
     * to argv,
     * so it shouldn't go away.
     */

    int argc = 1;
    char ** argv = this->getArgs(&argc);

    /*
     * Create a QApplication
     */
	new QtApplication(argc, argv);
	assert(qtApp != NULL);

    QRect screen_bounds = QApplication::desktop()->frameGeometry();

#ifdef QWS 
    // for zaurus, get the bounds of the screen dynamically instead of 
    // relying on awt.h values
    this->m_x      = screen_bounds.x() ;
    this->m_y      = screen_bounds.y() ;
    this->m_width  = screen_bounds.width() ;
    this->m_height = screen_bounds.height() ;
#else
    this->m_x      = 0 ;
    this->m_y      = 0 ;
    this->m_width  = SCREEN_X_RESOLUTION ;
    this->m_height = SCREEN_Y_RESOLUTION ;
#endif

    /* Set the basis frame size based on the environmental
       variable PBP_SCREEN_BOUNDS.
       Syntax should follow 0,0-640x480 for (0,0,640,480), for example 
    */
    char *userBounds = NULL;
    if((userBounds=getenv(PBP_SCREEN_BOUNDS_ENV))!=NULL) {
        int tmpx, tmpy;
        int tmpw, tmph;
        if(sscanf(userBounds, "%d,%d-%dx%d", &tmpx,&tmpy,&tmpw,&tmph) == 4) {
            QRect ubounds(tmpx, tmpy, tmpw, tmph) ;
            ubounds = screen_bounds.intersect(ubounds) ;
            if ( ubounds.isValid() && !ubounds.isNull() ) {
                this->m_x = ubounds.x() ;
                this->m_y = ubounds.y() ;
                this->m_width = ubounds.width() ;
                this->m_height = ubounds.height() ;
                this->m_bounds_restricted = TRUE;
            }
        }
    }

    /*
     * Now that we have computed the screen bounds, give the subclass a
     * chance to redefine it
     */
    this->computeBounds() ;

    /*
     * Create the Qt widget that backs the Window
     */
    this->createQtWindow();
    assert(this->m_window != NULL);

    AWT_QT_LOCK;
    qtApp->setMainWidget(this->m_window);

#ifdef QTOPIA
    qtApp->setStylusOperation(this->m_window, QPEApplication::RightOnHold);
#endif /* QTOPIA */
    AWT_QT_UNLOCK;
	
    QtGraphDescPool[0].used = 1;
    QtGraphDescPool[0].qid = 0;
    AWT_QT_LOCK;
    QtGraphDescPool[0].qp = new QPen();
    QtGraphDescPool[0].qb = new QBrush();
    AWT_QT_UNLOCK;
    QtGraphDescPool[0].blendmode = java_awt_AlphaComposite_SRC_OVER;
	
    return TRUE ;
}

void 
QtScreen::showWindow() {
    #ifdef QWS
        bool isQWS = TRUE ;
    #else
        bool isQWS = FALSE ;
    #endif //QWS

    /*
     * if we are running under QWS and the bounds are not restricted the
     * show the window as Maximized (which takes care of not going past
     * the taskbar on the QPE manager if any)
     * else we simply set the bounds and show the window
     */
    AWT_QT_LOCK;
    if ( isQWS && (!this->m_bounds_restricted) ) {
        this->m_window->showMaximized();
    }
    else {
        this->m_window->move(this->m_x, this->m_y);
        this->m_window->setFixedSize(this->m_width, this->m_height);
        this->m_window->show();
    }
    AWT_QT_UNLOCK;

    this->m_x      = this->m_window->x();
    this->m_y      = this->m_window->y();
    this->m_width  = this->m_window->width();
    this->m_height = this->m_window->height();

     /*
      * indicate to the subclass that the window is shown
      */
    this->windowShown();

    QtImageDescPool[0].width = this->m_width;	
    QtImageDescPool[0].height = this->m_height;
    QtImageDescPool[0].count = 1;
    QtImageDescPool[0].mask = this->m_window->mask();
    QtImageDescPool[0].qpd = this->m_window;
}

char **
QtScreen::getArgs(int *argc) {
    int count;
    // Use the "-sync" flag to force Xlib calls to be synchronous, which
    // would aid in debugging Xlib async errors.

    static char *argv[] = {"cvm", "-qws", NULL};

    count = 1;

#ifdef QWS
#ifndef QTOPIA
    // If environment varible QWS_CLIENT is not defined,
    // assume we are running as the server (in the qt-embedded sense).
    if (getenv("QWS_CLIENT") == NULL) {
        count++;
    }
#endif /* QTOPIA */
#endif /* QWS */

     *argc = count; 

    return (char **)&argv;
}

void
QtScreen::computeBounds() {
    // nothing special to do here
}

void
QtScreen::windowShown() {
    // nothing special to do here
}

void
QtScreen::createQtWindow() {
    AWT_QT_LOCK;
    this->m_window = new QtWindow(PBP_FRAME_WFLAGS);
    QBitmap bitmap(this->m_width, this->m_height);
    AWT_QT_UNLOCK;
    this->m_window->setMask(bitmap);
}

void 
QtScreen::setMouseCursor(int cursor)
{
	int QtCursorType;

	switch((jint)cursor) {
#if (QT_VERSION >= 0x030000)
    case java_awt_Cursor_CROSSHAIR_CURSOR:	
        QtCursorType = Qt::CrossCursor; break;
    case java_awt_Cursor_TEXT_CURSOR:	
        QtCursorType = Qt::IbeamCursor; break;
    case java_awt_Cursor_WAIT_CURSOR: 	
        QtCursorType = Qt::WaitCursor; break;
    case java_awt_Cursor_SW_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeBDiagCursor; break;
    case java_awt_Cursor_SE_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeFDiagCursor; break;
    case java_awt_Cursor_NW_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeFDiagCursor; break;
    case java_awt_Cursor_NE_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeBDiagCursor; break;
    case java_awt_Cursor_N_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeVerCursor; break;
    case java_awt_Cursor_S_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeVerCursor; break;
    case java_awt_Cursor_W_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeHorCursor; break;
    case java_awt_Cursor_E_RESIZE_CURSOR:	
        QtCursorType = Qt::SizeHorCursor; break;
    case java_awt_Cursor_HAND_CURSOR:	
        QtCursorType = Qt::PointingHandCursor; break;
    case java_awt_Cursor_MOVE_CURSOR:	
        QtCursorType = Qt::CrossCursor; break;
    default: QtCursorType = Qt::ArrowCursor; break;
#else
    case java_awt_Cursor_CROSSHAIR_CURSOR:	
        QtCursorType = CrossCursor; break;
    case java_awt_Cursor_TEXT_CURSOR:	
        QtCursorType = IbeamCursor; break;
    case java_awt_Cursor_WAIT_CURSOR: 	
        QtCursorType = WaitCursor; break;
    case java_awt_Cursor_SW_RESIZE_CURSOR:	
        QtCursorType = SizeBDiagCursor; break;
    case java_awt_Cursor_SE_RESIZE_CURSOR:	
        QtCursorType = SizeFDiagCursor; break;
    case java_awt_Cursor_NW_RESIZE_CURSOR:	
        QtCursorType = SizeFDiagCursor; break;
    case java_awt_Cursor_NE_RESIZE_CURSOR:	
        QtCursorType = SizeBDiagCursor; break;
    case java_awt_Cursor_N_RESIZE_CURSOR:	
        QtCursorType = SizeVerCursor; break;
    case java_awt_Cursor_S_RESIZE_CURSOR:	
        QtCursorType = SizeVerCursor; break;
    case java_awt_Cursor_W_RESIZE_CURSOR:	
        QtCursorType = SizeHorCursor; break;
    case java_awt_Cursor_E_RESIZE_CURSOR:	
        QtCursorType = SizeHorCursor; break;
    case java_awt_Cursor_HAND_CURSOR:	
        QtCursorType = PointingHandCursor; break;
    case java_awt_Cursor_MOVE_CURSOR:	
        QtCursorType = CrossCursor; break;
    default: QtCursorType = ArrowCursor; break;
#endif /* QT_VERSION */
	}

    AWT_QT_LOCK;
	QCursor qc = this->m_window->cursor();
	qc.setShape(QtCursorType);
	this->m_window->setCursor(qc);
    AWT_QT_UNLOCK;
}


void 
QtScreen::beep()
{

}

int 
QtScreen::x()
{
	return this->m_x;
}

int 
QtScreen::y()
{
	return this->m_y;
}

int 
QtScreen::width()
{
	return this->m_width;
}

int 
QtScreen::height()
{
	return this->m_height;
}

void 
QtScreen::close()
{
}

QtWindow *
QtScreen::window() {
    return this->m_window;
}

int 
QtScreen::dotsPerInch() {
    int width ,widthMM, calcDpi;

    AWT_QT_LOCK;
    {
    QWidget *d = QApplication::desktop();
    width = d->width();
    QPaintDeviceMetrics pdm( d );
    widthMM = pdm.widthMM();
    }
    AWT_QT_UNLOCK;
    /* 1 inch = 2.54 cm */
    calcDpi = (int) (width * 25.4) / widthMM;
    return calcDpi;
}

QtWindow::QtWindow(int flags, const char *name, QWidget *parent) : 
    QWidget( parent, name, (WFlags)flags)
{
	// install event filter...?
	// If mouse tracking is true, then mouseMoveEvent is called
	// whenever the mouse moved, even if button isn't held down.
    AWT_QT_LOCK;
	setMouseTracking(true);
    AWT_QT_UNLOCK;
	transMask = NULL;
}

void QtWindow::mousePressEvent(QMouseEvent *event)
{
	MouseButtonEvent ev;
	ev.type = EVENT_MOUSE_BUTTON_PRESSED;
	switch (event->button()) {
    case Qt::LeftButton:
        ev.button = MOUSE_BUTTON_LEFT;
        break;
    case Qt::MidButton:
        ev.button = MOUSE_BUTTON_MIDDLE;
        break;
    case Qt::RightButton:
        ev.button = MOUSE_BUTTON_RIGHT;
        break;
    default:
        ev.button = MOUSE_BUTTON_LEFT;
	}
	
	dispatchMouseButtonEvent(&ev);	

}


void QtWindow::mouseReleaseEvent(QMouseEvent *event)
{
	MouseButtonEvent ev;
	ev.type = EVENT_MOUSE_BUTTON_RELEASED;
	switch (event->button()) {
    case Qt::LeftButton:
        ev.button = MOUSE_BUTTON_LEFT;
        break;
    case Qt::MidButton:
        ev.button = MOUSE_BUTTON_MIDDLE;
        break;
    case Qt::RightButton:
        ev.button = MOUSE_BUTTON_RIGHT;
        break;
    default:
        ev.button = MOUSE_BUTTON_LEFT;
	}
	
	dispatchMouseButtonEvent(&ev);		

}


void QtWindow::mouseMoveEvent(QMouseEvent *event)
{
	MouseMoveEvent ev;
	ev.type = EVENT_MOUSE_MOVED;
	ev.x = event->x();
	ev.y = event->y();

	dispatchMouseMoveEvent(&ev);
}


void QtWindow::keyPressEvent(QKeyEvent *event)
{
	KeyboardEvent ev;
	ev.type = EVENT_KEY_PRESSED;
	ev.keyCode = getKeyCode(event->key());
    ev.keyChar = event->ascii();

	dispatchKeyEvent(&ev);
}


void QtWindow::keyReleaseEvent(QKeyEvent *event)
{
	KeyboardEvent ev;
	ev.type = EVENT_KEY_RELEASED;
	ev.keyCode = getKeyCode(event->key());
    ev.keyChar = event->ascii();
	
	dispatchKeyEvent(&ev);
}

void QtWindow::setMask(const QBitmap &bm)
{
#ifdef QT_AWT_1BIT_ALPHA
    AWT_QT_LOCK;
    {
	if(transMask)
		delete transMask;
	
	transMask = new QBitmap(bm);
	QPainter p(transMask);
	p.fillRect(0, 0, transMask->width(), transMask->height(), Qt::color1);
	
	QWidget::setMask(*transMask);
    }
    AWT_QT_UNLOCK;
#else
    this->transMask = NULL;
#endif /* QT_AWT_1BIT_ALPHA */
}

QBitmap* QtWindow::mask() const {
	return transMask;
}

/**
typedef struct
{
	long keyCode;
	int qtKey;
	int printable;
} KeymapEntry;
**/

KeymapEntry keymapTable[] =
{
	{VK_A, Qt::Key_A, TRUE},
	{VK_B, Qt::Key_B, TRUE},
	{VK_C, Qt::Key_C, TRUE},
	{VK_D, Qt::Key_D, TRUE},
	{VK_E, Qt::Key_E, TRUE},
	{VK_F, Qt::Key_F, TRUE},
	{VK_G, Qt::Key_G, TRUE},
	{VK_H, Qt::Key_H, TRUE},
	{VK_I, Qt::Key_I, TRUE},
	{VK_J, Qt::Key_J, TRUE},
	{VK_K, Qt::Key_K, TRUE},
	{VK_L, Qt::Key_L, TRUE},
	{VK_M, Qt::Key_M, TRUE},
	{VK_N, Qt::Key_N, TRUE},
	{VK_O, Qt::Key_O, TRUE},
	{VK_P, Qt::Key_P, TRUE},
	{VK_Q, Qt::Key_Q, TRUE},
	{VK_R, Qt::Key_R, TRUE},
	{VK_S, Qt::Key_S, TRUE},
	{VK_T, Qt::Key_T, TRUE},
	{VK_U, Qt::Key_U, TRUE},
	{VK_V, Qt::Key_V, TRUE},
	{VK_W, Qt::Key_W, TRUE},
	{VK_X, Qt::Key_X, TRUE},
	{VK_Y, Qt::Key_Y, TRUE},
	{VK_Z, Qt::Key_Z, TRUE},

	{VK_A, Qt::Key_A, TRUE},
	{VK_B, Qt::Key_B, TRUE},
	{VK_C, Qt::Key_C, TRUE},
	{VK_D, Qt::Key_D, TRUE},
	{VK_E, Qt::Key_E, TRUE},
	{VK_F, Qt::Key_F, TRUE},
	{VK_G, Qt::Key_G, TRUE},
	{VK_H, Qt::Key_H, TRUE},
	{VK_I, Qt::Key_I, TRUE},
	{VK_J, Qt::Key_J, TRUE},
	{VK_K, Qt::Key_K, TRUE},
	{VK_L, Qt::Key_L, TRUE},
	{VK_M, Qt::Key_M, TRUE},
	{VK_N, Qt::Key_N, TRUE},
	{VK_O, Qt::Key_O, TRUE},
	{VK_P, Qt::Key_P, TRUE},
	{VK_Q, Qt::Key_Q, TRUE},
	{VK_R, Qt::Key_R, TRUE},
	{VK_S, Qt::Key_S, TRUE},
	{VK_T, Qt::Key_T, TRUE},
	{VK_U, Qt::Key_U, TRUE},
	{VK_V, Qt::Key_V, TRUE},
	{VK_W, Qt::Key_W, TRUE},
	{VK_X, Qt::Key_X, TRUE},
	{VK_Y, Qt::Key_Y, TRUE},
	{VK_Z, Qt::Key_Z, TRUE},

	{VK_ENTER, Qt::Key_Return, TRUE},
	{VK_ENTER, Qt::Key_Enter, TRUE},

	{VK_BACK_SPACE, Qt::Key_Backspace, TRUE},
	{VK_TAB, Qt::Key_Tab, TRUE},
	{VK_CANCEL, 0, FALSE},
	{VK_CLEAR, 0, FALSE},
	{VK_SHIFT, Qt::Key_Shift, FALSE},
	{VK_CONTROL, Qt::Key_Control, FALSE},
	{VK_ALT, Qt::Key_Alt, FALSE},
	{VK_META, Qt::Key_Meta, FALSE},
	{VK_PAUSE, Qt::Key_Pause, FALSE},
	{VK_CAPS_LOCK, Qt::Key_CapsLock, FALSE},
	{VK_ESCAPE, Qt::Key_Escape, TRUE},
	{VK_SPACE, Qt::Key_Space, TRUE},

	{VK_PAGE_UP, Qt::Key_PageUp, FALSE},
	{VK_PAGE_DOWN, Qt::Key_PageDown, FALSE},
	{VK_END, Qt::Key_End, FALSE},
	{VK_HOME, Qt::Key_Home, FALSE},

	{VK_LEFT, Qt::Key_Left, FALSE},
	{VK_UP, Qt::Key_Up, FALSE},
	{VK_RIGHT, Qt::Key_Right, FALSE},
	{VK_DOWN, Qt::Key_Down, FALSE},
	{VK_INSERT, Qt::Key_Insert, FALSE},
	{VK_HELP, Qt::Key_Help, FALSE},

	//    {VK_KP_UP, XK_KP_Up, FALSE},
	//    {VK_KP_DOWN, XK_KP_Down, FALSE},
	//    {VK_KP_RIGHT, XK_KP_Right, FALSE},
	//    {VK_KP_LEFT, XK_KP_Left, FALSE},

	{VK_0, Qt::Key_0, TRUE},
	{VK_1, Qt::Key_1, TRUE},
	{VK_2, Qt::Key_2, TRUE},
	{VK_3, Qt::Key_3, TRUE},
	{VK_4, Qt::Key_4, TRUE},
	{VK_5, Qt::Key_5, TRUE},
	{VK_6, Qt::Key_6, TRUE},
	{VK_7, Qt::Key_7, TRUE},
	{VK_8, Qt::Key_8, TRUE},
	{VK_9, Qt::Key_9, TRUE},

	{VK_EQUALS, Qt::Key_Equal, TRUE},
	{VK_BACK_SLASH, Qt::Key_Backslash, TRUE},
	{VK_BACK_QUOTE, Qt::Key_QuoteLeft, TRUE},
	{							 // keyboard: [
		VK_OPEN_BRACKET, Qt::Key_BracketLeft, TRUE
	},
	{							 // keyboard: ]
		VK_CLOSE_BRACKET, Qt::Key_BracketRight, TRUE
	},
	{VK_SEMICOLON, Qt::Key_Semicolon, TRUE},
	{VK_QUOTE, Qt::Key_Apostrophe, TRUE},
	{VK_COMMA, Qt::Key_Comma, TRUE},
	{VK_MINUS, Qt::Key_Minus, TRUE},
	{VK_PERIOD, Qt::Key_Period, TRUE},
	{VK_SLASH, Qt::Key_Slash, TRUE},

	//    {VK_NUMPAD0, XK_KP_0, TRUE},
	//    {VK_NUMPAD1, XK_KP_1, TRUE},
	//    {VK_NUMPAD2, XK_KP_2, TRUE},
	//    {VK_NUMPAD3, XK_KP_3, TRUE},
	//    {VK_NUMPAD4, XK_KP_4, TRUE},
	//    {VK_NUMPAD5, XK_KP_5, TRUE},
	//    {VK_NUMPAD6, XK_KP_6, TRUE},
	//    {VK_NUMPAD7, XK_KP_7, TRUE},
	//    {VK_NUMPAD8, XK_KP_8, TRUE},
	//    {VK_NUMPAD9, XK_KP_9, TRUE},
	//    {VK_MULTIPLY, XK_KP_Multiply, TRUE},
	//    {VK_ADD, XK_KP_Add, TRUE},
	//    {VK_SUBTRACT, XK_KP_Subtract, TRUE},
	//    {VK_DECIMAL, XK_KP_Decimal, TRUE},
	//    {VK_DIVIDE, XK_KP_Divide, TRUE},
	//    {VK_EQUALS, XK_KP_Equal, TRUE},
	//    {VK_INSERT, XK_KP_Insert, FALSE},

	{VK_F1,  Qt::Key_F1,  FALSE},
	{VK_F2,  Qt::Key_F2,  FALSE},
	{VK_F3,  Qt::Key_F3,  FALSE},
	{VK_F4,  Qt::Key_F4,  FALSE},
	{VK_F5,  Qt::Key_F5,  FALSE},
	{VK_F6,  Qt::Key_F6,  FALSE},
	{VK_F7,  Qt::Key_F7,  FALSE},
	{VK_F8,  Qt::Key_F8,  FALSE},
	{VK_F9,  Qt::Key_F9,  FALSE},
	{VK_F10, Qt::Key_F10, FALSE},
	{VK_F11, Qt::Key_F11, FALSE},
	{VK_F12, Qt::Key_F12, FALSE},
	{VK_F13, Qt::Key_F13, FALSE},
	{VK_F14, Qt::Key_F14, FALSE},
	{VK_F15, Qt::Key_F15, FALSE},
	{VK_F16, Qt::Key_F16, FALSE},
	{VK_F17, Qt::Key_F17, FALSE},
	{VK_F18, Qt::Key_F18, FALSE},
	{VK_F19, Qt::Key_F19, FALSE},
	{VK_F20, Qt::Key_F20, FALSE},
	{VK_F21, Qt::Key_F21, FALSE},
	{VK_F22, Qt::Key_F22, FALSE},
	{VK_F23, Qt::Key_F23, FALSE},
	{VK_F24, Qt::Key_F24, FALSE},

	{VK_DELETE, Qt::Key_Delete, TRUE},
	//    {VK_DELETE, XK_KP_Delete, TRUE},

	{VK_NUM_LOCK, Qt::Key_NumLock, FALSE},
	{VK_SCROLL_LOCK, Qt::Key_ScrollLock, FALSE},
	{VK_PRINTSCREEN, Qt::Key_Print, FALSE},

	// js: Sun keyboard...
	{VK_AGAIN, 0, FALSE },
	{VK_UNDO, 0, FALSE },
	{VK_COPY, 0, FALSE },
	{VK_PASTE, 0, FALSE },
	{VK_CUT, 0, FALSE },
	{VK_FIND, 0, FALSE },
	{VK_PROPS, 0, FALSE },
	{VK_STOP, 0, FALSE },

	// js:
	{VK_COMPOSE, 0, FALSE },
	{VK_ALT_GRAPH, 0, FALSE },
	{VK_SEPARATER, 0, FALSE},
	{VK_FINAL, 0, FALSE},
	{VK_CONVERT, 0, FALSE},
	{VK_NONCONVERT, 0, FALSE},
	{VK_ACCEPT, 0, FALSE},
	{VK_MODECHANGE, 0, FALSE},
	{VK_LEFT_PARENTHESIS, 0, FALSE},
	{VK_RIGHT_PARENTHESIS, 0, FALSE},
	{VK_INPUT_METHOD_ON_OFF, 0, FALSE},

	{0, 0, 0}
};

static long getKeyCode(int qtKey)
{
	int i;
	for (i = 0; keymapTable[i].keyCode != 0; i++)
	{
		if (keymapTable[i].qtKey == qtKey)
		{
			return keymapTable[i].keyCode;
		}
	}
	return 0;
}



#if defined(Q_WS_X11)
#include <X11/Xlib.h>
#endif /* Q_WS_X11 */

QRgb *
defaultColorTable(int *numColors) {
    QRgb *ret = NULL;
    numColors[0] = 0;

    if ( QPixmap::defaultDepth() != 8 )
        return NULL;

    // Why are we using X11 API for default colormap ?
    //
    // When you use the Qt API
    //   QPixmap pixmap(x,y)
    //   QImage  image = pixmap.convertFromImage();
    // The pixmap contains some uninitialized data and Qt uses the 
    // default X11 color map and sets the colors in the color table
    // for the "image" that can be a subset of the default colormap.
    // In other words the QImage::colorTable() is a local table whose
    // indices may not even represent the same color in the system 
    // colormap. 
#if defined(Q_WS_X11)
    Colormap cmap   = QPaintDevice::x11AppColormap();
    int  ncells = QPaintDevice::x11AppCells();
    XColor *carr = new XColor[ncells];
    int i = 0;
    for ( i=0; i<ncells; i++ )
        carr[i].pixel = i;
    // Get default colormap
    XQueryColors( QPaintDevice::x11AppDisplay(), cmap, carr, ncells );
    ret = new QRgb[ncells];

    i = 0;
    int c = 0;
    for (c=0; c<ncells; c++ ) {
        ret[i++] = 0xff000000 | qRgb((carr[c].red   >> 8) & 255,
                                     (carr[c].green >> 8) & 255,
                                     (carr[c].blue  >> 8) & 255);
    }

    *numColors = ncells;
    delete carr;
#endif /* Q_WS_X11 */
    return ret ;
}

JNIEXPORT jobject JNICALL
Java_java_awt_QtDefaultGraphicsConfiguration_getQtColorModel (JNIEnv * env, jclass cls)
{
	jobject colorModel = NULL;
	jint rmask = 0, gmask = 0, bmask = 0, amask = 0;
	jint numbits = 0;
    
    QtScreen *screen = QtScreenFactory::getScreen();
    QtWindow *screenWindow = screen->window();
	if(screenWindow == NULL) return NULL;

    int depth = 0 ;
    AWT_QT_LOCK ; {
        QPaintDeviceMetrics pdm( screenWindow);
        depth = pdm.depth();
    }
    AWT_QT_UNLOCK;

#ifndef QT_AWT_1BIT_ALPHA
    numbits = depth ;
#endif /* QT_AWT_1BIT_ALPHA */

	switch(depth) {
    case 32:
#ifndef QT_AWT_1BIT_ALPHA
        amask = 0xff000000;
#endif /* QT_AWT_1BIT_ALPHA */
    case 24:
        rmask = 0x00ff0000;
        gmask = 0x0000ff00;
        bmask = 0x000000ff;
#ifdef QT_AWT_1BIT_ALPHA
        amask = 0x80000000;
        numbits = 32;
#endif /* QT_AWT_1BIT_ALPHA */
			break;
			
    case 16:
        rmask = 0x0000f800;
        gmask = 0x000007e0;
        bmask = 0x0000001f;
#ifdef QT_AWT_1BIT_ALPHA
        amask = 0x80000000;
        numbits = 32;
#endif /* QT_AWT_1BIT_ALPHA */
			break;
			
    case 15:
        rmask = 0x00007c00;
        gmask = 0x000003e0;
        bmask = 0x0000001f;
#ifdef QT_AWT_1BIT_ALPHA
        amask = 0x80000000;
        numbits = 32;
#endif /* QT_AWT_1BIT_ALPHA */
        break;
			
    case 8:
        QRgb *colorTable = NULL;
        int numColors = 0 ; 
        AWT_QT_LOCK;
        {
            colorTable = defaultColorTable(&numColors);
            if ( numColors > 0 ) {
                jbyteArray rarray = env->NewByteArray(numColors);
                jbyteArray garray = env->NewByteArray(numColors);
                jbyteArray barray = env->NewByteArray(numColors);
                if ( rarray != NULL && garray != NULL && barray != NULL ) {
                    jbyte rgb;
                    for (int i = 0; i < numColors; i++) {
                        rgb = (jbyte) (qRed(colorTable[i]));
                        env->SetByteArrayRegion(rarray, i, 1, &rgb);
                        rgb = (jbyte) (qGreen(colorTable[i]));
                        env->SetByteArrayRegion(garray, i, 1, &rgb);
                        rgb = (jbyte) (qBlue(colorTable[i]));
                        env->SetByteArrayRegion(barray, i, 1, &rgb);
                    }
                    colorModel = env->NewObject(QtCachedIDs.IndexColorModel,
                                    QtCachedIDs.IndexColorModel_constructor,
                                    (jint) depth,
                                    (jint) numColors,
                                    rarray,
                                    garray,
                                    barray);
                }
                delete colorTable;
            }
            else {
                printf("Unsupported Depth=8, and not Indexed Color\n");
            }
        }
        AWT_QT_UNLOCK;
        break;
	}

    //printf("Depth = %d\n", depth);

    if ( colorModel == NULL ) {
        colorModel =
        env->NewObject(QtCachedIDs.DirectColorModel,
                       QtCachedIDs.DirectColorModel_constructor,
                       numbits, rmask, gmask, bmask, amask); 
    }
	
    return colorModel;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphicsEnvironment_QTprocessEvents (JNIEnv * env, jclass cls)
{
	qtApp->processEvents();
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphicsEnvironment_QTexec(JNIEnv * env, jobject self)
{
	qtApp->exec();
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphicsEnvironment_QTshutdown(JNIEnv * env, jobject self)
{
	qtApp->shutdown();
}

