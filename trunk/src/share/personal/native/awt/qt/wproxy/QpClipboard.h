/*
 * @(#)QpClipboard.h	1.8 06/10/25
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
#ifndef _QpCLIPBOARD_h_
#define _QpCLIPBOARD_h_

#include <qclipboard.h>
#include "QpObject.h"

class QpClipboard : public QpObject {
public :
    static QpClipboard *instance() ;
    QpClipboard();

    QClipboard *getQClipboard() {
        return (QClipboard *)this->m_qobject;
    }
    void setText(QString *str);
    QString text(void);
#if (QT_VERSION >= 0x030000)
    void setText(QString *str, QClipboard::Mode mode);
    QString text(QClipboard::Mode mode);
#endif
protected :
    enum MethodId {
        SOM = QpObject::EOM,
        SetText = QpClipboard::SOM,
        Text,
	SetText_Mode,
	Text_Mode,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    static QpClipboard *SystemClipboard; // 6176847
    void execSetText(QString *str);
    QString execText(void);
#if (QT_VERSION >= 0x030000)
    void execSetText(QString *str, QClipboard::Mode mode);
    QString execText(QClipboard::Mode mode);
#endif
};

#endif
