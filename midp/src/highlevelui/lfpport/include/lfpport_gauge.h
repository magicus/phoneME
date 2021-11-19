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

#ifndef _LFPPORT_GAUGE_H_
#define _LFPPORT_GAUGE_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Gauge-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a gauge's native peer, but does not display it.
 * When this function returns successfully, the fields *gaugePtr will be
 * set.
 *
 * @param gaugePtr pointer to the gauge's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable 
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param interactive true if the gauge should be created as an 
 *                    interactive type or false otherwise.
 * @param maxValue the initial maximum value of the gauge.
 * @param initialValue the initial value of the gauge.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_create(MidpItem* gaugePtr, MidpDisplayable* ownerPtr,
			       const pcsl_string* label, int layout,
			       jboolean interactive,
			       int maxValue, int initialValue);
  
/**
 * Changes the gauge to have the given current value and maximum values.
 *
 * @param gaugePtr pointer to the gauge's MidpItem structure.
 * @param value the current value to be set on the gauge.
 * @param maxValue the maximum value to be set on the gauge.
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_set_value(MidpItem* gaugePtr, int value, int maxValue);

/**
 * Gets the native peer's current value for the gauge.
 *
 * @param value pointer that will be set to the gauge's current value. This
 * function sets value's value.
 * @param gaugePtr pointer to the gauge's MidpItem structure.
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_get_value(int* value, MidpItem* gaugePtr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_GAUGE_H_ */
