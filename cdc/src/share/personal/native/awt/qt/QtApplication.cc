/*
 *  @(#)QtApplication.cc	1.12 06/10/25
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
#include "QtApplication.h"
#include <stdio.h>

// 6176847 : exit codes used when calling QApplication::exit()
#define NORMAL_EXIT_CODE   (0)
#define SHUTDOWN_EXIT_CODE (1)

static QtORB *qt_orb_instance ;
QtORB *
QtORB::instance() {
    return qt_orb_instance ;
}

static void *qtEventThreadID ;
QtApplication::QtApplication(int &argc, 
                             char **argv) :QAPPLICATION_CLASS(argc,argv) {
#ifndef QT_THREAD_SUPPORT
    m_shutdown = FALSE; // 6176847: Indicates the call to shutdown()
#endif /* QT_THREAD_SUPPORT */
    m_is_event_loop_started = FALSE ;
    // 6176847
    // a condition variable and the associated flag that is used by the
    // shutdown() thread to know if exec() has terminated.
    m_exec_terminated       = FALSE ;
    m_shutdown_condition    = new QtCondVar();
    // 6176847
    qtEventThreadID = AWT_QT_CURRENT_THREAD() ;
    // The ORB's base class could be a QWidget and we cannot create an
    // instance of QWidget with the QApplication being created, so we 
    // create the ORB here.
    qt_orb_instance = new QtORB();
} 

int
QtApplication::exec(QObject* eventFilter) {
#if 0 // 6207033
    printf("*** QtApplication.exec(tid=%x) ***\n", 
           (int)AWT_QT_CURRENT_THREAD()) ;
#endif

    installEventFilter(eventFilter);

    m_is_event_loop_started = TRUE ;
    int ret = NORMAL_EXIT_CODE;
#ifdef QT_THREAD_SUPPORT
    ret = QAPPLICATION_CLASS::exec() ;
#else
    QtCondVar timer ;
    while ( !m_shutdown ) { // 6176847:Loop till we shutdown.
        AWT_QT_LOCK;
        this->processEvents(0);
        AWT_QT_UNLOCK;

        /* sleep for some time */
        timer.wait(100) ;
    }
    // 6176847: set the return code to simulate the same behavior as 
    //          exec() return value.
    ret = SHUTDOWN_EXIT_CODE;
#endif /* QT_THREAD_SUPPORT */

    removeEventFilter(eventFilter);

    // 6176847
    // if the event loop is stopped as part of the shutdown sequence then
    // create the illusion that the event loop is running. This will 
    // force the proxy calls to post the custom event and block forever.
    // If we dont do that, then proxy would directly call the Qt API
    // which we dont want during shutdown() (See QtORB::invoke())
    if ( ret == SHUTDOWN_EXIT_CODE ) {
        QtMutex *mutex = this->m_shutdown_condition->mutex();
        mutex->lock() ;
        this->m_exec_terminated = TRUE;
        this->m_shutdown_condition->notifyAll();
        mutex->unlock() ;
    }
    else {
        m_is_event_loop_started = FALSE ;
    }
    // 6176847
    return ret;
}

bool
QtApplication::isQtEventThread() {
    return (AWT_QT_CURRENT_THREAD() == qtEventThreadID) ;
}

// 6176847
void 
QtApplication::shutdown() {
    // there are cases where the event loop maynot be started, so 
    // without this check we would block forever for exec() to return
    if ( this->m_is_event_loop_started ) {
        // force the qt event loop to exit.
#ifdef QT_THREAD_SUPPORT
        // Calling QApplication::exit() on the event thread is the safest
        // way to ensure that the event loop terminates.
        QpObject::exitQApplication(SHUTDOWN_EXIT_CODE);
#else
        this->m_shutdown = TRUE;
#endif /* QT_THREAD_SUPPORT */

        // wait till exec() is finished
        QtMutex *mutex = this->m_shutdown_condition->mutex();
        mutex->lock() ;
        while ( !this->m_exec_terminated ) {
            this->m_shutdown_condition->wait();
        }
        mutex->unlock() ;
    }

    // Now that the event loop is terminated, grab the application
    // lock and exit this method. Once the lock is grabbed, peer code 
    // as well qt's window system code cannot mutate anything in the
    // qt library and is a safe point to call exit().
    AWT_QT_LOCK;
}
// 6176847

void
QtApplication::lock() {
#ifdef QT_THREAD_SUPPORT
    QAPPLICATION_CLASS::lock();
#else
    this->m_mutex.lock();
#endif /* QT_THREAD_SUPPORT */
}

void
QtApplication::unlock(bool wakeUpGUI) {
#ifdef QT_THREAD_SUPPORT
    QAPPLICATION_CLASS::unlock(wakeUpGUI);
#else
    this->m_mutex.unlock();
#endif /* QT_THREAD_SUPPORT */
}

void
QtApplication::setMainWidget(QpWidget *widget) {
    QAPPLICATION_CLASS::setMainWidget((QWidget *)widget->getQWidget());
}
