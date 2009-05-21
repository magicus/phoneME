/*
 * @(#)QpPushButton.cc	1.6 06/10/25
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

#include  <qpushbutton.h>
#include "QpWidgetFactory.h"
#include "QpPushButton.h"

/*
 * QpPushButton Methods
 */
QpPushButton::QpPushButton(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createPushButton(parent,
                                                                    name,
                                                                    flags);
    this->setQWidget(widget);
}

void
QpPushButton::setText(QString &text) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&text ;
    invokeAndWait(QpPushButton::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpPushButton::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpPushButton, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpPushButton::SetText: {
        QString *str = (QString *)((qtMethodParam *)args)->param;
        execSetText(*str);
        }
        break ;
    default :
        break;
    }
}
        
void
QpPushButton::execSetText(QString &text){
    QPushButton *button = (QPushButton *)this->getQWidget();
    button->setText(text);
}
