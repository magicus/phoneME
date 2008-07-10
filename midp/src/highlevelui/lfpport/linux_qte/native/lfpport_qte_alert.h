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
 * Qt port of Alert.
 */

#ifndef _LFPPORT_QTE_ALERT_H_
#define _LFPPORT_QTE_ALERT_H_

#include <qpushbutton.h>
#include <qmultilineedit.h>
#include <qpopupmenu.h>
#include <qwidget.h>
#include <qlabel.h>
#include "lfpport_qte_mscreen.h"        

extern "C" {
#include <lfpport_alert.h>
}

/**
 * Customized QPushButton that supports abstract command id.
 */
class CommandButton : public QPushButton {
    Q_OBJECT

    int id;

public:
    /**
     * Constructs a push button for the given alert's given abstract command.
     *
     * @param parent alert widget this button belongs to
     * @param id identifier of the abstract command for which to construct the
     * button.
     */
    CommandButton(QWidget* parent, int id);

    /**
    * Update command ID
    *
    * @param new command ID
    */
    void setCommandId(int cmdId);

protected slots:
  /**
   * Notifies the Java platform that the user has clicked this button.
   */
  void buttonActivated();
};

/**
 * Extend QMultiLineEdit to support query on whether scrolling is needed.
 */
class TextViewer : public QMultiLineEdit {

public:
    /**
     * Constructs a text viewer that supports line wrapping of its contents
     * for the given alert.
     * 
     * @param parent native peer of the alert to receive the command.
     *
     * @return the new uneditable, scrollable text area for the abstract
     * command
     */
    TextViewer(QWidget* parent);

    /**
     * Checks whether the user will need to scroll the contents of the text
     * viewer in order to see its entire contents.
     *
     * @return true if the contents will need to be scrolled; false otherwise.
     */
    bool needScrolling();

protected:
    /**
     * Override QMultiLineEdit to performe traversal.
     *
     * @param keyEvent key event to handle
     */
    void keyPressEvent ( QKeyEvent *key);

};

/**
 * Alert widget.
 */
class Alert : public QWidget {
    Q_OBJECT

    /**
     * Widget to show image.
     */
    QLabel     *imageHolder;
    /**
     * Widget to show text.
     */
    TextViewer *textViewer;
    /**
     * Array of abstract commands for this alert
     */
    QPushButton *buttons[ALERT_NUM_OF_BUTTONS];

    /**
    * Number of alert buttons
    */
    int numButtons;

    /**
    * Owner of alert
    */
    PlatformMScreen * mscreen;

    /**
     * Places on this alert, at the given y position with no more than the
     * given maximum height, the given image. This function centers the image
     * horizontally on the alert window.
     *
     * @param y the requested vertical position for the image. (The
     * actual image will begin at y plus room for cell spacing.)
     * @param maxHeight the most vertical space that the image can use.
     * @param img pointer to the image to place on the alert.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError setImage(int& y, int maxHeight, QPixmap* img);

    /**
     * Beginning at the given y position, sets the boundaries for a gauge on
     * this alert.
     *
     * @param y the requested vertical position for the gauge.
     * @param gaugeBounds structure that holds the upper-left and lower-right
     * coordinates for the gauge.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError setGauge(int& y, int* gaugeBounds); 

    /**
     * Places a text viewer on this alert, at the given y position, which
     * displays the given text.
     *
     * @param y the requested vertical position for the gauge.
     * @param text the text to display.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError setText(int& y, QString text);

public:
    /**
     * Constructs an alert widget.
     *
     * @param parent parent widget this alert should pop out from
     */
    Alert(QWidget* parent);

    /**
     * Deletes the alert dialog box. This function does not have to delete the
     * alert's components; that is done by the alert's parent.
     */
    ~Alert();

    /**
     * Override QWidget so that window cannot be moved.
     */    
    void setGeometry(const QRect & rect);

    /**
     * Override QWidget to not hide the widget and notify Java peer instead.
     *
     * @param alsoDelete whether the widget should also be deleted.
     *			Not used.
     * @return true if closed
     */
    bool close(bool alsoDelete);

    /**
     * Override QWidget::keyReleaseEvent() to grab the "cancel key"
     * event so we can tell MIDP to close the dialog box.
     *
     * @param key ptr to key event
     */
   void keyReleaseEvent(QKeyEvent *key);

   void keyPressEvent(QKeyEvent *key);    

    /**
     * Resets the contents of this alert using the given image, <i>gauge</i>
     * bounds, and text.
     *
     * @param img pointer to the new image for the alert.
     * @param gaugeBounds [0] and [1] are return values for gauge's (X, Y)
     *		      [2] and [3] are pass-in value for gauge's width and height
     * @param text new text of the alert.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError setContents(QPixmap* img, int* gaugeBounds, QString text);

    /**
     * Returns whether the user will have to scroll to see all of the alert's
     * current content.
     *
     * @return true if scrolling will be required; false otherwise
     */
    jboolean  needScrolling();

    /**
     * Set abstract commands of this Alert.
     * The commands should be mapped to buttons. If there is more commands
     * than buttons, the last command is turned into a popup menu to show
     * the remaining commands.
     *
     * @param cmds array of commands for this alert.
     * @param numOfCommands size of the <tt>cmds</tt> array.
     *
     * @return an indication of success or the reason for failure     
     */
    MidpError setCommands(MidpCommand* cmds, int numOfCmds);

    /**
     * Store the id of the command which close action is mapped.
     */
    int closeCommandId;

protected slots:
    /**
     * Notifies the Java platform that the user has selected the given command
     * from this alert.
     *
     * @param commandId identifier of the selected command.
     */
    void commandActivated(int commandId);
};

#endif /* _LFPPORT_QTE_ALERT_H_ */
