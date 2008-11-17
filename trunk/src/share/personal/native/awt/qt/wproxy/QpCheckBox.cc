/*
 * @(#)QpCheckBox.cc	1.6 06/10/25
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

#include  <qcheckbox.h>
#include "QpWidgetFactory.h"
#include "QpCheckBox.h"

/*
 * QpCheckBox Methods
 */
QpCheckBox::QpCheckBox(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createCheckBox(parent,
                                                                    name,
                                                                    flags);
    this->setQWidget(widget);
}

void
QpCheckBox::setText(QString &text) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&text ;
    invokeAndWait(QpCheckBox::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpCheckBox::setChecked(bool check){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)check;
    invokeAndWait(QpCheckBox::SetChecked,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpCheckBox::isOn(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpCheckBox::IsOn,argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpCheckBox::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpCheckBox, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpCheckBox::SetText: {
        QString *str = (QString *)((qtMethodParam *)args)->param;
        execSetText(*str);
        }
        break ;
    case QpCheckBox::SetChecked :
        execSetChecked((bool)((qtMethodParam *)args)->param);
        break;
                         
    case QpCheckBox::IsOn :
        ((qtMethodReturnValue *)args)->rvalue = (void *)execIsOn();
        break;

    default :
        break;
    }
}
        
void
QpCheckBox::execSetText(QString &text){
    QCheckBox *button = (QCheckBox *)this->getQWidget();
    button->setText(text);
}

void 
QpCheckBox::execSetChecked(bool check){
    QCheckBox *button = (QCheckBox *)this->getQWidget();
    button->setChecked(check);
}

bool
QpCheckBox::execIsOn() {
    QCheckBox *button = (QCheckBox *)this->getQWidget();
    return button->isOn();
}
