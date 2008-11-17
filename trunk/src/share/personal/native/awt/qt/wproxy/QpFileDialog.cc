/*
 * @(#)QpFileDialog.cc	1.8 06/10/25
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

#include "QpWidgetFactory.h"
#include "QpFileDialog.h"
#include "QxFileDialog.h"

/*
 * QpFileDialog Methods
 */
QpFileDialog::QpFileDialog(QpWidget *parent,char *name,int flags){
    QWidget *widget = QpWidgetFactory::instance()->createFileDialog(parent,
                                                                    name,
                                                                    flags);
    this->setQWidget(widget);
}

void
QpFileDialog::setDirectoryName(QString dir) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&dir;
    invokeAndWait(QpFileDialog::SetDirectoryName,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpFileDialog::setFileName(QString file) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&file ;
    invokeAndWait(QpFileDialog::SetFileName,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpFileDialog::populateFileList(bool emptySelection) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)emptySelection;
    invokeAndWait(QpFileDialog::PopulateFileList,argp);
    QT_METHOD_ARGS_FREE(argp);
}

/*
 * 6393054
 */
void
QpFileDialog::setWarningLabelHeight(int height) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)height;
    invokeAndWait(QpFileDialog::SetWarningLabelHeight,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpFileDialog::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpFileDialog, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpFileDialog::SetDirectoryName: {
        QString *str = (QString *)((qtMethodParam *)args)->param;
        execSetDirectoryName(*str);
        }
        break ;

    case QpFileDialog::SetFileName: {
        QString *str = (QString *)((qtMethodParam *)args)->param;
        execSetFileName(*str);
        }
        break ;

    case QpFileDialog::PopulateFileList: {
        bool emptySelection = (bool)((qtMethodParam *)args)->param;
        execPopulateFileList(emptySelection);
        }
        break ;

    case QpFileDialog::SetWarningLabelHeight: {
        // 6393054
        int height = (int)((qtMethodParam *)args)->param;
        execSetWarningLabelHeight(height);
        }
        break ;

    default :
        break;
    }
}
        
void
QpFileDialog::execSetDirectoryName(QString dir){
    QxFileDialog *fdialog = (QxFileDialog *)this->getQWidget();
    fdialog->setDirectoryName(dir);
}

void
QpFileDialog::execSetFileName(QString file){
    QxFileDialog *fdialog = (QxFileDialog *)this->getQWidget();
    fdialog->setFileName(file);
}

void
QpFileDialog::execPopulateFileList(bool emptySelection){
    QxFileDialog *fdialog = (QxFileDialog *)this->getQWidget();
    fdialog->populateFileList(emptySelection);
}

/*
 * 6393054
 */
void
QpFileDialog::execSetWarningLabelHeight(int height){
    QxFileDialog *fdialog = (QxFileDialog *)this->getQWidget();
    fdialog->setWarningLabelHeight(height);
}

