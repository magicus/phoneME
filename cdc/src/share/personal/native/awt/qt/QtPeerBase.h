/*
 * @(#)QtPeerBase.h	1.5 06/10/25
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
#ifndef _QTPEERBASE_H_
#define _QTPEERBASE_H_


#include "jni.h"
#include<qobject.h>


class QpWidget;

class QtPeerBase : public QObject
{
public:
    QtPeerBase(QObject* parent = 0, const char* name = 0):
        QObject(parent, name)
    {
    }

public:
    virtual void dispose(JNIEnv* env) = 0;

    virtual QpWidget* getWidget() const = 0;

    virtual jobject getPeerGlobalRef() const = 0;
};


#endif /* #ifndef _QTPEERBASE_H_ */
