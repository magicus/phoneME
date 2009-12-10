/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * 
 * This source file is specific for Qt-based configurations.
 */


/**
 * @file
 * Definition for Qt port of input mode.
 */

#ifndef _LFPPROT_QTE_INPUTMODE_H_
#define _LFPPROT_QTE_INPUTMODE_H_

#ifndef QT_NO_QWS_KEYBOARD

#include <qwindowsystem_qws.h>
#include <qtimer.h>


#define CLICK_DELAY 1000

class AbcKeyboardFilter;

class KeyTimerHandler : public QObject {
    friend class AbcKeyboardFilter;        
    Q_OBJECT
        
private:
    AbcKeyboardFilter* parent;
    KeyTimerHandler(AbcKeyboardFilter* p)
        : QObject(0, "KeyTimerHandler") {
        parent = p;
    };
    
protected slots:
    void processKey();
};


class AbcKeyboardFilter : public QWSServer::KeyboardFilter {
    friend class KeyTimerHandler;        
private:
    QWSServer* server;
    int code;
    int counter;
    bool caps;
        
    KeyTimerHandler* tHandler;
    QTimer* timer;
    void accept();
    void clear();

public:
    AbcKeyboardFilter();
    virtual bool filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat);
    virtual ~AbcKeyboardFilter(){ };
};

#endif

#endif /* _LFPPROT_QTE_INPUTMODE_H_ */
