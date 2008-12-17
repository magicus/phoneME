/*
 * @(#)QtComponentPeer.h	1.16 06/10/25
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
#ifndef _QTCOMPONENT_PEER_H_
#define _QTCOMPONENT_PEER_H_

#include "awt.h"
#include <qtimer.h>
#include <qwidget.h>
#include <qevent.h>
#include <qrect.h>
#include <qptrdict.h>
#include <qcursor.h>
#include "QpWidget.h"
#include "QtPeerBase.h"

/* If this class or its subclasses are used as an event handler, make
   sure to implement the function eventFilter() from QObject. */

class QtComponentPeer : public QtPeerBase
{
 public:
    static QtComponentPeer *getPeerForWidget(QWidget *widget);
    static QtComponentPeer *getPeerFromJNI(JNIEnv *env, jobject peer);

    QtComponentPeer(JNIEnv *env, jobject thisObj, QpWidget *widget,
		    QpWidget *refWidget);
    ~QtComponentPeer();

    virtual void dispose(JNIEnv *env);
    virtual QpWidget *getWidget(void) const;
    virtual jobject getPeerGlobalRef(void) const;

    // derived from QObject
    virtual bool eventFilter(QObject *obj, QEvent *evt);

    void resetClickCount(void);
    void incrementClickCount(void);
    int getClickCount(void) const;
    void updateWidgetStyle(JNIEnv *env);
    void updateWidgetCursor(JNIEnv *env);
    void setCursor(int javaCursorType);
    static jobject wrapInSequenced(jobject awtevent);

 protected:
    static void putPeerForWidget(QWidget *widget, QtComponentPeer *peer);
    static void removePeerForWidget(QWidget *widget);
    void timerEvent( QTimerEvent * );

 private:
    static void putPeerForWidget(QpWidget *widget, QtComponentPeer *peer);
    static void removePeerForWidget(QpWidget *widget);

    static jobject createColor(JNIEnv *env, const QColor *color);

    void postPaintEvent(const QRect *area);
    void propagateSettings(const QColor& background,
                           const QColor& foreground,
                           const QFont& font);

    /*
     * Collect paint events together to be dispatched to the
     * Java event thread all at once.
     */
    void coalescePaintEvents( const QRect *r );

    bool widgetShown(QShowEvent *evt);
    bool widgetFocusChanged(JNIEnv *env, QEvent::Type type,
                            QFocusEvent::Reason reason);

    /* a dictionary that stores the associations between a particular
       widget and its QtComponentPeer instance */
    static QPtrDict<QtComponentPeer> *widgetAssoc;

    /* The main QpWidget this QtComponentPeer manages. This widget is
       used for mouse signals which are then sent to the
       EventDispatchThread for processing. */
    QpWidget *widget;

    /* A global reference to the peer. This can be used by
       callbacks. */
    jobject peerGlobalRef;

    /* The last time the widget was clicked on. This is used for
       determining the click count for mouse click events. */
    /* guint32 lastClickTime; */

    /* The number of times the widget was clicked on. */
    int clickCount;

    /* The installed cursor. */
    QCursor *cursor;

    /* When a component peer is created we check to see if a
       foreground/background/font have be explicitly set for the
       component. If one hasn't then we set one which resembles the
       peer's background/foreground/font. This ensures that when the
       component is asked for any of these attributes Java will return
       something sensible that closely resembles the native peer's. We
       need to remember the ones that we set so we can remove them
       again when the peer is destroyed. We also use the installed
       background to decide if it is possible to use a pixmap to
       render the background or not (if a background has been set
       which is not the same as the installed background then we can't
       have a pixmap background). */
    jobject installedBackground, installedForeground, installedFont;

    /*
     * We keep a union of coalesced paint event rects in order
     * to be able to batch them up and reduce event traffic to
     * the Java side while not losing information.
     */
    int coalesceTimer;          // a timer identifier (max 128)
    QRect coalescedPaints;      // a bouncing area for repaints
    bool noCoalescing;          // true if coalescing has failed
    int coalescedCount;         // how many events we've collected
    
    QFont *qtFont;   //6228133
};

#endif
