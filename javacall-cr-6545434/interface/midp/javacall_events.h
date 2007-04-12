/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef __JAVACALL_EVENTS_H_
#define __JAVACALL_EVENTS_H_

/**
 * @file javacall_events.h
 * @ingroup MandatoryEvents
 * @brief Javacall interfaces for events
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"


/** @defgroup MandatoryEvents Events API
 * @ingroup JTWI
 *
 * Events APIs define the functionality for:
 * 
 * - Receiving binary buffered events
 * - Sending binary buffered events
 * 
 *  @{
 */

/**
 * Waits for an incoming event message and copies it to user supplied
 * data buffer
 * @param timeTowaitInMillisec max number of seconds to wait
 *              if this value is 0, the function should poll and return
 *              immediately.
 *              if this value is -1, the function should block forever.
 * @param binaryBuffer user-supplied buffer to copy event to
 * @param binaryBufferMaxLen maximum buffer size that an event can be
 *              copied to.
 *              If an event exceeds the binaryBufferMaxLen, then the first
 *              binaryBufferMaxLen bytes of the events will be copied
 *              to user-supplied binaryBuffer, and JAVACALL_OUT_OF_MEMORY will
 *              be returned
 * @param outEventLen user-supplied pointer to variable that will hold actual
 *              event size received
 *              Platform is responsible to set this value on success to the
 *              size of the event received, or 0 on failure.
 *              If outEventLen is NULL, the event size is not returned.
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> or if failed or no messages are avaialable
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> If an event's size exceeds the
 *         binaryBufferMaxLen
 */
javacall_result javacall_event_receive(
                            long                    timeTowaitInMillisec,
                            /*OUT*/ unsigned char*  binaryBuffer,
                            /*IN*/  int             binaryBufferMaxLen,
                            /*OUT*/ int*            outEventLen);

/**
 * copies a user supplied event message to a queue of messages
 *
 * @param binaryBuffer a pointer to binary event buffer to send
 *        The platform should make a private copy of this buffer as
 *        access to it is not allowed after the function call.
 * @param binaryBufferLen size of binary event buffer to send
 * @return <tt>JAVACALL_OK</tt> if an event successfully sent,
 *         <tt>JAVACALL_FAIL</tt> or negative value if failed
 */
javacall_result javacall_event_send(unsigned char* binaryBuffer,
                                    int binaryBufferLen);


/**
 * The function javacall_events_init is called during Java VM startup, allowing the
 * platform to perform specific initializations.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_bool javacall_events_init(void);

/**
 * The function javacall_lcd_finalize is called during Java VM shutdown,
 * allowing the platform to perform specific events-related shutdown
 * operations.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_bool javacall_events_finalize(void);

/**
 * The platform calls this function in slave mode to inform VM of new events.
 */
void javanotify_inform_event(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif

