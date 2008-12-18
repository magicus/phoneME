/*
 * @(#)QpComboBox.cc	1.6 06/10/25
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
#include "QpComboBox.h"
#include <qcombobox.h>

typedef struct {
    struct {
        QString *t;
        int index;
    } in ;
} qtMargsInsertItem;

/*
 * QpComboBox Methods
 */
QpComboBox::QpComboBox(bool rw, QpWidget *parent, char *name , int flags ){
    QWidget *widget =
        QpWidgetFactory::instance()->createComboBox(rw,
                                                    parent,
                                                    name,
                                                    flags);
    this->setQWidget(widget);
}

void
QpComboBox::insertItem(QString &t, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertItem, argp);
    argp->in.t     = &t ;
    argp->in.index = index;
    invokeAndWait(QpComboBox::InsertItem,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpComboBox::removeItem(int index) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)index;
    invokeAndWait(QpComboBox::RemoveItem,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpComboBox::setCurrentItem(int index){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)index;
    invokeAndWait(QpComboBox::SetCurrentItem,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpComboBox::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpComboBox, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpComboBox::InsertItem: {
        qtMargsInsertItem *a = (qtMargsInsertItem *)args;
        execInsertItem(*a->in.t, a->in.index);
        }
        break;
        
    case QpComboBox::RemoveItem:
        execRemoveItem((int)(((qtMethodParam *)args)->param));
        break;

    case QpComboBox::SetCurrentItem:
        execSetCurrentItem((int)(((qtMethodParam *)args)->param));
        break;

    default :
        break;
    }
}
        

void
QpComboBox::execInsertItem(QString &t, int index ) {
    QComboBox *combo = (QComboBox *)this->getQWidget();
    combo->insertItem(t, index);
}

void 
QpComboBox::execRemoveItem(int index) {
    QComboBox *combo = (QComboBox *)this->getQWidget();
    combo->removeItem(index);
}

void 
QpComboBox::execSetCurrentItem(int index) {
    QComboBox *combo = (QComboBox *)this->getQWidget();
    combo->setCurrentItem(index);
}

