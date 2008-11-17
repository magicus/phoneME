/*
 * @(#)QpTextEdit.h	1.6 06/10/25
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
#ifndef _QpTEXT_EDIT_H_
#define _QpTEXT_EDIT_H_

#include "QpWidget.h"
#include "QxTextEdit.h"
#include <qtextedit.h>

class QpTextEdit : public QpWidget {
public :
    QpTextEdit(QpWidget *parent, char *name="QtTextEdit",int flags = 0);
    /* QScrollView Methods */
    QWidget *viewport();

    /* QTextEdit Methods */
    void setReadOnly( bool b );
    QString text() ;
    void setText( const QString &txt );
    bool hasSelectedText() ;
    void getCursorPosition( int *parag, int *index ) ;
    void setSelection( int parag_from, int index_from, 
                       int parag_to, int index_to, int selNum = 0 );
    void setCursorPosition( int parag, int index );
    void setWrapPolicy( QTextEdit::WrapPolicy policy );
    void setWordWrap( QTextEdit::WordWrap mode );
    void setHScrollBarMode( QScrollView::ScrollBarMode );
    void setVScrollBarMode( QScrollView::ScrollBarMode );
    int length() ;

    /* QxTextEdit Methods */
    int  positionToParaIndex(int position, int *index) ;
    int  paraIndexToPosition(int para, int index) ;
    int  getCursorPosition() ;
    int  getSelectionStartPosition();
    int  getSelectionEndPosition();
    void insertAt(int position, QString text) ;
    void replaceAt(QString text, int startPos, int endPos);
    int  extraWidth();
    int  extraHeight();
protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetReadOnly = QpTextEdit::SOM,
        Text,
        SetText,
        HasSelectedText,
        GetCursorPosition,
        SetSelection,
        SetCursorPosition,
        SetWrapPolicy,
        SetWordWrap,
        SetHScrollBarMode,
        SetVScrollBarMode,
        Length,
        PositionToParaIndex,
        ParaIndexToPosition,
        GetCursorPosition_void,
        GetSelectionStartPosition,
        GetSelectionEndPosition,
        InsertAt,
        ReplaceAt,
        ExtraWidth,
        ExtraHeight,
        Viewport,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    void    execSetReadOnly( bool b );
    QString execText() ;
    void    execSetText( const QString &txt );
    bool    execHasSelectedText() ;
    void    execGetCursorPosition( int *parag, int *index ) ;
    void    execSetSelection( int parag_from, int index_from, 
                              int parag_to, int index_to, int selNum = 0 );
    void    execSetCursorPosition( int parag, int index );
    void    execSetWrapPolicy( QTextEdit::WrapPolicy policy );
    void    execSetWordWrap( QTextEdit::WordWrap mode );
    void    execSetHScrollBarMode( QScrollView::ScrollBarMode );
    void    execSetVScrollBarMode( QScrollView::ScrollBarMode );
    int     execLength() ;

    int     execPositionToParaIndex(int position, int *index) ;
    int     execParaIndexToPosition(int para, int index) ;
    int     execGetCursorPosition() ;
    int     execGetSelectionStartPosition();
    int     execGetSelectionEndPosition();
    void    execInsertAt(int position, QString text) ;
    void    execReplaceAt(QString text, int startPos, int endPos);
    int     execExtraWidth();
    int     execExtraHeight();
    QWidget *execViewport();
};
#endif
