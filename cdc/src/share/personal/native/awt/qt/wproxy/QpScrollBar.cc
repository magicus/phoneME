/*
 * @(#)QpScrollBar.cc	1.6 06/10/25
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

#include  <qscrollview.h>
#include "QpWidgetFactory.h"
#include "QpScrollBar.h"

/*
 * QpScrollBar Methods
 */
QpScrollBar::QpScrollBar(int type, QpWidget *parent, char *name,int flags){
    QWidget *widget = QpWidgetFactory::instance()->createScrollBar(type,
                                                                   parent,
                                                                   name,
                                                                   flags);
    this->setQWidget(widget);
}

void 
QpScrollBar::setTracking(bool track){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)track;
    invokeAndWait(QpScrollBar::SetTracking,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollBar::setPageStep(int step){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)step;
    invokeAndWait(QpScrollBar::SetPageStep,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollBar::setLineStep(int step){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)step;
    invokeAndWait(QpScrollBar::SetLineStep,argp);
    QT_METHOD_ARGS_FREE(argp);
}

int 
QpScrollBar::pageStep(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpScrollBar::PageStep,argp);
    int rv = (int)argp->rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}

int 
QpScrollBar::lineStep(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpScrollBar::LineStep,argp);
    int rv = (int)argp->rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}

void 
QpScrollBar::setMinValue(int value){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)value;
    invokeAndWait(QpScrollBar::SetMinValue,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollBar::setMaxValue(int value){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)value;
    invokeAndWait(QpScrollBar::SetMaxValue,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollBar::setValue(int value){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)value;
    invokeAndWait(QpScrollBar::SetValue,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpScrollBar::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpScrollBar, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpScrollBar::SetTracking:
        execSetTracking((bool)(((qtMethodParam *)args)->param));
        break ;
                         
    case QpScrollBar::SetPageStep:
        execSetPageStep((int)(((qtMethodParam *)args)->param));
        break ;
                         
    case QpScrollBar::SetLineStep:
        execSetLineStep((int)(((qtMethodParam *)args)->param));
        break ;
                         
    case QpScrollBar::PageStep:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execPageStep();
        break ;
                         
    case QpScrollBar::LineStep:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execLineStep();
        break ;
                         
    case QpScrollBar::SetMinValue:
        execSetMinValue((int)(((qtMethodParam *)args)->param));
        break ;
                         
    case QpScrollBar::SetMaxValue:
        execSetMaxValue((int)(((qtMethodParam *)args)->param));
        break ;
                         
    case QpScrollBar::SetValue:
        execSetValue((int)(((qtMethodParam *)args)->param));
        break ;
                         
    default :
        break;
    }
}
        
void 
QpScrollBar::execSetTracking(bool track){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    sbar->setTracking(track);
}

void 
QpScrollBar::execSetPageStep(int step){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    sbar->setPageStep(step);
}

void 
QpScrollBar::execSetLineStep(int step){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    sbar->setLineStep(step);
}

int  
QpScrollBar::execPageStep(void){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    return sbar->pageStep();
}

int  
QpScrollBar::execLineStep(void){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    return sbar->lineStep();
}

void 
QpScrollBar::execSetMinValue(int value){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    sbar->setMinValue(value);
}

void 
QpScrollBar::execSetMaxValue(int value){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    sbar->setMaxValue(value);
}

void 
QpScrollBar::execSetValue(int value){
    QScrollBar *sbar = (QScrollBar *)this->getQWidget();
    sbar->setValue(value);
}
