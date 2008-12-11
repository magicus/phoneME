/*
 * @(#)QpMenuBar.cc	1.7 06/10/25
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

#include <qmenubar.h>
#include "QpWidgetFactory.h"
#include "QpMenuBar.h"

/*
 * QpMenuBar Methods
 */
typedef struct {
    struct {
        int key;
        int id;
    } in ;
} qtMargsSetAccel;

typedef struct {
    struct {
        int id;
        const QString *text;
    } in ;
} qtMargsChangeItem;

typedef struct {
    struct {
        int id;
        bool enable;
    } in ;
} qtMargsSetItemEnabled;

typedef struct {
    struct {
        const QString *text;
        QpWidget *popup;
        int index;
        int id;
    } in ;
    struct {
        int rvalue;
    }out;
} qtMargsInsertItem;

QpMenuBar::QpMenuBar(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createMenuBar(parent,
                                                                 name,
                                                                 flags);
    this->setQWidget(widget);
    // 4678754
    this->helpMenuId = 0 ;
    this->isHelpMenuSet = FALSE ;
    // 4678754
}

void 
QpMenuBar::setAccel(int key, int id ) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetAccel, argp);
    argp->in.key = key;
    argp->in.id  = id;
    invokeAndWait(QpMenuBar::SetAccel, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMenuBar::changeItem( int id, const QString &text ) {
    QT_METHOD_ARGS_ALLOC(qtMargsChangeItem, argp);
    argp->in.id   = id;
    argp->in.text = &text;
    invokeAndWait(QpMenuBar::ChangeItem, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMenuBar::removeItem(int id) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param   = (void *)id;
    invokeAndWait(QpMenuBar::RemoveItem, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMenuBar::setItemEnabled( int id, bool enable ) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetItemEnabled, argp);
    argp->in.id     = id;
    argp->in.enable = enable;
    invokeAndWait(QpMenuBar::SetItemEnabled, argp);
    QT_METHOD_ARGS_FREE(argp);
}

//4678754
void
QpMenuBar::setAsHelpMenu(int menuId) {
    this->helpMenuId = menuId ;
    this->isHelpMenuSet = TRUE ;
}
//4678754

int 
QpMenuBar::insertItem(const QString &text, 
                      QpWidget *popup, int id, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertItem, argp);
    argp->in.text   = &text;
    argp->in.id     = id;
    argp->in.index  = index;
    argp->in.popup  = popup;
    invokeAndWait(QpMenuBar::InsertItem, argp);
    int rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpMenuBar::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpMenuBar, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpMenuBar::SetAccel:
        execSetAccel(((qtMargsSetAccel *)args)->in.key,
                     ((qtMargsSetAccel *)args)->in.id);
        break ;
    case QpMenuBar::ChangeItem:
        execChangeItem(((qtMargsChangeItem *)args)->in.id,
                       *((qtMargsChangeItem*)args)->in.text);
        break ;
    case QpMenuBar::RemoveItem:
        execRemoveItem((int)(((qtMethodParam *)args)->param)) ;
        break ;
    case QpMenuBar::SetItemEnabled:
        execSetItemEnabled(((qtMargsSetItemEnabled *)args)->in.id,
                           ((qtMargsSetItemEnabled *)args)->in.enable);
        break ;
    case QpMenuBar::InsertItem: {
        qtMargsInsertItem *a = (qtMargsInsertItem *)args;
        a->out.rvalue = execInsertItem(*a->in.text,
                                       a->in.popup,
                                       a->in.id,
                                       a->in.index);
        
        }
        break ;
    default :
        break;
    }
}

void 
QpMenuBar::execSetAccel(int key, int id ) {
    QMenuBar *menubar = (QMenuBar *)getQWidget();
    menubar->setAccel(key, id);
}

void 
QpMenuBar::execChangeItem( int id, const QString &text ) {
    QMenuBar *menubar = (QMenuBar *)getQWidget();
    menubar->changeItem(id, text);
}

void 
QpMenuBar::execRemoveItem(int id) {
    QMenuBar *menubar = (QMenuBar *)getQWidget();
    menubar->removeItem(id);
    //4678754
    if ( id == this->helpMenuId ) {
        this->isHelpMenuSet = FALSE ;
    }
    //4678754
}

void 
QpMenuBar::execSetItemEnabled( int id, bool enable ) {
    QMenuBar *menubar = (QMenuBar *)getQWidget();
    menubar->setItemEnabled(id, enable);
}

int 
QpMenuBar::execInsertItem(const QString &text, 
                          QpWidget *popup, int id, int index) {
    QMenuBar *menubar = (QMenuBar *)getQWidget();

    //4678754
    // if the helpmenu is set for the menubar, insert this item before
    // the help menu, so that the Help menu is always the last.
    if ( this->isHelpMenuSet ) {
        index = menubar->indexOf(this->helpMenuId);
    }
    //4678754

    return menubar->insertItem(text,
                               (QPopupMenu *)AWT_GET_QWIDGET(QpMenuBar,popup),
                               id,
                               index);
}

