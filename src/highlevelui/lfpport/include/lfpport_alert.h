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

#ifndef _LFPPORT_ALERT_H_
#define _LFPPORT_ALERT_H_

/**
 * @defgroup highui_lfpport Platform Widget Porting Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Alert-specific porting functions and data structures.
 */

#include <lfpport_component.h>
#include <lfpport_displayable.h>
#include <lfp_command.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates an alert's native peer, but does not display it.
 * The MIDP implementation should call this function in the background and
 * display the alert afterward it is given all of its content.
 * When this function returns successfully, *alertPtr will be filled.
 * Do not use this function for sound and abstract command
 * buttons.
 *
 * @param alertPtr pointer to the alert's MidpDisplayable structure.
 * @param title title string.
 * @param tickerText ticker text.
 * @param alertType alert type, as defined in MidpComponentType.
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_alert_create(MidpDisplayable* alertPtr, 
                               const pcsl_string* title,
			       const pcsl_string* tickerText,
			       MidpComponentType alertType);
  
/**
 * Sets or resets the contents of an alert that might already have content.
 * This function frees elements that become null and allocates native 
 * resources for those that become non-null.
 *
 * @param alertPtr pointer to the alert's MidpDisplayable structure.
 * @param imgPtr icon-image pointer or null if the alert will have no image.
 * @param gaugeBounds array of four integers that define the
 *            indicator gauge's geometry:
 *  <pre>
 *    [0] : x coordinate in the alert; this function sets the value.
 *    [1] : y coordinate in the alert; this function sets the value.
 *    [2] : preferred width in pixels, or null if there is no gauge.
 *          This function changes a non-null value to the granted width.
 *    [3] : preferred height in pixels, or null if there is no gauge.
 *          This function changes a non-null value to the granted height.
 *  </pre>
 * @param text the alert's message.
 * 
 * @return an indication of success or the reason for failure
 */    
MidpError lfpport_alert_set_contents(MidpDisplayable* alertPtr,
				     unsigned char* imgPtr,
				     int* gaugeBounds,
				     const pcsl_string* text);

/**
 * Tests whether the alert's contents require scrolling (that is,
 * whether the contents are more than one screen long).
 * 
 * @param needScrolling pointer that will be to true if the alert's
 *        contents require scrolling, false otherwise. This function sets
 *        needScrolling's value.
 * @param alertPtr pointer to the alert's MidpDisplayable structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_alert_need_scrolling(jboolean* needScrolling,
				       MidpDisplayable* alertPtr);

/**
 * Updates the abstract commands associated with the alert.
 *
 * @param alertPtr pointer to the alert's MidpFrame structure. 
 * @param cmds array of commands for the alert.
 * @param numCmds size of the array.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_alert_set_commands(MidpFrame* alertPtr,
				     MidpCommand* cmds, int numCmds);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_ALERT_H_ */
