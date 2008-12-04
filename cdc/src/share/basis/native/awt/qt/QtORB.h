/*
 *  @(#)QtORB.h	1.7 06/10/25
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
#ifndef _QtORB_H_
#define _QtORB_H_

#include <qobject.h>
#include <qevent.h>
#include <qptrdict.h>
#include "QtEvent.h"

/*
 * The protected virtual method "customEvent()" which the ORB relies on
 * is present in QWidget in qt2 and from qt3 it got moved up to QObject
 */
#if (QT_VERSION >= 0x030000)
#define QT_ORB_BASE_CLASS QObject
#else
#include <qwidget.h>
#define QT_ORB_BASE_CLASS QWidget
#endif /* QT_VERSION */

#define __ALLOC_METHOD_ARGS_ON_STACK 

#ifdef __ALLOC_METHOD_ARGS_ON_STACK
#define QT_METHOD_ARGS_ALLOC(_type, _ref) _type args; _type * _ref = &args
#define QT_METHOD_ARGS_FREE(_ref)
#else
#define QT_METHOD_ARGS_ALLOC(_type, _ref) \
        _type * _ref = (_type *)malloc(sizeof(_type))
#define QT_METHOD_ARGS_FREE(_ref) free(_ref)
#endif /* __ALLOC_METHOD_ARGS_ON_STACK */

typedef struct {
    void *param ;
} qtMethodParam ;

typedef struct {
    void *rvalue ;
} qtMethodReturnValue ;

typedef struct {
    qtMethodParam in ;
    qtMethodReturnValue out ;
} qtMethodArgs ;
    
class QpObject; // forward decl
/**
 */
class QtORB : public QT_ORB_BASE_CLASS {
private :
    // Key   : ThreadId 
    // Value : -1 or valid method Id
    QPtrDict<void> m_method_done_map ;
public :
    void invoke(QpObject *targetObj,
                int methodId, 
                void *args,
                bool wait=FALSE,
                long timeInMillis=ULONG_MAX);
    int getMethod(void *threadId) ;
    void setMethod(void *threadId, int methodId) ;
    static QtORB * instance() ;

protected :
    virtual void customEvent( QCustomEvent * );
};

#endif /* _QtORB_H_ */
