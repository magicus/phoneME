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
 * Qt port of the gauge.
 */

#ifndef _LFPPORT_QTE_GAUGE_H_
#define _LFPPORT_QTE_GAUGE_H_

#include <lfpport_gauge.h>
#include "lfpport_qte_item.h"

#include <qprogressbar.h>
#include <qslider.h>
#include <qtimer.h>
#include <qpainter.h>

/**
 * Constant used when no maximum value for the gauge is known. Its
 * value is specified by the <i>MIDP Specification</i>.
 */
#define INDEFINITE	     -1

/**
 * State that, when used with a non-interactive gauge of indefinite
 * range, indicates that no work is in progress. The constant's value
 * is specified by the <i>MIDP Specification</i>.
 */
#define CONTINUOUS_IDLE	      0

/**
 * State that, when used with a non-interactive gauge of indefinite
 * range, indicates that no work is in progress. The constant's value
 * is specified by the <i>MIDP Specification</i>.
 */
#define INCREMENTAL_IDLE      1

/**
 * State that, when used with a non-interactive gauge of indefinite
 * range, indicates that work is in progress. The constant's value is
 * specified by the <i>MIDP Specification</i>.
 */
#define CONTINUOUS_RUNNING    2

/**
 * State that, when used with a non-interactive gauge of indefinite
 * range, indicates that work is in progress. The constant's value is
 * specified by the <i>MIDP Specification</i>.
 */
#define INCREMENTAL_UPDATING  3


/**
 * Native widget for a Gauge.
 */
class Gauge : public Item {

   Q_OBJECT

    public :
      /**
       * gauge's constructor.
       *
       * @param parent owner screen's widget
       * @param label label text
       * @param layout layout directive associated with this gauge
       */
      Gauge(QWidget *parent, const QString &label, int layout);
    
      /**
       * gauge's destructor.
       */
      virtual ~Gauge();
    
    /**
     * Sets this gauge's value with the given value, and maximum value with
     * the given maximum.
     *
     * @param value new current value
     * @param maxValue new maximum value
     * 
     * @return an indication of success or the reason for failure
     */
    virtual MidpError setValue(int value, int maxValue) = 0;

    /**
     * Returns this gauge's value in the given pointer.
     *
     * @param value pointer to the current value of the gauge. This function
     *        sets value's value.
     * 
     * @return an indication of success or the reason for failure
     */
    virtual MidpError getValue(int *value) = 0;
};

/**
 * Body widget of non-interactive Gauge.
 * Extend QProgressBar to control percentage indicator and 
 * handle focus change.
 */
class MyProgressBar : public QProgressBar {
    /**
     * if label is on or off 
     */
    bool labelOn;
    bool labelChanged;

 public:
    /**
     * MyProgressBar's constructor.
     *
     * @param maxValue maximum value
     * @param parent Gauge's widget
     * @param name Qt widget name
     */
    MyProgressBar(int maxValue, QWidget *parent=0, const char* name=0);

    /**
     * MyProgressBar's destructor
     */
    ~MyProgressBar();

    /**
     * Shows/hides percentage indicator
     *
     * @param on if label should be switched on or off
     */ 
     void showLabel(bool on);

protected:
    /**
     * Called to generate the text displayed. 
     * Overrided to hide text when gauge is in continuous mode.
     */
    virtual bool setIndicator( QString & progress_str, int progress, int totalSteps);

};

/**
 * Body widget of interactive gauge.
 * Extend QSlider to handle focus change.
 */
class MySlider : public QSlider {
 protected:
   /**
    * Makes this item have focus, enabling any item-specific commands; in
    * addition, if the given event was caused by a user action, notifies the
    * Java platform of the change in focus.
    *
    * @param event pointer to the device event corresponding to the change in
    *        focus.
    */
    void focusInEvent(QFocusEvent *event);

    /**
     * Override QSlider to notify Java peer of traversal out.
     *
     * @param keyEvent key event to handle
     */
    void keyPressEvent(QKeyEvent *key);

    /**
     * Override QSlider to notify Java peer of traversal out.
     *
     * @param keyEvent key event to handle
     */
    void keyReleaseEvent(QKeyEvent *key);
    

 public:
    /**
     * Constructor.
     *
     * @param minValue minimum value
     * @param maxValue maximum value
     * @param steps page step value for QSlider
     * @param initialValue current value
     * @param parent Gauge's widget
     * @param name Qt widget name
     */
    MySlider(int minValue, int maxValue, int steps, int initialValue,
	     QWidget *parent=0, const char* name=0);

    /**
     * MySlider destructor.
     */
    ~MySlider();

};

/**
 * Main widget of non-interactive gauge.
 */
class NoninteractiveGauge : public Gauge {

    Q_OBJECT

    MyProgressBar *qNoninteractive;
    QTimer timer;
    int value;
    int maxValue;
    
protected slots:
      /**
       * Increase current value by a page step.
       */
      void progress();

 public:
    /**
     * Constructor.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this gauge
     * @param maxValue maximum value
     * @param initialValue current value
     */
    NoninteractiveGauge(QWidget *parent, const QString &label, 
			int layout, int maxValue, int initialValue);
    
    /**
     * NoninteractiveGauge destructor.
     */
    ~NoninteractiveGauge();

    /**
     * Sets this gauge's value with the given value, and maximum value with
     * the given maximum.
     *
     * @param value new current value
     * @param maxValue new maximum value
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError setValue(int value, int maxValue);

    /**
     * Returns this gauge's value in the given pointer.
     *
     * @param value pointer to the current value of the gauge. This function
     *        sets value's value.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError getValue(int *value);

    /**
     * Moves body widget so that its upper left corner is at the given x
     * and y coordinates.
     *
     * @param x the horizontal coordinate of the upper-left corner of this
     *        choice group.
     * @param y the vertical coordinate of the upper-left corner of this choice
     *        group.
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
     * Makes this item have focus, enabling any item-specific commands; in
     * addition, if the given event was caused by a user action, notifies the
     * Java platform of the change in focus.
     *
     * @param event pointer to the device event corresponding to the change in
     *        focus.
     */
    void focusInEvent(QFocusEvent *event);
    
    /**
     * Overriding, to draw the focus highlight.
     *
     * @param e the paint event
     */
    void paintEvent(QPaintEvent *e);

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
};

/**
 * Main widget for interactive gauge.
 */
class InteractiveGauge : public Gauge {
    Q_OBJECT

 private:
    /**
     * Slider that enables users to set this interactive gauge's
     * value.
     */
    MySlider *qSlider;

    /**
     * Current value of this interactive gauge.
     */
    int value;

 public:
    /**
     * InteractiveGauge constructor.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this gauge
     * @param maxValue maximum value
     * @param initialValue current value
     */
    InteractiveGauge(QWidget *parent, const QString &label, int layout,
		     int maxValue, int initialValue);

    /**
     * InteractiveGauge destructor
     */
    ~InteractiveGauge();

    /**
     * Sets this gauge's value with the given value, and maximum value with
     * the given maximum.
     *
     * @param value new current value
     * @param maxValue new maximum value
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError setValue(int value, int maxValue);

    /**
     * Returns this gauge's value in the given pointer.
     *
     * @param value pointer to the current value of the gauge. This function
     *        sets value's value.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError getValue(int *value);

    /**
     * Moves this body widget so that its upper left corner is at the given x
     * and y coordinates.
     *
     * @param x the horizontal coordinate of the upper-left corner of this
     *        choice group.
     * @param y the vertical coordinate of the upper-left corner of this choice
     *        group.
     */
    void bodyRelocate(int x, int y);

    /**
     * Resize this body widget.
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
     * Query whether the body widget can be on separated lines.
     * @return always true
     */
    bool isSingleLineBody();

public slots:
    /**
     * Sets this gauge's value with the given value.
     *
     * @param value new current value
     */
    void setValue(int newValue);

signals:

    /**
     * Signal for value change triggered by MIDlet.
     *
     * @param newValue new value
     */
    void valueChanged(int newValue);
};

#endif /* _LFPPORT_QTE_GAUGE_H_ */
