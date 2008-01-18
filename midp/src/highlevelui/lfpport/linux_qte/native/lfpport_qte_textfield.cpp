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

#include <lfpport_form.h>
#include "lfpport_qte_textfield.h"
#include <moc_lfpport_qte_textfield.cpp>

#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"

#include <qregexp.h>
#include <stdio.h>

/** The number of visible lines of a long ANY TextField */
#define LONG_ANY_TEXTFIELD_LINES	6
/** The number of visible lines of a long URL TextField */
#define LONG_URL_TEXTFIELD_LINES	2

/** Constructor */
TextFieldBody::TextFieldBody(QWidget *parent)
    : QMultiLineEdit(parent) {
    connect(this, SIGNAL(textChanged()), SLOT(notifyStateChanged()));
}

/** Set cursor position in one dimension manner. */
void TextFieldBody::setCursorPosition(int position) {
    if (position < 0) {
	home(FALSE);
    } else if (position > length()) {
	end(FALSE);
    } else {
	/* Iterate each line's length */
	for (int line = 0; line < numLines(); line++) {
	    int numChars = lineLength(line);
	    if (position <= numChars) {
		QMultiLineEdit::setCursorPosition(line, position);
		return;
	    }
            position -= numChars + 1; /* EOL is counted as one char */
	}
    }
    /* Should not reach here */
}

/**
 * Override QMultiLineEdit to notify Java peer of traversal out.
 *
 * @param keyEvent key event to handle
 */
void TextFieldBody::keyPressEvent(QKeyEvent *key)
{
    int k = key->key();
    // always handle select event because it switches 
    // between the modal and non-modal modes
#ifdef QT_KEYPAD_MODE
    if (k == Qt::Key_Select) {
        QMultiLineEdit::keyPressEvent(key);
    } else if (isModalEditing()) {
#endif
        if (isReadOnly()) {
            if ((k == Qt::Key_Up && rowIsVisible(0))
                || (k == Qt::Key_Down && rowIsVisible(numRows() - 1)))  {
                
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyPressEvent(key);
                
            } else {
                QMultiLineEdit::keyPressEvent(key);
            }
        } else {
            int line;
            int col;
            QMultiLineEdit::getCursorPosition(&line, &col);
            if ((k == Qt::Key_Up && line == 0)  
                || (k == Qt::Key_Down && (line == numLines() - 1))){
                
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyPressEvent(key);
                
            } else {
                QMultiLineEdit::keyPressEvent(key);
            }
        }
#ifdef QT_KEYPAD_MODE
    } else {
        // not handle events while it's in not modal state
        key->ignore();
    }
#endif
}

/**
 * Override QMultiLineEdit to notify Java peer of traversal out.
 *
 * @param keyEvent key event to handle
 */
void TextFieldBody::keyReleaseEvent(QKeyEvent *key)
{
    int k = key->key();
    // always handle select event because it switches 
    // between the modal and non-modal modes
#ifdef QT_KEYPAD_MODE
    if (k == Qt::Key_Select) {
        QMultiLineEdit::keyReleaseEvent(key);
    } else if (isModalEditing()) {
#endif
        int line;
        int col;
        QMultiLineEdit::getCursorPosition(&line, &col);
        if (k == Qt::Key_Up && line == 0)  {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyReleaseEvent(key);
        } else if (k == Qt::Key_Down && line == numLines() - 1) {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyReleaseEvent(key);
        } else {
            QMultiLineEdit::keyReleaseEvent(key);
        }
#ifdef QT_KEYPAD_MODE
    } else {
        // not handle events while it's in not modal state
        key->ignore();
    }
#endif
}


/** Get cursor position in one dimension manner. */
int TextFieldBody::getCursorPosition() {
    int position;

    if (atBeginning()) {
	position = 0;
    } else if (atEnd()) {
	position = length();
    } else {
	int line, col;

	QMultiLineEdit::getCursorPosition(&line, &col);

	/* Iterate each line's length */
	for (position = col, line--; line >= 0; line--) {
	    position += lineLength(line) + 1; /* EOL is counted as one char */
	}
    }

    return position;
}

/** Validate a string against current constraint */
bool TextFieldBody::validate(const QString &s, int line, int col) {
    bool ok = false;

    switch ((MidpConstraint)(constraints & MIDP_CONSTRAINT_MASK)) {
    case MIDP_CONSTRAINT_NUMERIC:
      {
	QRegExp numbers("^-?[0-9]*$");
	if (numbers.match(s) >= 0) {
	  if (length() == 0) {
	    ok = true;
	  } else if (s[0] == '-') {  /* only allow one '-' & at beginning */
	    if (text()[0] != '-' && line == 0 && col == 0) {
	      ok = true;
	    }
	  } else {  /* number(s) being entered */
	    if (text()[0] != '-') {  /* positive integer */
	      ok = true;
	    } else {  /* negative integer-allow digits only after '-' */
	      if (col > 0 || line > 0) {
		ok = true;
	      }
	    }
	  }
	}
      }
      break;
    case MIDP_CONSTRAINT_DECIMAL:
	{
	  QRegExp decimals("^-?[0-9]*[.]?[0-9]*$");
	  if (decimals.match(s) >= 0) {
	    if (length() == 0) {
	      ok = true;
	    } else {
	      if (s[0] == '-') {  /* only allow one '-' & at beginning */
		if (text()[0] != '-' && line == 0 && col == 0) {
		  ok = true;
		}
	      } else {     /* number(s) being entered */
		if (text()[0] != '-') {    /* positive decimal */
		  ok = true;
		} else {  /* negative decimal-allow digits only after '-' */
		  if (col > 0 || line > 0) {
		    ok = true;
		  }
		}
	      }
	      /* Only allow one dot(.) */
	      if (ok) {
		if (s.find('.') >= 0 && text().find('.') >= 0) {
		  ok = false;
		}
	      }
	    }
	  }
	}
	break;
    case MIDP_CONSTRAINT_PHONENUMBER:
    {
        /* the phone number has to accept the '+' at the start of the text, any digits, '#' and '*' */
        QRegExp numbers("^[\053]?[0-9\\s\052\043]*$");
        if (numbers.match(s) >= 0) {
            ok = true;
        }
    }
    break;
    case MIDP_CONSTRAINT_ANY:
    case MIDP_CONSTRAINT_EMAILADDR:
    case MIDP_CONSTRAINT_URL:
        ok = true;
        break;

    } /* end of switch(constraints) */
    
    return ok;
}


/**
 * Method format phone string
 * @param s string
 */
QString TextFieldBody::getStringForPhoneNumber(QString& s) {
    int res = s.find(" ",0);
    while (res != -1) {
        s.remove(res,1);
        res = s.find(" ",0); 
    }
    switch (s.length()) {
        case 5:
        case 6:
        case 7:
            s.insert(3," ");
            break;
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
            s.insert(3," ");
            s.insert(6," ");
            break;
        case 11:
            s.insert(1," ");
            s.insert(5," ");
            s.insert(9," ");
            break;
      }
    return s;

}

/** Override to validate constraint against new insertion */
void TextFieldBody::insertAt(const QString &s, int line, int col,
			     bool mark) {

    QString str;
 
    if (validate(s, line, col)) {
         switch ((MidpConstraint)(constraints & MIDP_CONSTRAINT_MASK)) {
            case MIDP_CONSTRAINT_PHONENUMBER:

                str = QMultiLineEdit::textLine(line);
                str.insert(col,s);

                QMultiLineEdit::removeLine(line);
                str = getStringForPhoneNumber(str);

                QMultiLineEdit::insertAt(str,line,0,mark);
                QMultiLineEdit::setCursorPosition(line,col + 2,mark);
            break;
            default:
                QMultiLineEdit::insertAt(s, line, col, mark);
        }
    }
}

/**
 * Override QMultLineEdit::backspace to format phone string
 */
void TextFieldBody::backspace() {

    QString str;

    int line, col;

    QMultiLineEdit::backspace();

    QMultiLineEdit::getCursorPosition(&line, &col);

    switch ((MidpConstraint)(constraints & MIDP_CONSTRAINT_MASK)) {
        case MIDP_CONSTRAINT_PHONENUMBER:
            str = QMultiLineEdit::textLine(line);
            QMultiLineEdit::removeLine(line);

            str = getStringForPhoneNumber(str);

            QMultiLineEdit::insertAt(str,line,0,FALSE);
            break;
        default:
            break;
    }
}

/**
 * Override QMultLineEdit::del to format phone string
 */
void TextFieldBody::del() {

    QString str;

    int line, col;

    QMultiLineEdit::del();

    QMultiLineEdit::getCursorPosition(&line, &col);

    switch ((MidpConstraint)(constraints & MIDP_CONSTRAINT_MASK)) {
        case MIDP_CONSTRAINT_PHONENUMBER:
            str = QMultiLineEdit::textLine(line);
            QMultiLineEdit::removeLine(line);

            str = getStringForPhoneNumber(str);
            QMultiLineEdit::insertAt(str,line,0,FALSE);
            break;
        default:
            break;
    }
}


/** Override to notify Form focus change */
void TextFieldBody::focusInEvent(QFocusEvent *event) {
    /* Notify Java if this is caused by user action */
    if (event->reason() != QFocusEvent::Other) {
	MidpFormFocusChanged(parent());
    }

    /* Continue with focus activation */
    QMultiLineEdit::focusInEvent(event);
}

/** Listening for text changes to notify Java Peer */
void TextFieldBody::notifyStateChanged() {
    MidpFormItemPeerStateChanged(parent(), 0);
}

/** Wrapper function to avoid triggering Java notification event */
void TextFieldBody::setTextQuietly(const QString &s) {
    bool wasBlocked = signalsBlocked();
    if (!wasBlocked) {
	/* Prevent setText() call from emitting textChanged() signal */
	blockSignals(true);
    }

    setText(s);

    /* Restore blocking state */
    if (!wasBlocked) {
	blockSignals(false);
    }
}

/**
 * Construct a TextField native peer with label and body.
 */
TextField::TextField(QWidget *parent, const QString &label, int layout,
		     const QString &text, int maxSize,
		     int constraints, const QString &inputMode)
    : Item(parent, label, layout)
{
    /* Suppress unused-parameter warning */
    (void)inputMode;

    qedit = new TextFieldBody(this);
    qedit->setMaxLength(maxSize);

    setConstraints(constraints);
    
    /* The text will be validated by constraints above */
    setString(text);
 
    setFocusPolicy(QWidget::StrongFocus);

    /* Delegate focus to MultiLineEdit */
    setFocusProxy(qedit);
}

/**
 * Construct a TextField native peer with label and body.
 */
TextField::~TextField() {
    delete qedit;
}

/** Implement virtual function (defined in lfpport_qte_item.h) */
void TextField::bodyResize(int w, int h) {
    // keep the old cursor position
    int oldPos = qedit->getCursorPosition();
    qedit->resize(w, h);
    qedit->setWordWrap(QMultiLineEdit::WidgetWidth);
    // try to set the cursor at the old position
    qedit->setCursorPosition(oldPos);
}

void TextField::bodyRelocate(int x, int y) {
  qedit->move(x, y);
}

/**
 * Get preferred height with the given width.
 * If input constraint is NOT ANY or URL, we prefer single line.
 * Otherwise, if the maximum size is less than a single
 * line can fit, we still prefer single line. Else,
 * we prefer to have 6 visible lines.
 */
int TextField::bodyHeightForWidth(int *takenWidth, int w) {

    int visibleLines;
    /* Deduct frame width on both ends */
    int border = qedit->frameWidth()*2;
    /* Maximum number of chars on a single line, plus a cursor */
    int maxWidth = (qedit->maxLength()+1)*qedit->fontMetrics().width('W');

    if (border + maxWidth <= w) {
        visibleLines = 1;
	*takenWidth  = border + maxWidth;
    } else {
	/* Decide preference basing on constraint */
	switch ((MidpConstraint)(qedit->constraints & MIDP_CONSTRAINT_MASK)) {

	case MIDP_CONSTRAINT_ANY:
	    visibleLines = LONG_ANY_TEXTFIELD_LINES;
	    break;

	case MIDP_CONSTRAINT_URL:
	    visibleLines = LONG_URL_TEXTFIELD_LINES;
	    break;

	default:
	    visibleLines = 1;
	}

	/* Need the full width */
	*takenWidth = w;
    }

    return (border + qedit->fontMetrics().lineSpacing()*visibleLines);
}

int TextField::bodyWidthForHeight(int *takenHeight, int h) {
    /* Suppress unused-parameter warning */
    (void)h;

    *takenHeight = 0;
    return 0;
}

bool TextField::bodyCanBeOnSameLine(int bodyHeight) {
  bool res = Item::bodyCanBeOnSameLine(bodyHeight);
  return res && (qedit->numLines() == 1);
}

/**
 * Set the text content of current TextField.
 */
MidpError
TextField::setString(const QString &text) {
    qedit->setTextQuietly(text);
    qedit->setCursorPosition(qedit->isReadOnly() ? 0 : qedit->length());
    return KNI_OK;
}

/**
 * Get the text content of current TextField.
 */
MidpError
TextField::getString(QString &text) {
    text = qedit->text();
    return KNI_OK;
}

/**
 * Set the maximum text length of current TextField.
 */
MidpError
TextField::setMaxSize(int maxSize) {

    qedit->setMaxLength(maxSize);

    /*
     * Workaround Qt feature in QMultiLineEdit::setMaxLength()
     * Truncate existing text if it is larger than the new maxSize.
     */
    if (maxSize < qedit->length()) {
	QString text = qedit->text();
	text.truncate(maxSize);
	qedit->setTextQuietly(text);
    }

    return KNI_OK;
}

/**
 * Get the insertion point of current TextField.
 */
MidpError
TextField::getCaretPosition(int &position) {
    position = qedit->getCursorPosition();
    return KNI_OK;
}

/**
 * Set input constraint and modifiers as defined in MIDP Spec.
 * Handle modifiers here. Constraint will be handled in insertAt().
 */
MidpError
TextField::setConstraints(int constraints) {
    /* Uneditable Modifier */
    if (constraints & MIDP_MODIFIER_UNEDITABLE) {
	qedit->setReadOnly(true);
    } else {
	qedit->setReadOnly(false);
    }

    /* Password Modifier */
    if (constraints & MIDP_MODIFIER_PASSWORD) {
	qedit->setEchoMode(QMultiLineEdit::Password);
    } else {
	qedit->setEchoMode(QMultiLineEdit::Normal);
    }

    /* Check the rest constraints in TextFieldBody::validate() */
    qedit->constraints = constraints;

    return KNI_OK;
}

/**
 * Create a TextField Qt peer without showing it yet.
 * Upon successful return, fields in *itemPtr should be set properly.
 */
extern "C" MidpError
lfpport_textfield_create(MidpItem* itemPtr, MidpDisplayable* formPtr,
			 const pcsl_string* label, int layout,
			 const pcsl_string* text, int maxSize,
			 int constraints, const pcsl_string* inputMode) {
    QString qlabel, qtext, qinputMode;

    pcsl_string2QString(*label, qlabel);
    pcsl_string2QString(*text, qtext);
    pcsl_string2QString(*inputMode, qinputMode);

    /* Fill in MidpItem structure */
    itemPtr->widgetPtr = 
      new TextField((formPtr == INVALID_NATIVE_ID
                     ? 0 : (QWidget *)formPtr->frame.widgetPtr),
		    qlabel,
		    layout,
		    qtext,
		    maxSize,
		    constraints,
		    qinputMode);

    initItemPtr(itemPtr, formPtr);
    return KNI_OK;
}

/**
 * Notifies native peer of a content change in the corresponding TextField.
 * @param text - the new string set
 */
extern "C" MidpError
lfpport_textfield_set_string(MidpItem* itemPtr, const pcsl_string* text)
{
    QString qtext;

    pcsl_string2QString(*text, qtext);
    
    return ((TextField *)itemPtr->widgetPtr)->setString(qtext);
}

/**
 * Query native peer for current text content.
 * @param text - pointer to the returned content text
 * @param newChange - pointer to the returned flag for whether new changes
 *			has been made since last call of this function
 */
extern "C" MidpError
lfpport_textfield_get_string(pcsl_string* text, jboolean* newChange,
			     MidpItem* itemPtr) {
    QString qtext;

    MidpError err = ((TextField *)itemPtr->widgetPtr)->getString(qtext);

    if (err == KNI_OK) {
	err = QString2pcsl_string(qtext, *text);
	*newChange = KNI_TRUE;
    }

    return err;
}

/**
 * Notifies native peer of a maximum size change in the corresponding TextField.
 * @param maxSize - the new maximum size
 */
extern "C" MidpError
lfpport_textfield_set_max_size(MidpItem* itemPtr, int maxSize) {
    return ((TextField *)itemPtr->widgetPtr)->setMaxSize(maxSize);
}

/**
 * Gets the current input position.
 * @return the current caret position, <code>0</code> if at the beginning
 */
extern "C" MidpError
lfpport_textfield_get_caret_position(int *position, MidpItem* itemPtr) {
    return ((TextField *)itemPtr->widgetPtr)->getCaretPosition(*position);
}

/**
 * Notifies native peer that constraints have to be changed.
 * @param constraints - the new input constraints
 */
extern "C" MidpError
lfpport_textfield_set_constraints(MidpItem* itemPtr, int constraints) {
    return ((TextField *)itemPtr->widgetPtr)->setConstraints(constraints);
}
