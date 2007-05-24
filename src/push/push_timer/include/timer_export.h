/*
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

#ifndef _TIMER_EXPORT_H_
#define _TIMER_EXPORT_H_

/**
 * @defgroup core_timer Timer Utility External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_timer
 *
 * @brief Timer utility external and porting interface.
 * Functions defined here should be ported to provide timer functionality.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create and start a new alarm timer, returning its handle.
 *
 * @param alarmHandle Handle to the alarm to create a timer for
 * @param time Time from the current moment to activate the alarm
 * @return Implementation specific handle of the newly created timer
 */
int createTimerHandle(int alarmHandle, jlong time);

/**
 * Destroy the alarm timer given its implementation specific handle.
 *
 * @param timerHandle The implementation specific handle to the timer object
 * @return 0 if successful
 */
int destroyTimerHandle(int timerHandle);


#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _TIMER_EXPORT_H_ */
