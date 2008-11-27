/*
 *  @(#)QtMenuComponentPeer.h	1.8 06/10/25
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
#ifndef _QtMENU_COMPONENT_PEER_H_
#define _QtMENU_COMPONENT_PEER_H_

#include "awt.h"
#include <qobject.h>
#include "QpWidget.h"
#include "QtPeerBase.h"

class QtMenuComponentPeer : public QtPeerBase
{
 public:
    QtMenuComponentPeer(JNIEnv *env, jobject thisObj, QpWidget *widget);
    virtual ~QtMenuComponentPeer();

    virtual void dispose(JNIEnv *env);
    QpWidget *getWidget(void) const;
    virtual jobject getPeerGlobalRef(void) const;
    virtual void setFont(QFont font);

 protected:
    static void *getPeerFromJNI(JNIEnv *env, 
                                jobject peer,
                                jfieldID dataFID);

 private:
    /* Menu Compoenent widget */
    QpWidget *widget;

    /* A global reference to the peer. This can be used by
       callbacks. */
    jobject peerGlobalRef;
};

#endif /* _QtMENU_COMPONENT_PEER_H_ */
