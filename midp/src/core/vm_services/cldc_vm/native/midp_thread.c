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

#include <stdlib.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midp_logging.h>
#include <midp_thread.h>

static VmThreadTimesliceProc vm_thread_timeslice_proc = NULL;

/**
 * Sets the routine for implementation-specific request for a VM time slice.
 * This routine will be called every time after a VM thread is unblocked.
 * If NULL is passed as an argument, no requests will be done.
 *
 * @param p routine to request for a VM time slice
 */
void midp_thread_set_timeslice_proc(VmThreadTimesliceProc p) {
  vm_thread_timeslice_proc = p;
}

/**
 * Blocks the current Java thread. The MidpReentryData block for 
 * the current Java thread is set to the passed values.
 * 
 * @param waitingFor set into MidpReentryData.waitingFor
 * @param descriptor set into  MidpReentryData.descriptor
 * @param pResult set into MidpReentryData.pResult
 *
 * @return 0 in case of succes
 *         -1 in case error occured
 */
int 
midp_thread_wait(midpSignalType waitingFor, int descriptor, void* pResult)
{
    MidpReentryData* p = 
        (MidpReentryData*)SNI_GetReentryData(NULL);

    if (p == NULL) {
        p = (MidpReentryData*)
             (SNI_AllocateReentryData(sizeof (MidpReentryData)));
        if (p == NULL) {
            REPORT_CRIT(LC_CORE, 
                        "midp_cond_wait: failed to allocate reentry data");
            return -1;
        }
    }

    p->descriptor = descriptor;
    p->waitingFor = waitingFor;
    p->status = 0;
    p->pResult = pResult;

    SNI_BlockThread();

    return 0;
}

/**
 * Find and unblock all Java threads based on what the thread is waiting 
 * for and which descriptor it is waiting on.
 * 
 * @param waitingFor used to match MidpReentryData.waitingFor
 * @param descriptor used to match MidpReentryData.descriptor
 * @param status the value stored into MidpReentryData.status for every 
 *               thread that is unblocked
 */
void 
midp_thread_signal(midpSignalType waitingFor, int descriptor, int status)
{
    int blocked_threads_count;
    JVMSPI_BlockedThreadInfo *blocked_threads;

    blocked_threads = SNI_GetBlockedThreads(&blocked_threads_count);

    midp_thread_signal_list(blocked_threads, blocked_threads_count, 
                            waitingFor, descriptor, status);
}

/**
 * Find and unblock all Java threads based on what the thread is waiting 
 * for and which descriptor it is waiting on.
 * 
 * @param blocked_threads list of blocked threads
 * @param blocked_threads_count number of blocked threads in the list
 * @param waitingFor used to match MidpReentryData.waitingFor
 * @param descriptor used to match MidpReentryData.descriptor
 * @param status the value stored into MidpReentryData.status for every 
 *               thread that is unblocked
 */
void 
midp_thread_signal_list(
        JVMSPI_BlockedThreadInfo *blocked_threads,
        int blocked_threads_count, midpSignalType waitingFor,
        int descriptor, int status)
{
    int i;
    MidpReentryData* pThreadReentryData;

    for (i = 0; i < blocked_threads_count; i++) {
        pThreadReentryData =
            (MidpReentryData*)(blocked_threads[i].reentry_data);

        if (pThreadReentryData != NULL 
                && pThreadReentryData->descriptor == descriptor 
                && pThreadReentryData->waitingFor == waitingFor) {
            pThreadReentryData->status = status;
            midp_thread_unblock(blocked_threads[i].thread_id);
        }
    }
}

/**
 * A midp internal function that unblocks the given Java thread. This should 
 * be called in preference to calling SNI_UnblockThread directly, since 
 * depending upon how the VM is scheduled (master mode or slave mode) 
 * additional work may need to be done. In particular, in slave mode, this 
 * function will need to arrange for a VM time slice to occur quickly.
 *
 * @param thr the Java thread to unblock
 */
void
midp_thread_unblock(JVMSPI_ThreadID thr) {
    /*
     * Tell the VM to unblock the thread, and then tell the platform-specific 
     * code to schedule the VM.
     */
    SNI_UnblockThread(thr);
    if (vm_thread_timeslice_proc != NULL) {
      vm_thread_timeslice_proc();
    }
}
