/*
 * @(#)QtSync.h	1.9 06/10/25
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
#ifndef _QtSYNC_H_
#define _QtSYNC_H_

#include <limits.h> // for ULONG_MAX

#ifdef QT_THREAD_SUPPORT 
#  if (QT_VERSION >= 0x030000)
#include <qmutex.h>
#include <qwaitcondition.h>
#  endif /* QT_VERSION */
#include <qthread.h>
#define AWT_QT_CURRENT_THREAD() ((void *)QThread::currentThread())
#define QtMutex QMutex
#else  /* QT_THREAD_SUPPORT */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/sync.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */
#define AWT_QT_CURRENT_THREAD() ((void *)CVMthreadSelf())
class QtMutex {
    friend class QtCondVar; // to access m_cvm_mutex
private :
    CVMMutex m_cvm_mutex ;
public :
    QtMutex() ;
    virtual ~QtMutex() ;
    void lock() ;
    void unlock() ;
};
#endif /* QT_THREAD_SUPPORT */

/**
 */
class QtCondVar {
private :
    QtMutex *m_mutex ;
#ifdef QT_THREAD_SUPPORT
    QWaitCondition m_wait_condition ;
#else
    CVMCondVar     m_wait_condition ;
#endif /* QT_THREAD_SUPPORT */
public :
    QtCondVar() ;
    ~QtCondVar() ;
    bool wait(long timeInMillis = ULONG_MAX) ;
    void notify() ;
    void notifyAll() ;
    QtMutex *mutex() { return m_mutex ; }
};

/**
 */
class QtReentrantMutex {
private:
    void    *m_owner;
    int     m_count;
    QtMutex *m_mutex; // non-reentrant-mutex
public:
    QtReentrantMutex();
    ~QtReentrantMutex();
    void lock() ;
    void unlock() ;
};

#endif /* _QtSYNC_H_ */
