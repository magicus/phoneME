/*
 *  @(#)QpObject.cpp	1.14 06/10/25
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
#include "QpObject.h"
#include <qapplication.h>
#include <stdio.h>

/*
 * QpObject methods
 */

#define QT_ORB_INVALID_METHOD_ID     (-1)

QpObject *QpObject::QObjectInstance = new QpObject(); // 6176847

void 
QpObject::deleteQObjectLater(QObject *qobj){
    QObjectInstance->deleteLater(qobj);
}

// 6176847
void 
QpObject::exitQApplication(int ret) {
    QObjectInstance->exitApp(ret);
}
// 6176847

QpObject::QpObject(void) {
    m_object_mutex     = new QtMutex() ;
    m_signal_condition = new QtCondVar() ;
    m_sync_invocations = 0;
    m_tobe_deleted     = FALSE;
}

QpObject::~QpObject(void) {
    delete this->m_object_mutex ;
    delete this->m_signal_condition ;
    this->m_object_mutex = NULL;
    this->m_signal_condition = NULL;
}

void
QpObject::waitForExecutionDone(QtORB *orb,
                               int methodId, 
                               void *txid, 
                               long timeInMills) {
    QtMutex *signal_mutex = m_signal_condition->mutex() ;
    signal_mutex->lock() ;
    while ( orb->getMethod(txid) != methodId ) {
        if ( !m_signal_condition->wait(timeInMills) ) {
            break ; // timedout, so just return instead of wait on the
                    // condition
        }
    }
    orb->setMethod(txid, QT_ORB_INVALID_METHOD_ID) ;
    signal_mutex->unlock() ;
}

void
QpObject::notifyExecutionDone(QtORB *orb,
                              int methodId, 
                              void *txid) {
    QtMutex *signal_mutex = m_signal_condition->mutex() ;
    signal_mutex->lock() ;

    // set the completed method for the transaction
    orb->setMethod(txid, methodId) ;

    // notify all threads waiting on this condition
    this->m_signal_condition->notifyAll() ;

    signal_mutex->unlock() ;
}

void 
QpObject::invokeLater(int methodId, void *args) {
    QtORB *orb = QtORB::instance() ;
    orb->invoke(this, methodId, args) ;
}

/*
 * QpObject Deletion Design 
 * 
 * The client calls QpObject::deleteLater() when it no longer needs the
 * object. In response to that we call QObject::deleteLater (qt3) and
 * check if there are any synchronous method invocations outstanding
 * (QpObject.m_sync_invocations), if so we mark the QpObject for deletion
 * (QpObject.m_tobe_deleted) and return. If there are no outstanding
 * synchronous invocations, we free the QpObject and expect the caller 
 * not to make any other invocations on the QpObject.
 *
 * The QpObject::invokeAndWait() frees the QpObject if it was marked 
 * for deletion (QpObject.m_tobe_deleted = TRUE) and there are no
 * other outstanding synchronous invoctions.
 */
void 
QpObject::invokeAndWait(int methodId, void *args, long timeInMillis) {
    this->m_object_mutex->lock();
    this->m_sync_invocations ++;
    this->m_object_mutex->unlock();

    QtORB *orb = QtORB::instance() ;
    orb->invoke(this, methodId, args, TRUE, timeInMillis) ;

    this->m_object_mutex->lock();
    this->m_sync_invocations -- ;
    bool deleteThis = (this->m_tobe_deleted && !this->m_sync_invocations);
    this->m_object_mutex->unlock();

    if ( deleteThis ) 
        delete this;
}

void
QpObject::deleteLater() {
#if (QT_VERSION >= 0x030000)
    this->m_qobject->deleteLater();
    bool deleteThis = FALSE;
    this->m_object_mutex->lock();
    if ( this->m_sync_invocations > 0 ){
        this->m_tobe_deleted = TRUE;
    }
    else {
        deleteThis = TRUE;
    }
    this->m_object_mutex->unlock();
    if ( deleteThis )
        delete this;
#else
    invokeLater(QpObject::DeleteLater,NULL) ;
#endif /* QT_VERSION */
}

void
QpObject::deleteLater(QObject *obj) {
#if (QT_VERSION >= 0x030000)
    obj->deleteLater();
#else
    invokeLater(QpObject::DeleteLater_QObject,obj) ;
#endif /* QT_VERSION */
}

// 6176847
void
QpObject::exitApp(int ret) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)ret;
    invokeAndWait(QpObject::ExitApp, argp);
    QT_METHOD_ARGS_FREE(argp);
}
// 6176847

void
QpObject::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpObject, mid);

    switch ( mid ) {
    case QpObject::DeleteLater :
        execDeleteLater();
        break ;
    case QpObject::DeleteLater_QObject :
        execDeleteLater((QObject *)args);
        break ;
    case QpObject::ExitApp : // 6176847
        execExitApp((int)((qtMethodParam *)args)->param);
        break;

    default :
        printf("QpObject::execute %d Unknown Method ID\n", mid);
    }
}

void
QpObject::execDeleteLater() {
    execDeleteLater(this->m_qobject);
    delete this ;
}

void
QpObject::execDeleteLater(QObject *obj) {
    // Send a Custom Event to all event filters listening on
    // the "object" that the "object" is going to be deleted. We use
    // sendEvent() instead of postEvent() since we are currently in the
    // Qt GUI thread and is safe to synchronously post the event. 
    QtEvent ce(QtEvent::DeferredDelete) ;
    QApplication::sendEvent(obj, &ce) ;

    // TODO : Try flushing all the events for "object" before calling 
    //        delete
    delete obj;
}

// 6176847
void 
QpObject::execExitApp(int ret) {
    qApp->exit(ret);
}
// 6176847

