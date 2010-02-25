/*
 * @(#)QpMultiLineEdit.h	1.6 06/10/25
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
#ifndef _QpMULTI_LINE_EDIT_H_
#define _QpMULTI_LINE_EDIT_H_

#include "QpWidget.h"
#include <qmultilineedit.h>

class QpMultiLineEdit : public QpWidget {

public:
    QpMultiLineEdit(QpWidget* parent, char* name = "QtMultiLineEdit",
                    int flags=0);
    /* QMultiLineEdit Methods */
    void setReadOnly( bool b );
    QString text() ;
    void setText( const QString &txt );
    void getCursorPosition( int *line, int *col );
    int  length();
    void setSelection( int row_from, int col_from, int row_to, int col_t );
    void setCursorPosition( int line, int col, bool mark = FALSE );
    void setWordWrap( QMultiLineEdit::WordWrap mode );
    void setWrapPolicy( QMultiLineEdit::WrapPolicy policy );
    void setAutoUpdate( bool );
    void insertAt( const QString &s, int line, int col, bool mark = FALSE );
    void insert( const QString& );

    /* QxMultiLineEdit.h Methods */
    bool rowIsVisible(int row);
    bool getSelectedRegion(int* line1, int* col1, int* line2, int* col2);
    void getRowCol(int position, int *row, int *col);
    int extraWidth();
    int extraHeight();
    int charsInLine();
    int charsInLine(int line);
    int getPosition(int line, int col);
    void setTableOptions(int flag);
    void clearTableOptions(int flag);
    void deleteSelectedText();

protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetReadOnly = QpMultiLineEdit::SOM,
        Text,
        SetText,
        GetCursorPosition,
        Length,
        SetSelection,
        SetCursorPosition,
        SetWordWrap,
        SetWrapPolicy,
        SetAutoUpdate,
        InsertAt,
        Insert,
        RowIsVisible,
        GetSelectedRegion,
        GetRowCol,
        ExtraWidth,
        ExtraHeight,
        CharsInLine,
        CharsInLine_int,
        GetPosition,
        SetTableOptions,
        ClearTableOptions,
        DeleteSelectedText,
        EOM
    };
    virtual void execute(int method, void *args);
private :
    void    execSetReadOnly( bool b );
    QString execText() ;
    void    execSetText( const QString &txt );
    void execGetCursorPosition( int *line, int *col );
    int  execLength();
    void execSetSelection( int row_from, int col_from, 
                           int row_to, int col_t );
    void execSetCursorPosition( int line, int col, bool mark = FALSE );
    void execSetWordWrap( QMultiLineEdit::WordWrap mode );
    void execSetWrapPolicy( QMultiLineEdit::WrapPolicy policy );
    void execSetAutoUpdate( bool );
    void execInsertAt( const QString &s, int line, int col, 
                       bool mark = FALSE );
    void execInsert( const QString& );
    bool execRowIsVisible(int row);
    bool execGetSelectedRegion(int* line1, int* col1, int* line2, int* col2);
    void execGetRowCol(int position, int *row, int *col);
    int execExtraWidth();
    int execExtraHeight();
    int execCharsInLine();
    int execCharsInLine(int line);
    int execGetPosition(int line, int col);
    void execSetTableOptions(int flag);
    void execClearTableOptions(int flag);
    void execDeleteSelectedText();

};

#endif
