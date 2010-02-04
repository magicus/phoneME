/*
 * @(#)QtTextComponentPeer.h	1.7 06/10/25
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
#ifndef _QtTEXT_COMPONENT_PEER_H_
#define _QtTEXT_COMPONENT_PEER_H_

#include "QtComponentPeer.h"

class QtTextComponentPeer : public QtComponentPeer
{
public: 
    QtTextComponentPeer(JNIEnv *env,
                        jobject thisObj,
                        QpWidget* panelWidget);
    
    ~QtTextComponentPeer(void);
    virtual void setEditable(jboolean editable) = 0;
    virtual jstring getTextNative(JNIEnv* env) = 0;
    virtual void setTextNative(JNIEnv* env, jstring text) = 0;
    
    virtual int getSelectionStart() = 0;
    virtual int getSelectionEnd() = 0;
    virtual void select(JNIEnv* env, jint selStart, jint selEnd) = 0;
    
    virtual void setCaretPosition(JNIEnv* env, jint pos) = 0;
    virtual int getCaretPosition() = 0;

private:
  
};

#endif
