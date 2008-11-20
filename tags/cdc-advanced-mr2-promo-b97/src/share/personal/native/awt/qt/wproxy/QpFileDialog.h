/*
 * @(#)QpFileDialog.h	1.8 06/10/25
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
#ifndef _QpFILE_DIALOG_H_
#define _QpFILE_DIALOG_H_

#include "QpWidget.h"

class QpFileDialog : public QpWidget {
public :
    QpFileDialog(QpWidget *parent, char *name = "QtFileDialog", int flags = 0);
    /* QxFileDialog Methods */
     void setDirectoryName(QString dir);
     void setFileName(QString file);
     void populateFileList(bool emptySelection = false);
     void setWarningLabelHeight(int height); //6393054
 
protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetDirectoryName = QpFileDialog::SOM,
        SetFileName,
        PopulateFileList,
        SetWarningLabelHeight, //6393054
        EOM
    };

    virtual void execute(int method, void *args);
private:
     void execSetDirectoryName(QString dir);
     void execSetFileName(QString file);
     void execPopulateFileList(bool emptySelection = false);
     void execSetWarningLabelHeight(int height); //6393054
};

#endif 
