/*
 * @(#)QpRadioButton.h	1.6 06/10/25
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
#ifndef _QpRADIO_BUTTON_H_
#define _QpRADIO_BUTTON_H_

#include <qstring.h>
#include "QpWidget.h"

class QpRadioButton : public QpWidget {
public :
    QpRadioButton(QpWidget *parent, char *name="QtRadioButton",int flags = 0);
    /* QButton Methods */
    void setText(QString &text);

    /* QRadioButton Methods */
    void setChecked(bool);
    bool isOn(void);
protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetText = QpRadioButton::SOM,
        SetChecked,
        IsOn,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    void execSetChecked(bool);
    void execSetText(QString &text);
    bool execIsOn(void);
};

#endif
