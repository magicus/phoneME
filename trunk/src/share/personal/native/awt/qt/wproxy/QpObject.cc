/*
 *  @(#)QpObject.cc	1.15 06/10/25
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
#include <qcursor.h>
#include <stdio.h>

/*
 * QpObject methods
 */

typedef struct {
    struct {
        const char *signal;
        QObject *receiver;
        const char *member;
    } in ;
} qtMargsConnect;

typedef struct {
    struct {
        QObject *receiver;
        QEvent *event;
    } in;
} qtMargsSendEvent;

typedef struct {
    struct {
        int x;
        int y;
        bool child;
   } in;
    struct {
        QWidget *rvalue;
    } out ;

} qtMargsWidgetAt;

#define QT_ORB_INVALID_METHOD_ID     (-1)

QpObject *QpObject::QObjectInstance = new QpObject(); // 6176847

void 
QpObject::deleteQObjectLater(QObject *qobj){
    QObjectInstance->deleteLater(qobj);
}

void
QpObject::staticSendEvent(QpObject *receiver, QEvent *event) {
    QObject *obj = receiver->m_qobject;
    QObjectInstance->sendEvent(obj, event);
}

QWidget *
QpObject::staticWidgetAt(int x, int y, bool child) {
   return QObjectInstance->widgetAt(x, y, child);
}

QPoint
QpObject::staticCursorPos() {
   return QObjectInstance->cursorPos();
}

void
QpObject::staticSetCursorPos(QPoint pos) {
   QObjectInstance->setCursorPos(pos);
}

QWidgetList*
QpObject::staticTopLevelWidgets() {
    return QObjectInstance->topLevelWidgets();
}

// 6176847
void 
QpObject::exitQApplication(int ret) {
    QObjectInstance->exitApp(ret);
}
// 6176847

QWidget*
QpObject::staticGetNativeFocusedWindow() {
    return QObjectInstance->getNativeFocusedWindow();
}

QWidget*
QpObject::staticGetNativeFocusOwner() {
    return QObjectInstance->getNativeFocusOwner();
}

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
QpObject::postEvent(QEvent *event){
    QApplication::postEvent(m_qobject, event) ;
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

void
QpObject::sendEvent(QObject *receiver, QEvent *event) {
    QT_METHOD_ARGS_ALLOC(qtMargsSendEvent, argp);
    argp->in.receiver = receiver ;
    argp->in.event = event ;
    invokeAndWait(QpObject::SendEvent, argp);
    QT_METHOD_ARGS_FREE(argp);
}

QWidget *
QpObject::widgetAt(int x, int y, bool child) {
   QT_METHOD_ARGS_ALLOC(qtMargsWidgetAt, argp);
   argp->in.x = x;
   argp->in.y = y;
   argp->in.child = child;
   invokeAndWait(QpObject::WidgetAt, argp);
   QWidget *rv = (QWidget *)argp->out.rvalue;
   QT_METHOD_ARGS_FREE(argp);
   return rv;
}

QPoint
QpObject::cursorPos() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpObject::CursorPos, argp);
    QPoint rv ((QPoint &)argp->rvalue);
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpObject::setCursorPos(QPoint pos) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = &pos;
    invokeAndWait(QpObject::SetCursorPos, argp);
    QT_METHOD_ARGS_FREE(argp);
}

QWidgetList*
QpObject::topLevelWidgets() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpObject::TopLevelWidgets, argp);
    QWidgetList* rv ((QWidgetList*)argp->rvalue);
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget*
QpObject::getNativeFocusedWindow() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpObject::GetNativeFocusedWindow, argp);
    QWidget* rv ((QWidget*)argp->rvalue);
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget*
QpObject::getNativeFocusOwner() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpObject::GetNativeFocusOwner, argp);
    QWidget* rv ((QWidget*)argp->rvalue);
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpObject::installEventFilter(const QObject *filter) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)filter;
    invokeAndWait(QpObject::InstallEventFilter, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpObject::removeEventFilter(const QObject *filter) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)filter;
    invokeAndWait(QpObject::RemoveEventFilter, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpObject::connect(const char *signal, QObject *receiver, const char *member){
    QT_METHOD_ARGS_ALLOC(qtMargsConnect, argp);
    argp->in.signal   = signal ;
    argp->in.receiver = receiver ;
    argp->in.member   = member ;
    invokeAndWait(QpObject::Connect, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpObject::disconnect(const char *signal,QObject *receiver,const char *member){
    QT_METHOD_ARGS_ALLOC(qtMargsConnect, argp);
    argp->in.signal   = signal ;
    argp->in.receiver = receiver ;
    argp->in.member   = member ;
    invokeAndWait(QpObject::Disconnect, argp);
    QT_METHOD_ARGS_FREE(argp);
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

//6271724
void
QpObject::syncX() {
    invokeAndWait(QpObject::SyncX, NULL);
}


void
QpObject::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpObject, mid);

    switch ( mid ) {
    case QpObject::DeleteLater :
        execDeleteLater();
        break ;
    case QpObject::InstallEventFilter :
        execInstallEventFilter((QObject *)((qtMethodParam *)args)->param);
        break;
    case QpObject::RemoveEventFilter :
        execRemoveEventFilter((QObject *)((qtMethodParam *)args)->param);
        break;
    case QpObject::Connect : {
        qtMargsConnect *a = (qtMargsConnect *)args ;
        execConnect(a->in.signal, a->in.receiver, a->in.member);
        }
        break;
    case QpObject::Disconnect : {
        qtMargsConnect *a = (qtMargsConnect *)args ;
        execDisconnect(a->in.signal, a->in.receiver, a->in.member);
        }
        break;
    case QpObject::DeleteLater_QObject :
        execDeleteLater((QObject *)args);
        break ;
    case QpObject::SendEvent: {
        qtMargsSendEvent *a = (qtMargsSendEvent *)args ;
        execSendEvent(a->in.receiver, a->in.event);
    }
	break;
    case QpObject::WidgetAt: {
        qtMargsWidgetAt *a = (qtMargsWidgetAt *)args;
        a->out.rvalue = execWidgetAt(a->in.x, a->in.y, a->in.child);
    }
	break;
    case QpObject::CursorPos: {
        QPoint rv = (QPoint)execCursorPos();
        ((qtMethodReturnValue *)args)->rvalue = &rv;
    }
	case QpObject::SetCursorPos: {
        QPoint *pos = (QPoint *)((qtMethodParam *)args)->param;
        execSetCursorPos(*pos);
        break;
    }
	case QpObject::TopLevelWidgets: {
	    QWidgetList* rv = (QWidgetList *)execTopLevelWidgets();
	    ((qtMethodReturnValue *)args)->rvalue = rv;
	}
    	break;
    case QpObject::ExitApp : // 6176847
        execExitApp((int)((qtMethodParam *)args)->param);
        break;
    case QpObject::GetNativeFocusOwner: {
            QWidget* rv = (QWidget *)execGetNativeFocusOwner();
	    ((qtMethodReturnValue *)args)->rvalue = rv;
    }
        break;
    case QpObject::GetNativeFocusedWindow: {
            QWidget* rv = (QWidget *)execGetNativeFocusedWindow();
	    ((qtMethodReturnValue *)args)->rvalue = rv;
        }
        break;
    case QpObject::SyncX:   //6271724
        execSyncX();
        break ;


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
QpObject::execInstallEventFilter(QObject *filter) {
    this->m_qobject->installEventFilter(filter);
}

void 
QpObject::execRemoveEventFilter(QObject *filter) {
    this->m_qobject->removeEventFilter(filter);
}

void 
QpObject::execConnect(const char *signal, 
                      QObject *receiver, 
                      const char *member){
    QObject::connect(this->m_qobject, signal, receiver, member);
}

void 
QpObject::execDisconnect(const char *signal, 
                         QObject *receiver, 
                         const char *member){
    QObject::disconnect(this->m_qobject, signal, receiver, member);
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

void
QpObject::execSendEvent(QObject *receiver, QEvent *event) {
    QApplication::sendEvent(receiver, event);
}

QWidget *
QpObject::execWidgetAt(int x, int y, bool child) {
   return QApplication::widgetAt(x, y, child);
}

QPoint
QpObject::execCursorPos() {
   return QCursor::pos();
}

void
QpObject::execSetCursorPos(QPoint pos) {
   QCursor::setPos(pos);
}

QWidgetList* 
QpObject::execTopLevelWidgets() {
    return QApplication::topLevelWidgets();
}

QWidget*
QpObject::execGetNativeFocusOwner() {
    return qApp->focusWidget();
}

QWidget*
QpObject::execGetNativeFocusedWindow() {
    return qApp->activeWindow();
}

// 6176847
void 
QpObject::execExitApp(int ret) {
    qApp->exit(ret);
}
// 6176847

//6271724
void 
QpObject::execSyncX() {
    QApplication::syncX();
}
