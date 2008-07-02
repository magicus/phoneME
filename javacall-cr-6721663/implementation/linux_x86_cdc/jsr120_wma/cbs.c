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

/**
 * @file
 *
 */

#include "javacall_cbs.h"
//#include "javacall_logging.h"

#include <stdio.h>
#define javacall_print(x) printf(x)

/**
 * check if the CBS service is available, and CBS messages can be received
 *
 * @return <tt>JAVACALL_OK</tt> if CBS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_cbs_is_service_available(void) {
    return JAVACALL_OK;
}

#define PORTS_MAX 8
static unsigned short portsList[PORTS_MAX] = {0,0,0,0,0,0,0,0};

/**
 * Registers a message ID number. 
 *
 * If this message ID has already been registered either by a native application 
 * or by another WMA application, then the API should return an error code.
 * 
 * @param msgID message ID to start listening to
 * @return <tt>JAVACALL_OK</tt> if started listening to port, or 
 *         <tt>JAVACALL_FAIL</tt> or negative value if unsuccessful
 */
javacall_result javacall_cbs_add_listening_msgID(unsigned short msgID) {

    int i;
    int free = -1;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == 0) {
            free = i;
            continue;
        }
        if (portsList[i] == msgID) {
            return JAVACALL_FAIL;
        }
    }

    if (free == -1) {
        javacall_print("ports amount exceeded");
        return JAVACALL_FAIL;
    }

    portsList[free] = msgID;

    return JAVACALL_OK;
}

javacall_result javacall_is_cbs_msgID_registered(unsigned short portNum) {
    int i;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == portNum) {
            return JAVACALL_OK;
        }
    }
    return JAVACALL_FAIL;
}

/**
 * Unregisters a message ID number. 
 * After unregistering a message ID, CBS messages received by the device for 
 * the specfied UD should not be delivered to the WMA implementation. 
 *
 * @param msgID message ID to stop listening to
 * @return <tt>JAVACALL_OK </tt> if stopped listening to port, 
 *          or <tt>JAVACALL_FAIL</tt> if failed, or port already not registered
 */
javacall_result javacall_cbs_remove_listening_msgID(unsigned short msgID) {

    int i;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == 0) {
            continue;
        }
        if (portsList[i] == msgID) {
            portsList[i] = 0;
            return JAVACALL_OK;
        }
    }

    return JAVACALL_FAIL;
}
