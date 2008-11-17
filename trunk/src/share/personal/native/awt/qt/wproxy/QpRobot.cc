/*
 * @(#)QpRobot.cc	1.14 06/10/25
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
#include "QtComponentPeer.h"
#include "QpWidget.h"
#include "QtDisposer.h"
#include <qwidgetlist.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qcursor.h>
#include <stdio.h>
#include <qframe.h>
#ifdef QWS
#include <qwsdecoration_qws.h>
#endif

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
        execMouseAction(a->in.x, a->in.y, a->in.buttons, a->in.pressed);
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
#ifdef QWS
    // initialize values for desktop decoration - they'll be the same for the
    // frame and dialog decoration (if there is any)
    QWidget *desktop = QApplication::desktop();
    QWSDecoration &decoration = QApplication::qwsDecoration();
    QRegion region = decoration.region(desktop, desktop->geometry());
    QRect desktopWithDecorBounds = region.boundingRect();
    QRect desktopBounds = desktop->geometry() ;
    /* Note :- desktopBounds.x and y are always greater than
     * desktopWithDecorBounds.x and y respectively
     */
    decorationHeight = desktopWithDecorBounds.height()- desktopBounds.height();
    decorationWidth = desktopWithDecorBounds.width() - desktopBounds.width();
#endif

    //6253974
    isButtonPressed = false;
    QWidget *mWidget = qApp->mainWidget();
    if (mWidget != NULL) {
        mWidget->setMouseTracking(TRUE);
    }
}

void 
QpRobot::execMouseAction(int x, int y, int buttons, bool pressed) {
    int button, state;
    QPoint pos(x, y);
    QWidget *widget = QApplication::widgetAt(pos, TRUE);
    //6253974
    QEvent::Type type;
    QtEvent::Type customType;
    //6253974

#ifdef QWS
    //need to check to see if x,y is in the frame area of a top-level widget
    if (widget == NULL) { 
        QWidgetList  *list = QApplication::topLevelWidgets();
        QWidgetListIt it( *list );  // iterate over the widgets
        QWidget * w;
        while ( (w=it.current()) != 0 && widget == NULL) {
            if (w->isA("QFrame") || w->inherits("QDialog")) {
                if (!w->testWFlags(Qt::WStyle_NormalBorder) &&
                    !w->testWFlags(Qt::WStyle_DialogBorder)) {
                    // undecorated frame or dialog
                    if (w->frameGeometry().contains(x,y )) {
                        widget = w;
                    }
                }
                else {
                    if (w->frameGeometry().contains(x + decorationWidth,
                                                    y + decorationHeight)) {
                        widget = w;
                    }
                }
            }
            ++it;
        }
        delete list;                // delete the list, not the widgets
    }
#endif

    //6253974 - using setPos to move the cursor doesn't allow us to generate
    //MOUSE_DRAGGED events correctly.  Instead, we'll directly generate a 
    //MouseMove event with the appropriate modifiers if a button is currently
    //pressed
    button = Qt::NoButton;
    if (!buttons) { //mouse move event
        QCursor::setPos(pos);
          type = QEvent::MouseMove;
          customType = QtEvent::MouseMove;
          if (isButtonPressed)
              state = Qt::LeftButton;
          else
              state = Qt::NoButton;
    }
    else { //mouse press / release event
        button = Qt::LeftButton;
        //6253974 - need to be able to differentiate between robot generated
        //MouseButton and MouseMove events - so add a new type to QtEvent
        customType = QtEvent::MouseButton;
        if (pressed) {
            isButtonPressed = true;
            type = QEvent::MouseButtonPress ;
            state = Qt::NoButton;
        }
        else {
            isButtonPressed = false;
            type = QEvent::MouseButtonRelease;
            state = Qt::LeftButton;
        }
        
    }

        // Please note that QPoint(1,1) assumes a certain size for the widget
        // and that it is ok to click in
        // the upper left corner.
        if (widget != NULL) {
            QPoint localPoint = widget->mapFromGlobal(pos);
            QMouseEvent *me = new QMouseEvent(type, localPoint, 
                                              pos, button, state );
            // send custom Mouse Event
            QtEvent cme(customType, me);

            QtDisposer::Trigger guard;

            if ( QtComponentPeer::getPeerForWidget(widget) != NULL ) 
                QApplication::sendEvent(widget, &cme);
            else 
                QApplication::sendEvent(widget, me);
            
           if (buttons) { // mouse press

                QWidget* parent;

                // 6291093 - don't call setActiveWindow if the target is a 
                // popup menu

                // 6331618 - don't call setActiveWindow if the target is
                // a popped-up list from the Choice menu.  The object is
                // the QWidget inside QComboBox's QListBox for qt, so we don't 
                // focus for "a QWidget who's parent is a QListBox 
                // that is a popup"
 
                if (! ( widget->isA("QPopupMenu") ||  
                      ( widget->isA("QWidget") &&   
                        (parent = (QWidget*)widget->parent()) != 0 && 
                        parent -> isPopup() )) 
                   )
                    widget->setActiveWindow();

                // set the focus if the widget is not a QFrame
                if (!widget->isA("QFrame")) {
                    QpWidget::setFocus(widget);
                }
            }
            delete (me);
        }
    //}
}

void
QpRobot::execKeyAction(int keycode, int unicode, bool pressed) {
    QEvent::Type type;
    QChar ch(unicode);
    QString text;
    text = ch.latin1();
    // 6253887 - robot doesn't handle metakeys properly.  Instead of overwriting
    // m_metaKey each time, for subsequent key presses we should orr the values
    // together.  Then for key releases we use xor to turn off the designated
    // bit.  Also, we no longer need metaKeyTyped because we don't just reset
    // the metaKey variable anymore.
    switch (keycode) {
        case Qt::Key_Shift : 
            //this->m_metaKey = Qt::ShiftButton;
            if (pressed)
                this->m_metaKey = this->m_metaKey|Qt::ShiftButton;
            else
                this->m_metaKey = this->m_metaKey & !Qt::ShiftButton;
  //          this->m_metaKeyTyped = TRUE;
            break;
        case Qt::Key_Control: 
            //this->m_metaKey = Qt::ControlButton;
            if (pressed)
                this->m_metaKey = this->m_metaKey|Qt::ControlButton;
            else
                this->m_metaKey = this->m_metaKey & !Qt::ControlButton;
 //           this->m_metaKeyTyped = TRUE;
            break;
        case Qt::Key_Alt : 
            //this->m_metaKey = Qt::AltButton;
            if (pressed)
                this->m_metaKey = this->m_metaKey|Qt::AltButton;
            else
                this->m_metaKey = this->m_metaKey & !Qt::AltButton;
//            this->m_metaKeyTyped = TRUE;
            break;
        default :
            //this->m_metaKeyTyped = FALSE;
            break;
    }

    int k = this->m_metaKey;

    QWidget *widget = NULL;

    // try to get the current focused widget
    widget = qApp->focusWidget();

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

    if ( widget == NULL ) {
        // looks like no widget is currently focused, so we return
        return;
    }

    QWidget *grab = QWidget::keyboardGrabber();
    if (!grab) { // send accel events
        QKeyEvent aa(QEvent::AccelOverride, 
                     keycode, ch.latin1(), k, text, FALSE);
        aa.ignore();
        QApplication::sendEvent(widget, &aa);
        if (!aa.isAccepted()) {
            QKeyEvent a(QEvent::Accel,
                        keycode, ch.latin1(), k, text, FALSE);
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
    QKeyEvent *ke = new QKeyEvent(type, 
                                  keycode, ch.latin1(), k, text, 
                                  FALSE);

    QtDisposer::Trigger guard;

    if ( QtComponentPeer::getPeerForWidget(widget) != NULL ){
      QtEvent cke(QtEvent::Key, ke);
      QApplication::sendEvent(widget, &cke);
    }
    else {
      QApplication::sendEvent(widget, ke);
    }

//    if (!this->m_metaKeyTyped) {
//        this->m_metaKey = Qt::NoButton;
//    }

    delete(ke);
}

