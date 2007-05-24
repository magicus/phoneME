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
 */

#ifndef _LFPPORT_DATEFIELD_H_
#define _LFPPORT_DATEFIELD_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief DateField-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a date field's native peer.
 * When this function returns successfully, *datefieldPtr will be filled.
 * 
 * @param datefieldPtr pointer to the date field's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable 
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param input_mode the mode to create the widget in, like date only, 
 *                   time only or date/time modes.
 * @param time seconds since January 1, 1970, 00:00:00 GMT (the epoch).
 * @param timezoneID identifier for the time zone to be used for 
 *                   creation of this date field.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_datefield_create(MidpItem* datefieldPtr,
				MidpDisplayable* ownerPtr,
				const pcsl_string* label, int layout,
				int input_mode,
				long time, const pcsl_string* timezoneID);

/**
 * Notifies the native peer that the date field's date has changed.
 *
 * @param datefieldPtr pointer to the date field's MidpItem structure.
 * @param time seconds since January 1, 1970, 00:00:00 GMT (the epoch).
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_datefield_set_date(MidpItem* datefieldPtr, long time);

/**
 * Gets the native peer's current date.
 *
 * @param time pointer that will be to the given date field's date in
 *        seconds since January 1, 1970, 00:00:00 GMT (the epoch).
 *        This method sets time's value.
 * @param datefieldPtr pointer to the date field's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_datefield_get_date(long* time, MidpItem* datefieldPtr);

/**
 * Notifies the native peer that the date field's input mode has changed. The
 * input modes are the values for <tt>DATE</tt>, <tt>TIME</tt>, and
 * <tt>DATE_TIME</tt> as specified in the <i>MIDP Specification</i>.
 * 
 * @param datefieldPtr pointer to the date field's MidpItem structure.
 * @param mode the new input mode.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_datefield_set_input_mode(MidpItem* datefieldPtr, int mode);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_DATEFIELD_H_ */
