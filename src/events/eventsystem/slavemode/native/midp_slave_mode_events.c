/*
 *  
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

#include <stddef.h>

#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midp_thread.h>
#include <midp_slavemode_port.h>

#include <midp_logging.h>

/**
 * This function is called by the VM periodically. It has to check if
 * any of the blocked threads are ready for execution, and call
 * SNI_UnblockThread() on those threads that are ready.
 *
 * @param blocked_threads Array of blocked threads
 * @param blocked_threads_count Number of threads in blocked_threads array
 * @param timeout Values for the paramater:
 *                >0 = Block until an event happens, or until <timeout> 
 *                     milliseconds has elapsed.
 *                 0 = Check the events sources but do not block. Return to the
 *                     caller immediately regardless of the status of the event
 *                     sources.
 *                -1 = Do not timeout. Block until an event happens.
 */
void midp_check_events(JVMSPI_BlockedThreadInfo *blocked_threads,
		       int blocked_threads_count,
		       jlong timeout) {
    /* Nothing to do in slave mode except, avoid compiler warnings. */
    (void)timeout;
    (void)blocked_threads;
    (void)blocked_threads_count;
}

/**
 * Runs the VM in either master or slave mode depending on the
 * platform. It does not return until the VM is finished. In slave mode
 * it will contain a system event loop.
 *
 * @param classPath string containing the class path
 * @param mainClass string containing the main class for the VM to run.
 * @param argc the number of arguments to pass to the main method
 * @param argv the arguments to pass to the main method
 *
 * @return exit status of the VM
 */
int midpRunVm(JvmPathChar* classPath,
	      char* mainClass,
	      int argc,
	      char** argv) {
    midp_thread_set_timeslice_proc(midp_slavemode_schedule_vm_timeslice);

    JVM_SetConfig(JVM_CONFIG_SLAVE_MODE, KNI_TRUE);
    JVM_Start(classPath, mainClass, argc, argv);
    midp_slavemode_event_loop();

    return 0;
}

jlong midp_slavemode_time_slice(void) {
    jlong to = JVM_TimeSlice();
    if ((jlong)-2 == to) {
        JVM_CleanUp();
    }
    return to;
}

/**
 * This function is called when the network initialization
 * or finalization is completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
void midp_network_status_event(int isInit, int status) {
    midp_network_status_event_port(isInit, status);
}
