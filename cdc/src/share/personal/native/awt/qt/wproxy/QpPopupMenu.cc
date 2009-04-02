/*
 * @(#)QpPopupMenu.cc	1.6 06/10/25
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

#include <qpopupmenu.h>
#include "QpWidgetFactory.h"
#include "QpPopupMenu.h"

/*
 * QpPopupMenu Methods
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
        int id;
        bool check;
    } in ;
}qtMargsSetItemChecked ;

typedef struct {
    struct {
        const QString *text;
        QpPopupMenu *popup;
        int index;
        int id;
    } in ;
    struct {
        int rvalue;
    }out;
} qtMargsInsertItem_QString_QpPopupMenu_int_int;

typedef struct {
    struct {
        QpWidget *widget;
        int index;
        int id;
    } in ;
    struct {
        int rvalue;
    }out;
} qtMargsInsertItem_QpWidget_int_int;

typedef struct {
    struct {
        int id;
        const QObject *receiver;
        const char* member;
    } in ;
    struct {
        bool rvalue;
    }out;
}qtMargsConnectItem ;

QpPopupMenu::QpPopupMenu(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createPopupMenu(parent,
                                                                 name,
                                                                 flags);

    this->setQWidget(widget);
}

void 
QpPopupMenu::setAccel(int key, int id ) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetAccel, argp);
    argp->in.key = key;
    argp->in.id  = id;
    invokeAndWait(QpPopupMenu::SetAccel, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpPopupMenu::changeItem( int id, const QString &text ) {
    QT_METHOD_ARGS_ALLOC(qtMargsChangeItem, argp);
    argp->in.id   = id;
    argp->in.text = &text;
    invokeAndWait(QpPopupMenu::ChangeItem, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpPopupMenu::removeItem(int id) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param   = (void *)id;
    invokeAndWait(QpPopupMenu::RemoveItem, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpPopupMenu::setItemEnabled( int id, bool enable ) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetItemEnabled, argp);
    argp->in.id     = id;
    argp->in.enable = enable;
    invokeAndWait(QpPopupMenu::SetItemEnabled, argp);
    QT_METHOD_ARGS_FREE(argp);
}

int 
QpPopupMenu::insertItem(const QString &text, 
                        QpPopupMenu *popup, int id, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertItem_QString_QpPopupMenu_int_int, argp);
    argp->in.text   = &text;
    argp->in.id     = id;
    argp->in.index  = index;
    argp->in.popup  = popup;
    invokeAndWait(QpPopupMenu::InsertItem_QString_QpPopupMenu_int_int, argp);
    int rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpPopupMenu::setItemChecked(int id, bool check){
    QT_METHOD_ARGS_ALLOC(qtMargsSetItemChecked, argp);
    argp->in.id    = id;
    argp->in.check = check;
    invokeAndWait(QpPopupMenu::SetItemChecked, argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpPopupMenu::isItemChecked(int id) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)id;
    invokeAndWait(QpPopupMenu::IsItemChecked, argp);
    bool rv = (bool)argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpPopupMenu::insertTearOffHandle(void) {
    invokeAndWait(QpPopupMenu::InsertTearOffHandle, NULL);
}

void 
QpPopupMenu::popup(QPoint pt) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&pt;
    invokeAndWait(QpPopupMenu::Popup, argp);
    QT_METHOD_ARGS_FREE(argp);
}

// InsertItem_QString_int_int
int  
QpPopupMenu::insertItem( const QString &text, int id, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertItem_QString_QpPopupMenu_int_int, argp);
    argp->in.text   = &text;
    argp->in.id     = id;
    argp->in.index  = index;
    invokeAndWait(QpPopupMenu::InsertItem_QString_int_int, argp);
    int rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}


// InsertItem_QpWidget_int_int
int  
QpPopupMenu::insertItem( QpWidget* widget, int id, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertItem_QpWidget_int_int, argp);
    argp->in.widget = widget;
    argp->in.id     = id;
    argp->in.index  = index;
    invokeAndWait(QpPopupMenu::InsertItem_QpWidget_int_int, argp);
    int rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpPopupMenu::insertSeparator( int index ) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)index;
    invokeAndWait(QpPopupMenu::InsertSeparator, argp);
    int rv = (int)argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
bool 
QpPopupMenu::connectItem( int id,
                          const QObject *receiver, const char* member ){
    QT_METHOD_ARGS_ALLOC(qtMargsConnectItem, argp);
    argp->in.id       = id;
    argp->in.receiver = receiver;
    argp->in.member   = member;
    invokeAndWait(QpPopupMenu::ConnectItem, argp);
    bool rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int 
QpPopupMenu::frameStyle(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpPopupMenu::FrameStyle, argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpPopupMenu::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpPopupMenu, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpPopupMenu::SetAccel:
        execSetAccel(((qtMargsSetAccel *)args)->in.key,
                     ((qtMargsSetAccel *)args)->in.id);
        break ;
    case QpPopupMenu::ChangeItem:
        execChangeItem(((qtMargsChangeItem *)args)->in.id,
                       *((qtMargsChangeItem*)args)->in.text);
        break ;
    case QpPopupMenu::RemoveItem:
        execRemoveItem((int)(((qtMethodParam *)args)->param)) ;
        break ;
    case QpPopupMenu::SetItemEnabled:
        execSetItemEnabled(((qtMargsSetItemEnabled *)args)->in.id,
                           ((qtMargsSetItemEnabled *)args)->in.enable);
        break ;
    case QpPopupMenu::InsertItem_QString_QpPopupMenu_int_int: {
        qtMargsInsertItem_QString_QpPopupMenu_int_int *a = 
            (qtMargsInsertItem_QString_QpPopupMenu_int_int *)args;
        a->out.rvalue = execInsertItem(*a->in.text,
                                       a->in.popup,
                                       a->in.id,
                                       a->in.index);
        }
        break ;
    case QpPopupMenu::SetItemChecked:
        execSetItemChecked(((qtMargsSetItemChecked*)args)->in.id,
                           ((qtMargsSetItemChecked*)args)->in.check);
        break;
    case QpPopupMenu::IsItemChecked:
        ((qtMethodArgs *)args)->out.rvalue = (void *)
            execIsItemChecked((int)(((qtMethodArgs *)args)->in.param));
        break;
    case QpPopupMenu::InsertTearOffHandle:
        execInsertTearOffHandle();
        break;
    case QpPopupMenu::Popup: {
        QPoint *pt = (QPoint *)((qtMethodParam *)args)->param;
        execPopup(*pt);
        }
        break;
    case QpPopupMenu::InsertItem_QString_int_int: {
        qtMargsInsertItem_QString_QpPopupMenu_int_int *a = 
            (qtMargsInsertItem_QString_QpPopupMenu_int_int *)args;
        a->out.rvalue = execInsertItem(*a->in.text, a->in.id, a->in.index);
        }
        break;
    case QpPopupMenu::InsertItem_QpWidget_int_int: {
        qtMargsInsertItem_QpWidget_int_int *a = 
            (qtMargsInsertItem_QpWidget_int_int *)args;
        a->out.rvalue = execInsertItem(a->in.widget, a->in.id, a->in.index);
        }
        break;
    case QpPopupMenu::InsertSeparator:
        ((qtMethodArgs *)args)->out.rvalue = (void *)
            execInsertSeparator((int)(((qtMethodArgs *)args)->in.param));
        break;
    case QpPopupMenu::ConnectItem: {
        qtMargsConnectItem *a = (qtMargsConnectItem *)args;
        a->out.rvalue = execConnectItem(a->in.id,a->in.receiver,a->in.member);
        }
        break;
    case QpPopupMenu::FrameStyle:
        ((qtMethodReturnValue *)args)->rvalue = (void *) execFrameStyle();
        break;
    default :
        break;
    }
}

void 
QpPopupMenu::execSetAccel(int key, int id ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->setAccel(key, id);
}

void 
QpPopupMenu::execChangeItem( int id, const QString &text ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->changeItem(id, text);
}

void 
QpPopupMenu::execRemoveItem(int id) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->removeItem(id);
}

void 
QpPopupMenu::execSetItemEnabled( int id, bool enable ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->setItemEnabled(id, enable);
}

int 
QpPopupMenu::execInsertItem(const QString &text, 
                            QpPopupMenu *popup, int id, int index) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->insertItem(text,
                            (QPopupMenu *)AWT_GET_QWIDGET(QpPopupMenu,popup),
                            id,
                            index);
}

void 
QpPopupMenu::execSetItemChecked(int id, bool check){
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->setItemChecked(id, check);
}

bool 
QpPopupMenu::execIsItemChecked(int id) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->isItemChecked(id);
}

void 
QpPopupMenu::execInsertTearOffHandle(void) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->insertTearOffHandle();
}

void 
QpPopupMenu::execPopup(QPoint pt) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    menu->popup(pt);
}

int  
QpPopupMenu::execInsertItem( const QString &text, int id, int index ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->insertItem(text, id, index);
}

int  
QpPopupMenu::execInsertItem( QpWidget* widget, int id, int index ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->insertItem(AWT_GET_QWIDGET(QpPopupMenu,widget),id, index);
}

int  
QpPopupMenu::execInsertSeparator( int index ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->insertSeparator(index);
}

bool 
QpPopupMenu::execConnectItem( int id,
                              const QObject *receiver, const char* member ) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->connectItem(id, receiver, member);
}

int 
QpPopupMenu::execFrameStyle(void) {
    QPopupMenu *menu = (QPopupMenu *)getQWidget();
    return menu->frameStyle();
}

