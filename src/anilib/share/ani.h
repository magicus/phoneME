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

#ifndef _JVM_CONFIG_H_
#include "jvmconfig.h"
#endif

#ifndef _ANI_H_
#define _ANI_H_

/**
 * ANI :== Asynchronous Native Interface
 * Asynchronous I/O implemented by native OS threads.
 */

#ifndef _JAVASOFT_KNI_H_
#include "kni.h"
#endif

#ifndef _JVMSPI_H_
#include "jvmspi.h"
#endif

#ifndef _SNI_H_
#include "sni.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**---------------------------------------------------------------------
 *
 * Asynchronous I/O with a native thread pool
 * serving lightweight Java threads:
 *
 *----------------------------------------------------------------------*/

/**
 * Initiates ANI usage in a native method.
 * Call this first in your native method.
 *
 * This function creates an association of the current Java thread
 * with a native thread that can be used to execute a blocking I/O
 * routine WITHOUT blocking the whole JVM.
 *
 * Returns 'KNI_TRUE' when a native thread could be acquired,
 * 'KNI_FALSE' if not.
 * In the latter case, the native method has the following
 * alternatives:
 * - do not proceed and indicate some sort of error status,
 * - proceed as a blocking routine that stalls the whole JVM,
 * - [not implemented yet: call 'ANI_Wait()'].
 *
 * On reentry after a 'KNI_TRUE' result and a call to
 * 'ANI_BlockThread()' in your preceding native method activation
 * this call will do nothing but return 'KNI_TRUE' again.
 */
jboolean ANI_Start();

/**
 * Call this directly after 'ANI_Start()' returns 'KNI_FALSE,
 * then return from the native method activation.
 * The current Java thread will be suspended until a native thread has become
 * available.
 * It will resume reentering the native method and 'ANI_Start()' will then
 * succeed.
 */
void ANI_Wait();

/**
 * The maximum amount of memory that a native thread can use as
 * parameter space.
 */
#define ANI_MAX_ALLOCATION_SIZE   (4 * 1024)

/**
 * Returns a pointer to a shared, position-fixed memory block
 * that both the Java thread and the native thread
 * can access freely without any interference by the GC or any other thread.
 *
 * This pointer will automatically be passed as an argument to 'function' in
 * 'ANI_UseFunction()' below.
 *
 * Returns 'NULL' if more than 'ANI_MAX_ALLOCATION_SIZE' is requested.
 *
 * The allocated memory block will NOT be cleared, i.e. filled with zero values
 * for you.
 * If you wish any initialization, you need to handle this yourself!
 */
void* ANI_AllocateParameterBlock(size_t parameter_size);

/**
 * Gains access to a previously allocated parameter block.
 *
 * Returns 'NULL' when no successful call to 'ANI_AllocateParameterBlock()' has
 * happened yet.
 * Otherwise returns the same result as that call.
 *
 * Use this function in conjunction with 'ANI_AllocateParameterBlock()'
 * to find out if you entered the native function the first or second time:
 * The first time the return value will be 'NULL', so you call
 * 'ANI_AllocateParameterBlock()'.
 * Due to this, the second time the return value will then be non-'NULL'.
 *
 * 'parameter_size' is an out parameter that reflects the value specified in
 * 'ANI_AllocateParameterBlock()'
 * Pass 'NULL' to 'parameter_size' if you are not interested in obtaining this
 * info.
 */
void* ANI_GetParameterBlock(size_t *parameter_size);

/**
 * Type of function that a native thread can execute on behalf of a
 * Java thread.
 *
 * 'parameter' is the shared, position-fixed memory block.
 * 'is_non_blocking' mandates whether the function
 * must return immediately no matter whether it has such a result yet
 * or whether it must block until it has determined a definitive final result.
 * Such a function must return 'KNI_TRUE' when a final result has been obtained
 * and 'KNI_FALSE' otherwise.
 *
 * IMPORTANT: any kind of error status to be reported generally counts as a
 * final result, too.
 * Returning 'KNI_FALSE' indicates that neither a desirable result nor an error
 * is available.
 */
typedef jboolean (*ANI_AsynchronousFunction)(void* parameter, 
                                             jboolean is_non_blocking);

/**
 * Set up the function that the native thread is supposed to execute on behalf
 * of the Java thread.
 * The function will receive the following arguments when called:
 * - a pointer to the shared position-fixed memory block as
 * 'ANI_GetParameterBlock()' would return it,
 * - an indication as to block or not until a final result is available.
 * 'try_non_blocking' is directly passed through to the latter.
 *
 * If 'try_non_blocking' is 'KNI_FALSE',
 * then 'function' gets set (to be executed later by the native
 * thread) only and 'ANI_UseFunction()' will return 'KNI_FALSE()'
 * immediately.
 *
 * If you pass 'KNI_TRUE' to 'try_non_blocking', however, then 'function will
 * be executed immediately
 * in the context of the Java thread and 'KNI_TRUE' will also be passed to its
 * 'is_non_blocking' parameter.
 * 'ANI_UseFunction()' will then return whether a final result has already been
 * obtained or not.
 * In case it has, do not call 'ANI_BlockThread()' unless you want to run
 * 'function' twice.
 */
jboolean ANI_UseFunction(ANI_AsynchronousFunction function, 
                         jboolean try_non_blocking);

/**
 * Suspend the current Java thread until its associated native thread
 * has executed the function set up by 'ANI_UseFunction()'.
 * After calling this, you should exit from the native method
 * and anticipate reentry into it by the Java thread upon which you
 * can access the outcome of the native thread's actions via
 * 'ANI_GetParameterBlock()'.
 *
 */
void ANI_BlockThread();

/**
 * If 'ANI_BlockThread()' has been called, do nothing.
 *
 * Otherwise dissolves the association between the current Java thread
 * and its native thread, releasing the latter and all its resources
 * including the parameter allocated by
 * 'ANI_AllocateParameterBlock()'.
 *
 * The runtime system may recycle the above for reuse or destroy it at its own
 * discretion.
 */
void ANI_End();

/**
 * Wait until a Java thread has become unblocked after completion of a
 * native thread's operation on its behalf or until a timeout
 * specified in milli seconds has passed, whichever comes
 * first. Negative timeout values indicate infinite timeout.
 *
 * Use this function with infinite (negative) timeout 
 * if there is no other event source but ANI.
 *
 * This function can also be used (with positive timeout values) for
 * alternating polling in 'JVMSPI_CheckEvents()', if there is no event
 * checking mechanism that covers all possible event sources
 * (including unblocking of Java threads by ANI as discussed here) in
 * one step.
 */
void ANI_WaitForThreadUnblocking(JVMSPI_BlockedThreadInfo *blocked_threads,
                                 int blocked_threads_count,
                                 jlong timeout_milli_seconds);

/*
 * Initialize the ANI library for the VM.
 */
void ANI_Initialize();

/*
 * Dispose of any resources allocated by the ANI library.
 */
void ANI_Dispose();

#ifdef __cplusplus
}
#endif

#endif /* _ANI_H_ */
