/*
 * @(#)QtDisposer.h	1.7 06/10/25
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
#ifndef _QTDISPOSER_H_
#define _QTDISPOSER_H_


#include "jni.h"


class QtPeerBase;


class QtDisposer
{
    QtDisposer();

private:
    static void
    peerReferenced();

    static void
    peerUnreferenced(JNIEnv* env);

public:
    class Trigger;
    friend class Trigger;

    static void
    postDisposeRequest(QtPeerBase* peer);

public:
    class Trigger
    {
        JNIEnv* env_;
    private:
        /* copy is forbidden */
        Trigger(const Trigger&);
        Trigger& operator = (const Trigger&);

    public:
        Trigger(): env_(0)
        {
            QtDisposer::peerReferenced();
        }

        Trigger(JNIEnv* env): env_(env)
        {
            QtDisposer::peerReferenced();
        }

        ~Trigger()
        {
            QtDisposer::peerUnreferenced(env_);
        }

        void
        setEnv(JNIEnv* env)
        {
            env_ = env;
        }
    };

public:
    static void
    init();
};


#endif /* #ifndef _QTDISPOSER_H_ */
