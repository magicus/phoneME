/*
 * @(#)QpRadioButton.cc	1.6 06/10/25
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

#include  <qradiobutton.h>
#include "QpWidgetFactory.h"
#include "QpRadioButton.h"

/*
 * QpRadioButton Methods
 */
QpRadioButton::QpRadioButton(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createRadioButton(parent,
                                                                    name,
                                                                    flags);
    this->setQWidget(widget);
}

void
QpRadioButton::setText(QString &text) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&text ;
    invokeAndWait(QpRadioButton::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpRadioButton::setChecked(bool check){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)check;
    invokeAndWait(QpRadioButton::SetChecked,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpRadioButton::isOn(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpRadioButton::IsOn,argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpRadioButton::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpRadioButton, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpRadioButton::SetText: {
        QString *str = (QString *)((qtMethodParam *)args)->param;
        execSetText(*str);
        }
        break ;
    case QpRadioButton::SetChecked :
        execSetChecked((bool)((qtMethodParam *)args)->param);
        break;

    case QpRadioButton::IsOn :
        ((qtMethodReturnValue *)args)->rvalue = (void *)execIsOn();
        break;
                         
    default :
        break;
    }
}
        
void
QpRadioButton::execSetText(QString &text){
    QRadioButton *button = (QRadioButton *)this->getQWidget();
    button->setText(text);
}

void 
QpRadioButton::execSetChecked(bool check){
    QRadioButton *button = (QRadioButton *)this->getQWidget();
    button->setChecked(check);
}

bool
QpRadioButton::execIsOn() {
    QRadioButton *button = (QRadioButton *)this->getQWidget();
    return button->isOn();
}
