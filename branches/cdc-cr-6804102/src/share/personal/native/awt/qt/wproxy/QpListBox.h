/*
 * @(#)QpListBox.h	1.7 06/10/25
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
#ifndef _QpLIST_PEER_H_
#define _QpLIST_PEER_H_

#include <qlistbox.h>
#include "QpWidget.h"

class QpListBox : public QpWidget {
public :
    QpListBox(QpWidget *parent, char *name="QtListBox",int flags = 0);
    /* QListBox Methods */
    void insertItem( const QString &text, int index=-1 );
    void removeItem( int index );
    void setCurrentItem( int index );
    void setSelectionMode( QListBox::SelectionMode );
    void setSelected( int, bool );
    bool isSelected( int );
    int numRows();
    void clear();
    void ensureCurrentVisible();
    int index( const QListBoxItem * );
    void clearSelection();
    uint count();
    int preferredWidth(int rows);
    int preferredHeight(int rows);

    /* QScrollView Methods */
    QWidget *viewport();
    
protected :
     enum MethodId {
        SOM = QpWidget::EOM,
        InsertItem = QpListBox::SOM,
        RemoveItem,
        SetCurrentItem,
        SetSelectionMode,
        SetSelected,
        IsSelected,
        NumRows,
        Clear,
        EnsureCurrentVisible,
        Index,
        ClearSelection,
        Count,
        PreferredWidth,
        PreferredHeight,
        Viewport,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    void execInsertItem( const QString &text, int index=-1 );
    void execRemoveItem( int index );
    void execSetCurrentItem( int index );
    void execSetSelectionMode( QListBox::SelectionMode );
    void execSetSelected( int, bool );
    bool execIsSelected( int );
    int  execNumRows();
    void execClear();
    void execEnsureCurrentVisible();
    int  execIndex( const QListBoxItem * );
    void execClearSelection();
    uint execCount();
    int  execPreferredWidth(int rows);
    int  execPreferredHeight(int rows);
    QWidget *execViewport();
};
 
#endif
