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

#ifndef _MIDP_THREAD_H
#define _MIDP_THREAD_H


/**
 * @file
 * @ingroup core_vmservices
 *
 * @brief Internal utility function to wake up a Java thread when 
 * status of the resource it is waiting for has changed.
 */

#include <jvmspi.h>
#include <midpServices.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A type for a request to schedule VM time slice.
 */
typedef void (*VmThreadTimesliceProc)(void);

/**
 * Sets the routine for implementation-specific request for a VM time slice.
 * This routine will be called every time after a VM thread is unblocked.
 * If NULL is passed as an argument, no requests will be done.
 *
 * @param p routine to request for a VM time slice
 */
void midp_thread_set_timeslice_proc(VmThreadTimesliceProc p);

/**
 * Blocks the current Java thread. The MidpReentryData block for 
 * the current Java thread is set to the passed values.
 * 
 * @param waitingFor set into MidpReentryData.waitingFor
 * @param descriptor set into MidpReentryData.descriptor
 * @param pResult set into MidpReentryData.pResult
 *
 * @return 0 in case of succes
 *         -1 in case error occured
 */
int midp_thread_wait(
        midpSignalType waitingFor, int descriptor, void* pResult);

/**
 * Finds and unblocks all Java threads based on what the thread is waiting 
 * for and which descriptor it is waiting on. This queries the VM for the list 
 * of blocked threads.
 * 
 * @param waitingFor used to match MidpReentryData.waitingFor
 * @param descriptor used to match MidpReentryData.descriptor
 * @param status the value stored into MidpReentryData.status for every 
 *               thread that is unblocked
 */
void midp_thread_signal(midpSignalType waitingFor, int descriptor, int status);

/**
 * Finds and unblocks all Java threads based on what the thread is waiting 
 * for and which descriptor it is waiting on. The list of threads to be 
 * searched is passed as a pair of parameters.
 * 
 * @param blocked_threads list of blocked threads
 * @param blocked_threads_count number of blocked threads in the list
 * @param waitingFor used to match MidpReentryData.waitingFor
 * @param descriptor used to match MidpReentryData.descriptor
 * @param status the value stored into MidpReentryData.status for every 
 *               thread that is unblocked
 */
void midp_thread_signal_list(
        JVMSPI_BlockedThreadInfo *blocked_threads,
        int blocked_threads_count, midpSignalType waitingFor,
        int descriptor, int status);

/**
 * A midp internal function that unblocks the given Java thread. This should 
 * be called in preference to calling SNI_UnblockThread directly, since 
 * depending upon how the VM is scheduled (master mode or slave mode) 
 * additional work may need to be done. In particular, in slave mode, this 
 * function will need to arrange for a VM time slice to occur quickly.
 *
 * @param thr the Java thread to unblock
 */
void midp_thread_unblock(JVMSPI_ThreadID thr);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_THREAD_H */
