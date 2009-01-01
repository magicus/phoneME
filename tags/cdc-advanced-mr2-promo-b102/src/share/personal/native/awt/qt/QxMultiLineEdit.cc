/*
 * @(#)QxMultiLineEdit.cc	1.7 06/10/25
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

#include <qscrollbar.h>
#include "QxMultiLineEdit.h"

/*
 * Constructor of class QxMultiLineEdit
 */
QxMultiLineEdit::QxMultiLineEdit(QWidget* parent, const char* name, int flags) 
    : QMultiLineEdit(parent, name) 
{
}

/*
 * Is this row currently visible in the view port?
 */
bool
QxMultiLineEdit::rowIsVisible(int row)
{
    return QTableView::rowIsVisible(row);
}

/*
 * Returns the position for selected region
 */
bool
QxMultiLineEdit::getSelectedRegion(int* line1, int* col1, int* line2, 
                                    int* col2)
{
    return this->getMarkedRegion(line1, col1, line2, col2);
}

/*
 * This method takes a position and calculates the row and column that position
 * is in.  The row and column are stored in row and col.
 */
void 
QxMultiLineEdit::getRowCol(int position, int *row, int *col)
{
    int &line = *row;
    int &cols = *col;

   for (line = cols = 0; line < numLines() && cols <= position; line++) {
        cols += lineLength(line);
        if (isEndOfParagraph(line)) {
            cols++;
        }
    }
    line--;
    cols = position - cols + lineLength(line);
    if ( isEndOfParagraph(line) ) {
        cols++;
    }
}

/*
 * Returns the width required for the vertical scrollbar, if required.
 */ 
int 
QxMultiLineEdit::extraWidth()
{
    constPolish();

    int w = hMargin()*2 + frameWidth()*2;
    if ( testTableFlags(Tbl_vScrollBar|Tbl_autoVScrollBar) ) {
	w += verticalScrollBar()->sizeHint().width();
    }
    return w;
}

/*
 * Returns the height required for the horizontal scrollbar, if required, and
 * an extra space (hMargin() for now) to leave some margin on the bottom of
 * the display area.
 */ 
int 
QxMultiLineEdit::extraHeight()
{
    constPolish();

    // The hMargin() is to get some extra space.
    int h = frameWidth()*2 + hMargin();
    if ( testTableFlags(Tbl_hScrollBar|Tbl_autoHScrollBar) ) {
	h += horizontalScrollBar()->sizeHint().height();
    }
    return h;
}

/*
 * Returns the maximum line length
 */ 
int 
QxMultiLineEdit::charsInLine() 
{
    return this->maxLineLength();
}

/*
 * Returns the number of characters in this line
 */ 
int 
QxMultiLineEdit::charsInLine(int line) 
{
    return this->lineLength(line);
}

/*
 * Returns the position given (row, col).
 */
int
QxMultiLineEdit::getPosition(int lines, int col) 
{
    if (lines == 0) {
        return col;
    }

    int temp = 0;

    // This flag registers whether eop has been encountered.
    bool eop = false;

    for (int i = 0; i < lines; i++) {
        if (isEndOfParagraph(i)) {
            temp++;
            eop = true;
        }
        temp += this->lineLength(i);
    }

    if (isEndOfParagraph(lines)) {
        temp++;
        eop = true;
    }

    if (eop) {
      temp--;
    }

    return temp + col;
}

void
QxMultiLineEdit::setTableOptions(int flag) 
{
    this->clearTableFlags();
    this->setTableFlags(flag);
    QTableView::setAutoUpdate(TRUE);
}

void
QxMultiLineEdit::clearTableOptions(int flag) 
{
    this->clearTableFlags(flag);
    QTableView::setAutoUpdate(TRUE);
}

void 
QxMultiLineEdit::deleteSelectedText() {
    del();
}

