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

#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"

#include <qpainter.h>
#include <qlayout.h>
#include <qbutton.h>

#include <lfpport_form.h>
#include "lfpport_qte_datefield.h"

// ************************************************************
// ************************************************************
// *** Class DateField implementation
// ************************************************************
// ************************************************************

/**
 * Construct a TextField native peer with label and body.
 */
DateField::DateField(QWidget *parent, 
		     const QString &labelSt,
		     int layout,
		     long time,
		     int mode, 
		     const QString &)
	: Item(parent, labelSt, layout)
{
  
  setFocusPolicy(StrongFocus);

  dateEdit = new DateTimeEditor(this, mode, time);

  dateEdit->setFocusPolicy(StrongFocus);

  setFocusProxy(dateEdit);

}

/**
 * Construct a TextField native peer with label and body.
 */
DateField::~DateField() {
  delete dateEdit;
}

/**
 * makes datefield accept focus, and notify the java peer of the focus
 * change, to enable item specific commands.
 */
void 
DateField::focusInEvent(QFocusEvent *event) 
{
  TRACE_DF(DateField::focusInEvent);

  // Notify Java if this is caused by user action
  if (event->reason() != QFocusEvent::Other) {
    MidpFormFocusChanged(this);
  }
  
  // Continue with focus activation
  QWidget::focusInEvent(event);
  
}

/** Implement virtual function (defined in lfpport_qte_item.h) */

void DateField::bodyResize(int w, int h)
{
  TRACE_DF(DateField::setSize);

  REPORT_INFO2(LC_HIGHUI, "\tw & h = %d & %d\n", w, h);

  QSize size = dateEdit->sizeHint();
  if (size.width() < w) {
      w = size.width();
  }

  dateEdit->resize(w, h);
}

void DateField::bodyRelocate(int x, int y) {
  TRACE_DF(DateField::bodyRelocate);

  REPORT_INFO2(LC_HIGHUI, "\tx,y = %d,%d\n", x, y);
  
  dateEdit->move(x, y);
}

int DateField::bodyHeightForWidth(int *takenWidth, int) {
  TRACE_DF(DateField::bodyHeightForWidth);

  QSize size = dateEdit->sizeHint();
  *takenWidth = size.width();

  return dateEdit->getHeight();
}

int DateField::bodyWidthForHeight(int *takenHeight, int) {
  TRACE_DF(DateField::bodyWidthForHeight);

  QSize size = dateEdit->sizeHint();
  *takenHeight = size.height();

  return size.width();
}


/////////////////////////////API/////////////////////////////////////////

/** 
 * Implement porting API function 
 * (routes from midp_datefield.c to lfpport_qte_datefield.cpp)
 */
MidpError
DateField::setDate(long ptime)
{

  if (dateEdit != NULL) {
    dateEdit->resetDateTime( ptime );
  }

  return KNI_OK;
}
    
/** 
 * Implement porting API function 
 * (routes from midp_datefield.c to lfpport_qte_datefield.cpp)
 */
MidpError
DateField::getDate(long &ptime)
  //DateField::getDate(uint &ptime)
{
  ptime = dateEdit->getTime();
  return KNI_OK;
}

/** 
 * Implement porting API function 
 * (routes from midp_datefield.c to lfpport_qte_datefield.cpp)
 */
MidpError
DateField::setInputMode(int dmode)
{
  dateEdit->setInputMode(dmode);
  return KNI_OK;
}


// ************************************************************
// * accessed from lfp_datefield.c
// ************************************************************

/**
 * Create the native peer of a DateField.
 * Upon successful return, *datefieldPtr should be filled in properly.
 * Param time is number of milliseconds since the standard base time known as
 * "the epoch", namely January 1, 1970, 00:00:00 GMT.
 * (defined in midp_datefield.h)
 */
extern "C" MidpError
lfpport_datefield_create(MidpItem* datefieldPtr,
			 MidpDisplayable* formPtr,
			 const pcsl_string* label, int layout,
			 int input_mode, long time, const pcsl_string* timezoneID)
{
  QString qlabel, qtzone;

  TRACE_DF(lfpport_datefield_Create);

  REPORT_INFO1(LC_HIGHUI, "\tinput_mode=%d\n", input_mode);

  pcsl_string2QString(*label, qlabel);
  pcsl_string2QString(*timezoneID, qtzone);

  // Fill in MidpItem structure
  datefieldPtr->widgetPtr = 
    new DateField((formPtr == INVALID_NATIVE_ID ? 
		   0 : (QWidget *)formPtr->frame.widgetPtr),
		  qlabel,
		  layout,
		  time,
		  input_mode,
		  qtzone);

  initItemPtr(datefieldPtr, formPtr);

  return KNI_OK;
}

/**
 * Notifies native peer of a date change in the corresponding DateField.
 * Param time is number of milliseconds since the standard base time known as
 * "the epoch", namely January 1, 1970, 00:00:00 GMT.
 * (defined in midp_datefield.h)
 */
extern "C" MidpError 
lfpport_datefield_set_date(MidpItem* datefieldPtr, long time)
{

  MidpError err = ((DateField *)datefieldPtr->widgetPtr)->setDate(time);

  return err;
}

/**
 * Query native peer for current user selected date.
 * Return time is number of milliseconds since the standard base time known as
 * "the epoch", namely January 1, 1970, 00:00:00 GMT.
 * (defined in midp_datefield.h)
 */
extern "C" MidpError 
lfpport_datefield_get_date(long* time, MidpItem* datefieldPtr) 
{
  long tmp;

  MidpError err = ((DateField *)datefieldPtr->widgetPtr)->getDate(tmp);

  *time = tmp;

  return err;
}

/**
 * Notifies native peer of a new input mode set in the corresponding 
 * DateField.
 * @param mode the new input mode set in the DateField.
 * (defined in midp_datefield.h)
 */
extern "C" MidpError 
lfpport_datefield_set_input_mode(MidpItem* datefieldPtr, int mode)
{

  MidpError err = ((DateField *)datefieldPtr->widgetPtr)->setInputMode(mode);

  return err;

}

//midp_datefield_qte
