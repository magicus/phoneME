/*
 *
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

#include <string.h>

#include "javacall_events.h"

#include "javacall_memory.h"
#include "javacall_logging.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Data struct for linked list with each node encapsulating a single event
 */
#define EVENT_DATA_SIZE  200

typedef struct _Event {
    unsigned char data[EVENT_DATA_SIZE];
    int dataLen;
} Event;

#define MAX_EVENTS 20
Event eventsArray[MAX_EVENTS];

static int index = 0;
static int size = 0;

/**
 * Waits for an incoming event message and copies it to user supplied
 * data buffer
 * @param waitForever indicate if the function should block forever
 * @param timeTowaitInMillisec max number of seconds to wait
 *              if waitForever is false
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
                                    long            timeTowaitInMillisec,
                            /*OUT*/ unsigned char*  binaryBuffer,
                            /*IN*/  int             binaryBufferMaxLen,
                            /*OUT*/ int*            outEventLen) {


    if (size == 0) {
        return JAVACALL_FAIL;
    }

    if (index == 0) {
        index = MAX_EVENTS - 1;
    }
    else {
        index--;
    }

    if(eventsArray[index].dataLen > binaryBufferMaxLen) {
        /*if not enough memory, we keep the event in the list so that client code can re-invoke with bigger buffer*/
        *outEventLen = 0;
        javacall_print("javacall_event_receive(): binaryBuffer is not big enough for first event");
        return JAVACALL_OUT_OF_MEMORY;
    }

    *outEventLen = eventsArray[index].dataLen;
    memcpy(binaryBuffer, eventsArray[index].data, *outEventLen);

    size--;

    return JAVACALL_OK;
}

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
                                    int binaryBufferLen) {
    if (size == MAX_EVENTS) {
        javacall_print("javacall_event_send() queue is full");
        return JAVACALL_FAIL;
    }

    if (EVENT_DATA_SIZE < binaryBufferLen) {
        javacall_print("javacall_event_send() binaryBufferLen is too long");
        return JAVACALL_FAIL;
    }

    eventsArray[index].dataLen = binaryBufferLen;
    memcpy(eventsArray[index].data, binaryBuffer, binaryBufferLen);
    index = (index + 1) % MAX_EVENTS;
    size++;
    return JAVACALL_OK;
}

#ifdef __cplusplus
}
#endif
