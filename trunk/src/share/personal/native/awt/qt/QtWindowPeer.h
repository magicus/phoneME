/*
 * @(#)QtWindowPeer.h	1.14 03/01/16
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
 *
 */
#ifndef _QTWINDOW_PEER_H_
#define _QTWINDOW_PEER_H_

#include "QtPanelPeer.h"

class QtWindowPeer : public QtPanelPeer
{
 public:
    QtWindowPeer(JNIEnv *env, jobject thisObj, QpWidget *windowFrame);
    virtual bool eventFilter(QObject *obj, QEvent *evt);
    
    void postWindowEvent(jint id);
    void handleFocusIn(JNIEnv *env);
    void handleFocusOut(JNIEnv *env);

    // Bug 5108647.
    void setUserResizable(bool flag) { userResizable = flag; }
    bool isUserResizable() { return userResizable; }

    // 6182409: Window.pack does not work correctly.
    // Allows the native layer a chance to guess the borders
    // for decorated windows (Frame/Dialog).
    void guessBorders(JNIEnv *env, jobject thisObj, bool isUndecorated);

    // We now use beforeFirstShow for both QWS and Q_WS_X11.
    // See also the QtWindowPeer::windowMoved() definition
    // for details.

    // 6182409: Window.pack does not work correctly.
    // This is to track whether the peer has been first shown.
    // For Qt/X11, we don't want to set the widget location by offsetting
    // (meaning add to) with the guessedLeftBorder in the x position and
    // the guessedTopBorder in the y position before the window is first
    // shown.  If we do so, the window with decoration appears in the
    // wrong location.
    bool beforeFirstShow;

#ifdef Q_WS_X11
    // 6182409: Window.pack does not work correctly.
    // This is to indicate whether the peer is using the guessed borders.
    bool bordersGuessed;
#endif /* Q_WS_X11 */

 private:
    bool firstResizeEvent;

    // Bug 5108647.
    bool userResizable;

    /* to calculate real frame insets*/
    bool firstPaintEvent;

    //event handler functions
    void windowResized(QObject *obj, QResizeEvent *evt);
    void windowMoved(QObject *obj, QMoveEvent *evt);
    void windowClosed(QObject *obj, QCloseEvent *evt);
    void windowActivated(QObject *obj, QEvent *evt);
    void windowDeactivated(QObject *obj, QEvent *evt);
    void windowShown(QObject *obj, QShowEvent *evt);
    void windowStateChanged(QObject *obj, QEvent *evt);
    void windowPainted(QObject *obj, QEvent *evt);

#if (QT_VERSION >= 0x030000)
    /* Added to accurately track the window state change. */
    bool minimized;
    bool wasMinimized() { return minimized; }
    void setWasMinimized(bool flag) { minimized = flag; }
#endif /* QT_VERSION */

 protected:
    /* An optional callback function that is called when the
       window is resized.  This is useful in the frame for example
       where we need to reposition the menu bar as the window
       resizes. */
    virtual void resize(QResizeEvent *evt);
};


#endif
