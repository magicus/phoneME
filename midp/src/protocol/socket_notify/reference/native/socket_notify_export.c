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


#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <midpError.h>
#include <push_server_export.h>
#include <midp_properties_port.h>
#include <midp_logging.h>
#include <midpResourceLimit.h>
#include <string.h>
#include <pcsl_network.h>
#include <midpServices.h>
#include <midp_thread.h>

#include <socket_notify_export.h>

/**
 * @file
 * 
 * The default implementation of the native functions that are needed
 * for supporting the "socket:" Generic Connection protocols.
 */

/*
 * IMPL NOTE: the coupling between platform-specific code (i.e., PCSL) and the 
 * functions in this file is ill-defined. NotifySocketStatusChanged() is a
 * "hidden" upcall from platform-specific code. The value of waitingFor is not 
 * the full set of midpSignalType values from midpServices.h, but instead is 
 * one of SD_RECV (0) or SD_SEND (1).
 */

#define SD_RECV   0

#define SD_SEND   1


/**
 * Find the first thread that can be unblocked for a given handle 
 * and signal type.
 *
 * @param handle Platform specific handle
 * @param signalType Enumerated signal type 
 *
 * @return JVMSPI_ThreadID Java thread id that can be unblocked
 */
static JVMSPI_ThreadID
getBlockedThreadFromHandle(long handle, int waitingFor) {
    JVMSPI_BlockedThreadInfo *blocked_threads;
    int n;
    int i;
 
    blocked_threads = SNI_GetBlockedThreads(&n);
  
    for (i = 0; i < n; i++) {
        MidpReentryData *p = 
            (MidpReentryData*)(blocked_threads[i].reentry_data);
        if (p == NULL) {
            continue;
        }

        /* wait policy: 1. threads waiting for network reads
                        2. threads waiting for network writes
                        3. threads waiting for network push event*/
        if (waitingFor == SD_RECV && p->waitingFor == NETWORK_READ_SIGNAL &&
                p->descriptor == handle) {
            return blocked_threads[i].thread_id;
        }
    
        if (waitingFor == SD_SEND && p->waitingFor == NETWORK_WRITE_SIGNAL &&
                p->descriptor == handle) {
            return blocked_threads[i].thread_id;
        }

        if ((p->waitingFor == PUSH_SIGNAL) && 
            (findPushBlockedHandle(handle) != 0)) {
            /*
             * Slave Mode (Qt-sockets only)
             * No need to explicitly disable the read and write notifiers 
             * for Qt sockets as it is done immediately upon receiving a 
             * callback in PCSL Qt-library code (readableSlot() and 
             * writeableSlot()  
             */

            return blocked_threads[i].thread_id;
        }

    }
  
    return 0;
}

/**
 * Unblock a Java thread and wakeup the VM
 *
 * @param handle Platform specific handle
 * @param signalType Enumerated signal type 
 *
 */
void 
NotifySocketStatusChanged(long handle, int waitingFor) { 
    JVMSPI_ThreadID id = getBlockedThreadFromHandle(handle, waitingFor);
    if (id != 0) {
        midp_thread_unblock(id);
    }
}

