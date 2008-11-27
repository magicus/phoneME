/*
 * @(#)QxTextEdit.cc	1.7 06/10/25
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
#include "QxTextEdit.h"

/*
 * Design Notes 
 * ------------
 *
 * 1.Mouse Clicks
 *   QTextEdit consumes the mouse pressed and released events for its
 *   own purpose. The java foucs system requires the MOUSE_PRESSED event
 *   to request focus for that component. To provide that feature we 
 *   override QTextEdit::contentsMousePressEvent() and 
 *   QTextEdit::contentsMouseReleaseEvent() protected methods (defined in
 *   QScrollView) and gather the event information in AWTMouseEvent 
 *   structure. QTextEdit provides a "clicked()" signal when mouse is 
 *   pressed on the widget. We catch the signal (QtTextAreaPeer::clicked())
 *   and synthesize a PRESSED, RELEASED, and CLICKED mouse event.
 *
 * 2.Mouse Dragged
 *   We also synthesize a DRAGGED mouse event by overriding QTextEdit::
 *   contentsMouseMoveEvent() protected method to post the event to the Java
 *   layer and then to call the QTextEdit::contentsMouseMoveEvent() itself.
 *
 * 3.Why create QxTextEdit class ?
 *   Java treats the text contained in the TextEdit as an array of text
 *   and uses a zero based index (position in the methods) to refer to
 *   point, whereas QTextEdit has a concept of paragraphs (which are text
 *   seperated by newline). QxTextEdit extends QTextEdit and provides 
 *   methods that take zero based index, allowing the peer class from
 *   this mismatch and can be reusable in other applications as well.
 */

QxTextEdit::QxTextEdit(QWidget* parent, const char* name)
    : QTextEdit(parent, name) {
    this->setTextFormat(Qt::PlainText) ;

    // 6253667.
    this->setTabChangesFocus(FALSE) ;
}

int 
QxTextEdit::paraIndexToPosition(int para, int index){
    int position = 0 ;
    int p = 0 ;
    for ( p = 0 ; p < para ; p ++ ) {
        position += this->paragraphLength(p) ;
    }
    // Add the number of newlines which is treated by Qt as the EndOfPara
    // and is never accounted for in the "paragraphLength()". Java expects
    // it as part of the text.
    position += para ;
    position += index ;

    return position ;
}

int 
QxTextEdit::positionToParaIndex(int position, int *index){
    int para = 0 ;
    *index = 0 ;
    int paras = this->paragraphs() ;
    int p = 0 ;
    int paraLength = 0 ;

    if ( position < 0 ) 
        position= 0 ;

    // Iterate through the paragraphs
    for (p = 0 ; p < paras ; p ++ ) {
        paraLength = this->paragraphLength(p) ;
        // Walk through the paragraph, iterating the index.
        if ( position <= paraLength ) {
            para = p ;
            *index = position;
            break ;
        }
        else {
            // Add one to the paraLength, if we're not at the last paragraph, 
            // to account for the "newLine"
            // which is treated by Qt as the EndOfPara, whereas Java treats
            // it as part of the text.
            position -= (paraLength+1) ;
        }
    }

    // TODO : Cleaner way to implement this would be to include newline in
    //        paragraph length and this would would go away. Note that 
    //        a para need not be terminated with a linebreak in QTextEdit
    if ( *index < 0 ) {
        if ( para > 0 ) {
            para -- ; // set to previous para
            // we add 1 to paragraph length to account for newline and
            // the -negative index to it. Typically the index would be -1
            // but this will cover any -negative index
            *index = (this->paragraphLength(para)+1) + *index ;
        } else {
            // this means that the index is not beyond the first character
            // of para 0, which should never happen. Force it to first 
            // position
            para = 0 ;
            *index = 0 ;
        }
    }
    return para ;
}

int
QxTextEdit::getCursorPosition() {
    int para = 0 ;
    int index = 0 ;
    ((QTextEdit *)this)->getCursorPosition(&para, &index) ;
    return this->paraIndexToPosition(para, index);
}

int
QxTextEdit::getSelectionStartPosition() {
    int paraFrom , paraTo ;
    int indexFrom, indexTo ;

    this->getSelection(&paraFrom, &indexFrom, &paraTo, &indexTo) ;

    return this->paraIndexToPosition(paraFrom, indexFrom);
}

int
QxTextEdit::getSelectionEndPosition() {
    int paraFrom , paraTo ;
    int indexFrom, indexTo ;

    this->getSelection(&paraFrom, &indexFrom, &paraTo, &indexTo) ;

    return this->paraIndexToPosition(paraTo, indexTo);
}

void
QxTextEdit::insertAt(int position, QString text) {
    int maxLength = this->length() ;
    int index = 0 ;
    int para = 0 ;
    if ( position >= maxLength ) {
        position = maxLength ; // force it to maxLength 
        para  = this->paragraphs()-1 ;  // last paragraph
        index = paragraphLength(para) ; // last character in last para
    }
    else {
        index = 0 ;
        para  = this->positionToParaIndex(position, &index) ;
    }

    ((QTextEdit *)this)->insertAt(text, para, index) ;

    // set the caret position at the end of text
    para = this->positionToParaIndex(position+text.length(), &index);
    this->setCursorPosition(para, index) ;
}

void
QxTextEdit::replaceAt(QString text, int startPos, int endPos) {
    int total_length = this->length();
    endPos = (endPos < total_length) ? endPos : total_length;
    int position = startPos = startPos < endPos ? startPos : endPos;

    int paraFrom, paraTo, indexFrom, indexTo ;
    paraFrom = this->positionToParaIndex(position, &indexFrom);
    paraTo   = this->positionToParaIndex(endPos, &indexTo);
   
    this->setSelection(paraFrom, indexFrom, paraTo, indexTo);
    this->removeSelectedText();
    ((QTextEdit *)this)->insertAt(text, paraFrom, indexFrom);
}

int 
QxTextEdit::extraWidth()
{
    // QFrame::frameWidth() is number of pixels used by QFrame to draw the
    // border around it. 
    int w = this->leftMargin() + this->rightMargin() + 
            this->frameWidth()*2; // left and right framewidth
    if ( this->vScrollBarMode() == QScrollView::Auto ||
         this->vScrollBarMode() == QScrollView::AlwaysOn ) {
        w += this->verticalScrollBar()->sizeHint().width();
    }
    return w;
}

int 
QxTextEdit::extraHeight()
{

    int h = this->frameWidth()*2 + // top and bottom framewidth
            this->topMargin()+ this->bottomMargin();
    if ( this->hScrollBarMode() == QScrollView::Auto ||
         this->hScrollBarMode() == QScrollView::AlwaysOn ) {
        h += this->horizontalScrollBar()->sizeHint().height();
    }
    return h;
}

