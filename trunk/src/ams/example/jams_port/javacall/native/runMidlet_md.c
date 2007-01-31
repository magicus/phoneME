/*
 *
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <midp_logging.h>
#include <midpAMS.h>
#include <suitestore_common.h>
#include <midpMalloc.h>
#include <jvm.h>
#include <jvmspi.h>
#include <findMidlet.h>
#include <midpUtilKni.h>
#include <midp_jc_event_defs.h>
#include <midp_properties_port.h>
#include <javacall_events.h>
#include <javacall_lifecycle.h>
#include <midpStorage.h>
#include <suitestore_task_manager.h>
#include <commandLineUtil.h>

unsigned char enable_java_debugger = 0;

extern int runMidlet(int argc, char** commandlineArgs);

static javacall_result
midpHandleStartArbitraryArgEvent(midp_jc_event_start_arbitrary_arg startArbitraryArgEvent);

/**
 * An entry point of a thread devoted to run java
 */
void JavaTask(void) {
    static unsigned long binaryBuffer[BINARY_BUFFER_MAX_LEN/sizeof(long)];
    midp_jc_event_union *event;
    javacall_bool res = JAVACALL_OK;
    javacall_bool JavaTaskIsGoOn = JAVACALL_TRUE;
    long timeTowaitInMillisec = -1;
    int binaryBufferMaxLen = BINARY_BUFFER_MAX_LEN;
    int outEventLen;

    REPORT_CRIT(LC_CORE,"JavaTask() >>\n");

    /* Outer Event Loop */
    while (JavaTaskIsGoOn) {

        if (midpInitializeMemory(-1) != 0) {
            REPORT_CRIT(LC_CORE,"JavaTask() >> midpInitializeMemory()  Not enough memory.\n");
            break;
        }
        REPORT_INFO(LC_CORE,"JavaTask() >> memory initialized.\n");

        res = javacall_event_receive(timeTowaitInMillisec,
            (unsigned char *)binaryBuffer, binaryBufferMaxLen, &outEventLen);

        if (!JAVACALL_SUCCEEDED(res)) {
            REPORT_ERROR(LC_CORE,"JavaTask() >> Error javacall_event_receive()\n");
            continue;
        }

        event = (midp_jc_event_union *) binaryBuffer;

        switch (event->eventType) {
        case MIDP_JC_EVENT_START_ARBITRARY_ARG:
            REPORT_INFO(LC_CORE,"JavaTask() MIDP_JC_EVENT_START_ARBITRARY_ARG>> \n");
            javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_STARTED,
                                             JAVACALL_OK);
            midpHandleStartArbitraryArgEvent(event->data.startMidletArbitraryArgEvent);

            JavaTaskIsGoOn = JAVACALL_FALSE;
            break;

        case MIDP_JC_EVENT_END:
            REPORT_INFO(LC_CORE,"JavaTask() >> MIDP_JC_EVENT_END\n");
            JavaTaskIsGoOn = JAVACALL_FALSE;
            break;

        default:
            REPORT_ERROR(LC_CORE,"Unknown event.\n");
            break;

        } /* end of switch */

        midpFinalizeMemory();

    }   /* end of while 'JavaTaskIsGoOn' */

        REPORT_CRIT(LC_CORE,"JavaTask() <<\n");

} /* end of JavaTask */


static javacall_result
midpHandleStartArbitraryArgEvent(midp_jc_event_start_arbitrary_arg startArbitraryArgEvent) {
    javacall_result res;

    res = runMidlet(startArbitraryArgEvent.argc, startArbitraryArgEvent.argv);

    javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN,
                                     (res == 1) ? JAVACALL_OK: JAVACALL_FAIL);

    return res;
}
