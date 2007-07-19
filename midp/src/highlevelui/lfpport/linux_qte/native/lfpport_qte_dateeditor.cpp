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

#include <qpainter.h>
#include <qevent.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qcombobox.h>

#include <sys/time.h>

#include <stdlib.h>
#include "lfpport_qte_util.h"
#include "lfpport_qte_patched.h"

#include <lfpport_form.h>
#include "lfpport_qte_dateeditor.h"
#include <moc_lfpport_qte_dateeditor.cpp>
#include <qteapp_export.h>

#include <midpStorage.h>
#include <midp_logging.h>

#define TIME_WIDGET_HEIGHT 25
#define DATE_WIDGET_HEIGHT 28
#define DATE_FORMAT_HINT   "www mm dd, yyyy"

static const int ValueAM = 0;
static const int ValuePM = 1;

/**
 * Constructs DateEditor widget
 *
 * @param longDate when TRUE selects long format for
 *   date string, otherwise short format will be used
 * @param parent reference to the parent container
 */
DateEditor::DateEditor(bool longDate, DateTimeEditor *parent) :
    QPushButton(parent) {

    TRACE_DE(  DateEditor::DateEditor..);

    QString qiconPath;
    pcsl_string* iconPath = (pcsl_string*)
        storage_get_config_root(INTERNAL_STORAGE_ID);
    pcsl_string2QString(*iconPath, qiconPath);
    QPixmap qicon(qiconPath + "calendar.png");

    setPixmap(qicon);

    setFixedWidth(qicon.width() + 6);
    setFixedHeight(qicon.height() + 6);

    dte = parent;

    setFocusPolicy(QWidget::StrongFocus);

    longFormat = longDate;
    currDate = QDate::currentDate();

    picker = NULL;

    connect(this,SIGNAL(clicked()),this,SLOT(pickDate()));

    TRACE_DE(..DateEditor::DateEditor);
}

/**
 * Makes this item have focus, enabling any item-specific commands; in
 * addition, if the given event was caused by a user action, notifies the
 * Java platform of the change in focus.
 *
 * @param event device event corresponding to the change in focus.
 */
void DateEditor::focusInEvent(QFocusEvent *event) {

    TRACE_DE(DateEditor::focusInEvent);

    // Notify Java if this is caused by user action
    if (event->reason() != QFocusEvent::Other) {
        MidpFormFocusChanged(parentWidget()->parentWidget());
    }

    QPushButton::focusInEvent(event);
}

/**
 * If in undefined state and min or hour got a new value,
 * than switch from undefined state to a defined default state
 */
void DateEditor::setToDefault() {
   dte->dateLabel->setText(longFormat ?
                           TimeUtils::longDateString(currDate) :
                           TimeUtils::shortDateString(currDate) );
}

/** The user has picked a date */
void DateEditor::pickDate() {
    TRACE_DE(  DateEditor::pickDate..);
    if (picker == NULL) {
        picker = new DatePicker(currDate, QPoint(), this);
    }
    picker->show();
    TRACE_DE(..DateEditor::pickDate);
}

/** Service slot for hiding the date editor */
void DateEditor::gotHide() {
    TRACE_DE(  DateEditor::gotHide..);
    // we have to redo the button...
    setDown( false );
    TRACE_DE(..DateEditor::gotHide);
}

/**
 * Slot used to set date as picked by the user from the editor
 *
 * @param y year value to be set
 * @param m month value to be set
 * @param d day value to be set
 */
void DateEditor::setDate(int y, int m, int d) {
    TRACE_DE(  DateEditor::setDate..);
    setDate( QDate( y,m,d) );
    TRACE_DE(..DateEditor::setDate);
}

/**
 * Slot used to set date as picked by the user from the editor
 *
 * @param d date value to be set
 */
void DateEditor::setDate(QDate d) {
    TRACE_DE(  DateEditor::setDate..);
    dte->time->stackCounter++;
    currDate = d;
    dte->dateLabel->setText(longFormat ?
        TimeUtils::longDateString(d) :
        TimeUtils::shortDateString(d) );

    // if was undefined state - set initial values to hour and minute
    if (dte->time->undefinedState) {
        dte->time->setUndefinedState(FALSE);
    }

    if (dte->time->needPeerNotification && dte->time->stackCounter == 1) {
        MidpFormItemPeerStateChanged(dte->parentWidget(), 1);
    }

    dte->time->stackCounter--;
    TRACE_DE(..DateEditor::setDate);
}

/**
 * Constructor of time editor widget
 *
 * @param parent parent widget
 * @param initialTime time to show on widget creation
 */
SetTime::SetTime(DateTimeEditor *parent, long initialTime ) :
    QFrame( parent ) {

    TRACE_DE(  SetTime::SetTime..);

    // initial state of peer notification is TRUE (on).
    // when value is set programmatically the flaf is set to false temporarily
    needPeerNotification = TRUE;
    stackCounter = 0;
    dte = parent;

    setFocusPolicy(QWidget::StrongFocus);

    use12hourTime = FALSE;

    QHBoxLayout *hb2 = new QHBoxLayout( this );
    hb2->setSpacing( 3 );

    QLabel *l = new QLabel( tr("(HH:MM)"), this );
    hb2->addWidget(l);

    sbHour = new QSpinBox( this );
    sbHour->setFocusPolicy(QWidget::StrongFocus);

    sbHour->setFixedWidth(40);
    if(use12hourTime) {
        sbHour->setMinValue(1);
        sbHour->setMaxValue(12);
        int show_hour = hour;
        if (hour > 12) {
            show_hour -= 12;
        }
        if (show_hour == 0) {
            show_hour = 12;
        }

        sbHour->setValue(show_hour);
    } else {
        sbHour->setMinValue(0);
        sbHour->setMaxValue(23);
    }
    sbHour->setWrapping(TRUE);
    connect( sbHour, SIGNAL(valueChanged(int)),
	    this, SLOT(hourChanged(int)) );

    hb2->addWidget(sbHour);

    // minutes
    sbMin = new QSpinBox( this );
    sbMin->setFocusPolicy(QWidget::StrongFocus);

    // sbMin->setBackgroundColor(Qt::white);
    sbMin->setMinValue(0);
    sbMin->setMaxValue(59);
    sbMin->setWrapping(TRUE);
    sbMin->setFixedWidth(40);

    connect(sbMin, SIGNAL(valueChanged(int)),
       this, SLOT(minuteChanged(int)));

    hb2->addWidget(sbMin);

    if (use12hourTime) {

        ampm = new QComboBox( this );
        // ampm->setBackgroundColor(Qt::white);
        ampm->insertItem(tr("AM"), ValueAM);
        ampm->insertItem(tr("PM"), ValuePM);
        ampm->setFixedWidth(40);
        connect(ampm, SIGNAL(activated(int)),
	        this, SLOT(checkedPM(int)));

        hb2->addWidget(ampm);
    }

    if (initialTime == 0) {
        undefinedState = FALSE;
        setUndefinedState(TRUE);
    } else {
        undefinedState = FALSE;

        QDateTime *init = new QDateTime();
        init->setTime_t((uint)(initialTime));
        hour = init->time().hour();
        minute = init->time().minute();

        sbHour->setValue(hour);
        sbMin->setValue(minute);
    }

    TRACE_DE(..SetTime::SetTime);
}

/**
 * Makes this item have focus, enabling any item-specific commands; in
 * addition, if the given event was caused by a user action, notifies
 * the Java platform of the change in focus.
 *
 * @param event device event corresponding to the change in focus.
 */
void SetTime::focusInEvent(QFocusEvent *event) {
    TRACE_DE(SetTime::focusInEvent);

    // show cursor if no cursor is shown
    if (!sbHour->hasFocus() && !sbMin->hasFocus()) {
        sbHour->setFocus();
    }

    // Notify Java if this is caused by user action
    MidpFormFocusChanged(parentWidget()->parentWidget());

    QWidget::focusInEvent(event);
}

/**
 * Undefined state appears as "HH:MM" and no date selected.
 * This method allows to switch from defined to undefined appearance.
 *
 * @param turnOn true if the state should be set to undefined mode
 */
void SetTime::setUndefinedState(bool turnOn) {

    // if already at the same state - return
    if (! (turnOn ^ undefinedState)) {
        return;
    }

    TRACE_DE(  SetTime::setUndefinedState..);

    if (turnOn && (undefinedState == FALSE)) {
        undefinedState = TRUE;
        sbHour->setSpecialValueText("HH");
        sbMin->setSpecialValueText("MM");

        sbHour->setMinValue(sbHour->minValue() - 1);
        sbMin->setMinValue(sbMin->minValue() - 1);

        sbHour->setValue(sbHour->minValue());
        sbMin->setValue(sbMin->minValue());

    } else if (!turnOn && (undefinedState == TRUE)) {
        undefinedState = FALSE;
        sbHour->setSpecialValueText("");
        sbMin->setSpecialValueText("");

        sbHour->setMinValue(sbHour->minValue() + 1);
        sbMin->setMinValue(sbMin->minValue() + 1);

        sbHour->setValue(sbHour->minValue());
        sbMin->setValue(sbMin->minValue());

    }
    TRACE_DE(..SetTime::setUndefinedState);
}

/**
 * Gets time set in time editor
 *
 * @return currently set QTime (hour and minute)
 */
QTime SetTime::time() const {
    return QTime(hour, minute);
}

/**
 * Uses this method to set value of this date widget.
 *
 * @param hour hour to be set
 * @param min minute to be set
 */
void SetTime::setTime(int ihour, int imin) {

    TRACE_DE(  SetTime::setTime..);
    hour = ihour;
    minute = imin;
    sbHour->setValue(hour);
    sbMin->setValue(minute);

    TRACE_DE(..SetTime::setTime);
}

/**
 * This method receives hour changed event.
 *
 * @param value new hour
 */
void SetTime::hourChanged(int value) {

    TRACE_DE(  SetTime::hourChanged..);
    stackCounter++;

    // sets parent's focus
    QWidget::setFocus();

    if (undefinedState) {
        if (value != sbHour->minValue()) {
            setUndefinedState(FALSE);
            sbHour->setValue(value);
            dte->date->setToDefault();
        } else {
            stackCounter--;
            return;
        }
    }

    if(use12hourTime) {
        int realhour = value;
        if (realhour == 12)
            realhour = 0;
        if (ampm->currentItem() == ValuePM )
            realhour += 12;
        hour = realhour;
    } else {
        hour = value;
    }

    sbHour->setFocus();

    if (needPeerNotification && stackCounter == 1) {
        // 2 = hint that hour has changed
        MidpFormItemPeerStateChanged(parentWidget()->parentWidget(), 2);
    }

    stackCounter--;
    TRACE_DE(..SetTime::hourChanged);
}

/**
 * This method receives minute changed event
 *
 * @param value new minute
 */
void SetTime::minuteChanged(int value) {

    TRACE_DE(  SetTime::minuteChanged..);
    stackCounter++;

    QWidget::setFocus();

    if (undefinedState) {
        if (value != sbMin->minValue()) {
            setUndefinedState(FALSE);
            sbMin->setValue(value);
            dte->date->setToDefault();
        } else {
            stackCounter--;
            return;
        }
    }

    minute = value;
    sbMin->setFocus();

    if (needPeerNotification && stackCounter == 1) {
        // 3 = hint that minutes has changed
        MidpFormItemPeerStateChanged(parentWidget()->parentWidget(), 3);
    }

    stackCounter--;
    TRACE_DE(..SetTime::minuteChanged);
}

/**
 * Changes display and edit mode to 12 or 24 hour.
 *
 * @param on 0 if 24 hour mode, otherwise 12 hour mode
 */
void SetTime::show12hourTime(int on) {

    TRACE_DE(  SetTime::show12hourTime..);

    use12hourTime = on;
    ampm->setEnabled(on);

    int show_hour = hour;
    if ( on ) {
	    /* this might change the value of hour */
	    sbHour->setMinValue(1);
	    sbHour->setMaxValue(12);

	    /* so use one we saved earlier */
	    if (show_hour >= 12) {
	        show_hour -= 12;
	        ampm->setCurrentItem( ValuePM );
	    } else {
	        ampm->setCurrentItem( ValueAM );
	    }
	    if (show_hour == 0)
	        show_hour = 12;

    } else {
	    sbHour->setMinValue( 0 );
	    sbHour->setMaxValue( 23 );
    }

    sbHour->setValue( show_hour );
    TRACE_DE(..SetTime::show12hourTime);
}

/**
 * This method receives pm/am changed event
 *
 * @param index the position of the selected item
 */
void SetTime::checkedPM(int c) {

    TRACE_DE(  SetTime::checkedPM..);
    int show_hour = sbHour->value();
    if (show_hour == 12)
	    show_hour = 0;

    if (c == ValuePM )
	    show_hour += 12;

    hour = show_hour;
    TRACE_DE(..SetTime::checkedPM);
}

/**
 * Constructs DateTimeEditor widget.
 *
 * @param parent a pointer to the parent
 * @param dmode date and/or time input mode selection.
 *   1 or 3 (binary mask 01) = date editor is on
 *   2 or 3 (binary mask 10) = time editor is on
 * @param initialTime UTC time to initialize widget instance with
 */
DateTimeEditor::DateTimeEditor(QWidget *parent, int dmode, long initialTime) :
    QWidget( parent) {

    TRACE_DE(  DateTimeEditor::DateTimeEditor..);

    // allows the item to be selected by clicking on the label
    setFocusPolicy(QWidget::StrongFocus);

    QVBoxLayout *vb = new QVBoxLayout( this, 5 );

    minH = 5;
    mode = dmode;
    thisTime = initialTime;

    // if (mode == 2 || mode == 3)
    // needs time widget

    time = new SetTime( this, initialTime );
    vb->addWidget( time );

    if (thisTime == 0) {
        time->setUndefinedState(TRUE);
    } else {
        // setup the time
        QDateTime *init = new QDateTime();
        init->setTime_t( (uint)(initialTime) );
        time->setTime(init->time().hour(),
              init->time().minute());
    }

    // hide if time widget is not needed
    if (mode != 2 && mode != 3) {
        time->hide();
    } else {
        minH += TIME_WIDGET_HEIGHT;
    }

    // if (mode == 1 || mode == 3)
    // has date widget
    QHBoxLayout *db = new QHBoxLayout( vb );
    dateLabel = new QLabel(DATE_FORMAT_HINT, this);
    QSize labelSize = dateLabel->sizeHint();
    dateLabel->setFixedWidth(labelSize.width());

    db->addWidget(dateLabel);

    date = new DateEditor( TRUE, this );

    if (thisTime != 0) {
        QDateTime *init = new QDateTime();
        init->setTime_t( (uint)(initialTime) );
        date->setDate(init->date().year(),
              init->date().month(),
              init->date().day());
    }

    db->addWidget(date);
    db->addStretch(1);

    // hide if date widget is not needed
    if (mode != 1 && mode != 3) {
        dateLabel->hide();
        date->hide();
    } else {
        minH += DATE_WIDGET_HEIGHT;
    }

    TRACE_DE(..DateTimeEditor::DateTimeEditor);
}

/**
 * Makes this item have focus, enabling any item-specific commands; in
 * addition, if the given event was caused by a user action, notifies the
 * Java platform of the change in focus.
 *
 * @param event device event corresponding to the change in focus.
 */
void DateTimeEditor::focusInEvent(QFocusEvent *event) {

    TRACE_DE(DateTimeEditor::focusInEvent);

    // Notify Java if this is caused by user action
    if (event->reason() != QFocusEvent::Other) {
        MidpFormFocusChanged(parentWidget());
    }

    QWidget::focusInEvent(event);

    REPORT_INFO1(LC_HIGHUI, "DateTimeEditor::focusInEvent -- mode=%d\n", mode);

    if (mode == 2 || mode == 3) {
        // time widget is visible
        time->sbHour->setFocus();
    } else {
        // only date is visible
        date->setFocus();
    }
}

#if _DRAW_BORDER
/**
 * Repaint current screen upon system notification:
 * drawing border around the editor
 *
 * @param e paint request event
 */
void DateTimeEditor::paintEvent(QPaintEvent *) {
    QPainter painter( this );

    // Beveled border effect
    painter.setPen(QColor(255,250,240));
    painter.drawRect(1,1,QWidget::width()-1,QWidget::height()-1);
    painter.setPen(QColor(205,200,190));
    painter.drawRect(0,0,QWidget::width()-1,QWidget::height()-1);
}
#endif

/**
 * Sets date and/or time input mode selection.
 * 1 or 3 (binary mask 01) = date editor is on
 * 2 or 3 (binary mask 10) = time editor is on
 */
void DateTimeEditor::setInputMode(int dmode) {

    TRACE_DE(  DateTimeEditor::setInputMode..);

    minH = 5;
    mode = dmode;

    if (mode != 2 && mode != 3) {
        // hide if time widget is not needed
        time->hide();

    } else {
        minH += TIME_WIDGET_HEIGHT;
        time->show();
    }

    if (mode != 1 && mode != 3) {
        // hide if date widget is not needed
        dateLabel->hide();
        date->hide();
    } else {
        minH += DATE_WIDGET_HEIGHT;
        dateLabel->show();
        date->show();
    }

    REPORT_INFO1(LC_HIGHUI,
          "\n\nDateTimeEditor::setInputMode -- minH is set to: %d\n",
           minH);
    REPORT_INFO1(LC_HIGHUI,
           "DateTimeEditor::setInputMode BEFORE -- size hint = %d\n",
           minimumSizeHint().height());

    if ( minH != minimumSizeHint().height() ) {

    layout()->invalidate();
    setFixedHeight(minH);

    // the size doesn't stick if we don't activate the layout right away:
    layout()->activate();

    // alternatively -
    // A way like "call serially"...
    // QApplication::sendPostedEvents();

    REPORT_INFO1(LC_HIGHUI,
         "\n\nDateTimeEditor::setInputMode AFTER -- size hint = %d\n",
         minimumSizeHint().height());

    }

    TRACE_DE(..DateTimeEditor::setInputMode);
}


/**
 * Changes content and displayed components to newTime,
 * sets value coming from java, need to suppress peer notifications.
 *
 * @param newTime UTC time to be set
 */
void DateTimeEditor::resetDateTime(long newTime) {

    TRACE_DE(DateTimeEditor::resetDateTime..);

    // the setting comes from the MIDlet (java peer), therefore no need to notify
    // java peer.
    time->needPeerNotification = FALSE;

    thisTime = newTime;

    if (thisTime == 0) {
        time->setUndefinedState(TRUE);
    } else {
        time->setUndefinedState(FALSE);

        // setup the time
        QDateTime *init = new QDateTime();
        init->setTime_t( (uint)(thisTime) );
        time->setTime(init->time().hour(),
              init->time().minute());
    }

    if (thisTime != 0) {
        QDateTime *init = new QDateTime();
        init->setTime_t( (uint)(thisTime) );
        date->setDate(init->date().year(),
              init->date().month(),
              init->date().day());
    } else {
        dateLabel->setText(DATE_FORMAT_HINT);
    }

    // release notifications suppression
    time->needPeerNotification = TRUE;

    TRACE_DE(..DateTimeEditor::resetDateTime);
}

/**
 * Gets date and time in UTC format currently set in the widget
 *
 * @return currently set date and time in UTC format
 */
long DateTimeEditor::getTime() {
    time_t currTime = TimeUtils::toUTC(date->date());

    currTime += time->getHour() * 3600;
    currTime += time->getMinute() * 60;

    return (long)currTime;
}

/** Gets date string in long format by date object
 *
 * @param date QDate instance to get string presentation for
 * @return date string in format "www mm dd, yyyy"
 */
QString TimeUtils::longDateString(const QDate &date) {
    // format date string "www mm dd, yyyy"
    QString str = date.dayName(date.dayOfWeek()) + " " +
        date.monthName(date.month());
    str += QString().sprintf(" %02d, %04d",
        date.day(), date.year());
    return str;
}

/** Gets date string in short format by date object
 *
 * @param date QDate instance to get string presentation for
 * @return date string in format "mm/dd/yy"
 */
QString TimeUtils::shortDateString(const QDate &date) {
    // format date string "mm/dd/yy"
    QString str = QString().sprintf("%02d/%02d/%02d",
        date.month(), date.day(), (date.year() % 100));
    return str;
 }

/** Converts broken-down time into time since the Epoch
 *
 * @param date QDate instance to get UTC time for
 * @return calendar UTC time representation
 */
time_t TimeUtils::toUTC( const QDateTime &dateTime) {
    // get current local time to obtain time zone value from the system
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);

    localTime->tm_sec  = dateTime.time().second();
    localTime->tm_min  = dateTime.time().minute();
    localTime->tm_hour = dateTime.time().hour();
    localTime->tm_mday = dateTime.date().day();
    localTime->tm_mon  = dateTime.date().month() - 1;    // 0-11
    localTime->tm_year  = dateTime.date().year() - 1900; // counted from 1900
    localTime->tm_wday = -1;
    localTime->tm_yday = -1;
    localTime->tm_isdst = -1;
    return mktime(localTime);
}

/**
 * Constructs DateEditor widget
 *
 * @param longDate when TRUE selects long format for
 *   date string, otherwise short format will be used
 * @param parent reference to the parent container
 */
DatePicker::DatePicker(
    const QDate &day, QPoint pos, DateEditor *parent, const char *name) :
        QDialog(parent, name, TRUE), currentDay(day), startDay(day) {

    de = parent;

    setCaption(tr("Calendar"));
    baseFont = font();
    if (baseFont.pointSize() >= 10) {
        baseFont.setPointSize(
            baseFont.pointSize() - 2);
    }
    captionFont = baseFont;
    captionFont.setBold(true);
    setFixedSize(calculateSize());
    setFocusPolicy(StrongFocus);

    /* Set dialog position either to specified point or
     * to the center of the main window by default */
    if (pos.isNull()) {
        QWidget *main = qteapp_get_main_window();
        pos = mapFromGlobal(QPoint((main->width() - this->width()) / 2,
            (main->height() - this->height()) / 2));
    }
    move(pos);
}

/**
 * Estimates size of the calendar dialog window
 * regarding the number of columns and rows in the grid
 *
 * @return size of the dialog window
 */
QSize DatePicker::calculateSize() const {
    QFontMetrics fm(captionFont);
    /* Use Wednesday as the widest day name */
    return QSize(COLUMNS * fm.width(tr(" Wed ")),
        (LINES + 2) * fm.height() * 3/2);
}


/**
 * Sets the date to be selected in the calendar dialog
 *
 * @param day date to be set in the calendar dialog
 */
void DatePicker::setDay(const QDate &day) {
    currentDay = day;
    update();
}

#define min(x, y) ( (x)<(y)?(x):(y) )
#define max(x, y) ( (x)>(y)?(x):(y) )

/**
 * Change the month part of the date by specified months number
 *
 * @param date the original date to change month for
 * @param nmonths number of months to shift the date, can be negative
 * @return the original date with month part changed by nmonths
 */
static QDate addMonths(const QDate &date, int nmonths) {
    int day = date.day();
    int month = date.month();
    int year = date.year();
    int raw = month-1 + nmonths;

    year += raw / 12;
    if (raw % 12 < 0) {
        year--;
    }
    year = max(year, 1752);
    year = min(year, 8000);

    month = raw % 12 + 1;
    if (month <= 0) {
        month += 12;
    }
    int daysInMonth = QDate(year, month, 1).daysInMonth();
    day = min(day, daysInMonth);

    return QDate(year, month, day);
}

/** Change the year part of the date by specified years number
 *
 * @param date the original date to change year for
 * @param nyears number of years to shift the date, can be negative
 * @return the original date with year part changed by nyears
 */
static QDate addYears(const QDate &date, int nyears) {
    int day = date.day();
    int month = date.month();
    int year = date.year();

    year += nyears;
    year = max(year, 1752);
    year = min(year, 8000);

    // Handle leap year variations
    int daysInMonth = QDate(year, month, 1).daysInMonth();
    day = min(day, daysInMonth);

    return QDate(year, month, day);
}

#undef min
#undef max

/**
 * Set current date to parent DateEditor
 */
void DatePicker::acceptDay() {
    if (de != NULL) {
      de->setDate(currentDay);
      de->setFocus();
    }
    accept();
  }

/**
 * Process mouse click event
 *
 * @param event mouse event
 */
void DatePicker::mousePressEvent(QMouseEvent *event) {
    QDate day = currentDay;
    int doubleRowHeight = (height() / (LINES + 2)) * 2;
    /* Handle clicks in left & right parts of the top 2 rows */
    if (event->y() < doubleRowHeight) {
        if (event->x() < width() / 2) {
            day = addMonths(day, -1);
        } else {
            day = addMonths(day, 1);
        }
    } else {
        day = QDate(currentDay.year(), currentDay.month(), 1);
        int column = event->x() / (width() / COLUMNS);
        int line = (event->y() - doubleRowHeight) / ((height() - doubleRowHeight) / LINES);
        day = day.addDays(column + COLUMNS * line);
    }
    if (day != currentDay) {
        setDay(day);
    }
}

/**
 * Process mouse double click event
 *
 * @param event mouse event
 */
void DatePicker::mouseDoubleClickEvent(QMouseEvent *) {
    acceptDay();
}

/**
 * Process key press event
 *
 * @param event key event
 */
void DatePicker::keyPressEvent(QKeyEvent *event) {
    QDate day = currentDay;
    switch (event->key()) {
        case Qt::Key_Left:      day = currentDay.addDays(-1); break;
        case Qt::Key_Right:     day = currentDay.addDays(1); break;
        case Qt::Key_Up:        day = currentDay.addDays(-COLUMNS); break;
        case Qt::Key_Down:      day = currentDay.addDays(COLUMNS); break;
        case Qt::Key_PageUp:    day = addMonths(currentDay, -1); break;
        case Qt::Key_PageDown:  day = addMonths(currentDay, 1); break;
        case Qt::Key_Home:      day = addYears(currentDay, -1); break;
        case Qt::Key_End:       day = addYears(currentDay, 1); break;
        case Qt::Key_Escape:    currentDay = startDay; accept(); break;
#ifdef QT_KEYPAD_MODE
        case Qt::Key_Select:
#endif
        case Qt::Key_Space:
        case Qt::Key_Enter:
        case Qt::Key_Return:    acceptDay(); return;
        default:            QDialog::keyPressEvent(event); return;
    }
    if (day != currentDay) {
        setDay(day);
    }
}

/**
 * Process paint request event
 *
 * @param event paint event
 */
void DatePicker::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setClipRegion(event->region());

    int width = this->width();
    int height = this->height();
    int cellHeight = height / (LINES + 2);
    int cellWidth = width / COLUMNS;
    QRect rect;
    QString currentYM = QString().sprintf("<<    %04d ", currentDay.year()) +
        currentDay.monthName(currentDay.month()) + "    >>";

    painter.setFont(captionFont);
    painter.drawText(1, 1, width - 1, cellHeight - 1, AlignHCenter, currentYM, -1, &rect);
    int y = cellHeight;
    QDate day(currentDay.year(), currentDay.month(), 1);
    int i;

    for (i = 0; i < COLUMNS; ++i) {
        painter.drawText(cellWidth * i + 1, y, cellWidth, cellHeight - 1,
             AlignHCenter, day.dayName(day.dayOfWeek()), -1, &rect);
        day = day.addDays(1);
    }
    day = day.addDays(-COLUMNS);
    y += cellHeight;

    painter.setFont(baseFont);
    for (int j = 0; j < LINES; ++j)
        for (i = 0; i < COLUMNS; ++i) {
            QColor color = day == currentDay ? colorGroup().mid() : colorGroup().light();
            painter.fillRect(cellWidth * i + 1, cellHeight * j + y + 1, cellWidth - 1, cellHeight - 1, color);
            painter.drawText(cellWidth * i + 2, cellHeight * j + y + 2, cellWidth,
                cellHeight, AlignTop | AlignHCenter, QString().sprintf("%d", day.day()));
            day = day.addDays(1);
        }
}
