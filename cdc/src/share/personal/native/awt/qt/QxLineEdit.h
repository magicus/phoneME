/*
 * @(#)QxLineEdit.h	1.6 06/10/25
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
#ifndef _QxLINE_EDIT_H_
#define _QxLINE_EDIT_H_

#include <qlineedit.h>

class QxLineEdit : public QLineEdit {

public:
    QxLineEdit(QWidget* parent, const char* name);
    
    QString txt;                // our copy of the Qt widget's QString
    bool isDirty;               // TRUE if our txt is more recent
#if (QT_VERSION >= 0x030000)
    /* override focusOutEvent() to retain the selected text */
    void focusOutEvent( QFocusEvent* );
#else
    int selStart;
    int selEnd;
    int selectionStart() { return selStart; }
    int selectionEnd()   { return selEnd; }
    /* override setSelection() to save the start and end values */
    void setSelectionPt(int start, int end);
#endif /* QT_VERSION */

    QString getString();
    void setString( QString text );
    void setDirty( bool tf );
    bool needsRefresh();        // TRUE if we need to change the Qt widget

    /* override this function of QLineEdit */
    void setText(const QString& text);

};

#endif
