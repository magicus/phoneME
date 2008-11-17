/*
 * @(#)QpRobot.cc	1.1 05/01/07
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
#include "QpRobot.h"
#include "QtEvent.h"
#include <qwidgetlist.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qcursor.h>
#include <stdio.h>
#include "java_awt_event_InputEvent.h"

typedef struct {
    struct {
        int x;
        int y;
        int buttons;
        bool pressed;
    } in ;
} qtMargsMouseAction;
        
typedef struct {
    struct {
        int keycode;
        int unicode;
        bool pressed;
    } in ;
} qtMargsKeyAction;
        
enum StateKey {NoKey, ShiftKey, ControlKey, AltKey};
static int stateKey;
static QWidget *mWidget;
static QWidget *widget;

QpRobot::QpRobot(QpRobot *parent, char *name, int flags) {
    this->m_qobject = NULL;
    this->m_metaKeyTyped = FALSE;
    this->m_metaKey = Qt::NoButton;
}

QpRobot::~QpRobot(void) {
}

void 
QpRobot::init() {
    invokeAndWait(QpRobot::Init, NULL);
}

void
QpRobot::mouseAction(int x, int y, int buttons, bool pressed) {
    QT_METHOD_ARGS_ALLOC(qtMargsMouseAction, argp);
    argp->in.x = x;
    argp->in.y = y;
    argp->in.buttons = buttons;
    argp->in.pressed = pressed;
    invokeAndWait(QpRobot::MouseAction, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpRobot::keyAction(int keycode, int unicode, bool pressed) {
    QT_METHOD_ARGS_ALLOC(qtMargsKeyAction, argp);
    argp->in.keycode = keycode;
    argp->in.unicode = unicode;
    argp->in.pressed = pressed;
    invokeAndWait(QpRobot::KeyAction, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpRobot::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpRobot, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpObject);

    switch ( mid ) {
    case QpRobot::Init:
        execInit();
        break;
    case QpRobot::MouseAction: {
        qtMargsMouseAction *a = (qtMargsMouseAction *)args ;
        execMouseAction (a->in.x, a->in.y, a->in.buttons, 
                                        a->in.pressed);
    }
    break ;
    case QpRobot::KeyAction: {
        qtMargsKeyAction *a = (qtMargsKeyAction *)args ;
        execKeyAction(a->in.keycode, a->in.unicode, a->in.pressed);
    }
    break ;
    default :
        break;
    }
}

void 
QpRobot::execInit() {
     mWidget = qApp->mainWidget();
    if (mWidget != NULL) {
        mWidget->setMouseTracking(TRUE);
    }
}

void
QpRobot::execMouseAction(int x, int y, int buttons, bool pressed) {
    int widgetX, widgetY;
    QEvent::Type type;
    
    widgetX = mWidget->geometry().x();
    widgetY = mWidget->geometry().y();
    QPoint pos(x+widgetX ,y+widgetY);  // workaround for basis
    
    if (!buttons) { //mouse move event
        type = QEvent::MouseMove;
        QCursor::setPos(pos);
        QMouseEvent me(type, mWidget->mapFromGlobal(pos), 0, 0 );
        QApplication::sendEvent( mWidget, &me );
    }
    else { //mouse press / release event
	//enum MouseButton { NoButton, LeftButton, MidButton, RightButton };
	//enum StateKey { NoKey, ShiftKey, ControlKey, AltKey };
	//bool mousePress( QWidget *widget, MouseButton button, StateKey key )
	
	if (pressed) {      
	    //printf("mousePressed Event\n");
	    type = QEvent::MouseButtonPress ;
	}
	else {      
	    //printf("mouseRelease event\n");
	    type = QEvent::MouseButtonRelease;
	}
	int b = Qt::LeftButton;
	
	switch (buttons)
	{
	    case java_awt_event_InputEvent_BUTTON1_MASK: b = Qt::LeftButton; 
		break;
	    case java_awt_event_InputEvent_BUTTON2_MASK: b = Qt::MidButton; 
		break;
	    case java_awt_event_InputEvent_BUTTON3_MASK: b = Qt::RightButton;
		break;
	    //case NoButton: b = Qt::NoButton; break;
	    //case LeftButton: b = Qt::LeftButton; break;
	    //case MidButton: b = Qt::MidButton; break;
	    //case RightButton: b = Qt::RightButton; break;
	    default: qWarning( "Incorrect mouse button specified" );
	}
	int k = Qt::NoButton;  // we are not storing special keys yet.
	
	// Please note that QPoint(1,1) assumes a certain size for the widget
	//and that it is ok to click in 
	// the upper left corner.
	if (mWidget != NULL) {
	    //printf("Original points  %d %d \n", pos->x(), pos->y());
	    //QMouseEvent me(type, pos, b, k );
	    QMouseEvent me(type, mWidget->mapFromGlobal(pos), b, k );
	    QApplication::sendEvent( mWidget, &me );
	}
    }
}

void
QpRobot::execKeyAction(int keycode, int unicode, bool pressed) {

    QEvent::Type type;
    QChar ch(unicode);
    QString text;
    text = ch.latin1();
    int k = stateKey;
    bool stateKeyPressedOrReleased = FALSE;
    switch (keycode) {
    case Qt::Key_Shift : stateKey = Qt::ShiftButton;         
        stateKeyPressedOrReleased = TRUE;
        break;
    case Qt::Key_Control: stateKey = Qt::ControlButton; 
        stateKeyPressedOrReleased = TRUE;
        break;
    case Qt::Key_Alt : stateKey = Qt::AltButton; 
        stateKeyPressedOrReleased = TRUE;
        break;
    }
    k = stateKey;

    // if widget is not set, set it to the toplevel with focus.
    if (widget == NULL) {
       QWidgetList  *list = QApplication::topLevelWidgets();
       QWidgetListIt it( *list );  // iterate over the widgets
       QWidget * w;
       while ( (w=it.current()) != 0 && widget == NULL) {   
           ++it;
           if ( w->isActiveWindow() )
               widget = w;
       }
       delete list;                // delete the list, not the widgets
    }
    QWidget *grab = QWidget::keyboardGrabber();
    if (!grab) { // send accel evetns
      QKeyEvent aa(QEvent::AccelOverride, keycode, ch.latin1(), k, text, 
		   FALSE);
      aa.ignore();
      QApplication::sendEvent(widget, &aa);
      if (!aa.isAccepted()) {
        QKeyEvent a(QEvent::Accel, keycode, ch.latin1(), k, text, FALSE);
        a.ignore();
        if (widget ->topLevelWidget() != NULL) {
          QApplication::sendEvent(widget->topLevelWidget(), &a);
          if (a.isAccepted())
             return;
        }
        
      }
    }
    if (!widget->isEnabled()) {
      return;
    }
    if (pressed) {
      type = QEvent::KeyPress ;
    } else { 
      type = QEvent::KeyRelease; 
    }
    QKeyEvent ke(type, keycode, ch.latin1(), k, text, FALSE);
    QApplication::sendEvent(widget, &ke);
    
    if (!stateKeyPressedOrReleased) {
      stateKey = Qt::NoButton;
    }
}

