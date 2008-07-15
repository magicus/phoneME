/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef __JAVACALL_PENEVENT_H_
#define __JAVACALL_PENEVENT_H_

/**
 * @file javacall_penevent.h
 * @ingroup Pen
 * @brief Javacall interfaces for pen event
 */

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup Pen Pen Event API 
 * @ingroup JTWI
 * @{
 */
/**
 * @defgroup NotificationPen Notification API for Pen
 * @ingroup Pen
 * @{
 */
/**
 * @enum javacall_penevent_type
 * @brief Pen notification event type
 */    
typedef enum {
    /** Pen is pressed. */
    JAVACALL_PENPRESSED = 1,
    /** Pen is released. */
    JAVACALL_PENRELEASED = 2,
    /** Pen is dragged. */
    JAVACALL_PENDRAGGED = 3,
    /** Pen is hold. */
    JAVACALL_PENHOLD = 4
} javacall_penevent_type;

/**
* The notification function to be called by platform for pen 
* press/release/drag occurences.
* The platfrom will invoke the call back in platform context for
* each pen press, pen release and pen dragg occurence
* @param x the x positoin when the pen was pressed/released/dragged
* @param y the y positoin when the pen was pressed/released/dragged
* @param type <tt>JAVACALL_PENPRESSED</tt> when pen was pressed
*             <tt>JAVACALL_PENRELEASED</tt> when pen was released
*             <tt>JAVACALL_PENDRAGGED</tt> when pen was dragged
*/
void javanotify_pen_event(int x, int y, javacall_penevent_type type);

/** @} */
/** @} */
    
#ifdef __cplusplus
}
#endif

#endif
