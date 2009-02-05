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

#include <stdio.h>
#include <sys/time.h>
#include <jvmconfig.h>
#include <kni.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midpAMS.h>
#include <midpInit.h>
#include <midp_foreground_id.h>
#include <keymap_input.h>

#include <lcdlf_export.h>
#include <fbapp_export.h>

#ifdef DIRECTFB
#include <directfbapp_export.h>
#endif

/* IMPL_NOTE - CDC declarations */
CVMInt64 CVMtimeMillis(void);

static int controlPipe[2]; /* [0] for read, [1] for write */

static void initCDCEvents();

static MidpReentryData newSignal;
static MidpEvent newMidpEvent;

/* in mastermode_handle_signal.c */
void handleKey(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent);

KNIEXPORT KNI_RETURNTYPE_LONG
JVM_JavaMilliSeconds() {
    return CVMtimeMillis();
}


/**
 * Get the current Isolate ID.
 *
 * @return ID of the current Isolate
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_main_MIDletSuiteLoader_getIsolateId) {
    (void)_p_mb;
    KNI_ReturnInt(0);
}

/**
 * Get the Isolate ID of the AMS Isolate.
 *
 * @return Isolate ID of AMS Isolate
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_main_MIDletSuiteLoader_getAmsIsolateId) {
    (void) _p_mb;
    KNI_ReturnInt(0);
}

/**
 * Register the Isolate ID of the AMS Isolate by making a native
 * method call that will call JVM_CurrentIsolateId and set
 * it in the proper native variable.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteLoader_registerAmsIsolateId) {
    (void) _arguments;
    (void) _p_mb;
    KNI_ReturnVoid();
}

/**
 * Send hint to VM about the begin of MIDlet startup phase
 * to allow the VM to fine tune its internal parameters to
 * achieve optimal peformance
 *
 * @param midletIsolateId ID of the started MIDlet isolate
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteLoader_vmBeginStartUp) {
    (void) _arguments;
    (void) _p_mb;
    KNI_ReturnVoid();
}

/**
 * Send hint to VM about the end of MIDlet startup phase
 * to allow the VM to restore its internal parameters
 * changed on startup time for better performance
 *
 * @param midletIsolateId ID of the started MIDlet isolate
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteLoader_vmEndStartUp) {
    (void) _arguments;
    (void) _p_mb;
    KNI_ReturnVoid();
}


/**
 * Initializes the UI.
 *
 * @return <tt>0</tt> upon successful initialization, otherwise
 *         <tt>-1</tt>
 */
static int
midpInitializeUI(void) {
    /*
    if (InitializeEvents() != 0) {
        return -1;
    }
    */

    /*
     * Porting consideration:
     * Here is a good place to put I18N init.
     * function. e.g. initLocaleMethod();
     */

#if ENABLE_JAVA_DEBUGGER
    {
        char* argv[2];

        /* Get the VM debugger port property. */
        argv[1] = (char *)getInternalProperty("VmDebuggerPort");
        if (argv[1] != NULL) {
            argv[0] = "-port";
            (void)JVM_ParseOneArg(2, argv);
        }
    }
#endif

    /* 
        IMPL_NOTE if (pushopen() != 0) {
            return -1;
        }
    */

    lcdlf_ui_init();
    return 0;
}

/**
 * Finalizes the UI.
 */
static void
midpFinalizeUI(void) {
    lcdlf_ui_finalize();

    /*
       IMPL_NOTE: pushclose();

       FinalizeEvents();

       Porting consideration:
       Here is a good place to put I18N finalization
       function. e.g. finalizeLocaleMethod();
    */

    /*
     * Note: the AMS isolate will have been registered by a native method
     * call, so there is no corresponding midpRegisterAmsIsolateId in the
     * midpInitializeUI() function.
     */
    /* midpUnregisterAmsIsolateId(); */
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_CDCInit_initMidpNativeStates) {
    jchar jbuff[1024];
    static char conf_buff[1024], store_buff[1024];
    int max = sizeof(conf_buff) - 1;
    int len, i;

    KNI_StartHandles(2);    
    KNI_DeclareHandle(config);
    KNI_DeclareHandle(storage);
    
    KNI_GetParameterAsObject(1, config);
    KNI_GetParameterAsObject(2, storage);

    initCDCEvents();

    len = KNI_GetStringLength(config);
    if (len > max) {
        len = max;
    }
    if (len >= 0) {
        /* config != null */
        KNI_GetStringRegion(config, 0, len, jbuff);
        for (i = 0; i<len; i++) {
            conf_buff[i] = (char)jbuff[i];
        }
        conf_buff[len] = 0;
        midpSetConfigDir(conf_buff);
    }
    len = KNI_GetStringLength(storage);
    if (len > max) {
        len = max;
    }
    if (len >= 0) {
        /* sotarage != null */
        KNI_GetStringRegion(storage, 0, len, jbuff);
        for (i = 0; i<len; i++) {
            store_buff[i] = (char)jbuff[i];
        }
        store_buff[len] = 0;
        midpSetAppDir(store_buff);
    }

    if (midpInitialize() != 0) {
        printf("midpInitialize() failed\n");

    }

    if (midpInitCallback(VM_LEVEL, midpInitializeUI, midpFinalizeUI) != 0) {
        printf("midpInitCallback(VM_LEVEL, ...) failed\n");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*
 * Event handling
 */

static void initCDCEvents() {
    if (pipe(controlPipe) != 0) {
        perror("pipe(controlPipe) failed");
    }
}

/*
 * Looking up field IDs takes some time, but does not change during a VM
 * session so the field IDs can be cached to save time. These IDs are only
 * access by native methods running in the VM thread.
 */
static int eventFieldIDsObtained = 0;
static jfieldID typeFieldID;
static jfieldID intParam1FieldID;
static jfieldID intParam2FieldID;
static jfieldID intParam3FieldID;
static jfieldID intParam4FieldID;
static jfieldID stringParam1FieldID;
static jfieldID stringParam2FieldID;
static jfieldID stringParam3FieldID;
static jfieldID stringParam4FieldID;
static jfieldID stringParam5FieldID;
static jfieldID stringParam6FieldID;

/**
 * Gets the field ids of a Java event object and cache them
 * in local static storage.
 *
 * @param eventObj handle to an NativeEvent Java object
 * @param classObj handle to the NativeEvent class
 */
static void
cacheEventFieldIDs(KNIDECLARGS jobject eventObj, jclass classObj) {
    (void) _arguments;
    (void) _p_mb;

    if (eventFieldIDsObtained) {
        return;
    }

    KNI_GetObjectClass(eventObj, classObj);

    typeFieldID = KNI_GetFieldID(classObj, "type", "I");

    intParam1FieldID = KNI_GetFieldID(classObj, "intParam1", "I");
    intParam2FieldID = KNI_GetFieldID(classObj, "intParam2", "I");
    intParam3FieldID = KNI_GetFieldID(classObj, "intParam3", "I");
    intParam4FieldID = KNI_GetFieldID(classObj, "intParam4", "I");

    stringParam1FieldID = KNI_GetFieldID(classObj, "stringParam1",
                                         "Ljava/lang/String;");
    stringParam2FieldID = KNI_GetFieldID(classObj, "stringParam2",
                                         "Ljava/lang/String;");
    stringParam3FieldID = KNI_GetFieldID(classObj, "stringParam3",
                                         "Ljava/lang/String;");
    stringParam4FieldID = KNI_GetFieldID(classObj, "stringParam4",
                                         "Ljava/lang/String;");
    stringParam5FieldID = KNI_GetFieldID(classObj, "stringParam5",
                                         "Ljava/lang/String;");
    stringParam6FieldID = KNI_GetFieldID(classObj, "stringParam6",
                                         "Ljava/lang/String;");

    eventFieldIDsObtained = 1;
}

static void readControlIntField(jobject objectHandle, jfieldID fieldID) {
    int n;
    read(controlPipe[0], &n, sizeof(int));
    KNI_SetIntField(objectHandle, fieldID, n);
}

static void sendControlIntField(jobject objectHandle, jfieldID fieldID) {
    int n = KNI_GetIntField(objectHandle, fieldID);
    write(controlPipe[1], &n, sizeof(int));
#ifdef CVM_DEBUG
    printf("%d ", n);
#endif
}

static void readControlStringField(KNIDECLARGS jobject objectHandle,
                                   jfieldID fieldID) {
    int len, i;
    jchar *data;
    (void) _arguments;
    (void) _p_mb;

    read(controlPipe[0], &len, sizeof(int));

    data = (jchar*)midpMalloc(len * sizeof(jchar));
    if (data != NULL) {
        read(controlPipe[0], data, len * sizeof(jchar));
    } else {
        for (i=0; i<len; i++) {
            jchar dummy;
            read(controlPipe[0], &dummy, sizeof(jchar));
        }
        len = 0; /* IMPL_NOTE: throw out of memory */
    }

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);

    KNI_NewString(data, len, stringObj);
    KNI_SetObjectField(objectHandle, fieldID, stringObj);

    KNI_EndHandles();
}

static void sendControlStringField(KNIDECLARGS jobject objectHandle,
                                   jobject stringObj, jfieldID fieldID) {
    int len;
    jchar *data = NULL;
    (void) _arguments;
    (void) _p_mb;
    (void) _ee;

    KNI_GetObjectField(objectHandle, fieldID, stringObj);
    len = KNI_GetStringLength(stringObj);
    if (len > 0) {
        data = (jchar*)midpMalloc(len * sizeof(jchar));
        if (data == NULL) {
            len = 0; /* IMPL_NOTE: throw out of memory */
        } else {
            KNI_GetStringRegion(stringObj, 0, len, data);
        }
    }

    write(controlPipe[1], &len, sizeof(int));
    if (len > 0) {
        write(controlPipe[1], data, len * sizeof(jchar));
        midpFree(data);
    }
}

/**
 * Blocks the Java event thread until an event has been queued and returns
 * that event and the number of events still in the queue.
 *
 * @param event An empty event to be filled in if there is a queued
 *              event.
 *
 * @return number of events waiting in the native queue
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_events_NativeEventMonitor_waitForNativeEvent) {
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    int num_fds = 0, num_ready;
    /* IMPL_NOTE    jlong sec, usec; */
    /* IMPL_NOTE    struct timeval timeout; */
#ifdef DIRECTFB
    int keyboard_has_event = 0;
#else
    int keyboardFd = fbapp_get_keyboard_fd();
#endif /* defined(DIRECTFB) */
    int done = 0;

    do {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);

        FD_SET(controlPipe[0], &read_fds);
        if (num_fds <= controlPipe[0]) {
            num_fds = controlPipe[0] + 1;
        }

#ifndef DIRECTFB
        keyboardFd = fbapp_get_keyboard_fd();
        if (keyboardFd >= 0) {
            FD_SET(keyboardFd, &read_fds);
            if (num_fds <= keyboardFd) {
                num_fds = keyboardFd + 1;
            }
        }

        CVMD_gcSafeExec(_ee, {
            num_ready= select(num_fds+1, &read_fds, &write_fds, &except_fds,
                              NULL);
        });
#else
        /* We should get events from two different sources:
         * from controlPipe and from DirectFB's event queue (keyboard events).
         * At first we check for keyboard events. If this queue is empty
         * we wait 0.2s for pipe's events.
         */
        if (!directfbapp_event_is_waiting()) {
            struct timeval timeout;        
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000; /* wait 0.2 sec */
            /* When the timeout expires num_ready will contain 0 */
            CVMD_gcSafeExec(_ee, {
                num_ready= select(num_fds+1, &read_fds, &write_fds, &except_fds,
                                  &timeout);
            });
        } else {
            keyboard_has_event = 1;
            num_ready = 0;
        }
#endif /* !defined(DIRECTFB) */

        KNI_StartHandles(3);
        KNI_DeclareHandle(eventObj);
  /* IMPL_NOTE        KNI_DeclareHandle(stringObj); */
        KNI_DeclareHandle(classObj);

        KNI_GetParameterAsObject(1, eventObj);
        cacheEventFieldIDs(KNIPASSARGS eventObj, classObj);

        if (num_ready >= 0) {
#ifdef DIRECTFB
            /* check if a keyboard event was received while we was waiting in the select */
            if (keyboard_has_event || directfbapp_event_is_waiting()) {
                keyboard_has_event = 0;
#else
            if (FD_ISSET(keyboardFd, &read_fds)) {
#endif /* defined(DIRECTFB) */
                newSignal.waitingFor = 0;
                newSignal.pResult = NULL;
                MIDP_EVENT_INITIALIZE(newMidpEvent);

                handleKey(&newSignal, &newMidpEvent);
                /* Need to set the event DISPLAY (intParam4) to forground. See
                   midpStoreEventAndSignalForeground() in midpEventUtil.c. */
                newMidpEvent.DISPLAY = gForegroundDisplayId;
                if (newSignal.waitingFor == UI_SIGNAL
                    || newSignal.waitingFor == AMS_SIGNAL) {
                    KNI_SetIntField(eventObj, typeFieldID, newMidpEvent.type);
                    KNI_SetIntField(eventObj, intParam1FieldID,
                                    newMidpEvent.intParam1);
                    KNI_SetIntField(eventObj, intParam2FieldID,
                                    newMidpEvent.intParam2);
                    KNI_SetIntField(eventObj, intParam3FieldID,
                                    newMidpEvent.intParam3);
                    KNI_SetIntField(eventObj, intParam4FieldID,
                                    newMidpEvent.intParam4);

#ifdef CVM_DEBUG
                    printf("Got key%s: %d\n",
                           (newMidpEvent.ACTION == KEYMAP_STATE_PRESSED) ? 
                           "down" : "up",
                           newMidpEvent.CHR);
#endif
                    done = 1;
                }
            } else if (FD_ISSET(controlPipe[0], &read_fds)) {
                readControlIntField(eventObj, typeFieldID);
                readControlIntField(eventObj, intParam1FieldID);
                readControlIntField(eventObj, intParam2FieldID);
                readControlIntField(eventObj, intParam3FieldID);
                readControlIntField(eventObj, intParam4FieldID);

                readControlStringField(KNIPASSARGS eventObj,
                                       stringParam1FieldID);
                readControlStringField(KNIPASSARGS eventObj,
                                       stringParam2FieldID);
                readControlStringField(KNIPASSARGS eventObj,
                                       stringParam3FieldID);
                readControlStringField(KNIPASSARGS eventObj,
                                       stringParam4FieldID);
                readControlStringField(KNIPASSARGS eventObj,
                                       stringParam5FieldID);
                readControlStringField(KNIPASSARGS eventObj,
                                       stringParam6FieldID);
                done = 1;
            }
        }
        KNI_EndHandles();

    } while (!done);

    KNI_ReturnInt(0);
}

#ifdef ENABLE_DEBUG
static const char * const eventNames[] = {
    "???_EVENT",    /* 0 */
    "KEY_EVENT",  /* 1 */
    "PEN_EVENT",  /* 2 */
    "COMMAND_EVENT",  /* 3 */
    "REPAINT_EVENT",  /* 4 */
    "SCREEN_CHANGE_EVENT",  /* 5 */
    "INVALIDATE_EVENT", /* 6 */
    "ITEM_EVENT", /* 7 */
    "PEER_CHANGED_EVENT", /* 8 */
    "CALL_SERIALLY_EVENT",  /* 9 */
    "FOREGROUND_NOTIFY_EVENT",  /* 10 */
    "ACTIVATE_MIDLET_EVENT",  /* 11 */
    "PAUSE_MIDLET_EVENT", /* 12 */
    "DESTROY_MIDLET_EVENT", /* 13 */
    "SHUTDOWN_EVENT", /* 14 */
    "ACTIVATE_ALL_EVENT", /* 15 */
    "PAUSE_ALL_EVENT",  /* 16 */
    "MIDLET_CREATED_NOTIFICATION",  /* 17 */
    "MIDLET_ACTIVE_NOTIFICATION", /* 18 */
    "MIDLET_PAUSED_NOTIFICATION", /* 19 */
    "MIDLET_DESTROYED_NOTIFICATION",  /* 20 */
    "FOREGROUND_REQUEST_EVENT", /* 21 */
    "BACKGROUND_REQUEST_EVENT", /* 22 */
    "SELECT_FOREGROUND_EVENT",  /* 23 */
    "PREEMPT_EVENT",  /* 24 */
    "MIDLET_START_ERROR_EVENT", /* 25 */
    "EXECUTE_MIDLET_EVENT", /* 26 */
    "MIDLET_DESTROY_REQUEST_EVENT", /* 27 */
    "FOREGROUND_TRANSFER_EVENT",  /* 28 */
    "EVENT_QUEUE_SHUTDOWN", /* 29 */
    "FATAL_ERROR_NOTIFICATION", /* 30 */
    "MM_EOM_EVENT", /* 31 */
    "MM_SAT_EVENT", /* 32 */
    "MM_TONEEOM_EVENT", /* 33 */
    "FC_DISKS_CHANGED_EVENT", /* 34 */
    "TEST_EVENT", /* 35 */
    "MIDLET_RESUME_REQUEST",  /* 36 */
    "NATIVE_MIDLET_EXECUTE_REQUEST",  /* 37 */
    "NATIVE_MIDLET_RESUME_REQUEST", /* 38 */
    "???_EVENT",
    "NATIVE_MIDLET_PAUSE_REQUEST",  /* 40 */
    "NATIVE_MIDLET_DESTROY_REQUEST",  /* 41 */
    "NATIVE_SET_FOREGROUND_REQUEST",  /* 42 */
    "SET_FOREGROUND_BY_NAME_REQUEST"  /* 43 */
};

static const int numEvents = sizeof(eventNames) / sizeof(eventNames[0]);

static const char*  getEventNameBasic(int eventId)
{
    if (eventId >= numEvents) {
  return "<illegal event id>";
    } else {
  return eventNames[eventId];
    }
}

static const char*  getEventName(KNIDECLARGS jobject objectHandle,
         jfieldID fieldID)
{
    int eventId = KNI_GetIntField(objectHandle, fieldID);
    return getEventNameBasic(eventId);
}

#else
static const char*  getEventNameBasic(int eventId)
{
    return "";
}

static const char*  getEventName(KNIDECLARGS jobject objectHandle, jfieldID fieldID)
{
    return "";
}
#endif


/**
 * Sends a native event a given Isolate.
 *
 * @param event A event to queued
 * @param isolateId ID of the target Isolate
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_events_EventQueue_sendNativeEventToIsolate) {
    KNI_StartHandles(3);

    KNI_DeclareHandle(eventObj);
    KNI_DeclareHandle(stringObj);
    KNI_DeclareHandle(classObj);

    KNI_GetParameterAsObject(1, eventObj);
    cacheEventFieldIDs(KNIPASSARGS eventObj, classObj);

#ifdef CVM_DEBUG
    /* IMPL_NOTE: Remove the printf. Or we should make it as a trace */
    printf("sendNativeEventToIsolate====: ");
    printf("(type=%s) ", getEventName(KNIPASSARGS eventObj, typeFieldID));
#endif
    sendControlIntField(eventObj, typeFieldID);
    sendControlIntField(eventObj, intParam1FieldID);
    sendControlIntField(eventObj, intParam2FieldID);
    sendControlIntField(eventObj, intParam3FieldID);
    sendControlIntField(eventObj, intParam4FieldID);
#ifdef CVM_DEBUG
    printf("\n");
#endif

    sendControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam1FieldID);
    sendControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam2FieldID);
    sendControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam3FieldID);
    sendControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam4FieldID);
    sendControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam5FieldID);
    sendControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam6FieldID);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#ifdef CVM_DEBUG
void dummy_called() {}
#define DUMMY(x) void x() {printf("dummy: %s\n", #x); dummy_called();}
#else
#define DUMMY(x) void x() {}
#endif

DUMMY(platformRequest)
DUMMY(handleFatalError)
DUMMY(NotifySocketStatusChanged)

DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_open0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_read0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_write0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_available0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_close0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_finalize)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_getIpNumber0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_getHost0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_getPort0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_getSockOpt0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_setSockOpt0)
DUMMY(CNIcom_sun_midp_io_j2me_socket_Protocol_shutdownOutput0)
DUMMY(CNIcom_sun_midp_io_NetworkConnectionBase_initializeInternal)

DUMMY(CNIcom_sun_midp_events_EventQueue_handleFatalError)
DUMMY(CNIcom_sun_midp_events_NativeEventMonitor_readNativeEvent)

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_events_EventQueue_resetNativeEventQueue) {
    (void) _arguments;
    (void) _p_mb;
     KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_events_EventQueue_getNativeEventQueueHandle) {
    (void) _p_mb;
    KNI_ReturnInt(0);
}

DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_checkInByMidlet0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_add0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_getMIDlet0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_getEntry0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_addAlarm0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_del0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_checkInByName0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_checkInByHandle0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_list0)
DUMMY(CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_delAllForSuite0)

DUMMY(CNIcom_sun_midp_crypto_MD5_nativeUpdate)
DUMMY(CNIcom_sun_midp_crypto_MD5_nativeFinal)
DUMMY(CNIcom_sun_midp_crypto_MD2_nativeUpdate)
DUMMY(CNIcom_sun_midp_crypto_MD2_nativeFinal)
DUMMY(CNIcom_sun_midp_crypto_SHA_nativeUpdate)
DUMMY(CNIcom_sun_midp_crypto_SHA_nativeFinal)
DUMMY(CNIcom_sun_midp_main_MIDletSuiteVerifier_getJarHash)
DUMMY(CNIcom_sun_midp_main_MIDletSuiteVerifier_checkJarHash)
DUMMY(CNIcom_sun_midp_main_MIDletSuiteVerifier_useClassVerifier)
DUMMY(CNIcom_sun_midp_main_MIDletAppImageGenerator_removeAppImage)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_getHandler)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_getMaxByteLength)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_getByteLength)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_byteToChar)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_charToByte)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_sizeOfByteInUnicode)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_sizeOfUnicodeInByte)

/* IMPL_NOTE removed
DUMMY(lockStorage)
DUMMY(lock_storage)
DUMMY(unlockStorage)
DUMMY(unlock_storage)

DUMMY(findStorageLock)
DUMMY(find_storage_lock)
DUMMY(removeAllStorageLock)
DUMMY(removeStorageLock)
DUMMY(remove_storage_lock)
DUMMY(pushdeletesuite)
*/

DUMMY(midpStoreEventAndSignalForeground)

int getCurrentIsolateId() {return 0;}

int midpGetAmsIsolateId() {return 0;}

/* IMPL_NOTE - removed duplicate
 * DUMMY(midp_getCurrentThreadId)
 */
