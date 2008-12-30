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
#include <javaTask.h>
#include <exe_entry_point.h>

static javacall_result midpHandleSetVmArgs(int argc, char* argv[]);
static javacall_result midpHandleSetHeapSize(midp_event_heap_size heap_size);
static javacall_result midpHandleListMIDlets(void);
static javacall_result midpHandleListStorageNames(void);
static javacall_result midpHandleRemoveMIDlet(midp_event_remove_midlet removeMidletEvent);

/**
 * An entry point of a thread devoted to run java
 */
void JavaTask(void) {
    midp_jc_event_union event;
    javacall_bool res = JAVACALL_OK;
    javacall_bool JavaTaskIsGoOn = JAVACALL_TRUE;
    long timeTowaitInMillisec = -1;
    int outEventLen;
    int main_memory_chunk_size;

    /* Get java heap memory size */
    main_memory_chunk_size = getInternalPropertyInt("MAIN_MEMORY_CHUNK_SIZE");
    if (main_memory_chunk_size == 0) {
	main_memory_chunk_size = -1;
    }

    /* Outer Event Loop */
    while (JavaTaskIsGoOn) {

        if (midpInitializeMemory(main_memory_chunk_size) != 0) {
            REPORT_CRIT(LC_CORE,"JavaTask() >> midpInitializeMemory()  Not enough memory.\n");
            break;
        }
        REPORT_INFO(LC_CORE,"JavaTask() >> memory initialized.\n");

#if !ENABLE_CDC
        res = javacall_event_receive(timeTowaitInMillisec,
            (unsigned char *)&event, sizeof(midp_jc_event_union), &outEventLen);
#else
        res = javacall_event_receive_cvm(MIDP_EVENT_QUEUE_ID,
            (unsigned char *)&event, sizeof(midp_jc_event_union), &outEventLen);
#endif

        if (!JAVACALL_SUCCEEDED(res)) {
            REPORT_ERROR(LC_CORE,"JavaTask() >> Error javacall_event_receive()\n");
            continue;
        }

        switch (event.eventType) {
        case MIDP_JC_EVENT_START_ARBITRARY_ARG:
            REPORT_INFO(LC_CORE, "JavaTask() MIDP_JC_EVENT_START_ARBITRARY_ARG >>\n");
            javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_STARTED,
                                             JAVACALL_OK,
                                             NULL);
            JavaTaskImpl(event.data.startMidletArbitraryArgEvent.argc,
                         event.data.startMidletArbitraryArgEvent.argv);

            JavaTaskIsGoOn = JAVACALL_FALSE;
            break;

        case MIDP_JC_EVENT_SET_VM_ARGS:
            REPORT_INFO(LC_CORE, "JavaTask() MIDP_JC_EVENT_SET_VM_ARGS >>\n");
            midpHandleSetVmArgs(event.data.startMidletArbitraryArgEvent.argc,
                                event.data.startMidletArbitraryArgEvent.argv);
            break;

        case MIDP_JC_EVENT_SET_HEAP_SIZE:
            REPORT_INFO(LC_CORE, "JavaTask() MIDP_JC_EVENT_SET_HEAP_SIZE >>\n");
            midpHandleSetHeapSize(event.data.heap_size);
            break;

        case MIDP_JC_EVENT_LIST_MIDLETS:
            REPORT_INFO(LC_CORE, "JavaTask() MIDP_JC_EVENT_LIST_MIDLETS >>\n");
            midpHandleListMIDlets();
            JavaTaskIsGoOn = JAVACALL_FALSE;
            break;

        case MIDP_JC_EVENT_LIST_STORAGE_NAMES:
            REPORT_INFO(LC_CORE, "JavaTask() MIDP_JC_EVENT_LIST_STORAGE_NAMES >>\n");
            midpHandleListStorageNames();
            JavaTaskIsGoOn = JAVACALL_FALSE;
            break;

        case MIDP_JC_EVENT_REMOVE_MIDLET:
            REPORT_INFO(LC_CORE, "JavaTask() MIDP_JC_EVENT_REMOVE_MIDLET >>\n");
            midpHandleRemoveMIDlet(event.data.removeMidletEvent);
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

} /* end of JavaTask */

/**
 * 
 */
static javacall_result midpHandleSetVmArgs(int argc, char* argv[]) {
    int used;

    while ((used = JVM_ParseOneArg(argc, argv)) > 0) {
        argc -= used;
        argv += used;
    }
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result midpHandleSetHeapSize(midp_event_heap_size heap_size) {
    JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY, heap_size.heap_size);
    JVM_SetConfig(JVM_CONFIG_HEAP_MINIMUM, heap_size.heap_size);
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result midpHandleListMIDlets() {
    char *argv[3];
    int argc = 0;
    javacall_result res;

    argv[argc++] = "runMidlet";
    argv[argc++] = "internal";
    argv[argc++] = "com.sun.midp.scriptutil.SuiteLister";

    res = JavaTaskImpl(argc, argv);
    return res;
}

/**
 * 
 */
static javacall_result midpHandleListStorageNames() {
    char *argv[3];
    int argc = 0;
    javacall_result res;

    argv[argc++] = "runMidlet";
    argv[argc++] = "internal";
    /**
     * IMPL_NOTE: introduce an argument for SuiteLister allowing to print
     *            paths to jars only.
     */
    argv[argc++] = "com.sun.midp.scriptutil.SuiteLister";

    res = JavaTaskImpl(argc, argv);
    return res;
}

/**
 * 
 */
static javacall_result
midpHandleRemoveMIDlet(midp_event_remove_midlet removeMidletEvent) {
    char *argv[4];
    int argc = 0;
    javacall_result res;

    argv[argc++] = "runMidlet";
    argv[argc++] = "internal";
    argv[argc++] = "com.sun.midp.scriptutil.SuiteRemover";
    argv[argc++] = removeMidletEvent.suiteID;

    res = JavaTaskImpl(argc, argv);
    return res;
}
