/*
 * @(#)debugInit.c	1.49 06/10/10
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
#include <stdlib.h>
#include <string.h>
#include "jvmdi.h"
#include "util.h"
#include "debugInit.h"
#include "commonRef.h"
#include "debugDispatch.h"
#include "eventHandler.h"
#include "eventHelper.h"
#include "threadControl.h"
#include "stepControl.h"
#include "transport.h"
#include "classTrack.h"
#include "VirtualMachineImpl.h"
#include "bag.h"
#include "JDWP.h"
#include "invoker.h"
#include "version.h"

static JVMDI_RawMonitor initMonitor;
static jboolean initComplete;
static jbyte currentSessionID; 

/*
 * Options set through the JVM_OnLoad options string. All of these values
 * are set once at VM startup and never reset. 
 */
static jboolean isServer = JNI_FALSE;     /* Listens for connecting debuggers? */
static jboolean isStrict = JNI_FALSE;     /* Strict reading of JVMDI spec? */
static jboolean useStandardAlloc = JNI_FALSE;  /* Use standard malloc/free? */
static struct bag *transports;            /* of TransportSpec */

static jboolean initOnStartup = JNI_TRUE;   /* init immediately */
static char *initOnException = NULL;        /* init when this exception thrown */
static jboolean initOnUncaught = JNI_FALSE; /* init when uncaught exc thrown */

static char *launchOnInit = NULL;           /* launch this app during init */
static jboolean suspendOnInit = JNI_TRUE;   /* suspend all app threads after init */
static char *names;                         /* strings derived from OnLoad options */

#ifdef DEBUG
jboolean assertOn = JNI_TRUE;               /* check assertions */
jboolean assertFatal = JNI_TRUE;            /* if an assertion occurs it is fatal */
#else
jboolean assertOn = JNI_FALSE;              /* check assertions */
jboolean assertFatal = JNI_FALSE;           /* if an assertion occurs it is fatal */
#endif

/*
 * Elements of the transports bag
 */
typedef struct TransportSpec {
    char *name;
    char *address;
    jboolean loaded;
    void *cookie;
} TransportSpec;

/*
 * Forward Refs
 */
static void initialEventHook(JNIEnv *env, JVMDI_Event *event);
static void initialize(JNIEnv *env, JVMDI_Event *triggeringEvent);
static jboolean parseOptions(char *str);

/*
 * Phase 1: Initial load.
 *
 * JVM_OnLoad is called by the VM immediately after the back-end
 * library is loaded. We can do very little in this function since 
 * the VM has not completed initialization. So, we parse the JDWP
 * options and set up a simple initial event hook for JVMDI events. 
 * When a triggering event occurs, that hook will begin debugger initialization.
 */
static jint 
setInitialNotificationMode(void)
{
    #define SET_MODE(mode, type) { \
            jint error = threadControl_setEventMode(mode, type, NULL); \
            if (error != JVMDI_ERROR_NONE) { \
                return error; \
            } \
    }
    jint mode;

    /* SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_SINGLE_STEP); */
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_BREAKPOINT);    
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_FRAME_POP);     
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_USER_DEFINED);  
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_CLASS_PREPARE); 
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_CLASS_UNLOAD);  
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_CLASS_LOAD);    
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_FIELD_ACCESS);       
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_FIELD_MODIFICATION); 
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_EXCEPTION_CATCH);    
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_METHOD_ENTRY);       
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_METHOD_EXIT); 
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_THREAD_START);  
    SET_MODE(JVMDI_DISABLE, JVMDI_EVENT_THREAD_END);    

    SET_MODE(JVMDI_ENABLE, JVMDI_EVENT_VM_DEATH);           
    SET_MODE(JVMDI_ENABLE, JVMDI_EVENT_VM_INIT);            
    if (initOnUncaught || (initOnException != NULL)) {
        mode = JVMDI_ENABLE;
    } else {
        mode = JVMDI_DISABLE;
    }
    SET_MODE(mode, JVMDI_EVENT_EXCEPTION);
    return JVMDI_ERROR_NONE;

    #undef SET_MODE
}

static jvmdiError
jvmdiAlloc(jlong size, jbyte **bufferPtr)
{
    void *buffer;

    /*
     * Our malloc impls can't handle full jlongs
     */
    if ((size >> 32) != 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }

    buffer = jdwpAlloc((jint)size);
    if (buffer == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    } else {
        *bufferPtr = buffer;
        return JVMDI_ERROR_NONE;
    }
}

static jvmdiError
jvmdiFree(jbyte *buffer)
{
    jdwpFree(buffer);
    return JVMDI_ERROR_NONE;
}

JNIEXPORT jint JNICALL 
JVM_OnLoad(JavaVM *vm, char *options, void *reserved) 
{
    jint error;

    if (!parseOptions(options)) {
        return -1;
    }

    /*
     * Immediately set the two global function table pointers
     */
    jvm = vm;
    error = (*vm)->GetEnv(vm, (void **)(void *)&jvmdi, JVMDI_VERSION_1);
    if (error != JNI_OK) {   
        if (error == JVMDI_ERROR_ACCESS_DENIED) {
            fprintf(stderr, "JDWP not initialized properly.  Add -Xdebug to command line\n");
        } else {
            fprintf(stderr, "JDWP unable to access JVMDI Version 1\n");
        }
        return -1;
    }

    error = setInitialNotificationMode();
    if (error != JVMDI_ERROR_NONE) {
        fprintf(stderr, "JDWP unable to configure JVMDI events\n");
        return -1;
    }

    error = jvmdi->SetAllocationHooks(jvmdiAlloc, jvmdiFree);
    if (error != JVMDI_ERROR_NONE) {
        fprintf(stderr, "JDWP unable to set JVMDI allocation hooks\n");
        return -1;
    }

    error = jvmdi->SetEventHook(initialEventHook);
    if (error != JVMDI_ERROR_NONE) {
        fprintf(stderr, "JDWP unable to register for JVMDI events\n");
        return -1;
    }
    return 0;
}

/*
 * Phase 2: Initial events. Phase 2 consists of waiting for the 
 * event that triggers full initialization. Under normal circumstances 
 * (initOnStartup == TRUE) this is the JVMDI_EVENT_VM_INIT event. 
 * Otherwise, we delay initialization until the app throws a 
 * particular exception. The triggering event invokes 
 * the bulk of the initialization, including creation of threads and 
 * monitors, transport setup, and installation of a new event hook which
 * handles the complete set of events.
 *
 * Since the triggering event comes in on an application thread, some of the 
 * initialization is difficult to do here. Specifically, this thread along
 * with all other app threads may need to be suspended until a debugger
 * connects. These kinds of tasks are left to the third phase which is 
 * invoked by one of the spawned debugger threads, the event handler.
 */

/*
 * Wait for a triggering event; then kick off debugger 
 * initialization. A different event hook will be installed by 
 * debugger initialization, and this function will not be called
 * again.
 */
static void 
initialEventHook(JNIEnv *env, JVMDI_Event *event) 
{
    static jboolean vmInitialized = JNI_FALSE;

    /*
     * %comment gordonh007
     */
    switch (event->kind) {
        case JVMDI_EVENT_VM_INIT: {
            /*
             * Create a lock for use by the allocator.
             */
            JVMDI_RawMonitor allocLock;
            jint error = jvmdi->CreateRawMonitor("JDWP Alloc Lock", &allocLock);
            if (error != JVMDI_ERROR_NONE) {
                ERROR_MESSAGE_EXIT("JDWP unable to create allocator lock\n");
            }
            util_setAllocLock(allocLock);
            vmInitialized = JNI_TRUE;
        
            if (initOnStartup) {
                initialize(env, event);
                return;
            }
            break;
        }

        case JVMDI_EVENT_EXCEPTION: {
            char *signature;
            jclass clazz;
            jvmdiError error;

            if (vmInitialized) {
                /*
                 * We want to preserve any current exception that might get wiped
                 * out during event handling (e.g. JNI calls). We have to rely on
                 * space for the local reference on the current frame because
                 * doing a PushLocalFrame here might itself generate an exception. 
                 */
                jthrowable currentException = (*env)->ExceptionOccurred(env);
                (*env)->ExceptionClear(env);
    
                if (initOnUncaught && event->u.exception.catch_clazz == NULL) {
                    initialize(env, event);
                    return;
                }
    
                if (initOnException == NULL) {
                    /* Not initing on throw, skip check below */
                    return;
                }
    
                clazz = (*env)->GetObjectClass(env, event->u.exception.exception);
                error = jvmdi->GetClassSignature(clazz, &signature);
                if ((error == JVMDI_ERROR_NONE) && (signature != NULL) && 
                    (strcmp(signature, initOnException) == 0)) {
                    initialize(env, event);
                    return;
                }
    
                /*
                 * Restore exception state from before hook call
                 */
                if (currentException != NULL) {
                    (*env)->Throw(env, currentException);
                } else {
                    (*env)->ExceptionClear(env);
                }
            }

            break;
        }
    }
}

typedef struct EnumerateArg {
    jboolean isServer;
    jint error;
    jint startCount;
} EnumerateArg;

static jboolean
startTransport(void *item, void *arg) 
{
    TransportSpec *transport = item;
    EnumerateArg *enumArg = arg;
    jint error;

    error = transport_startTransport(enumArg->isServer, transport->name,
                                     transport->address, &transport->cookie);
    if (error != JDWP_ERROR(NONE)) {
        fprintf(stderr, "Transport %s failed to initialize, rc = %d.\n",
                transport->name, error);
        enumArg->error = error;
    } else {
        /* (Don't overwrite any previous error) */

        enumArg->startCount++;
    }

    return JNI_TRUE;   /* Always continue, even if there was an error */
}

static jboolean
stopTransport(void *item, void *arg) 
{
    TransportSpec *transport = item;
    transport_stopTransport(transport->cookie);
    return JNI_TRUE;
}

static jboolean
unloadTransport(void *item, void *arg) 
{
    TransportSpec *transport = item;
    transport_unloadTransport(transport->cookie);
    return JNI_TRUE;
}

static void
signalInitComplete(void)
{
    /*
     * Initialization is complete
     */
    debugMonitorEnter(initMonitor);
    initComplete = JNI_TRUE;
    debugMonitorNotifyAll(initMonitor);
    debugMonitorExit(initMonitor);
}

/*
 * Determine if  initialization is complete.
 */
jboolean 
debugInit_isInitComplete() 
{
    return initComplete;
}

/*
 * Wait for all initialization to complete.
 */
void 
debugInit_waitInitComplete() 
{
    debugMonitorEnter(initMonitor);
    while (!initComplete) {
        debugMonitorWait(initMonitor);
    }
    debugMonitorExit(initMonitor);
}

/*
 * Initialize debugger back end modules
 */              
static void 
initialize(JNIEnv *env, JVMDI_Event *triggeringEvent) 
{   
    EnumerateArg arg;
    struct bag *initEventBag;
    jbyte suspendPolicy;

    JDI_ASSERT(triggeringEvent != NULL);

    currentSessionID = 0;
    initComplete = JNI_FALSE;

    /* Remove initial event hook */
    jvmdi->SetEventHook(NULL);

    commonRef_initialize();
    util_initialize();
    threadControl_initialize();
    stepControl_initialize();
    invoker_initialize();
    debugDispatch_initialize();
    version_initialize();
    classTrack_initialize();
    VirtualMachine_initialize();

    initMonitor = debugMonitorCreate("JDWP Initialization Monitor");


    /*
     * Initialize transports
     */
    arg.isServer = isServer;
    arg.error = JDWP_ERROR(NONE);
    arg.startCount = 0;

    transport_initialize(env);
    bagEnumerateOver(transports, startTransport, &arg);

    /*
     * Exit with an error only if 
     * 1) none of the transports was successfully started, and
     * 2) the application has not yet started running
     */
    if ((arg.error != JDWP_ERROR(NONE)) && 
        (arg.startCount == 0) && 
        initOnStartup) {
        (*env)->FatalError(env, "No transports initialized");
    }

    eventHandler_initialize(currentSessionID);

    signalInitComplete();

    suspendPolicy = suspendOnInit ? JDWP_SuspendPolicy_ALL 
                                  : JDWP_SuspendPolicy_NONE;
    if (triggeringEvent->kind == JVMDI_EVENT_VM_INIT) {
        jthread thread = currentThread();
        eventHelper_reportVMInit(currentSessionID, thread, suspendPolicy);
        (*env)->DeleteGlobalRef(env, thread);
    } else {
        /*
         * %comment gordonh008
         */
        initEventBag = eventHelper_createEventBag();
        eventHelper_recordEvent(triggeringEvent, 0, 
                                suspendPolicy, initEventBag);
        eventHelper_reportEvents(currentSessionID, initEventBag);
    }

}

/*
 * Restore all static data to the initialized state so that another
 * debugger can connect properly later.
 */
void 
debugInit_reset(jboolean shutdown)
{
    EnumerateArg arg;

    currentSessionID++;
    initComplete = JNI_FALSE;

    eventHandler_reset(currentSessionID);
    transport_reset();
    VirtualMachine_reset();
    version_reset();
    debugDispatch_reset();
    invoker_reset();
    stepControl_reset();
    threadControl_reset();
    util_reset();
    commonRef_reset();

    /*
     * If this is a server, we are now ready to accept another connection.
     * If it's a client, then we've cleaned up some (more should be added 
     * later) and we're done.
     */
    if (isServer && !shutdown) {
        arg.isServer = JNI_TRUE;
        arg.error = JDWP_ERROR(NONE);
        arg.startCount = 0;
        bagEnumerateOver(transports, startTransport, &arg);
    }
    signalInitComplete();
}


char *
debugInit_launchOnInit()
{
    return launchOnInit;
}

jboolean
debugInit_suspendOnInit()
{
    return suspendOnInit;
}

/*
 * code below is shamelessly swiped from hprof.
 */
typedef struct {
    char *name;
    jboolean *ptr;
} binary_switch_t;

static binary_switch_t binary_switches[] = {
    {"suspend", &suspendOnInit},
    {"server", &isServer},
    {"strict", &isStrict},
    {"onuncaught", &initOnUncaught},
    {"stdalloc", &useStandardAlloc}
};

static int 
get_tok(char **src, char *buf, int buflen, char sep)
{
    int i;
    char *p = *src;
    for (i = 0; i < buflen; i++) {
        if (p[i] == 0 || p[i] == sep) {
            buf[i] = 0;
            if (p[i] == sep) {
                i++;
            }
            *src += i;
            return i;
        }
        buf[i] = p[i];
    }
    /* overflow */
    return 0;
}

static void 
printUsage(void)
{
     fprintf(stdout,
	     "\n"
	     "-Xrunjdwp usage: -Xrunjdwp:[help]|[<option>=<value>, ...]\n"
	     "\n"
	     "Option Name and Value\t\tDescription\t\t\tDefault\n"
	     "---------------------\t\t-----------\t\t\t-------\n"
	     "suspend=y|n\t\t\twait on startup?\t\ty\n"
	     "transport=<name>\t\ttransport spec\t\t\tnone\n"
	     "address=<listen/attach address>\ttransport spec\t\t\t\"\"\n"
	     "server=y|n\t\t\tlisten for debugger?\t\tn\n"
	     "launch=<command line>\t\trun debugger on event\t\tnone\n"
	     "onthrow=<exception name>\tdebug on throw\t\t\tnone\n"
	     "onuncaught=y|n\t\t\tdebug on any uncaught?\t\tn\n"
	     "strict=y|n\t\t\tskip JVMDI bug workarounds?\tn\n"
	     "stdalloc=y|n\t\t\tUse C Runtime malloc/free?\tn\n"
	     "\n"
	     "Example: java -Xrunjdwp:transport=dt_socket,address=localhost:8000\n\n");
}

jboolean checkAddress(void *bagItem, void *arg)
{
    TransportSpec *spec = (TransportSpec *)bagItem;
    if (spec->address == NULL) {
        fprintf(stderr, "ERROR: Non-server transport %s must have a connection "
                "address specified through the 'address=' option\n", spec->name);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}
             
static jboolean 
parseOptions(char *options)
{
    TransportSpec *currentTransport = NULL;
    char *end;
    char *current;
    int length;
    char *str;

    if (options == NULL) {
        options = "";
    }

    /*
     * At this point during initialization, we haven't determined which 
     * allocator to use. For the allocations that take place here, we set
     * things up to use the regular malloc. These buffers are never freed;
     * if that changes, proper steps will need to be taken to free them
     * with the right allocator.
     */
    {
        char *envOptions;
        jboolean savedStandardAlloc = useStandardAlloc;
        useStandardAlloc = JNI_TRUE;
    
        /*
         * Add environmentally specified options.
         */
        envOptions = getenv("_JAVA_JDWP_OPTIONS");
        if (envOptions != NULL) {
            int originalLength;
            char *combinedOptions;

            fprintf(stderr, "Picked up _JAVA_JDWP_OPTIONS: %s\n", envOptions);

            /*
             * Allocate enough space for both strings and
             * comma in between.
             */
            originalLength = strlen(options);
            combinedOptions = jdwpAlloc(originalLength + 1 +
                                        strlen(envOptions) + 1);
            if (combinedOptions == NULL) {
                fprintf(stderr, "JDWP unable to allocate memory\n");
                return -1;
            }

            strcpy(combinedOptions, options);
            strcat(combinedOptions, ",");
            strcat(combinedOptions, envOptions);

            options = combinedOptions;
        }

        /*
         * Allocate a buffer for names derived from option strings. It should
         * never be longer than the original options string itself.
         */
        length = strlen(options);
        names = jdwpAlloc(length + 1); 
        if (names == NULL) {
            fprintf(stderr, "JDWP unable to allocate memory\n");
            return -1;
        }
    
        transports = bagCreateBag(sizeof(TransportSpec), 3);
        if (transports == NULL) {
            fprintf(stderr, "ERROR: unable to allocate memory.\n");
            return JNI_FALSE;
        }
    
        useStandardAlloc = savedStandardAlloc;
        /* Back to default allocator */
    }

    current = names;
    end = names + length;
    str = options;

    if ((strcmp(str, "help")) == 0) {
        printUsage();
        /*
         * This is the only silent way to exit. We need a jvmdi->exit.
         */
        exit(0);
    }

    while (*str) {
        char buf[100];
        if (!get_tok(&str, buf, sizeof(buf), '=')) {
            goto bad_option;
        }
        if (strcmp(buf, "transport") == 0) {
            currentTransport = bagAdd(transports);
            if (!get_tok(&str, current, end - current, ',')) {
                goto bad_option;
            }
            currentTransport->name = current;
            current += strlen(current) + 1;
        } else if (strcmp(buf, "address") == 0) {
            if (currentTransport == NULL) {
                fprintf(stderr, "ERROR: address specified without transport.\n");
                goto bad_option_no_msg;
            }
            if (!get_tok(&str, current, end - current, ',')) {
                goto bad_option;
            }
            currentTransport->address = current;
            currentTransport = NULL;
            current += strlen(current) + 1;
        } else if (strcmp(buf, "launch") == 0) {
            if (!get_tok(&str, current, end - current, ',')) {
                goto bad_option;
            }
            launchOnInit = current;
            current += strlen(current) + 1;
        } else if (strcmp(buf, "onthrow") == 0) {
            /* Read class name and convert in place to a signature */
            *current = 'L';
            if (!get_tok(&str, current + 1, end - current - 1, ',')) {
                goto bad_option;
            }
            initOnException = current;
            while (*current != '\0') {
                if (*current == '.') {
                    *current = '/';
                }
                current++;
            }
            *current++ = ';';
            *current++ = '\0';
        } else if (strcmp(buf, "assert") == 0) {
            if (!get_tok(&str, current, end - current, ',')) {
                goto bad_option;
            }
            if (strcmp(current, "y") == 0) {
                assertOn = JNI_TRUE;
                assertFatal = JNI_FALSE;
            } else if (strcmp(current, "fatal") == 0) {
                assertOn = JNI_TRUE;
                assertFatal = JNI_TRUE;
            } else if (strcmp(current, "n") == 0) {
                assertOn = JNI_FALSE;
                assertFatal = JNI_FALSE;
            } else {
                goto bad_option;
            }
            current += strlen(current) + 1;
        } else {
            int i;
            int n_switches = 
            sizeof(binary_switches) / sizeof(binary_switch_t); 
            for (i = 0; i < n_switches; i++) {
                if (strcmp(binary_switches[i].name, buf) == 0) {
                    if (!get_tok(&str, buf, sizeof(buf), ',')) {
                        goto bad_option;
                    }
                    if (strcmp(buf, "y") == 0) {
                        *(binary_switches[i].ptr) = JNI_TRUE;
                    } else if (strcmp(buf, "n") == 0) {
                        *(binary_switches[i].ptr) = JNI_FALSE;
                    } else {
                        goto bad_option;
                    }
                    break;
                }
            }
            if (i >= n_switches) {
                goto bad_option;
            }
        }
    }

    if (bagSize(transports) == 0) {
        fprintf(stderr, "ERROR: No transport specified.\n");
        goto bad_option_no_msg;
    }

    /*
     * %comment gordonh009
     */
    if (bagSize(transports) > 1) {
        fprintf(stderr, "ERROR: Multiple transports are not supported in this release.\n");
        goto bad_option_no_msg;
    }


    /*if (!isServer && (bagSize(transports) > 1)) {
        fprintf(stderr, "ERROR: Only 1 transport allowed with \"server=n\" option.\n");
        goto bad_option_no_msg;
    }*/

    if (!isServer) {
        jboolean specified = bagEnumerateOver(transports, checkAddress, NULL);
        if (!specified) {
            /* message already printed */
            goto bad_option_no_msg;
        }
    }

    /*
     * The user has selected to wait for an exception before init happens
     */
    if ((initOnException != NULL) || (initOnUncaught)) {
        initOnStartup = JNI_FALSE;

        if (launchOnInit == NULL) {
            /*
             * These rely on the launch=/usr/bin/foo
             * suboption, so it is an error if user did not
             * provide one.
             */
            fprintf(stderr, "ERROR: missing JDWP suboption.  Specify "
                    "launch=<command line> when using onthrow or onuncaught suboption\n");
            return JNI_FALSE;
        }
    }

    return JNI_TRUE;

    bad_option:
    fprintf(stderr, "ERROR: bad JDWP option\n");
    bad_option_no_msg:
    fprintf(stderr, "Invalid JDWP options: %s\n", options);
    return JNI_FALSE;
}

jboolean 
debugInit_isStrict(void)
{
    return isStrict;
}

jboolean 
debugInit_useStandardAlloc(void)
{
    return useStandardAlloc;
}


JNIEXPORT void JNICALL 
JVM_OnUnload(JavaVM *vm)
{
    bagEnumerateOver(transports, stopTransport, NULL);
    eventHelper_shutdown();
    threadControl_joinAllDebugThreads();
    jvmdi->SetEventHook(NULL);
    bagEnumerateOver(transports, unloadTransport, NULL);
}
