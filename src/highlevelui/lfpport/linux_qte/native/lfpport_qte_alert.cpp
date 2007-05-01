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
 *
 * Qt port of Alert.
 */

#include <stdlib.h>
#include "lfpport_qte_util.h"
#include <qteapp_export.h>

#include "lfpport_qte_patched.h"

#include <lfp_command.h>
#include "lfpport_qte_alert.h"
#include "lfpport_qte_displayable.h"
#include "lfpport_qte_mscreen.h"

#include <midpUtilKni.h>

/* DECOUPLING: resolve cyclic dependency on lcdlf */
#include <lfpport_command.h>

#include <gxpportqt_image.h>

#include <moc_lfpport_qte_alert.cpp>

#include <midp_logging.h>

/**
 * Construct a push button with an abstract command id.
 *
 * @param parent parent widget
 * @param cmdId abstract command id this button is representing
 */
CommandButton::CommandButton(QWidget* parent, int cmdId)
    : QPushButton(parent) {

    id = cmdId;

    connect(this, SIGNAL(clicked()), SLOT(buttonActivated()));
}

/**
 * Update command ID
 *
 * @param new command ID
 */
void CommandButton::setCommandId(int cmdId) {
    id = cmdId;
}

/** Notify button is clicked */
void
CommandButton::buttonActivated() {
    // Send an event to Java
    MidpCommandSelected(id);
}

/**
 * Construct a text viewer that supports wrapping.
 * @param parent parent widget pointer
 */
TextViewer::TextViewer(QWidget* parent) : QMultiLineEdit(parent) {
    setReadOnly(true);
    setWordWrap(QMultiLineEdit::WidgetWidth);
}

/**
 * Query whether the text contents can fit one one page or needs scrolling.
 *
 * @return true if scrolling is needed to show all the contents.
 */
bool TextViewer::needScrolling() {
    return  lastRowVisible() != numLines() - 1;
}

/**
 * Override QMultiLineEdit to performe traversal.
 *
 * @param keyEvent key event to handle
 */
void TextViewer::keyPressEvent(QKeyEvent *key)
{
    int k = key->key();
    if ((k == Qt::Key_Up && rowIsVisible(0))
        || (k == Qt::Key_Down && rowIsVisible(numRows() - 1))
        || (k == Qt::Key_Tab))  {
        parentWidget()->setFocus();
    } else {
        QMultiLineEdit::keyPressEvent(key);
    }
}



/**
 * Construct a new modal window.
 *
 * @param parent parent widget pointer
 */
Alert::Alert(QWidget* parent)
    : QWidget(parent, NULL, WType_TopLevel | WType_Modal) {

    imageHolder = NULL;
    textViewer = NULL;
    numButtons = 0;

    for (int i = 0; i < ALERT_NUM_OF_BUTTONS; i++) {
	buttons[i] = NULL;
    }

    mscreen = (PlatformMScreen*)qteapp_get_mscreen();
    // Set geometry
    QWidget::setGeometry(
        (mscreen->getDisplayFullWidth() - mscreen->getAlertWidth()) / 2,                       // X
        MENUBAR_HEIGHT + (mscreen->getDisplayFullHeight() - mscreen->getAlertHeight()) / 2,    // Y
		mscreen->getAlertWidth(), mscreen->getAlertHeight());
	// Prevents resizing
    QWidget::setFixedSize(QWidget::size());
}

/**
 * Override QWidget::setGeometry() so that window cannot be moved.
 *
 * @param rect geometry to set alert to (ignored)
 */    
void Alert::setGeometry(const QRect & rect) {
  (void)rect;
}

/** Delete alert */
Alert::~Alert() {
    // The parent destructor will delete all its children QWidgets for us.
    // Don't have to do anything here
}

/**
 * Override QWidget::close() to stop close Alert.
 * Instead, activate the abstract command that a close user action
 * is mapped to.
 *
 * @param alsoDelete request to delete the widget 
 * @return TRUE if the widget was closed, otherwise FALSE.
 */
bool Alert::close(bool alsoDelete) {

  /* Suppress unused-parameter warning */
    (void)alsoDelete;
    // Send an event to Java
    MidpCommandSelected(closeCommandId);
    return FALSE;
}

void Alert::keyPressEvent(QKeyEvent *key) {

  if (key->key() == Qt::Key_F3) {
    mscreen->keyPressEvent(key);
  } else {
    QWidget::keyPressEvent(key);
  }
}

/**
 * Override QWidget::keyReleaseEvent() to grab the "cancel key"
 * event so we can tell MIDP to close the dialog box.
 *
 * @param key ptr to key event
 */
void Alert::keyReleaseEvent(QKeyEvent *key) {

  if (key->key() == Qt::Key_Escape) {
    MidpCommandSelected(closeCommandId);
  } else {
    QWidget::keyReleaseEvent(key);  
  }
}



/**
 * Reset the contents of this dialog.
 * 
 * @param img image component
 * @param gaugeBounds [0] and [1] are return-out values for gauge's (X, Y)
 *		      [2] and [3] are pass-in value for gauge's width and height
 * @param text message text
 * @return status of this call
 */
MidpError
Alert::setContents(QPixmap* img, int* gaugeBounds, QString text) {
    MidpError err;

    int y = 0;
    int i;
    int hspace;

    int new_x = 0;
    int new_y = mscreen->getAlertHeight()
	    // - ALERT_CELL_SPACING // space to dialog frame
	    - ALERT_BUTTON_HEIGHT; // button fixed height

    int maxImgHeight = mscreen->getAlertHeight()
			// - ALERT_CELL_SPACING // button space to frame
			- ALERT_BUTTON_HEIGHT // buttons
			- ALERT_CELL_SPACING // text space to buttons
			- 16 // minimum one text line
			- (gaugeBounds == NULL
			    ? 0
			    : gaugeBounds[3]+ALERT_CELL_SPACING); // gaugeHeight

    // Image first, centered
    err = setImage(y, maxImgHeight, img);
    if (err != KNI_OK) return err;

    // Gauge in the middle, centered
    err = setGauge(y, gaugeBounds);
    if (err != KNI_OK) return err;

    hspace = (mscreen->getAlertWidth() - ALERT_BUTTON_WIDTH*numButtons)/(numButtons + 1);

    for (i = 0; i < ALERT_NUM_OF_BUTTONS; i++) {

        if (buttons[i] != NULL) {
            new_x += hspace;
            buttons[i]->setGeometry(new_x, new_y,
                    ALERT_BUTTON_WIDTH, ALERT_BUTTON_HEIGHT);
            new_x += ALERT_BUTTON_WIDTH;
        }
    }
    
    
    // Text last
    return setText(y, text);
}

/**
 * Set the image component of this Alert.
 *
 * @param y starting Y coordinate in the Alert window.
 * @param maxHeight maximum allowed height for the image
 * @param img the image
 * @return status of this call
 */
MidpError
Alert::setImage(int& y, int maxHeight, QPixmap* img) {

    if (img == NULL) {
	if (imageHolder != NULL) {
	    delete imageHolder;
	    imageHolder = NULL;
	}
    } else {
	if (imageHolder == NULL) {
	    imageHolder = new QLabel(this);
	    if (imageHolder == NULL) return KNI_ENOMEM;
	    if (isVisible()) imageHolder->show();
	}
	imageHolder->setPixmap(*img);
	
	// Set imageHolder's geometry
	int x, w, h;
	// X

    x = (mscreen->getAlertWidth() - img->width()) >> 1;
	if (x < 0) {
	    x = 0;
	}

	// Y
	y += ALERT_CELL_SPACING;

	// W
	w = (img->width() > mscreen->getAlertWidth()) ? mscreen->getAlertWidth() : img->width();

	// H
	h = (img->height() > maxHeight) ? maxHeight : img->height();

	imageHolder->setGeometry(x, y, w, h);

	// Adjust y for next element
	y += h;
    }

    return KNI_OK;
}

/**
 * Calculate the (X, Y) location of Gauge.
 *
 * @param y starting Y coordinate for Gauge
 * @param gaugeBounds [0] and [1] are return-out values for gauge's (X, Y)
 *		      [2] and [3] are pass-in value for gauge's width and height
 * @return status of this call
 */
MidpError
Alert::setGauge(int& y, int* gaugeBounds) {

    if (gaugeBounds != NULL) {
	// X
	gaugeBounds[0] = (mscreen->getAlertWidth() - gaugeBounds[2]) >> 1;
	
	// Y
	y += ALERT_CELL_SPACING;
	gaugeBounds[1] = y;

	// Adjust y for next element
	y += gaugeBounds[3];
    }

    return KNI_OK;
}

/**
 * Set the message text of this Alert.
 *
 * @param y starting Y coordinate for text
 * @param text message text
 * @return status of this call
 */
MidpError
Alert::setText(int& y, QString text) {

    if (text == NULL || text.isNull() || text.isEmpty()) {
	if (textViewer != NULL) {
	    delete textViewer;
	    textViewer = NULL;
	}
    } else {
	// create textview
	if (textViewer == NULL) {
	    textViewer = new TextViewer(this);
	    if (textViewer == NULL) return KNI_ENOMEM;
	    if (isVisible()) textViewer->show();
	}
	// set text content
	textViewer->setText(text);
	// set geometry
	y += ALERT_CELL_SPACING;

	textViewer->setGeometry(ALERT_CELL_SPACING, 			// X
			        y, 					// Y
			        mscreen->getAlertWidth() - 2*ALERT_CELL_SPACING, 	// W
			        mscreen->getAlertHeight()
				// - ALERT_CELL_SPACING // gap buttons and frame
				- ALERT_BUTTON_HEIGHT // buttons
				- ALERT_CELL_SPACING // gap text and buttons
				- y);// H
	// No need to adjust y
    }

    return KNI_OK;
}

/**
 * Return whether current content requires scrolling.
 *
 * @return true if scrolling is needed to show all the contents.
 */
jboolean
Alert::needScrolling() {
    // It is only decided by the text
    if (textViewer == NULL) {
	return KNI_FALSE;
    } else {
	return textViewer->needScrolling();
    }
}

/**
 * Set command buttons.
 *
 * @param cmds array of abstract commands to map to buttons
 * @param numOfCmds size of the array
 * @return status of this call
 */
MidpError
Alert::setCommands(MidpCommand* cmds, int numOfCmds) {

    numButtons = (numOfCmds > ALERT_NUM_OF_BUTTONS
			? ALERT_NUM_OF_BUTTONS : numOfCmds);
    int hspace = (mscreen->getAlertWidth() - ALERT_BUTTON_WIDTH*numButtons)/(numButtons + 1);
    int x = 0;
    int y = mscreen->getAlertHeight()
	    // - ALERT_CELL_SPACING // space to dialog frame
	    - ALERT_BUTTON_HEIGHT; // button fixed height
    int i;
    QString label;

    // Setup push buttons
    for (i = 0; i < ALERT_NUM_OF_BUTTONS; i++) {

        x += hspace; // x coordinate of this button

	    if (i < numOfCmds) {
            if (i < ALERT_NUM_OF_BUTTONS-1
            || numOfCmds == ALERT_NUM_OF_BUTTONS) {
                if (buttons[i] == NULL) {
                // Create new command button and map to command
                buttons[i] = new CommandButton(this, cmds[i].id);
                if (buttons[i] == NULL) {
                    return KNI_ENOMEM;
                }
                } else {
                // Update command ID
                ((CommandButton*)buttons[i])->setCommandId(cmds[i].id);
                }
                // Set lable
                pcsl_string* short_label = &cmds[i].shortLabel_str;
                GET_PCSL_STRING_DATA_AND_LENGTH(short_label) {
                    if(PCSL_STRING_PARAMETER_ERROR(short_label)) {
                        REPORT_ERROR(LC_HIGHUI, "out-of-memory error"
                                    " in Alert::setCommands - set label");
                    } else {
                        label.setUnicodeCodes(short_label_data,
                                            short_label_len);
                        buttons[i]->setText(label);
                    }
                } RELEASE_PCSL_STRING_DATA_AND_LENGTH
    
                buttons[i]->show();
                buttons[i]->setFocusPolicy(QWidget::StrongFocus);
                // Set geometry
                buttons[i]->setGeometry(x, y,
                    ALERT_BUTTON_WIDTH, ALERT_BUTTON_HEIGHT);
    
            } else {
                if (buttons[i] == NULL) {
                // Create new push button
                buttons[i] = new QPushButton(this);
                if (buttons[i] == NULL) {
                    return KNI_ENOMEM;
                }
                }
                // Set lable
                buttons[i]->setText("More...");
    
                buttons[i]->show();
                buttons[i]->setFocusPolicy(QWidget::StrongFocus);
                // Set geometry
                buttons[i]->setGeometry(x, y,
                    ALERT_BUTTON_WIDTH, ALERT_BUTTON_HEIGHT);
    
                break; // break out to use popup list
            }
	    } else if (buttons[i] != NULL) {
	        // This button is no longer needed, delete it
	        delete buttons[i];
	        buttons[i] = NULL;
	    }

	    x += ALERT_BUTTON_WIDTH; // x coordinate of next button
    }

    // Setup popup list
    if (i == ALERT_NUM_OF_BUTTONS-1) {
        // We breaked out from the previous for loop
        // So there must be a need for the popup list
        QPopupMenu* popup = buttons[i]->popup();
        if (popup == NULL) {
           popup = new PatchedQPopupMenu(this);
           if (popup == NULL) return KNI_ENOMEM;
           connect(popup, SIGNAL(activated(int)), SLOT(commandActivated(int)));
           buttons[i]->setPopup(popup);
        } else {
            // Empty existing list
            popup->clear();
        }
        // Add remaining commands to the list
        for (; i < numOfCmds; i++) {
            pcsl_string* short_label = &cmds[i].shortLabel_str;
            GET_PCSL_STRING_DATA_AND_LENGTH(short_label) {
                if(PCSL_STRING_PARAMETER_ERROR(short_label)) {
                    REPORT_ERROR(LC_HIGHUI, "out-of-memory error"
                                 " in Alert::setCommands - setup popup list");
                } else {
                    label.setUnicodeCodes(short_label_data,
                                          short_label_len);
                }
            } RELEASE_PCSL_STRING_DATA_AND_LENGTH
            popup->insertItem(label, cmds[i].id);
        }
    } else if (buttons[ALERT_NUM_OF_BUTTONS-1] != NULL) {
        buttons[ALERT_NUM_OF_BUTTONS-1]->setPopup(NULL);
    }
    /* set focus to first command button 
        & make it focusProxy */
    if ((numOfCmds > 0) && (buttons[0] != NULL)) {
        setFocusProxy(buttons[0]);
        buttons[0]->setFocus();
    }

    closeCommandId = MidpCommandMapNegative(cmds, numOfCmds);

    return KNI_OK;
}

/**
 * Notify that a command is selected.
 *
 * @param commandId id of the abstract command
 */
void
Alert::commandActivated(int commandId) {
    // Send an event to Java
    MidpCommandSelected(commandId);
}

/**
 * Popup the Alert dialog and show its contents.
 * 
 * @param screenPtr pointer to the alert
 * @return status of this call
 */
extern "C" MidpError
alert_show(MidpFrame* screenPtr) {

    ((Alert *)screenPtr->widgetPtr)->show();
    return KNI_OK;
}

/**
 * Hide and delete resource function pointer.
 * This function should notify its Items to hide as well.
 * @param screenPtr pointer to the alert
 * @param onExit - true if this is called during VM exit.
 * 		   All native resource must be deleted in this case.
 * @return status of this call
 */
extern "C" MidpError
alert_hide_and_delete(MidpFrame* screenPtr, jboolean onExit) {

  /* Suppress unused-parameter warning */
    (void)onExit;

    ((Alert *)screenPtr->widgetPtr)->hide();
    delete (Alert *)screenPtr->widgetPtr;

    return KNI_OK;
}

/**
 * Screen event handling function pointer.
 * Return true if the event has been handled and should not be further
 * dispatched.
 * If the event is:
 * - For a particular Item (identified by its pointer/id)
 *   Forward it to that Item's handleEvent() function.
 * - Ticker or scroll bar events
 *   Handle it locally.
 *
 * QT NOTE: Since Qt uses SIGNAL/SLOT to deliver events, this function
 * 	is not used. Do nothing here.
 *
 * @param screenPtr pointer to the Alert
 * @param eventPtr pointer to the event
 * @return true if the event is handled and no further handling is needed.
 */
extern "C" jboolean
alert_handle_event(MidpFrame* screenPtr, PlatformEventPtr eventPtr) {
    /* Suppress unused-parameter warning */
    (void)screenPtr;
    (void)eventPtr;

    return KNI_FALSE; // Do nothing
}

/**
 * Set title for alert dialog.
 *
 * @param alertPtr pointer to the Alert
 * @param title new title string
 * @return status of this call
 */
extern "C" MidpError
alert_set_title(MidpDisplayable* alertPtr, const pcsl_string* title) {
    QString qtitle;
    pcsl_string2QString(*title, qtitle);
    truncateQString(qtitle,
                ((Alert *)alertPtr->frame.widgetPtr)->font(),
                calculateCaptionWidth((Alert *)alertPtr->frame.widgetPtr));
    ((Alert *)alertPtr->frame.widgetPtr)->setCaption(qtitle);

    return KNI_OK;
}

/**
 * Create a Alert's native peer without showing yet.
 * Upon successful return, *alertPtr will be filled properly.
 * Sound and abstract command buttons should not be handled in this
 * function. Separated function calls are used.
 *
 * @param alertPtr pointer to MidpDisplayable structure to be filled in
 * @param title title string
 * @param tickerText ticker text
 * @param alertType alert type as defined in MidpComponentType
 * @return Status of this call
 */
extern "C" MidpError
lfpport_alert_create(MidpDisplayable* alertPtr, const pcsl_string* title,
		     const pcsl_string*  tickerText, MidpComponentType alertType) {

    /* Suppress unused-parameter warning */
    (void)alertType;
    (void)tickerText;

    PlatformMScreen* mscreen = (PlatformMScreen*)qteapp_get_mscreen();

    // Fill in MidpDisplayable structure
    alertPtr->frame.widgetPtr	 = new Alert(mscreen);
    alertPtr->frame.show	 = alert_show;
    alertPtr->frame.hideAndDelete = alert_hide_and_delete;
    alertPtr->frame.handleEvent	 = alert_handle_event;
    alertPtr->setTitle		 = alert_set_title; // dialog title
    alertPtr->setTicker		 = displayable_set_ticker;

    alertPtr->setTitle(alertPtr, title);
    /* alertPtr->setTicker(alertPtr, tickerText); */
    return KNI_OK;
}

/**
 * (Re)set the content of alert dialog.
 * The alert could already have content. It should free those elements
 * that becomes null and allocate native resource for those that
 * becomes not-null.
 *
 * @param alertPtr pointer to MidpDisplayable structure whose fields should be
 *                        filled in
 * @param imgPtr icon image pointer. If NULL, no image should be shown.
 * @param gaugeBounds a 4 integer array defining indicator gauge's geometry
 *                        <pre>
 *                        [0] : x coordinate in alert dialog to be set on return
 *                        [1] : y coordinate in alert dialog to be set on return
 *                        [2] : preferred width in pixels. Can be changed to
 *                              granted width on return
 *                        [3] : preferred height in pixels. Can be changed to
 *                              granted height on return
 *                        null if no indicator gauge present.
 *                        </pre>
 * @param text alert message text
 * @return error code
 */
extern "C" MidpError
lfpport_alert_set_contents(MidpDisplayable* alertPtr,
			   unsigned char* imgPtr,
			   int* gaugeBounds,
			   const pcsl_string* text) {
    QString qtext;
    pcsl_string2QString(*text, qtext);

    PlatformMScreen* mscreen = (PlatformMScreen*)qteapp_get_mscreen();

    Alert* alertWidgetPtr = (Alert *)alertPtr->frame.widgetPtr;
    alertWidgetPtr->setFixedSize(mscreen->getAlertWidth(), mscreen->getAlertHeight());
    return alertWidgetPtr->setContents(gxpportqt_get_immutableimage_pixmap(
				           imgPtr),
				       gaugeBounds, qtext);
}

/**
 * Test whether the content requires scrolling.
 * 
 * @param needScrolling pointer to a boolean whose value should be set to true
 *                      if content requires scrolling to see all
 * @return error code
 */
extern "C" MidpError
lfpport_alert_need_scrolling(jboolean* needScrolling, 
			     MidpDisplayable* alertPtr) {
    *needScrolling = ((Alert *)alertPtr->frame.widgetPtr)->needScrolling();
    return KNI_OK;
}

/**
 * Update content of the menu.
 * @param alertPtr MidpFrame structure of current alert
 * @param cmds array of commands the menu should contain
 * @param numCmds size of the array
 * @return status of this call
 */
MidpError lfpport_alert_set_commands(MidpFrame* alertPtr,
				     MidpCommand* cmds, int numCmds) {
    return ((Alert *)alertPtr->widgetPtr)->setCommands(cmds, numCmds);
}
