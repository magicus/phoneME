/*
 *  @(#)QpObject.h	1.15 06/10/25
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
#ifndef _QpOBJECT_H_
#define _QpOBJECT_H_

#include <qevent.h>
#include <qobject.h>
#include "QtSync.h"
#include "QtORB.h"

/**
 * Declares <_class>::MethodId mid
 */
#define QP_METHOD_ID_DECLARE(_class,_mid) \
        _class::MethodId _mid = (_class::MethodId)methodId

#define QP_INVOKE_SUPER_ON_INVALID_MID(_mid, _superclass) \
        if ( _mid < SOM || _mid >= EOM ) {\
            _superclass::execute(methodId, args); \
            return ; \
        }
/**
 */
class QpObject {
    // QtORB is a friend to get access to the private methods.
    friend class QtORB ;
private :
    // counts the number of synchronous method invocations outstanding
    int       m_sync_invocations; 
    // marker to indicate that "this" should be deleted at a safe point
    bool      m_tobe_deleted;
    QtMutex   *m_object_mutex ;
    QtCondVar *m_signal_condition ;

public :
    /* Deletes the passed QObject in the Qt event thread */
    static void deleteQObjectLater(QObject *qobj);
    static void staticSendEvent(QpObject *receiver, QEvent *event);
    static QWidget* staticWidgetAt(int x, int y, bool child=FALSE);
    static QPoint staticCursorPos();
    static void staticSetCursorPos(QPoint pos);
    static QWidgetList *staticTopLevelWidgets();
    static void exitQApplication(int ret); // 6176847
    static QWidget* staticGetNativeFocusOwner();
    static QWidget* staticGetNativeFocusedWindow();

    QpObject();
    virtual ~QpObject();
    inline bool isSameObject(QObject *obj) { return obj == m_qobject ; }
    virtual void postEvent(QEvent *event);

    /* QObject Methods */
    void deleteLater() ;
    void sendEvent(QObject *receiver, QEvent *event);
    QWidget* widgetAt(int x, int y, bool child=FALSE);
    QPoint cursorPos();
    void setCursorPos(QPoint pos);
    QWidgetList* topLevelWidgets();
    void installEventFilter(const QObject *);
    void removeEventFilter(const QObject *);
    void connect(const char *signal, QObject *receiver, const char *member);
    void disconnect(const char *signal, 
                    QObject *receiver, 
                    const char *member);
    void syncX();   //6271724
    QWidget* getNativeFocusOwner();
    QWidget* getNativeFocusedWindow();
protected :
    enum MethodId {
        DeleteLater = 0,
        InstallEventFilter,
        RemoveEventFilter,
        Connect,
        Disconnect,
        DeleteLater_QObject, // private
        SendEvent,
        WidgetAt,
        CursorPos,
        SetCursorPos,
        TopLevelWidgets,
        GetNativeFocusOwner,
        GetNativeFocusedWindow,
        SyncX,   //6271724
        ExitApp,            // private
        EOM
    };
    QObject *m_qobject ;
    void invokeLater(int methodId, void *args);
    void invokeAndWait(int methodId, void *args,
                       long timeInMillis = ULONG_MAX);

    virtual void execute(int methodId, void *args) ;

private:
    static QpObject *QObjectInstance; // 6176847

    void deleteLater(QObject *obj);
    void exitApp(int ret); // 6176847
    void execDeleteLater(QObject *obj) ;

    void waitForExecutionDone(QtORB *orb,
                              int methodId, 
                              void *txId, 
                              long timeInMills) ;
    void notifyExecutionDone(QtORB *orb,
                             int methodId, 
                             void *txId) ;

    void execDeleteLater(void) ;
    void execInstallEventFilter(QObject *qobject) ;
    void execRemoveEventFilter(QObject *qobject) ;
    void execConnect(const char *signal, 
                     QObject *receiver, const char *member);
    void execDisconnect(const char *signal, 
                        QObject *receiver, const char *member);
    void execSendEvent(QObject *receiver, QEvent *event);
    QWidget* execWidgetAt(int x, int y, bool child);
    QPoint execCursorPos();
    void execSetCursorPos(QPoint pos);
    QWidgetList* execTopLevelWidgets();
    QWidget* execGetNativeFocusOwner();
    QWidget* execGetNativeFocusedWindow();
    void execSyncX();   //6271724
    void execExitApp(int ret); // 6176847
};

#endif /* _QpOBJECT_H_ */
