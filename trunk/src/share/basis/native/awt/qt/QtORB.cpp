/*
 *  @(#)QtORB.cpp	1.8 06/10/25
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
#include "QtORB.h"
#include "QtApplication.h"
#include "QpObject.h"
#include <stdio.h>

#define QT_ORB_INVALID_METHOD_ID     (-1)

//#define QT_ORB_DEBUG

#ifdef QT_ORB_DEBUG
#define QT_ORB_PRINT_METHOD_INFO(_pre,_tObj,_mid,_a,_wait,_txid,_suf) \
{\
    const char *name = (_tObj->m_qobject)?_tObj->m_qobject->name():\
                                                     "nullqobject";\
    printf("%s QtORB(pObj=%x,qObj=%x,name=%s,mid=%d,args=%x,txid=%x %s)%s\n",\
           _pre, _tObj, _tObj->m_qobject,name,_mid,_a,_txid,            \
           (_wait ? "SYNC" : "ASYNC"),_suf);                            \
}
#else
#define QT_ORB_PRINT_METHOD_INFO(_pre,_tObj,_mid,_a,_wait,_txid,_suf)
#endif

/**
 */
class QtMethodCallEvent : public QtEvent {
 private :
    int  m_method ;
    void *m_args;
    void *m_transaction_id ;
    bool m_synchronous_call;

 public :
    QtMethodCallEvent(void *qobject,
                      int mid,
                      void *args,
                      void *txid,
                      bool isSync) :
        QtEvent(MethodCall, qobject),
        m_method(mid),
        m_args(args),
        m_transaction_id(txid),
        m_synchronous_call(isSync){}

    int method()             { return m_method ; }
    void *transaction()      { return m_transaction_id ; }
    void *args()             { return m_args ; }
    bool isCallSynchronous() { return m_synchronous_call ; }
};

/*
 * QtORB methods
 */

/* Note : the static method QtORB::instance() is implemented in 
 * QtApplication.cc, since the ORB is only created when the QApplication
 * is created.
 */

int 
QtORB::getMethod(void *threadId) {
    // the value returned are 
    // 0 - indicates that no value exist for the id
    // QT_ORB_INVALID_METHOD_ID 
    // id greater than or equal to one
    return (int)m_method_done_map.find(threadId) ;
}

void 
QtORB::setMethod(void *threadId, int methodId) {
    m_method_done_map.replace(threadId,(void *)(methodId)) ;
}

void
QtORB::customEvent( QCustomEvent * e ) {
    if ( (int)e->type() != (int)QtEvent::MethodCall ) {
        return ;
    }

    QtMethodCallEvent *mce = (QtMethodCallEvent *)e ;
    QpObject *targetObj = (QpObject *)e->data() ;
    int mid = mce->method();
    void *txid = mce->transaction();

    QT_ORB_PRINT_METHOD_INFO("%%",targetObj, mid, mce->args(),
                             mce->isCallSynchronous(),txid,"%%");

    // execute the method
    targetObj->execute(mid, mce->args());

    // if the call is synchronous (meaning the caller is waiting for
    // completion) then notify.
    if ( mce->isCallSynchronous()) {
        targetObj->notifyExecutionDone(this,mid, txid);
    }
}

void
QtORB::invoke(QpObject *targetObj,
              int methodId, 
              void *args, 
              bool wait,
              long timeInMillis) {
    bool isEventLoopRunning = QtApplication::instance()->isEventLoopRunning();

    // We execute the method (as specified in the methodId) synchronously
    // in the context of this thread for the following cases
    // 1) this thread is the Qt's main event thread
    // 2) the event loop is not running and the caller wants us to "wait".
    //    Rationale as follows, 
    //    If the qt event loop is not running and if the caller wants us to
    //    wait, then we could be waiting for ever (consider the case when 
    //    this method is called on the "main" thread which is supposed to
    //    start the qt event pump). 
    if ( (!isEventLoopRunning && wait) || QtApplication::isQtEventThread()) {
        // execute the target method synchronously.
        QT_ORB_PRINT_METHOD_INFO("**",
                                 targetObj,
                                 methodId,
                                 args,
                                 wait,
                                 0,
                                 "**");
        targetObj->execute(methodId, args);
        return ;
    }

#ifdef QT_THREAD_SUPPORT
    // Use the current thread id as the transaction identifier.
    void *txid = AWT_QT_CURRENT_THREAD() ;

    // create a method call "custom event"
    QtMethodCallEvent *event = new QtMethodCallEvent(targetObj, 
                                                     methodId, 
                                                     args, 
                                                     txid,
                                                     wait); // sync or asycnc
    if ( !event ) {
        return ;
    }

    QT_ORB_PRINT_METHOD_INFO(">>",targetObj, methodId, args, wait, txid,"<<");

    // post the method call event to the orb
    // Note :- Qt takes owenership of "event" and destroys it at the right
    //         time, so we dont have to bother about deleting it.
    //         (See QApplication::postEvent() for details)
    QApplication::postEvent(this, event) ;

    // if the caller wants us to wait till the execution is complete, wait.
    // we would be notified when the execution finishes.
    if ( wait ) {
        targetObj->waitForExecutionDone(this,methodId, txid, timeInMillis) ;
    }

    QT_ORB_PRINT_METHOD_INFO("<<",targetObj,methodId,args,wait,txid,">>");
#else
    AWT_QT_LOCK;
    targetObj->execute(methodId, args);
    AWT_QT_UNLOCK;
#endif
}

