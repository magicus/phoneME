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

#include <string.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <pcsl_print.h>
#include <pcsl_memory.h>

#include <midpAMS.h>
#include <midpInit.h>
#include <midpMalloc.h>
#include <midpCommandState.h>
#include <midpStorage.h>
#include <midpServices.h>
#include <midp_properties_port.h>
#include <midpTimeZone.h>
#include <midp_logging.h>
#include <midp_properties_port.h>
#include <midpMIDletProxyList.h>
#include <midpEvents.h>
#include <suitestore_task_manager.h>
#include <midpError.h>
#include <midp_run_vm.h>
#include <midp_check_events.h>
#include <midpMidletSuiteUtils.h>
#include <midp_constants_data.h>
#if (ENABLE_JSR_205 || ENABLE_JSR_120)
#include <jsr120_types.h>
#include <wmaInterface.h>
#endif

#include <lcdlf_export.h>
#include <midpUtilKni.h>
#include <push_server_export.h>

#if !ENABLE_CDC
#include <suspend_resume.h>
#include <midp_ams_exceptions.h>
#endif


#include <stdio.h>
#include <stdlib.h>

/**
 * @file
 *
 * Platform-specific VM startup and shutdown routines.
 *
 * This file provides platform-specific virtual machine
 * startup and shutdown routines.  Refer to file
 * "/src/vm/share/runtime/JVM.hpp" and the Porting
 * Guide for details.
 */

/**
 * @def MAX_VM_PROFILE_LEN
 * Maximal length of the VM profile name.
 */
#define MAX_VM_PROFILE_LEN 256

/**
 * @def ROTATION_ARG
 * Name of the system property with initial screen rotation mode.
 * The property can be set to 1 for rotated mode, any other value
 * is ignored and normal screen mode is used.
 */
#define ROTATION_ARG "rotation"

static JvmPathChar getCharPathSeparator();
static MIDP_ERROR getClassPathPlus(SuiteIdType storageName,
    JvmPathChar** userClassPath, char* classPathExt);

#if VERIFY_ONCE
static MIDP_ERROR getClassPathForVerifyOnce(
    JvmPathChar** classPath, SuiteIdType suiteId,
    const pcsl_string* midletName, const pcsl_string* jarPath);
#endif

#if (REPORT_LEVEL <= LOG_INFORMATION) && !ENABLE_CONTROL_ARGS_FROM_JAD

#define STACK_SIZE 8192

/* Stack grows down */
void measureStack(int clearStack) {
    char  stack[STACK_SIZE];
    char  tag = (char)0xef;
    int   i;

    if (clearStack) {
        for (i = 0; i < STACK_SIZE; i++) {
            stack[i] = tag;
        }
    } else {
        for (i = 0; i < STACK_SIZE; i++) {
            if (stack[i] != tag) {
                reportToLog(LOG_INFORMATION, LC_CORE_STACK,
                            "Max Native Stack Size:  %d",
                            (STACK_SIZE - i));
                break;
            }
        }
    }
}
#undef STACK_SIZE

#else

void measureStack(int clearStack) {
    (void)clearStack;
    return;
}

#endif

#if 0
void monitorHeap() {
    static const int EXPECTED_HEAP_DELTA = 256; /* bytes */
    /* Remember initial free heap size */
    static int firstHeap = 0;

    /* Compare free memory before each VM run, to detect memory leak */
    int currentHeap = midpGetFreeHeap();
    if (firstHeap == 0) {
        firstHeap = currentHeap;
    } else if (firstHeap - currentHeap > EXPECTED_HEAP_DELTA) {
        printf("Possible memory leak between VM runs : %d",
               firstHeap - currentHeap);
    }
}
#else
#define monitorHeap()
#endif

#if ENABLE_MONET
/**
 * Convert Java classes to Monet bundle (In-Place Execution format)
 * upon the first run of a newly installed MIDletSuite.
 *
 * @param userClassPath pointer to current classpath string.
 */
static void setMonetClassPath(JvmPathChar **userClassPath, int pathLen) {
    if (*userClassPath != NULL && pathLen > 0 &&
            (*userClassPath)[pathLen - 4] == (JvmPathChar)'.' &&
            (*userClassPath)[pathLen - 3] == (JvmPathChar)'j' &&
            (*userClassPath)[pathLen - 2] == (JvmPathChar)'a' &&
            (*userClassPath)[pathLen - 1] == (JvmPathChar)'r') {
        int i;
        int j;
        pcsl_string uBinFile = PCSL_STRING_NULL;
        int errorcode;
#ifdef PRODUCT
        char* suffix = ".bun";
#elif defined(AZZERT)
        char* suffix = "_g.bun";
#else
        char* suffix = "_r.bun";
#endif
        int suffixLen = strlen(suffix);
        /* add the new suffix, but don't include ".jar" */
        int binNameLen = pathLen - 4 + suffixLen;
        JvmPathChar *binFile = (JvmPathChar*)midpMalloc(
                               (binNameLen + 1) * sizeof (JvmPathChar));

        /*
         * JvmPathChars can be either 16 bits or 8 bits so we can't
         * assume either.
         */
        memcpy(binFile, *userClassPath, pathLen * sizeof (JvmPathChar));

        /* replace the ".jar" with a new suffix */
        for (i = pathLen - 4, j = 0; j < suffixLen; i++, j++) {
            binFile[i] = (JvmPathChar)suffix[j];
        }

        binFile[i] = 0;

        ASSERT(sizeof(JvmPathChar) == sizeof(jchar));
        /*
         * Assuming that JvmPathChar is 16-bit (jchar): this is always true if
         * CLDC is based on PCSL.
         * In other case (which shouldn't happen) build error "Incompatible
         * pointer types" will occur. Then the following code should be used
         * instead of simple conversion from UTF-16:
         *
         *   if (sizeof(JvmPathChar) == 1) {
         *       pcsl_string_convert_from_utf8(binFile, i, &uBinFile);
         *   } else {
         *       pcsl_string_convert_from_utf16(binFile, i, &uBinFile);
         *   }
         */
        pcsl_string_convert_from_utf16(binFile, i, &uBinFile);
        /* IMPL_NOTE: can we do anything meaningful in case of out-of-memory? */

        /* If Monet bundle file does not exist, convert now */
        if (storage_file_exists(&uBinFile) == 0) {
            errorcode = JVM_CreateAppImage(*userClassPath, binFile,
                                      JVM_REMOVE_CLASSES_FROM_JAR);
        } else {
            errorcode = 0;
        }

        /* Once we have monet bundle, we set classpath to be:
         *      <monet_bundle>:<original_jar>
         * The original jar is still needed because it may still
         * contain resource files.
         */
        if (errorcode == 0) {
            /* binary file + separator + jar file + terminator */
            int newPathLen = binNameLen + 1 + pathLen + 1;
            JvmPathChar *newPath =
                (JvmPathChar*)midpMalloc(newPathLen * sizeof (JvmPathChar));
            int j;

            for (i = 0; i < binNameLen; i++) {
                newPath[i] = binFile[i];
            }

            newPath[i] = getCharPathSeparator();
            i++;

            for (j = 0; j < pathLen; i++, j++) {
                newPath[i] = (*userClassPath)[j];
            }

            newPath[i] = 0;

            midpFree(*userClassPath);
            *userClassPath = newPath;
        }

        pcsl_string_free(&uBinFile);
        midpFree(binFile);
    }
}
#else
#define setMonetClassPath(x, y)
#endif

/** The name of the runtime main internal class. */
#define MIDP_MAIN "com.sun.midp.main.MIDletSuiteLoader"

char*
JVMSPI_GetSystemProperty(const char* prop_name) {

    char *result = (char *)getSystemProperty(prop_name);

    if (result == NULL && prop_name != NULL) {
        if (strcmp(prop_name, TIMEZONE_PROP_NAME) == 0) {
            /* Get the local timezone from the native platform */
            result = getLocalTimeZone();
        }
    }

    return result;
}

void
JVMSPI_SetSystemProperty(const char* propName, const char* value) {
    /*
     * override internal configuration parameters.
     */
    setInternalProperty(propName, value);

     /*
      * Also override System.getProperty() for backward compatibility
      * with CLDC uses of property vales.
      */
    setSystemProperty(propName, value);
}

void
JVMSPI_FreeSystemProperty(const char* prop_value) {
    (void)prop_value;    /* No-op */
}

void
JVMSPI_DisplayUsage(char* message) {
    (void)message;      /* No-op */
}

jboolean
JVMSPI_CheckExit(void) {
    return KNI_FALSE;       /* Never allow System.exit() to succeed */
}

void
JVMSPI_Exit(int code) {
    midpFinalize();
    exit(code);
}

/**
 * This function is called by the VM periodically. It has to check if
 * any of the blocked threads are ready for execution, and call
 * SNI_UnblockThread() on those threads that are ready.
 *
 * @param blocked_threads Array of blocked threads
 * @param blocked_threads_count Number of threads in the blocked_threads array
 * @param timeout Values for the paramater:
 *                >0 = Block until an event happens, or until <timeout> 
 *                     milliseconds has elapsed.
 *                 0 = Check the events sources but do not block. Return to the
 *                     caller immediately regardless of the status of the event
 *                     sources.
 *                -1 = Do not timeout. Block until an event happens.
 */
void JVMSPI_CheckEvents(JVMSPI_BlockedThreadInfo *blocked_threads,
                        int blocked_threads_count,
                        jlong timeout) {
  midp_check_events(blocked_threads, blocked_threads_count, timeout);
}

#if !ENABLE_CDC

/**
 * This function is called by the VM before a Java thread is terminated
 * because of an uncaught exception.
 *
 * @param isolate_id ID of an isolate in which this exception was thrown
 *                   (always 1 in SVM mode).
 * @param exception_class_name name of the class containing the method.
 *              This string is a fully qualified class name
 *              encoded in internal form (see JVMS 4.2).
 *              This string is NOT 0-terminated.
 * @param exception_class_name_length number of UTF8 characters in
 *              <exception_class_name>.
 * @param message exception message as a 0-terminated ASCII string
 * @param flags a bitmask of flags
 * @param exit_code a pointer to store the exit code of the VM or the isolate
 *     in case of JVMSPI_ABORT return value. Not used for other return values.
 *
 * @return
 * If this function returns JVMSPI_ABORT, the VM will terminate the isolate
 *     in MVM mode, or the whole VM in SVM mode.
 * If this function returns JVMSPI_IGNORE, the VM will apply default handling
 *     actions to this exception - the thread in which the exception occurred
 *     will be terminated.
 * If this function returns JVMSPI_SUSPEND, the VM will suspend the isolate
 *     in MVM mode, in SVM mode the behavior is undefined.
 * For other return values the VM behavior is undefined.
 */
int JVMSPI_HandleUncaughtException(const int isolate_id,
                                   const char * exception_class_name,
                                   const int exception_class_name_length,
                                   const char * message,
                                   const int flags,
                                   int * exit_code) {
    (void)exit_code;

    if (midp_ams_uncaught_exception(isolate_id,
                                    exception_class_name,
                                    exception_class_name_length,
                                    message,
                                    flags)) {
        return JVMSPI_SUSPEND;
    } else {
        return JVMSPI_IGNORE;
    }
}

/**
 * This function is called by the VM when it fails to fulfil
 * a memory allocation request.
 *
 * @param isolate_id ID of an isolate in which the allocation was requested
 *    (always 1 in SVM mode).
 * @param limit in SVM mode - heap capacity, in MVM mode - memory limit for
 *    the isolate, i.e. the max amount of heap memory that can possibly
 *    be allocated
 * @param reserve in SVM mode - heap capacity, in MVM mode - memory reservation
 *    for the isolate, i.e. the max amount of heap memory guaranteed
 *    to be available
 * @param used how much memory is already allocated on behalf of this isolate
 *    in MVM mode, or for the whole VM in SVM mode.
 * @param alloc_size the requested amount of memory that the VM failed to allocate
 * @param flags a bitmask of flags
 * @param exit_code a pointer to store the exit code of the VM or the isolate
 *     in case of JVMSPI_ABORT return value. Not used for other return values.
 *
 * @return
 * If this function returns JVMSPI_ABORT, the VM will terminate the isolate
 *     in MVM mode, or the whole VM in SVM mode.
 * If this function returns JVMSPI_IGNORE, the VM will apply default handling
 *     actions to this failure - OutOfMemoryError will be thrown
 *     in this thread.
 * If this function returns JVMSPI_RETRY, the VM will redo the allocation
 *     attempt. This code should be returned if the function freed up some
 *     memory in the Java heap.
 * If this function returns JVMSPI_SUSPEND, in SVM mode the behavior is
 *     undefined, in MVM mode the VM will suspend the isolate. When the isolate
 *     will be resumed, the VM will redo the allocation attempt.
 * For other return values the VM behavior is undefined.
 */
int JVMSPI_HandleOutOfMemory(const int isolate_id,
                             const int limit,
                             const int reserve,
                             const int used,
                             const int alloc_size,
                             const int flags,
                             int * exit_code) {
    (void)exit_code;

    if (midp_ams_out_of_memory(isolate_id, limit, reserve, used,
                               alloc_size, flags)) {
        return JVMSPI_SUSPEND;
    } else {
        return JVMSPI_IGNORE;
    }
}

#endif /* !ENABLE_CDC */

#if ENABLE_JAVA_DEBUGGER
void
JVMSPI_DebuggerNotification(jboolean is_active) {
    /* Avoid a compiler warning. */
    (void)is_active;
    return;
}
#endif

/**
 * This function provides an implementation
 * used by the logging service, as well as by the
 * Java system output streams (System.out.println(), etc.)
 *
 * @param s a string sent to a system specific output stream
 */
void JVMSPI_PrintRaw(const char* s, int length) {
  pcsl_print_chars(s, length);
}

/**
 * Initializes the Debugger.
 */
static void
midpInitializeDebugger(void) {
#if ENABLE_ON_DEVICE_DEBUG || ENABLE_WTK_DEBUG
    {
#if ENABLE_MULTIPLE_ISOLATES
    #define OPT_NUM 3
#else
    #define OPT_NUM 2
#endif

        char* argv[OPT_NUM] = {
            "-nosuspend", "-debugger"
#if ENABLE_MULTIPLE_ISOLATES
            , "-debug_isolate"
#endif
        };
        int i;

        for (i = 0; i < OPT_NUM; i++) {
            (void)JVM_ParseOneArg(1, &argv[i]);
        }
#undef OPT_NUM

     }
#endif /* ENABLE_ON_DEVICE_DEBUG || ENABLE_WTK_DEBUG */

#if ENABLE_JAVA_DEBUGGER
    {
        char* argv[2];

        /* memory profiler */
        if (getInternalProperty("VmMemoryProfiler") != NULL) {
            argv[0] = "-memory_profiler";
            (void)JVM_ParseOneArg(1, argv);
        }

        /* Get the VM debugger port property. */
        argv[1] = (char *)getInternalProperty("VmDebuggerPort");
        if (argv[1] != NULL) {
            argv[0] = "-port";
            (void)JVM_ParseOneArg(2, argv);
        }
    }
#endif
#if ENABLE_MEMMON
    if (getInternalProperty("MemoryMonitor") != NULL) {
        char* argv[1];
        argv[0] = "-monitormemory";
        (void)JVM_ParseOneArg(1, argv);
    }
#endif
}

/**
 * Initializes the UI.
 *
 * @return <tt>0</tt> upon successful initialization, otherwise
 *         <tt>-1</tt>
 */
static int
midpInitializeUI(void) {
    if (0 == lcdlf_ui_init()) {

        /* Get the initial screen rotation mode property */
        const char* pRotationArg = getSystemProperty(ROTATION_ARG);
        if (pRotationArg) {
            if (atoi(pRotationArg) == 1) {
	      lcdlf_reverse_orientation(lcdlf_get_current_hardwareId());
            }
        }

        return 0;
    } else {
        return -1;
    }
}

/**
 * Initializes the VM.
 *
 * @return <tt>0</tt> upon successful initialization, otherwise
 *         <tt>-1</tt>
 */
static int
midpInitializeVM(void) {
    if (InitializeEvents() != 0) {
        return -1;
    }

    /*
     * Porting consideration:
     * Here is a good place to put I18N init.
     * function. e.g. initLocaleMethod();
     */

    midpInitializeDebugger();

    if (pushopen() != 0) {
        return -1;
    }

    if (midpInitializeUI() != 0) {
        return -1;
    }

    return 0;
}

/**
 * Finalizes the AMS.
 */
static void
midpFinalizeAMS(void) {
    finalizeCommandState();

    /*
     * Note: the AMS isolate will have been registered by a native method
     * call, so there is no corresponding midpRegisterAmsIsolateId in VM
     * initialization code.
     */
    midpUnregisterAmsIsolateId();
}

/**
 * Finalizes the UI.
 */
static void
midpFinalizeUI(void) {
    lcdlf_ui_finalize();
}

/**
 * Finalizes the VM.
 */
static void
midpFinalizeVM(void) {
    midpFinalizeUI();

    pushclose();

    midpFinalizeAMS();

    /* 
     * Porting consideration:
     * Here is a good place to put I18N finalization
     * function. e.g. finalizeLocaleMethod();
     */

    FinalizeEvents();
}

/**
 * Sets the the system property "classpathext" to the given value.
 *
 * @param classPathExt the new value of the "classpath" system property
 */
static void
putClassPathExtToSysProperty(char* classPathExt) {
    /* store class path as system variable for another isolates */
    if (NULL != classPathExt) {
        char* argv[1];
        const char prefix[] = "-Dclasspathext=";
        argv[0] = midpMalloc(sizeof(prefix) + strlen(classPathExt));
        if (NULL != argv[0]) {
            memcpy(argv[0], prefix, sizeof(prefix));
            /* copy extention + trailing zero */
            memcpy(argv[0] + sizeof(prefix) - 1, classPathExt,
                   strlen(classPathExt) + 1);
            (void)JVM_ParseOneArg(1, argv);
            midpFree(argv[0]);
        }
    }
}

#if ENABLE_JAVA_DEBUGGER

/**
 * Passes an argument corresponding to the given debug option to the VM.
 *
 * @param debugOption debug option to pass to the VM
 */
static void setDebugOption(int debugOption) {
    char* argv[1];

    if (debugOption > MIDP_NO_DEBUG) {
        argv[0] = "-debugger";
        (void)JVM_ParseOneArg(1, argv);

#if ENABLE_MULTIPLE_ISOLATES
        argv[0] = "-debug_isolate";
        (void)JVM_ParseOneArg(1, argv);
#endif

        if (debugOption == MIDP_DEBUG_SUSPEND) {
            argv[0] = "-suspend";
            (void)JVM_ParseOneArg(1, argv);
        } else if (debugOption == MIDP_DEBUG_NO_SUSPEND) {
            argv[0] = "-nosuspend";
            (void)JVM_ParseOneArg(1, argv);
        }
    }
}

#endif


/**
 * Runs the given MIDlet from the specified MIDlet suite with the
 * given arguments. Up to 3 arguments will be made available to
 * the MIDlet as properties <tt>arg-&lt;num&gt;</tt>, where
 * <tt><i>num</i></tt> is <tt>0</tt> for the first argument, etc.
 *
 * @param suiteId The MIDlet Suite ID that the MIDlet is in
 * @param midletClassName The class name of MIDlet to run
 * @param arg0 The first argument for the MIDlet to be run.
 * @param arg1 The second argument for the MIDlet to be run.
 * @param arg2 The third argument for the MIDlet to be run.
 * @param debugOption 0 for no debug, 1 debug: suspend the VM until the
 *   debugger sends a continue command, 2 debug: do not wait for the debugger
 * @param classPathExt additional path to be passed to the VM
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error,
 *         <tt>SUITE_NOT_FOUND_STATUS</tt> if the MIDlet suite not found
 */
int
midp_run_midlet_with_args_cp(SuiteIdType suiteId,
                             const pcsl_string* midletClassName,
                             const pcsl_string* arg0,
                             const pcsl_string* arg1,
                             const pcsl_string* arg2,
                             int debugOption,
                             char* classPathExt) {
    int vmStatus = 0;
    MIDPCommandState* commandState;
    JvmPathChar* classPath = NULL;
    pcsl_string_status res;

#if (VERIFY_ONCE)
    jboolean classVerifier = JVM_GetUseVerifier();
#endif

    if (midpInitCallback(VM_LEVEL, midpInitializeVM, midpFinalizeVM) != 0) {
        REPORT_WARN(LC_CORE, "Out of memory during init of VM.\n");
        return MIDP_ERROR_STATUS;
    }

#if (ENABLE_JSR_205 || ENABLE_JSR_120)
    /*
     * Start listening for wireless messages.
     * Consider moving this as appropriate in course of pause/resume
     * framework implementation
     */
    if (init_jsr120() != WMA_NET_SUCCESS) {
        REPORT_WARN(LC_CORE, "Cannot init WMA");
        return MIDP_ERROR_STATUS;
    }
#endif

    commandState = midpGetCommandState();

    commandState->suiteId = suiteId;

    if (! pcsl_string_is_null(midletClassName)) {
        pcsl_string_free(&commandState->midletClassName);
        res = pcsl_string_dup(midletClassName, &commandState->midletClassName);
        if (PCSL_STRING_OK != res) {
            REPORT_WARN(LC_CORE, "Out of memory: could not dup classname.\n");
            return MIDP_ERROR_STATUS;
        }
    }

    if (! pcsl_string_is_null(arg0)) {
        pcsl_string_free(&commandState->arg0);
        res = pcsl_string_dup(arg0, &commandState->arg0);
        if (PCSL_STRING_OK != res) {
            REPORT_WARN(LC_CORE, "Out of memory: could not dup arg0.\n");
            return MIDP_ERROR_STATUS;
        }
    }

    if (! pcsl_string_is_null(arg1)) {
        pcsl_string_free(&commandState->arg1);
        res = pcsl_string_dup(arg1, &commandState->arg1);
        if (PCSL_STRING_OK != res) {
            REPORT_WARN(LC_CORE, "Out of memory: could not dup arg1.\n");
            return MIDP_ERROR_STATUS;
        }
    }

    if (! pcsl_string_is_null(arg2)) {
        pcsl_string_free(&commandState->arg2);
        res = pcsl_string_dup(arg2, &commandState->arg2);
        if (PCSL_STRING_OK != res) {
            REPORT_WARN(LC_CORE, "Out of memory: could not dup arg2.\n");
            return MIDP_ERROR_STATUS;
        }
    }

#if ENABLE_JAVA_DEBUGGER
    setDebugOption(debugOption);
#else
    (void)debugOption;
#endif

    putClassPathExtToSysProperty(classPathExt);

    do {
        MIDP_ERROR status = MIDP_ERROR_NONE;

#if (VERIFY_ONCE)
        /* For cached suite verification we should add the suite classpath */
        status = getClassPathForVerifyOnce(&classPath,
            commandState->suiteId, &commandState->midletClassName,
            &commandState->arg1);

        /* Since it is not suite verifier call follow the common way */
        if (status == MIDP_ERROR_UNSUPPORTED) {
            status = getClassPathPlus(commandState->suiteId,
                &classPath, classPathExt);
        }
#else
        status = getClassPathPlus(commandState->suiteId,
                    &classPath, classPathExt);
#endif
        switch (status) {
        case MIDP_ERROR_OUT_MEM:
            REPORT_WARN(LC_CORE, "Out of memory: could allocate classpath.\n");
            return MIDP_ERROR_STATUS;

        case MIDP_ERROR_AMS_SUITE_CORRUPTED:
            REPORT_WARN(LC_CORE, "I/O error reading appdb.\n");
            return MIDP_ERROR_STATUS;

        case MIDP_ERROR_AMS_SUITE_NOT_FOUND:
            REPORT_WARN(LC_CORE, "Suite not found.\n");
            return SUITE_NOT_FOUND_STATUS;

        default:
            break;
        }

        midp_resetEvents();
        midpMIDletProxyListReset();

        pushcheckinLeftOvers(commandState->suiteId);

        measureStack(KNI_TRUE);
        monitorHeap();

#if (VERIFY_ONCE)
        /*
         * Restore class verifier state for new VM being started,
         * since it could be disabled by the previous VM start
         * for faster startup of a preverified MIDlet suite.
         */
        JVM_SetUseVerifier(classVerifier);
#endif

#if !ENABLE_CDC
        sr_repairSystem();
#endif

        /*
         * The VM can exit abruptly with a status of zero or -1.
         * But our Java Main returns a specific positive code,
         * so we can tell if the VM aborted.
         *
         * Arguments to MIDlets are pass in the command state.
         */
        vmStatus = midpRunVm(classPath, MIDP_MAIN, 0, NULL);

        measureStack(KNI_FALSE);

        if (classPath != NULL) {
            midpFree(classPath);
            classPath = NULL;
        }

        /** 
         * Any subsequent VM runs that will or not happen after 
         * the first one counts as VM restart.
         */
        commandState->vmRestarted = KNI_TRUE;

        if (vmStatus != MAIN_EXIT) {
            /*
             * The VM aborted, most likely a bad class file in an installed
             * MIDlet.
             */
            vmStatus = MIDP_ERROR_STATUS;

            if (commandState->lastSuiteId == UNUSED_SUITE_ID) {
                /* nothing to run last so just break out */
                break;
            }

            /* Something like autotest is running, do not exit. */
            commandState->suiteId = commandState->lastSuiteId;
            commandState->lastSuiteId = UNUSED_SUITE_ID;

            pcsl_string_free(&commandState->midletClassName);
            commandState->midletClassName = commandState->lastMidletClassName;
            commandState->lastMidletClassName = PCSL_STRING_NULL;

            /* Set memory quotas (if specified) for the next midlet. */
            if (commandState->runtimeInfo.memoryReserved >= 0) {
                JVM_SetConfig(JVM_CONFIG_HEAP_MINIMUM,
                              commandState->runtimeInfo.memoryReserved);
                commandState->runtimeInfo.memoryReserved = -1;
            }

            if (commandState->runtimeInfo.memoryTotal >= 0) {
                JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY,
                              commandState->runtimeInfo.memoryTotal);
                commandState->runtimeInfo.memoryTotal = -1;
            }

#if ENABLE_VM_PROFILES
            /* Set the VM profile (if specified) for the next midlet. */
            if (pcsl_string_length(&commandState->profileName) > 0) {
                jbyte profileName[MAX_VM_PROFILE_LEN];
                jsize profileNameLen;

                if (pcsl_string_convert_to_utf8(&commandState->profileName,
                        profileName,
                        MAX_VM_PROFILE_LEN,
                        &profileNameLen) != PCSL_STRING_OK) {
                    REPORT_WARN(LC_CORE, "Out of memory: "
                                "could not set the VM profile.\n");
                    commandState->status = MIDP_ERROR_OUT_MEM;
                    break;
                }

                JVM_SetProfile((char*)profileName);

                pcsl_string_free(&commandState->profileName);
                commandState->profileName = PCSL_STRING_NULL;
            }
#endif /* ENABLE_VM_PROFILES */

            pushcheckinall();
            continue;
        }

        vmStatus = commandState->status;
        if (vmStatus > 0) {
            /* shutdown */
            vmStatus = MIDP_SHUTDOWN_STATUS;
            break;
        }

#if ENABLE_JAVA_DEBUGGER
#if ENABLE_WTK_DEBUG
        /*
         * If ENABLE_ON_DEVICE_DEBUG is also enabled and debug session is in
         * progress, debugOption will be overridden in the following code block.
         */
        setDebugOption(debugOption);
#endif /* ENABLE_WTK_DEBUG */

#if ENABLE_ON_DEVICE_DEBUG
        if (commandState->debugMode != MIDP_NO_DEBUG) {
            setDebugOption(commandState->debugMode);
            commandState->debugMode = MIDP_NO_DEBUG;
        }
#endif /* ENABLE_ON_DEVICE_DEBUG */
#endif /* ENABLE_JAVA_DEBUGGER */
    } while (commandState->suiteId != UNUSED_SUITE_ID);

    pushcheckinall();

#if (ENABLE_JSR_120 || ENABLE_JSR_205)
    finalize_jsr120();
#endif

    return vmStatus;
}

static JvmPathChar getCharPathSeparator() {
    return (JvmPathChar)storageGetPathSeparator();
}

/**
 * Runs the given MIDlet from the specified MIDlet suite with the
 * given arguments. Up to 3 arguments will be made available to
 * the MIDlet as properties <tt>arg-&lt;num&gt;</tt>, where
 * <tt><i>num</i></tt> is <tt>0</tt> for the first argument, etc.
 *
 * @param suiteId The MIDlet Suite ID that the MIDlet is in
 * @param midletClassName The class name of MIDlet to run
 * @param arg0 The first argument for the MIDlet to be run.
 * @param arg1 The second argument for the MIDlet to be run.
 * @param arg2 The third argument for the MIDlet to be run.
 * @param debugOption 0 for no debug, 1 debug: suspend the VM until the
 *   debugger sends a continue command, 2 debug: do not wait for the debugger
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error,
 *         <tt>SUITE_NOT_FOUND_STATUS</tt> if the MIDlet suite not found
 */
int
midp_run_midlet_with_args(SuiteIdType suiteId,
                          const pcsl_string* midletClassName,
                          const pcsl_string* arg0,
                          const pcsl_string* arg1,
                          const pcsl_string* arg2,
                          int debugOption) {
    return midp_run_midlet_with_args_cp(suiteId, midletClassName,
        arg0, arg1, arg2, debugOption, NULL);
}

/**
 * Generates a complete classpath in preparation to run a MIDlet
 * Suite. This generated value is stored in the global variable
 * <tt>MidpCommandLineClassPath</tt>.
 *
 * @param suiteId The MIDlet Suite ID.
 * @param userClassPath The classpath to be used when executing a
 *                      MIDlet Suite in <tt>suiteId</tt>.
 * @param classPathExt The classpath extension to be appended to
 *                 the generated classpath. May be NULL or empty.
 * @return <tt>MIDP_ERROR_NONE</tt> if the classpath was generated,
 *    MIDP_ERROR_AMS_SUITE_NOT_FOUND mean the suite does not exist,
 *    OUT_OF_MEM_LEN if out of memory for the new string,
 *    IO_ERROR if an IO_ERROR.
 */
static MIDP_ERROR getClassPathPlus(SuiteIdType suiteId,
                                   JvmPathChar** userClassPath,
                                   char* classPathExt) {
    pcsl_string jarPath;
    const jchar* jarPathData;
    jsize jarPathLen;
    JvmPathChar* newPath;
    char* additionalPath =        /* replace NULL with empty string */
            classPathExt!=NULL ? classPathExt : "";
    int additionalPathLength = strlen(additionalPath);
    int i,j;

    if (suiteId == UNUSED_SUITE_ID) {
        return MIDP_ERROR_AMS_SUITE_NOT_FOUND;
    }

    if (suiteId == INTERNAL_SUITE_ID) {
        jarPath = PCSL_STRING_EMPTY;
    } else {
        MIDPError error;
        StorageIdType storageId;

        error = midp_suite_get_suite_storage(suiteId, &storageId);

        if (error == ALL_OK) {
            error = midp_suite_get_class_path(suiteId, storageId,
                                              KNI_TRUE, &jarPath);
        }

        if (error != ALL_OK) {
            return MIDP_ERROR_AMS_SUITE_NOT_FOUND;
        }
    }

    jarPathLen = pcsl_string_utf16_length(&jarPath);
    newPath = (JvmPathChar*)midpMalloc(sizeof(JvmPathChar)
                    /* generatedPath separator pathExt terminator */
                    * (jarPathLen + 1 + additionalPathLength + 1));
    if (NULL == newPath) {
        pcsl_string_free(&jarPath);
        return OUT_OF_MEM_LEN;
    }

    jarPathData = pcsl_string_get_utf16_data(&jarPath);
    if (NULL == jarPathData) {
        pcsl_string_free(&jarPath);
        return OUT_OF_MEM_LEN;
    }
    for (i = 0; i < jarPathLen; i++) {
        newPath[i] = (JvmPathChar)jarPathData[i];
    }
    pcsl_string_release_utf16_data(jarPathData, &jarPath);

    if (additionalPathLength!=0) {
        if (i != 0) {
            newPath[i++]=(JvmPathChar)getCharPathSeparator();
        }

        for (j = 0; j < additionalPathLength; j++,i++) {
            newPath[i] = (JvmPathChar)additionalPath[j];
        }
    }

    newPath[i] = 0;

    pcsl_string_free(&jarPath);

    *userClassPath = newPath;

    setMonetClassPath(userClassPath, jarPathLen);

    return MIDP_ERROR_NONE;
}


#if (VERIFY_ONCE)
/**
 * Check whether MIDlet to be started is the suite verifier MIDlet.
 * If that's the case set class path value to a specified JAR path
 * provided to the verifier MIDlet as one of the arguments.
 *
 * @param pClassPath reference to classpath to be used for the nearest
 *   MIDlet start; classpath value could be allocated in this function
 *   and caller is responsible to free its memory
 * @param suiteId id of the suite scheduled for the nearest VM start
 * @param midletName name of the MIDlet scheduled for the nearest VM start
 * @param jarPath if MIDlet name is the name of suite verifier MIDlet from
 *   internal suite, jarPath will be set as classpath value for the nearest
 *   VM start
 */
static MIDP_ERROR getClassPathForVerifyOnce(
    JvmPathChar** pClassPath, SuiteIdType suiteId,
    const pcsl_string* midletName, const pcsl_string* jarPath) {

    const jchar* jarPathData;
    jsize jarPathLen;
    int i;

    /* Change classpath for suite verifier MIDlet only */
    if (pcsl_string_is_null(midletName) || pcsl_string_is_null(jarPath) ||
        suiteId != INTERNAL_SUITE_ID ||
        !pcsl_string_equals(midletName, &SUITE_VERIFIER_MIDLET) ) {

        return MIDP_ERROR_UNSUPPORTED;
    }

    /* Free previously allocated classpath */
    if (*pClassPath != NULL) {
        midpFree(*pClassPath);
    }

    /* Allcoate new classpath variable */
    jarPathLen = pcsl_string_utf16_length(jarPath);
    *pClassPath = (JvmPathChar*)midpMalloc(
        sizeof(JvmPathChar) * (jarPathLen + 1));
    if (NULL == *pClassPath) {
        return OUT_OF_MEM_LEN;
    }

    jarPathData = pcsl_string_get_utf16_data(jarPath);
    if (NULL == jarPathData) {
        return OUT_OF_MEM_LEN;
    }

    for (i = 0; i < jarPathLen; i++) {
        (*pClassPath)[i] = (JvmPathChar)jarPathData[i];
    }
    pcsl_string_release_utf16_data(jarPathData, jarPath);
    (*pClassPath)[i] = 0;

    return MIDP_ERROR_NONE;
}
#endif /*VERIFY_ONCE */

/**
 * Starts the system and instructs the VM to run the main() method of
 * the specified class. Does not return until the system is stopped.
 *
 * @param classPath string containing the class path
 * @param mainClass string containing the main class for the VM to run.
 * @param argc the number of arguments to pass to the main method
 * @param argv the arguments to pass to the main method
 *
 * @return <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down or
 *         <tt>MIDP_ERROR_STATUS</tt> if an error
 */
int midpRunMainClass(JvmPathChar *classPath,
                     char *mainClass,
                     int argc,
                     char **argv) {
    int vmStatus = 0;

    midpInitialize();

    if (midpInitCallback(VM_LEVEL, midpInitializeVM, midpFinalizeVM) != 0) {
        REPORT_WARN(LC_CORE, "Out of memory during init of VM.\n");
        return MIDP_ERROR_STATUS;
    }

    /*
     * The VM can exit abruptly with a status of zero or -1.
     * But our Java Main returns a specific positive code,
     * so we can tell if the VM aborted.
     */
    vmStatus = midpRunVm(classPath, mainClass, argc, argv);


    if (0 == vmStatus) {
        vmStatus = MIDP_RUNNING_STATUS;
    } else {
        pushcheckinall();
        midp_resetEvents();
        midpMIDletProxyListReset();
        midpFinalize();
        if (vmStatus != MAIN_EXIT) {
            /*
             * The VM aborted, most likely a bad class file in an installed
             * MIDlet.
             */
            vmStatus = MIDP_ERROR_STATUS;
        } else {
            vmStatus = MIDP_SHUTDOWN_STATUS;
        }
    }


    return vmStatus;
}

