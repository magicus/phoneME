/*
 * @(#)QpLabel.cc	1.6 06/10/25
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

#include  <qlabel.h>
#include "QpWidgetFactory.h"
#include "QpLabel.h"

/*
 * QpLabel Methods
 */
QpLabel::QpLabel(QString text, QpWidget *parent, char *name,int flags){
    QWidget *widget = QpWidgetFactory::instance()->createLabel(text,
                                                               parent,
                                                               name,
                                                               flags);
    this->setQWidget(widget);
}

void
QpLabel::setText(QString text) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&text ;
    invokeAndWait(QpLabel::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpLabel::setAlignment(int align){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)align;
    invokeAndWait(QpLabel::SetAlignment,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpLabel::setFrameStyle(int style){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)style;
    invokeAndWait(QpLabel::SetFrameStyle,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpLabel::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpLabel, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpLabel::SetText: {
        QString *str = (QString *)((qtMethodParam *)args)->param;
        execSetText(*str);
        }
        break ;

    case QpLabel::SetAlignment:
        execSetAlignment((int)((qtMethodParam *)args)->param);
        break;
                         
    case QpLabel::SetFrameStyle:
        execSetFrameStyle((int)((qtMethodParam *)args)->param);
        break;

    default :
        break;
    }
}
        
void
QpLabel::execSetText(QString text){
    QLabel *label = (QLabel *)this->getQWidget();
    label->setText(text);
}

void 
QpLabel::execSetAlignment(int align){
    QLabel *label = (QLabel *)this->getQWidget();
    label->setAlignment(align);
}

void 
QpLabel::execSetFrameStyle(int style){
    QLabel *label = (QLabel *)this->getQWidget();
    label->setFrameStyle(style);
}
