/*
 *  @(#)QtApplication.h	1.10 06/10/25
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
#ifndef _QtAPPLICATION_H_
#define _QtAPPLICATION_H_

#ifdef QWS
#  ifdef QTOPIA
#    include <qpe/qpeapplication.h>
#    define QAPPLICATION_CLASS QPEApplication
#  else 
#    include <qapplication.h>
#    define QAPPLICATION_CLASS QApplication
#  endif /* QTOPIA */
#else
#  include <qapplication.h>
#  define QAPPLICATION_CLASS QApplication
#endif /* QWS */
#include "QtSync.h"

#define qtApp         ((QtApplication *)qApp)
#define AWT_QT_LOCK   (qtApp)->lock() 
#define AWT_QT_UNLOCK (qtApp)->unlock() 

/**
 */
class QtApplication : public QAPPLICATION_CLASS {
private :
#ifndef QT_THREAD_SUPPORT
    // If thread support is defined we use the QApplication mutex else
    // we create a portable reentrant mutex.
    QtReentrantMutex m_mutex;
    bool m_shutdown;                   // 6176847
#endif /* QT_THREAD_SUPPORT */
    bool m_is_event_loop_started ;
    bool m_exec_terminated ;           // 6176847
    QtCondVar *m_shutdown_condition;   // 6176847
public :
    QtApplication(int &argc, char **argv);
    int exec() ; // overloaded exec from QApplication
    bool isEventLoopRunning() { return m_is_event_loop_started ; }
    void lock();
    void unlock(bool wakeUpGui = TRUE); 
    void shutdown(void);  // 6176847

    static QtApplication *instance() { return qtApp ; }
    static bool isQtEventThread() ;
};

#endif /* _QtAPPLICATION_H_ */
