/*
 * @(#)QtWindowPeer.cc	1.34 06/10/25
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
#include "sun_awt_qt_QtWindowPeer.h"
#include "java_awt_event_WindowEvent.h"
#include "QtWindowPeer.h"
#include "QpFrame.h"
#include "QtSync.h"
#include "QtApplication.h"
#include "QtDisposer.h"

QtWindowPeer::QtWindowPeer(JNIEnv *env,
                           jobject thisObj, 
                           QpWidget *windowFrame)
    : QtPanelPeer(env, thisObj, windowFrame) 
{
    // 6182409: Window.pack does not work correctly.
    // Tracks whether the window has been first shown yet
    // and whether the window has guessed its borders.
    // See QtFramePeer's and QtDialogPeer's JNI create impls
    // and QtWindowPeer::guessBorders() which gets called.
    this->beforeFirstShow = TRUE;
// This variable is used only for Q_WS_X11.
#ifdef Q_WS_X11
    this->bordersGuessed = FALSE;
#endif /* Q_WS_X11 */

    this->firstResizeEvent = TRUE;
    this->userResizable = TRUE;

#if (QT_VERSION >= 0x030000)
    /* Added to accurately track the window state change. */
    this->setWasMinimized(false);
#endif /* QT_VERSION */

    //initialize qwsInit
    jboolean qwsInit =
        env->GetStaticBooleanField(QtCachedIDs.QtWindowPeerClass,
                                   QtCachedIDs.QtWindowPeer_qwsInitFID);
    if (qwsInit == JNI_FALSE) {
        int qwsX = 0 , qwsY = 0 ;
#ifdef QWS
#ifdef QTOPIA
        QPoint origin = ((QpFrame *)windowFrame)->getOriginWithDecoration();
        qwsX = origin.x();
        qwsY = origin.y();
#endif /* QTOPIA */
#endif /* QWS */

        env->CallStaticVoidMethod(QtCachedIDs.QtWindowPeerClass,
                                  QtCachedIDs.QtWindowPeer_setQWSCoordsMID, 
                                  (jint) qwsX, (jint) qwsY);
	
        env->SetStaticBooleanField(QtCachedIDs.QtWindowPeerClass,
                                   QtCachedIDs.QtWindowPeer_qwsInitFID, 
                                   JNI_TRUE);
    }

    //6233632
    jobject target = env->GetObjectField(thisObj,
                                         QtCachedIDs.QtComponentPeer_targetFID);
    jstring warningString = (jstring)env->GetObjectField (target,
                                                          QtCachedIDs.java_awt_Window_warningStringFID);
    // If the warning string is null, then we don't need to display a warning string.
    if (warningString != NULL) {
        QString *qStr = awt_convertToQString(env, warningString);
        ((QpWidget *)windowFrame)->createWarningLabel(*qStr);
    }
    //6233632
}

// For Q_WS_X11 and QWS, we use different guesses.

#ifdef Q_WS_X11

static jint guessedTop = 29;
static jint guessedLeft = 3;
static jint guessedBottom = 3;
static jint guessedRight = 3;

#endif /* Q_WS_X11 */

#ifdef QWS

#ifdef QT_KEYPAD_MODE
static jint guessedTop = 24;
#else
static jint guessedTop = 34;
#endif
static jint guessedLeft = 4;
static jint guessedBottom = 4;
static jint guessedRight = 4;

#endif /* QWS */

// Provides the peer implementation a chance to guess the borders of a decorated
// window.  It reduces the flicker seen when doing the resize dance.
// See also QtWindowPeer.show()/restoreBounds().
void
QtWindowPeer::guessBorders(JNIEnv *env, jobject thisObj, bool isUndecorated)
{
    if ( isUndecorated ) {
        env->CallVoidMethod(thisObj, QtCachedIDs.QtWindowPeer_setInsetsMID,
    			0, 0, 0, 0);
    }
    else {
    env->CallVoidMethod(thisObj, QtCachedIDs.QtWindowPeer_setInsetsMID,
    			guessedTop, guessedLeft, guessedBottom, guessedRight);
    }

// This variable is used only for Q_WS_X11.
#ifdef Q_WS_X11
    bordersGuessed = TRUE;
#endif /* Q_WS_X11 */
}

bool
QtWindowPeer::eventFilter(QObject *obj, QEvent *evt) 
{
    JNIEnv *env = 0;
    bool done = FALSE;

    if( JVM->AttachCurrentThread( (void **) &env, NULL ) != 0 ) {
        return FALSE;
    }
  
    if (this->getWidget()->isSameObject(obj)) {
        switch (evt->type()) {
        case QEvent::Resize :
            this->windowResized(obj, (QResizeEvent *)evt);
            // give  a chance for the widget to look at the event. 
            // Fixes the QxFileDialog resize filling up the frame problem 
            break;
        case QEvent::Move :
            this->windowMoved(obj, (QMoveEvent *)evt);
            break;
        case QEvent::Close :
            this->windowClosed(obj, (QCloseEvent *)evt);
            done = TRUE;
            break;
	    /*   
        case QEvent::WindowActivate :
            this->windowActivated(obj, evt);
            done = TRUE;
            break;
        case QEvent::WindowDeactivate :
            this->windowDeactivated(obj, evt);
            done = TRUE;
            break;
*/
        case QEvent::Show :
            this->windowShown(obj, (QShowEvent *)evt);
            break;
        case QEvent::Paint :
            this -> windowPainted(obj, evt);
            break;
#if (QT_VERSION >= 0x030000)
            /*
             * In qt-3.3, ShowNormal and ShowMinimized events are obsolete.
             * See also windowStateChanged().
             */
        case QEvent::WindowStateChange :
            this->windowStateChanged(obj, evt);
            done = TRUE;
            break;
#else
            /*
             * this is the case when qt version is 2.3.2
             * See also windowStateChanged().
             */
        case QEvent::ShowNormal :
        case QEvent::ShowMinimized :
            this->windowStateChanged(obj, evt);
            done = TRUE;
            break;
#endif /* QT_VERSION */
    case QEvent::FocusIn :
        //this->windowActivated(obj, evt);
        break;
	default:
	    break;
	}
    }
    if ( ! done ) {
        done = QtPanelPeer::eventFilter( obj, evt );
    }
    return done;
}

void
QtWindowPeer::resize(QResizeEvent *evt) 
{
   //6393054
   QpWidget *widget = (QpWidget *) this->getWidget();
   widget->resizeWarningLabel();
}

void
QtWindowPeer::windowResized (QObject *obj, QResizeEvent *evt)
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    jobject target;
    jint topBorder, leftBorder, bottomBorder, rightBorder;

    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
        return;
    guard.setEnv(env);

    // Don't process this event when the file dialog is not even visible.
    if (obj->inherits("QDialog") && !((QWidget*)obj)->isVisible()) {
        return;
    }

    const QSize &size = evt->size();

    // Qt automatically sends out a resize event when it creates a new widget,
    // resizing it to be full screen.  We want to ignore that event.
#ifdef QWS
    if (firstResizeEvent) {
        QWidget* desktop = QApplication::desktop();
        int dheight = desktop->height();
        int dwidth = desktop->width();
        firstResizeEvent = false;
        // "Magic" numbers come from the formula used by qwidget_qws in 
        // create() function for a toplevel widget in Qt 2.3.2.  
        //This formula will have to be rechecked if the version of Qt changes.
        if ((size.width() == (dwidth/2)) && 
            (size.height() == (dheight*4/10))) {
            
            return;
        }
    }
#endif /* QWS */
    
    this->resize(evt);
    
    /* Because the size we get here is the size of our actual X window
       and not the window manager's we need to add the border sizes on
       because Java expects the size of a window to include any
       external border. */
    
    topBorder = env->GetIntField (thisObj,
                                  QtCachedIDs.QtWindowPeer_topBorderFID);
    leftBorder = env->GetIntField (thisObj,
                                   QtCachedIDs.QtWindowPeer_leftBorderFID);
    bottomBorder = env->GetIntField (thisObj,
                                     QtCachedIDs.QtWindowPeer_bottomBorderFID);
    rightBorder = env->GetIntField (thisObj,
                                    QtCachedIDs.QtWindowPeer_rightBorderFID);
    
    target = env->GetObjectField (thisObj,
                                  QtCachedIDs.QtComponentPeer_targetFID);
    
    env->SetIntField (target, QtCachedIDs.java_awt_Component_widthFID,
                      (jint)size.width() + leftBorder + rightBorder);
    env->SetIntField (target,
                      QtCachedIDs.java_awt_Component_heightFID,
                      (jint)size.height() + topBorder + bottomBorder);

    env->DeleteLocalRef(target);

    env->CallVoidMethod (thisObj, QtCachedIDs.QtWindowPeer_postResizedEventMID);

    if (env->ExceptionCheck())
        env->ExceptionDescribe();
}

void
QtWindowPeer::windowMoved (QObject *obj, QMoveEvent *evt)
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    jobject target;
    jint x, y;
    jint oldX, oldY;
    jint topBorder, leftBorder;

    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
        return;
    guard.setEnv(env);

    // 6253643: Unexpected window movement running functional awtfocus
    // ModalDialogInFocusEventTest.
    // Qt sometimes spews out an extra move event during the show.
    // It will confuse the Java layer into thinking the window pos
    // should be changed accordingly.  And therefore, subsequently
    // QtWindowPeer.restoreBounds() will yank the window into a wrong
    // position.  The extra check "beforeFirstShow" is to prevent that.
    // 
    // 6182409: Window.pack does not work correctly.
    // The following check was originally setup for FileDialog.
    // It is now applied for all window peers.  The reason is that
    // before the window is made visible, qt delivers the move event
    // with (x, y) as what the Java layer wants, and it would be
    // inappropriate if we shift the (x, y) reported from qt by
    // some northwest vector and resyncs it with the Java layer.
    // Don't process this event when the window is not even visible.
    // 
    // Old comment:
    // Don't process this event when the file dialog is not even visible.
    if (!((QWidget*) obj)->isVisible() || beforeFirstShow) {
        return;
    }

    target = env->GetObjectField (thisObj, QtCachedIDs.QtComponentPeer_targetFID);
		
    /* Because the position is of our actual X window and not the
       window manager's we need to subtract the top and left border
       sizes to get the position of the window on the screen. */
	
    const QPoint& newPos = evt->pos();
    topBorder = env->GetIntField (thisObj,
                                  QtCachedIDs.QtWindowPeer_topBorderFID);
    leftBorder = env->GetIntField (thisObj,
                                   QtCachedIDs.QtWindowPeer_leftBorderFID);

    x = newPos.x() - leftBorder ;
    y = newPos.y() - topBorder ;

    /* We get the old values to see if the window has actually been moved. */
    oldX = env->GetIntField (target, QtCachedIDs.java_awt_Component_xFID);
    oldY = env->GetIntField (target,
                             QtCachedIDs.java_awt_Component_yFID);
    
    //is it easier to just compare the old and new pos in QMoveEvent?
    if (oldX != x || oldY != y)	{
        env->SetIntField (target, QtCachedIDs.java_awt_Component_xFID, x);
        env->SetIntField (target, QtCachedIDs.java_awt_Component_yFID, y);
	
        env->CallVoidMethod (thisObj, QtCachedIDs.QtWindowPeer_postMovedEventMID);
	
        if (env->ExceptionCheck())
            env->ExceptionDescribe();
    }

    env->DeleteLocalRef(target);
}

void
QtWindowPeer::windowClosed (QObject *obj, QCloseEvent *evt)
{
    this->postWindowEvent (java_awt_event_WindowEvent_WINDOW_CLOSING);
}

void
QtWindowPeer::windowActivated (QObject *obj, QEvent *evt)
{
}

void
QtWindowPeer::windowDeactivated (QObject *obj, QEvent *evt)
{
}

void
QtWindowPeer::windowShown (QObject *obj, QShowEvent *evt)
{
}

void 
QtWindowPeer::windowStateChanged(QObject *obj, QEvent *evt) 
{
#if (QT_VERSION >= 0x030000)
    /*
     * Takes care of the state transitions between iconified <-> deiconified
     * and posts the ICONIFIED/DEICONIFIED events judiciously.
     */
    QWidget *w = (QWidget *) obj;
    if (w->isMinimized()) {
        this->postWindowEvent(java_awt_event_WindowEvent_WINDOW_ICONIFIED);
        this->setWasMinimized(TRUE);
    } else {
        if (this->wasMinimized()) {
            this->postWindowEvent(java_awt_event_WindowEvent_WINDOW_DEICONIFIED);
        }
        this->setWasMinimized(FALSE);
    }
#else
    /*
     * This is the case with qt-2.3.2.  In qt-3.3, ShowNormal and
     * ShowMinimized events are obsolete.  The while loop below is not such a
     * good idea and causes qt-3.3 in an endless loop.
     */
    if (evt->type() == QEvent::ShowNormal) {
        while (((QWidget *) obj)->isMinimized()) {}
        this->postWindowEvent(java_awt_event_WindowEvent_WINDOW_DEICONIFIED);
    } else if (evt->type() == QEvent::ShowMinimized) {
        while (!((QWidget *) obj)->isMinimized()) {}
        this->postWindowEvent(java_awt_event_WindowEvent_WINDOW_ICONIFIED);
    }
#endif /* QT_VERSION */
}

void QtWindowPeer::handleFocusIn(JNIEnv *env) {
    jobject windowEvent, target;

    QtDisposer::Trigger guard(env);

    jobject thisObj = this->getPeerGlobalRef();
    if(!thisObj)
    {
        return;
    }    

    target = env->GetObjectField (thisObj, QtCachedIDs.QtComponentPeer_targetFID);

    //create new WindowEvent class and wrap it in a sequenced event.  
    windowEvent = 
	env->NewObject(QtCachedIDs.WindowEventClass, 
		       QtCachedIDs.java_awt_event_WindowEvent_constructorMID,
		       target, 
		       java_awt_event_WindowEvent_WINDOW_GAINED_FOCUS);

    jobject awtevent = QtComponentPeer::wrapInSequenced(windowEvent);
    
//printf("QtWindowPeer.cc: posting WINDOW_GAINED_FOCUS\n");
    env->CallVoidMethod (thisObj, QtCachedIDs.QtComponentPeer_postEventMID,
			 awtevent);
    env->DeleteGlobalRef(awtevent);

    // 6256796: Encountered java.lang.OutOfMemoryError running the whole
    // PP-TCK test suite.  Added the missing DeleteLocalRef() calls.
    env->DeleteLocalRef(target);
    env->DeleteLocalRef(windowEvent);

    if (env->ExceptionCheck())
        env->ExceptionDescribe();
}

void QtWindowPeer::handleFocusOut(JNIEnv *env) {
    jobject windowEvent, target;

    QtDisposer::Trigger guard(env);

    jobject thisObj = this->getPeerGlobalRef();
    if (thisObj == NULL)
        return;
    target = env->GetObjectField (thisObj, QtCachedIDs.QtComponentPeer_targetFID);

    //create new WindowEvent class and wrap it in a sequenced event.  
    windowEvent = 
	env->NewObject(QtCachedIDs.WindowEventClass, 
		       QtCachedIDs.java_awt_event_WindowEvent_constructorMID,
		       target, 
		       java_awt_event_WindowEvent_WINDOW_LOST_FOCUS);

    jobject awtevent = QtComponentPeer::wrapInSequenced(windowEvent);
    
//printf("QtWindowPeer.cc: posting WINDOW_LOST_FOCUS\n");
    env->CallVoidMethod (thisObj, QtCachedIDs.QtComponentPeer_postEventMID,
			 awtevent);
    env->DeleteGlobalRef(awtevent);

    // 6256796: Encountered java.lang.OutOfMemoryError running the whole
    // PP-TCK test suite.  Added the missing DeleteLocalRef() calls.
    env->DeleteLocalRef(target);
    env->DeleteLocalRef(windowEvent);

    if (env->ExceptionCheck())
        env->ExceptionDescribe();
}

void
QtWindowPeer::postWindowEvent (jint id)
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
    
    if (!thisObj || JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
        return;
    guard.setEnv(env);

    env->CallVoidMethod (thisObj, QtCachedIDs.QtWindowPeer_postWindowEventMID, id);
    if (env->ExceptionCheck())
        env->ExceptionDescribe();
}


void
QtWindowPeer::windowPainted(QObject *obj, QEvent *evt)
{
}

// 6182365
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_computeInsets(JNIEnv *env, jobject thisObj)
{
    /* This method is here to reposition the frame to the location that 
       it really ought to be, and to calculate the proper inset values of this frame. 
       We call setGeometry() on the frame before it's first shown on the screen
       with the values requested by the Java land via setBounds(), but
       setGeometry() doesn't necessarily work as expected on x86 before the frame
       is first shown.  setGeom(x,y,w,h) before show() positions the frame at 
       (x+leftinset, y+topinset, w,h) on x86 suse gnome, and this is apparently a 
       known issue for qt x86.

       The inset of the frame is not known until the frame is decorated by
       the window manager.  On x86, this info seem to arrive sometime between the
       delivery of the show event to the delivery of the paint event.  Thus at
       the first paint event delivery, this method calls frameGeometry() and 
       geometry() to inspect the true inset values of this window, and updates
       the inset data members in the Window/Frame/DialogPeer.

       Once the inset is known, we can call setGeometry() on this widget to make
       it have the bound the java land requested.  This causes brief flashing
       of the frame (it appears at some other location or size for a brief moment
       and then gets resized/repositioned) when the frame is shown for the 
       first time.

       The alternative to the flashing is to merely update Java's data values 
       of this window after it is shown to reflect it's true location and size
       on the screen.  The awt spec allows the peer to overwrite Java settings
       once the window is connected to the native resource.  

       On zaurus, the value is not available either way, and we have to figure
       the inset out from the desktop and the QWS decoration.  This can be done 
       before the first paint event, but done here for the code simplicity.

    */
    // Get the proxy widget
    QtDisposer::Trigger guard(env);
    QtComponentPeer *component = QtComponentPeer::getPeerFromJNI (env, thisObj);
    QpFrame *qpFrame = (QpFrame*)component->getWidget();

    int currentLeft,currentRight,currentTop, currentBottom;

#ifdef QWS
    /*
     * The right way to compute Insets for a top level Window should be
     * after the window is shown using QWidget.frameGeometry() and 
     * QWidget.geometry(), but on the Zaurus both are the same after show()
     * so we use the following method to compute the Insets. These insets
     * can be used for both java.awt.Frame and java.awt.Dialog, since 
     * both have the same decoration.
     */

    /* if (w->testWFlags(Qt::WStyle_NoBorder)) { 
       workaround, WStyle_NoBorder is zero for qt 2.3 */
    if (!qpFrame->testWFlags(Qt::WStyle_NormalBorder) && 
        !qpFrame->testWFlags(Qt::WStyle_DialogBorder)) {
        /* undecorated frame, just return */
        return;
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
#else
    /* 
     * on x86, simply take the difference betwen frameGeometry() and geometry().
     */


    // If this window is not supposed to have a border, just return.
    if (qpFrame->testWFlags(Qt::WStyle_NoBorder)) {
        return;
    }

    long maxWaitTime = 1500;
    long waitTime = 1;
    long accumTime = 0;
    int maxTitle = 60;   //6271724
    int maxBorder=15;   //6271724
    QtCondVar timer;
    QRect geom;
    QRect frameGeom;

    /*
      Try to get the insets (calculated by getting the difference
      between the geometry and frame geometry) periodically.  If
      we don't have the insets by a certain time (e.g. maybe the
      framestyle has insets of size 0), then bail.
    */

    while (accumTime < maxWaitTime) {
        timer.mutex()->lock();
        timer.wait(waitTime);
        timer.mutex()->unlock();

        qpFrame->syncX();   //6271724

        geom = qpFrame->geometry();
        frameGeom = qpFrame->frameGeometry();

        currentLeft = (geom.x() - frameGeom.x());
        currentTop = (geom.y() - frameGeom.y());
        currentRight = (frameGeom.width() - geom.width() - currentLeft);
        currentBottom = (frameGeom.height() - geom.height() - currentTop);

        accumTime += waitTime;

        //6271724
        if ((geom != frameGeom) &&
            ((currentTop >= 0) &&
             (currentTop <= maxTitle) &&
             (currentLeft >= 0) &&
             (currentLeft <= maxBorder) &&
             (currentRight >= 0) &&
             (currentRight <= maxBorder) &&
             (currentBottom >= 0) &&
             (currentBottom <= maxBorder)))
        {
            accumTime = maxWaitTime;
        }
    }

#endif

    QtWindowPeer *window = (QtWindowPeer *)
	QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    // 6182409: Window.pack does not work correctly.
    // Now we can update the guessed insets to be the computed insets.
    // Also sets the beforeFirstShow flag to false.

    if (window) {
	window->beforeFirstShow = FALSE;;
    }

    if (currentTop != guessedTop
        || currentLeft != guessedLeft
        || currentBottom != guessedBottom
        || currentRight != guessedRight)
    {
	guessedTop = currentTop;
	guessedLeft = currentLeft;
	guessedBottom = currentBottom;
	guessedRight = currentRight;
    }

    /* Update the Inset data in the java land */
    env->CallVoidMethod(thisObj, QtCachedIDs.QtWindowPeer_updateInsetsMID,
                        currentTop, currentLeft, currentBottom, currentRight);

    /* now that the frame is shown, set bounds with proper inset values */
    env->CallVoidMethod (thisObj,
                         QtCachedIDs.QtWindowPeer_restoreBoundsMID);

    if (env->ExceptionCheck())
        env->ExceptionDescribe();
}
// 6182365

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_initIDs (JNIEnv *env, jclass cls)
{
    QtCachedIDs.QtWindowPeerClass = (jclass) env->NewGlobalRef (cls);

    GET_FIELD_ID (QtWindowPeer_topBorderFID, "topBorder", "I");
    GET_FIELD_ID (QtWindowPeer_leftBorderFID, "leftBorder", "I");
    GET_FIELD_ID (QtWindowPeer_bottomBorderFID, "bottomBorder", "I");
    GET_FIELD_ID (QtWindowPeer_rightBorderFID, "rightBorder", "I");

    // 6182409: Window.pack does not work correctly.
    // This method sets/guesses the initial insets for the Java peer.
    GET_METHOD_ID (QtWindowPeer_setInsetsMID, "setInsets", "(IIII)V");

    // 6182409: Window.pack does not work correctly.
    // This method updates the Java peer about the insets after the window
    // is mapped on screen.
    GET_METHOD_ID (QtWindowPeer_updateInsetsMID, "updateInsets", "(IIII)V");

    GET_METHOD_ID (QtWindowPeer_postResizedEventMID, "postResizedEvent", "()V");
    GET_METHOD_ID (QtWindowPeer_postMovedEventMID, "postMovedEvent",
                   "()V");
    GET_METHOD_ID (QtWindowPeer_postShownEventMID, "postShownEvent",
                   "()V");
    // 6211281 start
    GET_METHOD_ID (QtWindowPeer_postWindowEventMID, "postWindowEvent", "(I)V");
    GET_METHOD_ID (QtComponentPeer_postEventMID, "postEvent",
		   "(Ljava/awt/AWTEvent;)V");
    // 6211821 end
    // 5108404
    GET_METHOD_ID (QtWindowPeer_hideLaterMID, "hideLater", "()V");
    GET_METHOD_ID (QtWindowPeer_restoreBoundsMID, "restoreBounds", "()V");

    //GET_METHOD_ID (QtWindowPeer_notifyShownMID, "notifyShown", "()V");

    // 5108404
    GET_STATIC_FIELD_ID (QtWindowPeer_qwsInitFID, "qwsInit", "Z");
    GET_STATIC_FIELD_ID (QtWindowPeer_qwsXFID, "qwsX", "I");
    GET_STATIC_FIELD_ID (QtWindowPeer_qwsYFID, "qwsY", "I");    
    GET_STATIC_METHOD_ID (QtWindowPeer_setQWSCoordsMID, "setQWSCoords",
                          "(II)V");

    cls = env->FindClass( "java/awt/Window" );
    if (cls != NULL) {
        GET_FIELD_ID (java_awt_Window_warningStringFID, "warningString", "Ljava/lang/String;");
    }

    // Bug 6211281 - upFocusCycle doesn't work correctly
    cls = env->FindClass("java/awt/event/WindowEvent");
                                                                                
    if (cls == NULL)
        return;
                                                                                
    QtCachedIDs.WindowEventClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_event_WindowEvent_constructorMID, "<init>",
		   "(Ljava/awt/Window;I)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_create (JNIEnv *env, jobject thisObj, 
                                     jobject parent)
{
    QtWindowPeer *window;

    // Create the window widget and the peer object
    QpWidget *parentWidget = 0;
  
    if (parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
            env->GetIntField (parent,
                              QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
 
    /* As per PP-spec, it should not contain any borders */
    QpFrame *windowFrame = new QpFrame(parentWidget,
                                       "QtWindow",
                                       Qt::WStyle_Customize|Qt::WStyle_NoBorder);
    window = new QtWindowPeer(env, thisObj, windowFrame);
    if (window == NULL) {
        env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }
    
    //6393054
    window->guessBorders(env, thisObj, TRUE);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_setActiveNative (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *window = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (window == NULL || window->getWidget() == NULL)
	return;
		
    window->getWidget()->setActiveWindow();
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_toFront (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *window = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (window == NULL || window->getWidget() == NULL)
        return;
		
    // Fix for bug 6175437 toFront does not focus the window
    // Qt doesn't automatically focus a window just because it is moved to
    // the front.  So, you need to call setActiveWindow as well
    // in order for the window to also receive focus
    window->getWidget()->setActiveWindow();
    window->getWidget()->raise();
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_toBack (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *window = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (window == NULL || window->getWidget() == NULL)
        return;
		
    window->getWidget()->lower();
}

//6233632, 6393054
JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtWindowPeer_warningLabelHeight (JNIEnv *env, 
                                              jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *component = QtComponentPeer::getPeerFromJNI (env, thisObj);
    QpWidget *qpWidget = (QpFrame*)component->getWidget();
    return qpWidget->warningLabelHeight();
}
//6233632, 6393054

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_setResizable (JNIEnv *env, 
                                           jobject thisObj,
                                           jboolean resizable) 
{
    QtDisposer::Trigger guard(env);
    
    QtWindowPeer *window = (QtWindowPeer *)
        QtComponentPeer::getPeerFromJNI (env, thisObj);
    
    if (window == NULL || window->getWidget() == NULL) {
        return;
    }

    QpWidget *widget = window->getWidget();

    // Fixed bug: 4750381 set the window to fixed size if it is 
    // not resizable. 
    if (resizable == JNI_FALSE) {
        QSize size = widget->frameSize();
        widget->setFixedSize(size.width(), size.height());
        window->setUserResizable(FALSE);
    } else {
        /*
         * Undoes the effect of the qt call widget->setFixedSize()
         * when the widget was meant to be not resizable so that the widget is
         * resizable afterwards. 
         */
        widget->setMinimumSize(0, 0);
        widget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        window->setUserResizable(TRUE);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_setBoundsNative (JNIEnv *env, jobject thisObj, jint x, jint y, jint width, jint height)
{
    // Bug 5108647.
    //
    // If the target is not resizable by the user, it means the qt widget has
    // a fixed size set on it.  Resets the fixed size before calling the
    // QWidget's setGeometry() method.
    //
    // See also Java_sun_awt_qt_QtWindowPeer_setResizable() and
    // Java_sun_awt_qt_QtComponentPeer_setBoundsNative().

    QtDisposer::Trigger guard(env);

    QtWindowPeer *window = (QtWindowPeer *)
        QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (window == NULL) {
        return;
    }

    QpWidget *widget = window->getWidget();

    if (! window->isUserResizable()) {
        widget->setFixedSize(width, height);
    }

    // 6182409: Window.pack does not work correctly.
    // For Qt/X11, we don't want to set the widget location by offsetting
    // (meaning add to) with the guessed left border in the x position and
    // the guessed top border in the y position before the window is first
    // shown.  If we do so, the window with decoration appears in the
    // wrong location when it is first shown.
#ifdef Q_WS_X11
    if (window->beforeFirstShow && window->bordersGuessed) {
	x -= guessedLeft;
	y -= guessedTop;
    }
#endif /* Q_WS_X11 */

    widget->setGeometry((int) x, (int) y, (int) width, (int) height);
}

/* Partial bug fix for 6186499 - make sure that the right java component has
 * the native focus when a window is first show.  If we don't do this before 
 * showing the window, we may get an extra focus event on the frame before 
 * java switches the focus to the correct component
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtWindowPeer_setFocusInWindow(JNIEnv *env, jobject thisObj,
                                         jobject focusedComponent)
{
    QtDisposer::Trigger guard(env);
    
    QpWidget *focusedWidget = 0;
                                                                                
    if (focusedComponent) {
        QtComponentPeer *focusedPeer = QtComponentPeer::getPeerFromJNI(env, 
                                             focusedComponent);
        if (focusedPeer != NULL) {
            focusedWidget = focusedPeer->getWidget();
            if (focusedWidget != NULL)
                focusedWidget->setFocus();
        }
    }
}
 
JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtWindowPeer_wrapInSequenced(JNIEnv *env, jobject thisObj,
					     jobject awtevent)
{
    return QtComponentPeer::wrapInSequenced(awtevent);
}

#ifdef PERSONAL_APPMANAGER
#include "sun_mtask_xlet_XletFrame.h"

JNIEXPORT void JNICALL
Java_sun_mtask_xlet_XletFrame_synthesizeFocusIn (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);

#ifdef QWS
#ifndef QT_KEYPAD_MODE

    jobject peer = env->GetObjectField (thisObj, QtCachedIDs.java_awt_Component_peerFID);

    if (peer == NULL) {
        return;
    }

    QtWindowPeer *window = (QtWindowPeer *)QtComponentPeer::getPeerFromJNI (env, peer);
	
    if (window == NULL || window->getWidget() == NULL) {
        return;
    }
	
    window->handleFocusIn(env);

#endif //QT_KEYPAD_MODE
#endif //QWS
}
#endif  // #ifdef PERSONAL_APPMANAGER
