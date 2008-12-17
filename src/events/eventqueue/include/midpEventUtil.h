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
#ifndef _MIDP_LCDUI_UTIL_H_
#define _MIDP_LCDUI_UTIL_H_

/**
 * @file
 * @ingroup events_queue
 *
 * @brief Utility Functions shared between the 
 * platform widgets and java widgets build.
 */

#include <midpEvents.h>
#include <midp_foreground_id.h>

#ifdef __cplusplus
extern "C" {
#endif


/*Add a native MIDP event to Java event queue of the AMS Isolate.
 * This is done asynchronously.
 *
 * @param evt The event to store
 */
void midpStoreEventAndSignalAms(MidpEvent evt);


/**
 * Add a native MIDP event to Java event queue.
 * This is done asynchronously.
 *
 * @param evt The event to store
 */
void midpStoreEventAndSignalForeground(MidpEvent evt);

/**
 * Determines if the Display object with the passed in 
 * <code>displayId</code> has foreground.
 * Since displayId is unique across Isolates, the given displayId will be simply
 * checked against current foreground display id.
 *
 * @param displayId The display ID of the checked Display object
 * @return true if Display object with <code>displayId</code> has 
 *         foreground, false - otherwise.
 */


#if ENABLE_MULTIPLE_DISPLAYS
#define midpHasForeground(displayId) (isForegroundDisplay(displayId))
#else
#define midpHasForeground(displayId) (displayId == gForegroundDisplayId)
#endif /* ENABLE_MULTIPLE_DISPLAYS */
#ifdef __cplusplus
}
#endif

#endif /* _MIDP_LCDUI_UTIL_H_ */
