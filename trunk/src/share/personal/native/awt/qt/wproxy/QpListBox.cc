/*
 * @(#)QpListBox.cc	1.7 06/10/25
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
#include "QpListBox.h"
#include <qlistbox.h>

typedef struct {
    struct {
        const QString *t;
        int index;
    } in ;
} qtMargsInsertItem;

typedef struct {
    struct {
        int index;
        bool select;
    } in ;
} qtMargsSetSelected;

/*
 * QpListBox Methods
 */
QpListBox::QpListBox(QpWidget *parent, char *name , int flags ){
    QWidget *widget = QpWidgetFactory::instance()->createListBox( parent,
                                                                 name,
                                                                 flags);
    this->setQWidget(widget);
}

void
QpListBox::insertItem(const QString &t, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertItem, argp);
    argp->in.t     = &t ;
    argp->in.index = index;
    invokeAndWait(QpListBox::InsertItem,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpListBox::removeItem(int index) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)index;
    invokeAndWait(QpListBox::RemoveItem,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpListBox::setCurrentItem(int index){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)index;
    invokeAndWait(QpListBox::SetCurrentItem,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpListBox::setSelectionMode( QListBox::SelectionMode mode) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpListBox::SetSelectionMode,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpListBox::setSelected( int index, bool select) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetSelected, argp);
    argp->in.index  = index;
    argp->in.select = select ;
    invokeAndWait(QpListBox::SetSelected,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpListBox::isSelected( int i) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)i;
    invokeAndWait(QpListBox::IsSelected,argp);
    bool rv = (bool)argp->out.rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int 
QpListBox::numRows() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpListBox::NumRows,argp);
    int rv = (int)argp->rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpListBox::clear() {
    invokeAndWait(QpListBox::Clear,NULL);
}

void 
QpListBox::ensureCurrentVisible() {
    invokeAndWait(QpListBox::EnsureCurrentVisible,NULL);
}

int 
QpListBox::index( const QListBoxItem *item ) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)item;
    invokeAndWait(QpListBox::Index,argp);
    int rv = (int)argp->out.rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpListBox::clearSelection() {
    invokeAndWait(QpListBox::ClearSelection,NULL);
}

uint 
QpListBox::count() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpListBox::Count,argp);
    uint rv = (uint)argp->rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int
QpListBox::preferredWidth(int rows) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)rows;
    invokeAndWait(QpListBox::PreferredWidth,argp);
    int rv = (int)argp->out.rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int 
QpListBox::preferredHeight(int rows) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)rows;
    invokeAndWait(QpListBox::PreferredHeight,argp);
    int rv = (int)argp->out.rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpListBox::viewport() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpListBox::Viewport,argp);
    QWidget *rv = (QWidget *)argp->rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpListBox::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpListBox, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpListBox::InsertItem: {
        qtMargsInsertItem *a = (qtMargsInsertItem *)args;
        execInsertItem(*a->in.t, a->in.index);
        }
        break;
        
    case QpListBox::RemoveItem:
        execRemoveItem((int)(((qtMethodParam *)args)->param));
        break;

    case QpListBox::SetCurrentItem:
        execSetCurrentItem((int)(((qtMethodParam *)args)->param));
        break;

    case QpListBox::SetSelectionMode:{
        int mode = (int)((qtMethodParam *)args)->param;
        execSetSelectionMode((QListBox::SelectionMode)mode);
        }
        break ;

    case QpListBox::SetSelected:{
        qtMargsSetSelected *a = (qtMargsSetSelected *)args;
        execSetSelected(a->in.index, a->in.select);
        }
        break ;

    case QpListBox::IsSelected: {
        qtMethodArgs *a = (qtMethodArgs *)args;
        a->out.rvalue = (void *)execIsSelected((int)a->in.param);
        }
        break ;

    case QpListBox::NumRows:
        ((qtMethodReturnValue *)args)->rvalue = (void *) execNumRows();
        break ;

    case QpListBox::Clear:
        execClear();
        break ;

    case QpListBox::EnsureCurrentVisible:
        execEnsureCurrentVisible();
        break ;

    case QpListBox::Index: {
        qtMethodArgs *a = (qtMethodArgs *)args;
        a->out.rvalue = (void *)execIndex((const QListBoxItem *)a->in.param);
        }
        break ;

    case QpListBox::ClearSelection:
        execClearSelection();
        break ;

    case QpListBox::Count:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execCount();
        break ;

    case QpListBox::PreferredWidth: {
        qtMethodArgs *a = (qtMethodArgs *)args;
        a->out.rvalue = (void *)execPreferredWidth((int)a->in.param );
        }
        break ;

    case QpListBox::PreferredHeight: {
        qtMethodArgs *a = (qtMethodArgs *)args;
        a->out.rvalue = (void *)execPreferredHeight((int)a->in.param);
        }
        break ;

    case QpListBox::Viewport:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execViewport();
        break ;

    default :
        break;
    }
}
        
void
QpListBox::execInsertItem(const QString &t, int index ) {
    QListBox *list = (QListBox *)this->getQWidget();
    list->insertItem(t, index);
}

void 
QpListBox::execRemoveItem(int index) {
    QListBox *list = (QListBox *)this->getQWidget();
    list->removeItem(index);
}

void 
QpListBox::execSetCurrentItem(int index) {
    QListBox *list = (QListBox *)this->getQWidget();
    list->setCurrentItem(index);
}

void 
QpListBox::execSetSelectionMode( QListBox::SelectionMode mode){
    QListBox *list = (QListBox *)this->getQWidget();
    list->setSelectionMode(mode);
}

void 
QpListBox::execSetSelected( int index, bool select) {
    QListBox *list = (QListBox *)this->getQWidget();
    list->setSelected(index, select);
}

bool 
QpListBox::execIsSelected( int index) {
    QListBox *list = (QListBox *)this->getQWidget();
    return list->isSelected(index);
}

int  
QpListBox::execNumRows() {
    QListBox *list = (QListBox *)this->getQWidget();
    return list->numRows();
}

void 
QpListBox::execClear() {
    QListBox *list = (QListBox *)this->getQWidget();
    list->clear();
}

void 
QpListBox::execEnsureCurrentVisible(){
    QListBox *list = (QListBox *)this->getQWidget();
    list->ensureCurrentVisible();
}

int  
QpListBox::execIndex( const QListBoxItem *item ) {
    QListBox *list = (QListBox *)this->getQWidget();
    return list->index(item);
}

void 
QpListBox::execClearSelection() {
    QListBox *list = (QListBox *)this->getQWidget();
    list->clearSelection();
}

uint 
QpListBox::execCount() {
    QListBox *list = (QListBox *)this->getQWidget();
    return list->count();
}

int
QpListBox::execPreferredWidth(int rows) {
    QListBox* list = (QListBox*) this->getQWidget();
    int preferredWidth = 2*list->frameWidth() + list->maxItemWidth();
    if (list->numRows() > rows) {
        preferredWidth += list->verticalScrollBar()->sizeHint().width();
    }
    return preferredWidth;
}

int
QpListBox::execPreferredHeight(int rows) {
    QListBox* list = (QListBox*) this->getQWidget();
    return 2*list->frameWidth() + rows*list->itemHeight();
}

QWidget *
QpListBox::execViewport() {
    QListBox *list = (QListBox *)this->getQWidget();
    return list->viewport();
}

