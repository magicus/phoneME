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

#include <suspend_resume_port.h>
#include <midp_logging.h>
#include <midpServices.h>
#include <midp_jc_event_defs.h>

/**
 * Checks if there is a request for java stack to resume normal operation.
 *
 * This function requires porting only if midp_checkAndResume() is used for
 * stack resuming. In case midp_resume() is called directly, this function
 * can be removed from the implementation as well as midp_checkAndResume().
 *
 * @return KNI_TRUE if java stack is requested to resume, KNI_FALSE
 *         if it is not.
 */
jboolean midp_checkResumeRequest() {
    midp_jc_event_union *event;
    static unsigned char binaryBuffer[BINARY_BUFFER_MAX_LEN];
    javacall_bool res;

#if !ENABLE_CDC
    /* timeout == -1 means "wait forever" */
    res = javacall_event_receive(-1, binaryBuffer,
                                 BINARY_BUFFER_MAX_LEN, NULL);
#else
    res = javacall_event_receive_cvm(MIDP_EVENT_QUEUE_ID, binaryBuffer,
                                 BINARY_BUFFER_MAX_LEN, NULL);
#endif

    if (!JAVACALL_SUCCEEDED(res)) {
        return KNI_FALSE;
    }

    event = (midp_jc_event_union *) binaryBuffer;

    return (event->eventType == MIDP_JC_EVENT_RESUME ? KNI_TRUE : KNI_FALSE);
}
