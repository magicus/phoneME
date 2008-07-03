/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * 
 * This source file is specific for Qt-based configurations.
 */

/**
 * @file
 * Definition for Qt port of text field.
 */

#ifndef _LFPPROT_QTE_TEXTFIELD_H_
#define _LFPPROT_QTE_TEXTFIELD_H_

#include <lfpport_textfield.h>
#include "lfpport_qte_item.h"

#include <qmultilineedit.h>

/**
 * Body widget of TextField.
 */
class TextFieldBody : public QMultiLineEdit {
    Q_OBJECT

protected:
    /**
     * Validates a string against the current constraint
     *
     * @param s new inserted string
     * @param line line number of the insertion point
     * @param col column number of the insertion point
     * @return true if this insertion is allowed by current input constraints
     */
    bool validate(const QString &s, int line, int col);

   /**
    * Makes this item have focus, enabling any item-specific commands; in
    * addition, if the given event was caused by a user action, notifies the
    * Java platform of the change in focus.
    *
    * @param event pointer to the device event corresponding to the change in
    *        focus.
    */
    void focusInEvent(QFocusEvent *event);

protected slots:
    /**
     * Listens for text changes to this form item, and notifies the Java
     * platform when they occur.
     */
    void notifyStateChanged();

    /**
     * Override QMultiLineEdit to notify Java peer of traversal out.
     *
     * @param keyEvent key event to handle
     */
    void keyPressEvent ( QKeyEvent *key);

    /**
     * Override QMultiLineEdit to notify Java peer of traversal out.
     *
     * @param keyEvent key event to handle
     */
    void keyReleaseEvent(QKeyEvent *key);

    /**
     * Method format phone string
     * @param s string 
     */
    QString getStringForPhoneNumber(QString& s);

public:
    /**
     * Constructor.
     *
     * @param parent Main widget of this TextField
     */
    TextFieldBody(QWidget *parent);

    /**
     * Sets the current position of the cursor in the text field to the given
     * position. It uses a one-dimensional algorithm: in a multi-line text
     * field, an EOL is simply counted as another character. That is, the
     * length of a line is <i>numChars</i>+1, and the first character of the
     * next line is <i>sumOfPreviousLineLengths</i>+1.
     *
     * <p>If the given position is a negative number, the cursor is placed at
     * the beginning of the text field; if the given position is greater than
     * the number of characters in the text field, the cursor is placed at the
     * end of the text field.</p>
     *
     * @param position the position in the text field where the cursor should
     *        be, in number of characters.
     */
    void setCursorPosition(int position);

    /**
     * Gets the current position of the cursor in the text field. It uses a
     * one-dimensional algorithm: in a multi-line text field, an EOL is simply
     * counted as another character.  That is, the length of a line is
     * <i>numChars</i>+1, and the first character of the next line is
     * <i>sumOfPreviousLineLengths</i>+1.
     *
     * @return the position of the cursor in the text field, in number of
     * characters. If the cursor is at the front of the text field, this
     * function returns zero.
     */
    int getCursorPosition();
    
    /**
     * Updates the text of this text field without triggering a notification
     * to the Java platform. This is used when its contents is changed 
     * programmatically by MIDlet instead of by user.
     * 
     * @param s new text
     */
    void setTextQuietly(const QString &s);

    /**
     * Override QMultLineEdit to validate new insertion against input
     * constraint.
     *
     * @param s new inserted string
     * @param line line number of the insertion point
     * @param col column number of the insertion point
     * @param mark whether the text is marked
     */
    void insertAt(const QString &s, int line, int col, bool mark = FALSE);

    /**
     * Override QMultLineEdit::backspace to format phone string
     */
    void backspace();

    /**
     * Override QMultLineEdit::del to format phone string
     */
    void del();

    /** This text field's input constraints, if any. As discussed in the
     * <i>MIDP Specification</i>, an input constraint makes it easier for
     * users to enter certain characters. The MIDP implementation uses the
     * value of <tt>constraints</tt> to validate input text */
    int constraints;
};

/**
 * Main widget of a TextField.
 */
class TextField : public Item {

    /**
     * Body (current content and associated constraints) of this
     * TextField.
     */
    TextFieldBody *qedit;

 protected:
    /**
     * Move body widget.
     *
     * @param x the horizontal coordinate of the upper-left corner of this main widget.
     * @param y the vertical coordinate of the upper-left corner of this main widget.
     */
    void bodyRelocate(int x, int y);

    /**
     * Resize body widget.
     * 
     * @param w new width
     * @param h new height
     */
    void bodyResize(int w, int h);

    /**
     * Calculate body widget height when width is limited to a given value.
     *
     * @param takenWidth return value of the real width used
     * @param w maximum width
     * @return body widget height
     */
    int  bodyHeightForWidth(int *takenWidth, int w);

    /**
     * Calculate body widget width when height is limited to a given value.
     *
     * @param takenHeight return value of the real height used
     * @param h maximum height
     * @return body widget width
     */
    int  bodyWidthForHeight(int *takenHeight, int h);

    /**
     * Test whether the text can fit in the given height.
     *
     * @param bodyHeight maximum height
     * @return true if the text height is smaller than the given maximum height
     */
    bool bodyCanBeOnSameLine(int bodyHeight);


public : 
    /**
     * TextField's constructor.
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this textfield
     * @param text text contents
     * @param maxSize maximum allowed number of characters
     * @param constraints lower 16 bits: type of MidpConstraints,
     *			  high  16 bits: type of MidpModifier
     * @param inputMode suggested input method
     */
    TextField(QWidget *parent, const QString &label, int layout,
	      const QString &text,
	      int maxSize, int constraints, const QString &inputMode);
	      
    /**
     * TextField's destructor.
     */
    ~TextField();

    /**
     * Update text contents.
     *
     * @param text new text contents
     * @return status of this call
     */
    MidpError setString(const QString &text);

    /**
     * Get the current text contents.
     *
     * @param text text to be set to the current contents
     * @return status of this call
     */
    MidpError getString(QString &text);

    /**
     * Change maximum size.
     * If current contents is longer than this size, it will be truncated.
     *
     * @param maxSize new maximum number of characters
     * @return status of this call
     */
    MidpError setMaxSize(int maxSize);

    /**
     * Get current cursor position in one dimensional manner.
     *
     * @param position return value of the cursor position
     * @return status of this call
     */
    MidpError getCaretPosition(int &position);

    /**
     * Change input constraints.
     *
     * @param constraints new input constraints
     * @return status of this call
     */
    MidpError setConstraints(int constraints);

};

#endif /* _LFPPROT_QTE_TEXTFIELD_H_ */
