/*
 *  @(#)QpObject.h	1.14 06/10/25
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
    static void exitQApplication(int ret); // 6176847

    QpObject();
    virtual ~QpObject();
    inline bool isSameObject(QObject *obj) { return obj == m_qobject ; }

    /* QObject Methods */
    void deleteLater() ;

protected :
    enum MethodId {
        DeleteLater = 0,
        DeleteLater_QObject, // private
        ExitApp,             // private
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

    void waitForExecutionDone(QtORB *orb,
                              int methodId, 
                              void *txId, 
                              long timeInMills) ;
    void notifyExecutionDone(QtORB *orb,
                             int methodId, 
                             void *txId) ;

    void execDeleteLater(QObject *obj) ;
    void execDeleteLater(void) ;
    void execExitApp(int ret); // 6176847
};

#endif /* _QpOBJECT_H_ */
