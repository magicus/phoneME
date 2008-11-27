/*
 * @(#)QpLineEdit.h	1.6 06/10/25
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
#ifndef _QpLINE_EDIT_H_
#define _QpLINE_EDIT_H_

#include "QxLineEdit.h"
#include "QpWidget.h"

class QpLineEdit : public QpWidget {
public :
    QpLineEdit(QpWidget *parent, char *name="QtLineEdit",int flags = 0);
    /* QLineEdit Methods */
    void setReadOnly( bool );
    int cursorPosition() ;
    void setCursorPosition( int );
    void setSelection( int, int );
    void setEchoMode( QLineEdit::EchoMode );
    void setText( const QString &);
    int selectionStart();
#if (QT_VERSION >= 0x030000)
    QString selectedText() ;
    bool hasSelectedText() ;
#else
    /* QxLineEdit Methods */
    int selectionEnd();
    void setSelectionPt(int start, int end);
#endif /* QT_VERSION */

    /* QxLineEdit Methods */
    QString getString();
    void setString( QString text );
    void setDirty( bool tf );
    bool needsRefresh(); 
protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetReadOnly = QpLineEdit::SOM,
        CursorPosition,
        SetCursorPosition,
        SetSelection,
        SetEchoMode,
        SetText,
        SelectionStart,
        GetString,
        SetString,
        SetDirty,
        NeedsRefresh,
#if (QT_VERSION >= 0x030000)
        SelectedText,
        HasSelectedText,
#else
        SelectionEnd,
        SetSelectionPt,
#endif /* QT_VERSION */
        EOM
    };

    virtual void execute(int method, void *args);
private :
    void execSetReadOnly( bool );
    int execCursorPosition() ;
    void execSetCursorPosition( int );
    void execSetSelection( int, int );
    void execSetEchoMode( QLineEdit::EchoMode );
    void execSetText( const QString &);
    int  execSelectionStart(void);
    QString execGetString();
    void execSetString( QString text );
    void execSetDirty( bool tf );
    bool execNeedsRefresh(); 
#if (QT_VERSION >= 0x030000)
    QString execSelectedText() ;
    bool execHasSelectedText() ;
#else
    int execSelectionEnd();
    void execSetSelectionPt(int start, int end);
#endif /* QT_VERSION */
};

#endif
