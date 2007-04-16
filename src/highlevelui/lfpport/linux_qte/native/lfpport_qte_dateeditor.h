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
 * This file contains three classes: DateTimeEditor, DateEditor and SetTime.
 * DateTimeEditor is the main container of the date/time widget;
 * DateEditor is the date editor part;
 * SetTime is the time editor part.
 * DateEditor or SetTime are always created, but at most one of them may be
 * hidden at any given time (as defined by setInputMode()).
 */

#ifndef _LFPPORT_QTE_DATEEDITOR_H_
#define _LFPPORT_QTE_DATEEDITOR_H_

#include <qdatetime.h>
#include <qdialog.h>
#include <qlabel.h>

#include <time.h>
#include <qpopupmenu.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qpushbutton.h>

class DateTimeEditor;
class DatePicker;

/** Time editor built of two spin boxes. */
class SetTime : public QFrame {
    Q_OBJECT
protected slots:
    /**
     * This method receives hour changed event.
     *
     * @param value new hour
     */
    void hourChanged(int value);

    /**
     * This method receives minute changed event
     *
     * @param value new minute
     */
    void minuteChanged(int value);

    /**
     * This method receives pm/am changed event
     *
     * @param index the position of the selected item
     */
    void checkedPM(int index);

protected:
    /** Hour value member */
    int hour;

    /** Minute value member */
    int minute;

    /** Flag for 12/24 hour display and edit mode */
    bool use12hourTime;

    /** Combobox for am/pm selection used in 12 hour mode */
    QComboBox *ampm;

public:
    /**
     * Constructor of time editor widget
     *
     * @param parent parent widget
     * @param initialTime time to show on widget creation
     */
    SetTime(DateTimeEditor *parent=0, long initialTime=0);

    /**
     * Date editor can be displayed in an undefined state, showing
     * HH:MM as the hour and DATE_FORMAT_HINT ("www, mm dd yyyy") as
     * the date. When the user interacts with the date editor (any of the
     * widgets comprising the datefield), the undefined state is changed
     * automatically to FALSE (ie. to defined state).
     */
    bool undefinedState;

    /**
     * Gets time set in time editor
     *
     * @return currently set QTime (hour and minute)
     */
    QTime time() const;

    /**
     * Undefined state appears as "HH:MM" and no date selected.
     * This method allows to switch from defined to undefined appearance.
     *
     * @param turnOn true if the state should be set to undefined mode
     */
    void setUndefinedState(bool turnOn);

    /**
     * Uses this method to set value of this date widget.
     *
     * @param hour hour to be set
     * @param min minute to be set
     */
    void setTime(int hour, int min);

    /**
     * Gets the hour value set in time editor
     *
     * @return the hour value of the widget
     */
    int getHour() { return hour; };

    /**
     * Gets the minute value set in time editor
     *
     * @return the minute value of the widget
     */
    int getMinute() { return minute; };

    /**
     * Makes this item have focus, enabling any item-specific commands; in
     * addition, if the given event was caused by a user action, notifies
     * the Java platform of the change in focus.
     *
     * @param event device event corresponding to the change in focus.
     */
    void focusInEvent(QFocusEvent *event);

    /** Spin box for hour selection */
    QSpinBox *sbHour;

    /** Spin box for minute selection */
    QSpinBox *sbMin;

    /**
     * Peer notification is needed only once per user interaction with the
     * native widget. There is no need to notify java when a set is originated
     * from the MIDlet (java);
     */
    bool needPeerNotification;

    /** Used to join related peer notifications */
    int stackCounter;

public slots:
    /**
     * Changes display and edit mode to 12 or 24 hour.
     *
     * @param on 0 if 24 hour mode, otherwise 12 hour mode
     */
     void show12hourTime(int on);

 private:
    /** A pointer to the parent */
    DateTimeEditor *dte;

};

/** Time conversion and presentation routines */
class TimeUtils {

public:
    /** Gets date string in long format by date object
     *
     * @param date QDate instance to get string presentation for
     * @return date string in format "www mm dd, yyyy"
     */
    static QString longDateString(const QDate &date);

    /** Gets date string in short format by date object
     *
     * @param date QDate instance to get string presentation for
     * @return date string in format "mm/dd/yy"
     */
    static QString shortDateString(const QDate &date);

    /** Converts broken-down time into time since the Epoch
     *
     * @param date QDate instance to get UTC time for
     * @return calendar UTC time representation
     */
    static time_t toUTC(const QDateTime &dateTime);
};

/**
 * Date editor widget built from a label for the date,
 * and a QButton to open a month view popup selector,
 * displays date and change button
 */
class DateEditor : public QPushButton {
    Q_OBJECT
public:
    /**
     * Constructs DateEditor widget
     *
     * @param longDate when TRUE selects long format for
     *   date string, otherwise short format will be used
     * @param parent reference to the parent container
     */
    DateEditor(bool longDate, DateTimeEditor *parent);

    /**
     * Gets the date currently set in the widget
     *
     * @return the currently set date
     */
    QDate date() const { return currDate; }

    /**
     * Makes this item have focus, enabling any item-specific commands; in
     * addition, if the given event was caused by a user action, notifies the
     * Java platform of the change in focus.
     *
     * @param event device event corresponding to the change in focus.
     */
    void focusInEvent(QFocusEvent *event);

    /**
     * If in undefined state and min or hour got a new value,
     * than switch from undefined state to a defined default state
     */
    void setToDefault();

public slots:
    /**
     * Slot used to set date as picked by the user from the editor
     *
     * @param y year value to be set
     * @param m month value to be set
     * @param d day value to be set
     */
    void setDate(int y, int m, int d);

    /**
     * Slot used to set date as picked by the user from the editor
     *
     * @param d date value to be set
     */
    void setDate(QDate d);

private slots:
    /** Service slot for pick date event */
    void pickDate();

    /** Service slot for hiding the date editor */
    void gotHide();

private:
    /**
     * Date display mode, long or short
     * ("www mm dd, yyyy" or "mm/dd/yy") */
    bool longFormat;

    /** Current date set on this widget */
    QDate currDate;

    /** A pointer to the parent */
    DateTimeEditor *dte;

    /** Current picker for this widget*/
    DatePicker *picker;
};

/** Parent container for all date/time editors components */
class DateTimeEditor : public QWidget {
    Q_OBJECT
public:
    /**
     * Constructs DateTimeEditor widget.
     *
     * @param parent a pointer to the parent
     * @param dmode date and/or time input mode selection.
     *   1 or 3 (binary mask 01) = date editor is on
     *   2 or 3 (binary mask 10) = time editor is on
     * @param initialTime UTC time to initialize widget instance with
     */
    DateTimeEditor(QWidget *parent=0, int dmode=0, long initialTime=0);

    /**
     * Changes content and displayed components to newTime
     *
     * @param newTime UTC time to be set
     */
    void resetDateTime( long newTime );

    /**
     * Gets date and time in UTC format currently set in the widget
     *
     * @return currently set date and time in UTC format
     */
    long getTime();

    /**
     * Sets date and/or time input mode selection.
     * 1 or 3 (binary mask 01) = date editor is on
     * 2 or 3 (binary mask 10) = time editor is on
     */
    void setInputMode(int mode);

    /**
     * Makes this item have focus, enabling any item-specific commands; in
     * addition, if the given event was caused by a user action, notifies the
     * Java platform of the change in focus.
     *
     * @param event device event corresponding to the change in focus.
     */
    void focusInEvent(QFocusEvent *event);

    /**
     * The label of the date part of the editor.
     * Displays the date currently set on the widget.
     */
    QLabel *dateLabel;

    /**
     * Gets height of the widget depending on input mode
     *
     * @returns the minimum height for the body
     */
    int getHeight() {
      return minH;
    }

    /** Time editor member */
    SetTime *time;

    /** Date editor member */
    DateEditor *date;

protected:

#define _DRAW_BORDER 1

#if _DRAW_BORDER
    /**
     * Repaint current screen upon system notification:
     * drawing border around the editor
     *
     * @param e paint request event
     */
    virtual void paintEvent(QPaintEvent *e);
#endif

    /** 1=DATE, 2=TIME, 3=DATE_TIME */
    int mode;

    /** Currently set time and date */
    long thisTime;

 private:
    /** Holds minimum height for the body */
    int minH;

};

/**
 * The dialog with calendar visualization
 * to request user for a date selection
 */
class DatePicker : public QDialog {
        Q_OBJECT
public:
    /** Constructs date picker dialog
     *
     * @param day the date to be selected on dialog opening
     * @param pos the postition of the left upper corner of the dialog,
     *   by default the dialog will be centered
     * @param parent a pointer to the parent
     * @param name name of the dialog instance
     */
    DatePicker(const QDate &day = QDate::currentDate(),
        QPoint pos = QPoint(), DateEditor *parent = 0, const char *name = 0);

    /**
     * Gets the date currently selected in the calendar dialog
     *
     * @return current date selected in the dialog
     */
    const QDate &getDay() const { return currentDay; }

    /**
     * Sets the date to be selected in the calendar dialog
     *
     * @param day date to be set in the calendar dialog
     */
    void setDay(const QDate &day);

    /**
     * Estimates size of the calendar dialog window
     * regarding the number of columns and rows in the grid
     *
     * @return size of the dialog window
     */
    QSize calculateSize() const;

protected:
    /**
     * Process mouse click event
     *
     * @param event mouse event
     */
    void mousePressEvent(QMouseEvent *event);

    /**
     * Process mouse double click event
     *
     * @param event mouse event
     */
    void mouseDoubleClickEvent(QMouseEvent *);

    /**
     * Process key press event
     *
     * @param event key event
     */
    void keyPressEvent(QKeyEvent *event);

    /**
     * Process paint request event
     *
     * @param event paint event
     */
    void paintEvent(QPaintEvent *event);

private:
    /** Number of lines and columns in the calendar grid */
    enum { LINES = 6, COLUMNS = 7 };

    /** The font to render day numbers */
    QFont baseFont;
    /** The font to render all captions */
    QFont captionFont;
    /** The date currently selected in calendar */
    QDate currentDay;
    /** Start date provided by dialog creator */
    const QDate startDay;
    /** A pointer to the parent */
    DateEditor *de;

    /** Set current date to parent DateEditor */
    void acceptDay();
};

#endif /* _LFPPORT_QTE_DATEEDITOR_H_ */
