/*
 * @(#)QpPopupMenu.h	1.6 06/10/25
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
#ifndef _QpPOPUP_MENU_H_
#define _QpPOPUP_MENU_H_

#include "QpMenuItemContainer.h"
#include "QpWidget.h"

class QpPopupMenu : public QpMenuItemContainer {
public :
    QpPopupMenu(QpWidget *parent=NULL, char *name = "QtPopupMenu", 
                int flags=0);

    /* QPopupMenu Methods */
    void setAccel(int key, int id );
    void changeItem( int id, const QString &text );
    void removeItem(int id);
    void setItemEnabled( int id, bool enable );
    int  insertItem( const QString &text, QpPopupMenu *popup,
                     int id=-1, int index=-1 );

    void setItemChecked(int id, bool check); 
    bool isItemChecked(int id) ;
    void insertTearOffHandle(void);
    void popup(QPoint);
    int  insertItem( const QString &text, int id=-1, int index=-1 );
    int  insertItem( QpWidget* widget, int id=-1, int index=-1 );
    int  insertSeparator( int index=-1 );
    bool connectItem( int id,
                      const QObject *receiver, const char* member );
    int frameStyle(void);
    virtual QpMenuItemContainer::Type containerType() {
        return QpMenuItemContainer::PopupMenu ;
    }

protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetAccel = QpPopupMenu::SOM,
        ChangeItem,
        RemoveItem,
        SetItemEnabled,
        InsertItem_QString_QpPopupMenu_int_int,
        SetItemChecked,
        IsItemChecked,
        InsertTearOffHandle,
        Popup,
        InsertItem_QString_int_int,
        InsertItem_QpWidget_int_int,
        InsertSeparator,
        ConnectItem,
        FrameStyle,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    void execSetAccel(int key, int id );
    void execChangeItem( int id, const QString &text );
    void execRemoveItem(int id);
    void execSetItemEnabled( int id, bool enable );
    int  execInsertItem( const QString &text, QpPopupMenu *popup,
                         int id=-1, int index=-1 );

    void execSetItemChecked(int id, bool check); 
    bool execIsItemChecked(int id);
    void execInsertTearOffHandle(void);
    void execPopup(QPoint);
    int  execInsertItem( const QString &text, int id=-1, int index=-1 );
    int  execInsertItem( QpWidget* widget, int id=-1, int index=-1 );
    int  execInsertSeparator( int index=-1 );
    bool execConnectItem( int id,
                          const QObject *receiver, const char* member );
    int execFrameStyle(void);
};
#endif
