/*
 * @(#)QpComboBox.h	1.6 06/10/25
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
#ifndef _QpCOMBO_BOX_H_
#define _QpCOMBO_BOX_H_

#include "QpWidget.h"

class QpComboBox : public QpWidget {
public :
    QpComboBox(bool rw, QpWidget *parent, 
               char *name = "QtComboBox",int flags = 0);
    /* QComboBox Methods */
    void insertItem(QString &t, int index = -1);
    void removeItem(int index);
    void setCurrentItem(int index);

protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        InsertItem = QpComboBox::SOM,
        RemoveItem,
        SetCurrentItem,
        EOM
    };

    virtual void execute(int method, void *args);
private:
    void execInsertItem(QString &t, int index = -1);
    void execRemoveItem(int index);
    void execSetCurrentItem(int index);
};

#endif
