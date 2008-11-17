/*
 * @(#)QxLineEdit.cc	1.6 06/10/25
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
#include <qapplication.h>
#include "QxLineEdit.h" 

/*
 * QT3 updates
 * - getSelectionStart() and End() uses Qt3 methods to return the start
 *   and end of selected text
 * - Removed out private copies of selection start and end, since we
 *   can get it from QLineEdit
 * - Removed setSelectionPt() method
 * - select() directly calls QLineEdit::setSelection()
 */

/** 
 * Constructor of class QxLineEdit
 */
QxLineEdit :: QxLineEdit(QWidget* parent, const char* name) 
    : QLineEdit(parent, name) 
{
    isDirty = FALSE;
}

/*
 * Return our local copy of the widget's QString.
 */
QString 
QxLineEdit :: getString()
{
    return this->txt;
}

/*
 * Set our local copy of the widget's QString.
 */
void 
QxLineEdit :: setString(QString str) 
{
    this->txt = str;
}

/*
 * Set our "dirty" flag. TRUE indicates that our local
 * copy of the text for the widget may be more recent than
 * the QString value of the Qt widget iself, and we should
 * update it when appropriate.
 */
void
QxLineEdit::setDirty( bool tf ) 
{
    this->isDirty = tf;
    return;
}

/*
 * Indicate whether we need to set the Qt widget's
 * text value.
 */
bool
QxLineEdit::needsRefresh()
{
    return this->isDirty;
}

/**
 * set the text in the textfield
 */
void
QxLineEdit :: setText( const QString& text ) 
{
  static int dWidth = 0, dHeight = 0;
  if ( dWidth == 0 && dHeight == 0 ) {
    QWidget* desktop = QApplication::desktop();
    dWidth = desktop->width();
    dHeight = desktop->height();
  }
  // Keep track of what text the widget should have
  this->txt = QString(text);
  
  // Set the text only when on screen. If not on the
  // screen, mark ourselves as "dirty" so we can update
  // when we get a paint event.
  QPoint pos = this->mapToGlobal( QPoint(0,0) );
  if ( pos.x() <= dWidth ) { //|| (pos.y() <= desktop->height())) {
    isDirty = FALSE;
    this->validateAndSet( text, 0, 0, 0 );
  } else {
    isDirty = TRUE;
  }
  return;
}

#if (QT_VERSION >= 0x030000)
/**
 * virtual QLineEdit::focusOutEvent()
 *
 * QLineEdit implementation of this method deselects the selected text 
 * (if any), so we override this function and do the following
 *
 * if ( hasSelectedText ) {
 *   get the selection start and end ;
 * }
 *
 * call base class of implementation of this method ;
 *
 * if ( text was selected when this method was called AND
 *      text is not selected now ) {
 *    set the selection to the same state when this method was called ;
 * }
 */
void 
QxLineEdit :: focusOutEvent( QFocusEvent*e )
{
    int start = 0 ; // selection start
    int end   = 0 ; // selection end

    if ( this->hasSelectedText() ) {
        // text was selected, so get the start and end 
        start = this->selectionStart() ;
        QString selText = this->selectedText() ;
        end = start+selText.length() ;
    }
    
    // call the base class implementation
    QLineEdit::focusOutEvent(e) ;

    // if ( text was selected ) && (text not selected ) 
    //   then it means that the base class implementation has deselected
    //   the selection, so set the selection back
    if ( (start < end) && 
         !(this->hasSelectedText()) ) {
        this->setSelection(start, (end-start)) ;
    }
}
#else
void
QxLineEdit :: setSelectionPt(int start, int end)
{
    this->setSelection(start, end - start);
    this->selStart = start;
    this->selEnd = end;
}

#endif /* QT_VERSION */
