/*
 * @(#)Qt3TextAreaPeer.h	1.11 06/10/25
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
#ifndef _QtTEXT_AREA_PEER_H_
#define _QtTEXT_AREA_PEER_H_

#include "QtTextComponentPeer.h"

class QtTextAreaPeer : public QtTextComponentPeer
{
    Q_OBJECT
public:
    /* Constructor of this class */
    QtTextAreaPeer (JNIEnv* env,
                    jobject thisObj,
                    QpWidget* textAreaWidget );
    /* Destructor of this class */ 
    ~QtTextAreaPeer ();
    virtual void dispose(JNIEnv *env);

    /* Pure vitual functions from the parent class QtTextComponentPeer */
    void setEditable(jboolean editable);
    jstring getTextNative(JNIEnv* env);
    void setTextNative(JNIEnv* env, jstring text);  
    int getSelectionStart();
    int getSelectionEnd();
    void setCaretPosition(JNIEnv* env, jint pos);
    int getCaretPosition();
    void select(JNIEnv* env, jint selStart, jint selEnd); 

public slots: 
    void textChanged ();

private:
    QWidget* viewport;
};

#endif
