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

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midp_mastermode_port.h>
#include <midpEventUtil.h>
#include <push_server_export.h>
#include <midp_thread.h>
#include <midp_run_vm.h>
#include <suspend_resume.h>

#if (ENABLE_JSR_120 || ENABLE_JSR_205)
#include <wmaInterface.h>
#endif

static MidpReentryData newSignal;
static MidpEvent newMidpEvent;

/**
 * Unblock a Java thread.
 * Returns 1 if a thread was unblocked, otherwise 0.
 */
static int 
eventUnblockJavaThread(
        JVMSPI_BlockedThreadInfo *blocked_threads,
        int blocked_threads_count, unsigned int waitingFor,
        int descriptor, int status)
{
    /*
     * IMPL NOTE: this functionality is similar to midp_thread_signal_list. 
     * It differs in that it reports to the caller whether a thread was
     * unblocked. This is a poor interface and should be removed. However,
     * the coupling with Push needs to be resolved first. In addition,
     * freeing of pResult here seems unsafe. Management of pResult needs
     * to be revisited.
     */
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
            return 1;
        }
    }

    return 0;
}

/*
 * This function is called by the VM periodically. It has to check if
 * any of the blocked threads are ready for execution, and call
 * SNI_UnblockThread() on those threads that are ready.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until an event happens, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the events sources but do not block. Return to the
 *       caller immediately regardless of the status of the event sources.
 *  -1 = Do not timeout. Block until an event happens.
 */
void midp_check_events(JVMSPI_BlockedThreadInfo *blocked_threads,
		       int blocked_threads_count,
		       jlong timeout) {
    if (midp_waitWhileSuspended()) {
        /* System has been requested to resume. Returning control to VM
         * to perform java-side resume routines. Timeout may be too long
         * here or even -1, thus do not check other events this time.
         */
        return;
    }

    newSignal.waitingFor = 0;
    newSignal.pResult = NULL;
    MIDP_EVENT_INITIALIZE(newMidpEvent);

    checkForSystemSignal(&newSignal, &newMidpEvent, timeout);

    switch (newSignal.waitingFor) {
#if ENABLE_JAVA_DEBUGGER
    case VM_DEBUG_SIGNAL:
        if (midp_isDebuggerActive()) {
            JVM_ProcessDebuggerCmds();
        }

        break;
#endif // ENABLE_JAVA_DEBUGGER

    case AMS_SIGNAL:
        midpStoreEventAndSignalAms(newMidpEvent);
        break;

    case UI_SIGNAL:
        midpStoreEventAndSignalForeground(newMidpEvent);
        break;

    case NETWORK_READ_SIGNAL:
        if (eventUnblockJavaThread(blocked_threads,
                                   blocked_threads_count, newSignal.waitingFor,
                                   newSignal.descriptor,
                                   newSignal.status))
            /* Processing is done in eventUnblockJavaThread. */;
        else if (findPushBlockedHandle(newSignal.descriptor) != 0) {
            /* The push system is waiting for a read on this descriptor */
            midp_thread_signal_list(blocked_threads, blocked_threads_count, 
                                    PUSH_SIGNAL, 0, 0);
        }
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
        else
            jsr120_check_signal(newSignal.waitingFor, newSignal.descriptor, newSignal.status);
#endif
        break;

    case HOST_NAME_LOOKUP_SIGNAL:
    case NETWORK_WRITE_SIGNAL:
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
        if (!jsr120_check_signal(newSignal.waitingFor, newSignal.descriptor, newSignal.status))
#endif
            midp_thread_signal_list(blocked_threads, blocked_threads_count,
                                    newSignal.waitingFor, newSignal.descriptor,
                                    newSignal.status);
        break;

    case NETWORK_EXCEPTION_SIGNAL:
        /* Find both the read and write threads and signal the status. */
        eventUnblockJavaThread(blocked_threads, blocked_threads_count,
            NETWORK_READ_SIGNAL, newSignal.descriptor,
            newSignal.status);
        eventUnblockJavaThread(blocked_threads, blocked_threads_count,
            NETWORK_WRITE_SIGNAL, newSignal.descriptor,
            newSignal.status);
        return; 

    case PUSH_ALARM_SIGNAL:
        if (findPushTimerBlockedHandle(newSignal.descriptor) != 0) {
            /* The push system is waiting for this alarm */
            midp_thread_signal_list(blocked_threads,
                blocked_threads_count, PUSH_SIGNAL, 0, 0);
        }

        break;
#if (ENABLE_JSR_135 || ENABLE_JSR_234)
    case MEDIA_EVENT_SIGNAL:
        StoreMIDPEventInVmThread(newMidpEvent, newMidpEvent.MM_ISOLATE);
        eventUnblockJavaThread(blocked_threads, blocked_threads_count,
                MEDIA_EVENT_SIGNAL, newSignal.descriptor, 
                newSignal.status);
        break;
    case MEDIA_SNAPSHOT_SIGNAL:
        eventUnblockJavaThread(blocked_threads, blocked_threads_count,
                MEDIA_SNAPSHOT_SIGNAL, newSignal.descriptor, 
                newSignal.status);
        break;
#endif
#ifdef ENABLE_JSR_179
    case JSR179_LOCATION_SIGNAL:
        midp_thread_signal_list(blocked_threads,
            blocked_threads_count, JSR179_LOCATION_SIGNAL, newSignal.descriptor, newSignal.status);
        break;
#endif /* ENABLE_JSR_179 */

#if (ENABLE_JSR_120 || ENABLE_JSR_205)
    case WMA_SMS_READ_SIGNAL:
    case WMA_CBS_READ_SIGNAL:
    case WMA_MMS_READ_SIGNAL:
    case WMA_SMS_WRITE_SIGNAL:
    case WMA_MMS_WRITE_SIGNAL:
         jsr120_check_signal(newSignal.waitingFor, newSignal.descriptor, newSignal.status);
         break;
#endif
#ifdef ENABLE_JSR_177
    case CARD_READER_DATA_SIGNAL:
        midp_thread_signal_list(blocked_threads, blocked_threads_count,
                                newSignal.waitingFor, newSignal.descriptor,
                                newSignal.status);
        break;
#endif /* ENABLE_JSR_177 */
#ifdef ENABLE_JSR_256
    case JSR256_SIGNAL:
		midp_thread_signal_list(blocked_threads, blocked_threads_count,
			newSignal.waitingFor, newSignal.descriptor, newSignal.status);
        break;
#endif /* ENABLE_JSR_256 */
    default:
        break;
    } /* switch */
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
    /* Master mode does not need VM time slice requests. */
    midp_thread_set_timeslice_proc(NULL);

    return JVM_Start(classPath, mainClass, argc, argv);
}
