/*
 *  @(#)QtSync.cc	1.7 06/10/25
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
#include "QtSync.h"

/*
 * QtMutex methods 
 * Note :we use QMutex is QT_THREAD_SUPPORT is defined. See QtSync.h
 */
#ifndef QT_THREAD_SUPPORT 

QtMutex::QtMutex() {
    CVMmutexInit(&m_cvm_mutex);
}

QtMutex::~QtMutex() {
    CVMmutexDestroy(&m_cvm_mutex);
}

void
QtMutex::lock() {
    CVMmutexLock(&m_cvm_mutex);
}

void
QtMutex::unlock() {
    CVMmutexUnlock(&m_cvm_mutex);
}

#endif /* ! QT_THREAD_SUPPORT */

/*
 * QtConVar methods
 */

QtCondVar::QtCondVar() {
    m_mutex = new QtMutex() ;
#ifdef QT_THREAD_SUPPORT
    // QWaitCondition is constructed automatically
#else    
    CVMcondvarInit(&m_wait_condition, &m_mutex->m_cvm_mutex) ;
#endif /* QT_THREAD_SUPPORT */
}

QtCondVar::~QtCondVar() {
#ifdef QT_THREAD_SUPPORT
    // QWaitCondition is destructed automatically
#else    
    CVMcondvarDestroy(&m_wait_condition) ;
#endif /* QT_THREAD_SUPPORT */
    delete m_mutex ;
}

bool
QtCondVar::wait(long timeInMillis) {
#ifdef QT_THREAD_SUPPORT
    return m_wait_condition.wait(m_mutex, timeInMillis) ;
#else    
    return (bool)CVMcondvarWait(&m_wait_condition, 
                                &m_mutex->m_cvm_mutex, 
                                timeInMillis) ;
#endif /* QT_THREAD_SUPPORT */
}

void
QtCondVar::notify() {
#ifdef QT_THREAD_SUPPORT
    m_wait_condition.wakeOne() ;
#else    
    CVMcondvarNotify(&m_wait_condition) ;
#endif /* QT_THREAD_SUPPORT */
}

void
QtCondVar::notifyAll() {
#ifdef QT_THREAD_SUPPORT
    m_wait_condition.wakeAll() ;
#else    
    CVMcondvarNotifyAll(&m_wait_condition) ;
#endif /* QT_THREAD_SUPPORT */
}


/*
 * QtReentrantMutex methods 
 */

QtReentrantMutex::QtReentrantMutex() {
    this->m_count = 0 ;
    this->m_owner = NULL ;
    this->m_mutex = new QtMutex();
}

QtReentrantMutex::~QtReentrantMutex() {
    delete m_mutex;
}

void
QtReentrantMutex::lock() {
    void *thisThread = AWT_QT_CURRENT_THREAD();
    if ( this->m_owner == thisThread ) {
        this->m_count++;
    }
    else {
        this->m_mutex->lock();
        this->m_owner = thisThread;
        this->m_count = 1;
    }
}

void
QtReentrantMutex::unlock() {
    void *thisThread = AWT_QT_CURRENT_THREAD();
    if ( thisThread == this->m_owner ) {
        this->m_count --;
        if ( this->m_count == 0 ) {
            this->m_owner = NULL;
            this->m_mutex->unlock();
        }
    }
}
