/*
 * @(#)QpRobot.h	1.7 06/10/25
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
#ifndef _QpROBOT_H_
#define _QpROBOT_H_

#include "QpObject.h"

/**
 */
class QpRobot : public QpObject {
private :
    // Indicates if a META key (Ctrl, Shift, Alt) has been typed
    bool m_metaKeyTyped;
    // The meta key will contain the type of meta key typed.
    int m_metaKey;
public :
    QpRobot(QpRobot *parent=NULL, char *name = NULL, int flags = 0);
    virtual ~QpRobot();

    void init();
    void mouseAction(int x, int y, int buttons, bool pressed);
    void keyAction(int keycode, int unicode, bool pressed);
protected :
    enum MethodId {
        SOM = QpObject::EOM,
        Init = QpRobot::SOM,
        MouseAction,
        KeyAction,
        EOM
    };

    virtual void execute(int methodId, void *args) ;

private:
    void execInit();
    void execMouseAction(int x, int y, int buttons, bool pressed);
    void execKeyAction(int keycode, int unicode, bool pressed);
    
};

#endif /* _QpROBOT_H_ */
