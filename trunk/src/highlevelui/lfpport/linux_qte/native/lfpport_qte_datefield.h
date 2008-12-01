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
 * Definition of the datefield class and its helper classes.
 */

#ifndef _LFPPORT_QTE_DATEFIELD_H_
#define _LFPPORT_QTE_DATEFIELD_H_

#include <lfpport_datefield.h>
#include "lfpport_qte_item.h"
#include "lfpport_qte_dateeditor.h"

#include <qwidget.h>
#include <qpainter.h>
#include <qabstractlayout.h>

// **********************************************************************
// **********************************************************************
// ** class DateField definition
// **********************************************************************
// **********************************************************************

/**
 * Qt port of DateField widget.
 */
class DateField : public Item {

  /**
   * A popup date editor
   */
  DateTimeEditor *dateEdit;


public:
  /**
   * Constructor.
   *
   * @param parent owner screen's widget
   * @param label label text
   * @param layout layout directive associated with this datefield
   * @param time EPOC time
   * @param mode date and/or time input mode, bit 0 and 1 controls date and time
   *			input separately
   * @param timezone time zone id string
   */
  DateField(QWidget *parent, const QString &label, int layout,
	    long time, int mode, const QString &timezone);

  /**
   * date field's destructor.
   */
  ~DateField();

  /**
   * Move body widget.
   * Coordinates are relative to top left corner of DateField widget (0, 0).
   *
   * @param x X coordinate
   * @param y Y coordinate
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
   * Calculate body widget's height when its width is limited to a given value.
   *
   * @param takenWidth return value of real width used
   * @param w maximum width
   * @return body widget's height
   */
  int  bodyHeightForWidth(int *takenWidth, int w);

  /**
   * Calculate body widget's width when its height is limited to a given value.
   *
   * @param takenHeight return value of real height used
   * @param h maximum height
   * @return body widget's width
   */
  int  bodyWidthForHeight(int *takenHeight, int h);

  /** 
   * Makes the date for this date field the given date.
   *
   * <p><b>Implementation Note:</b> This function routes from
   * <tt>midp_customitem.c</tt> to <tt>lfpport_qte_customitem.cpp</tt>.
   *
   * @param ptime seconds since January 1, 1970, 00:00:00 GMT (the epoch).
   * 
   * @return an indication of success or the reason for failure
   */
  MidpError setDate(long ptime);
    
  /** 
   * Returns this date field's date in the given pointer.
   *
   * <p><b>Implementation Note:</b> This function routes from
   * <tt>midp_datefield.c</tt> to <tt>lfpport_qte_datefield.cpp</tt>.
   *
   * @param ptime pointer to this date field's time. This function sets
   *        ptime's value.
   * 
   * @return an indication of success or the reason for failure
   */
  MidpError getDate(long &ptime);

  /**
   * Sets this date field's input mode.  The input modes are the values for
   * <tt>DATE</tt>, <tt>TIME</tt>, and <tt>DATE_TIME</tt> as specified in the
   * <i>MIDP Specification</i>.
   *
   * <p><b>Implementation Note:</b> This function routes from
   * <tt>midp_datefield.c</tt> to <tt>lfpport_qte_datefield.cpp</tt>.
   *
   * @param mode the new input mode.
   * 
   * @return an indication of success or the reason for failure
   */
  MidpError setInputMode(int mode);

 protected:

  /**
   * Makes this item have focus, enabling any item-specific commands; in
   * addition, if the given event was caused by a user action, notifies the
   * Java platform of the change in focus.
   *
   * @param event device event corresponding to the change in focus.
   */
  void focusInEvent(QFocusEvent *event);

};


#endif /* _LFPPORT_QTE_DATEFIELD_H_ */
