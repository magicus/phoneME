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
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midpAMS.h>
#include <midpInit.h>
#include <midp_mastermode_port.h>
#include <midp_foreground_id.h>
#include <keymap_input.h>

#include <windows.h>
#include <Msgqueue.h>

static HANDLE controlPipe[2]; // [0] for read, [1] for write
static MidpReentryData newSignal;
static MidpEvent newMidpEvent;

static void initCDCEvents();


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
    /*if (InitializeEvents() != 0) {*/
    /*    return -1;*/
    /*}*/

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

    /* IMPL_NOTE if (pushopen() != 0) {*/
    /*    return -1;*/
    /*}*/

    lcdlf_ui_init();
    return 0;
}

/**
 * Finalizes the UI.
 */
static void
midpFinalizeUI(void) {
    lcdlf_ui_finalize();

    /*IMPL_NOTE: pushclose();*/
    /*finalizeCommandState();*/

    /*FinalizeEvents();*/

    /* Porting consideration:*/
    /* Here is a good place to put I18N finalization*/
    /*function. e.g. finalizeLocaleMethod();*/

    /*
     * Note: the AMS isolate will have been registered by a native method
     * call, so there is no corresponding midpRegisterAmsIsolateId in the
     * midpInitializeUI() function.
     */
    /*midpUnregisterAmsIsolateId();*/
    CloseMsgQueue(controlPipe[0]);
    CloseMsgQueue(controlPipe[1]);
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
	
    len = KNI_GetStringLength(config);
    if (len > max) {
        len = max;
    }
    KNI_GetStringRegion(config, 0, len, jbuff);
    jbuff[len]=0;
    convertWtoU8(conf_buff, jbuff);
    len = KNI_GetStringLength(storage);
    if (len > max) {
        len = max;
    }
    KNI_GetStringRegion(storage, 0, len, jbuff);
    jbuff[len]=0;
    convertWtoU8(store_buff, jbuff);

    initCDCEvents();

    midpSetAppDir(store_buff);
    midpSetConfigDir(conf_buff);
    
    if (midpInitialize() != 0) {
        printf("midpInitialize() failed\n");

    }

    if (midpInitCallback(VM_LEVEL, midpInitializeUI, midpFinalizeUI) != 0) {
        printf("midpInitCallback(VM_LEVEL, ...) failed\n");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

static MSGQUEUEOPTIONS  readEventQueueOptions;
static MSGQUEUEOPTIONS  writeEventQueueOptions;

/*
 * Event handling
 */
static void initCDCEvents() {
    DWORD err;

    enum {
        MAX_MESSAGE_SIZE = 1024
    };

    readEventQueueOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    readEventQueueOptions.dwFlags = MSGQUEUE_NOPRECOMMIT;
    readEventQueueOptions.dwMaxMessages = 0;
    readEventQueueOptions.cbMaxMessage = MAX_MESSAGE_SIZE;
    readEventQueueOptions.bReadAccess = TRUE;
    /* Read*/
    controlPipe[0] = CreateMsgQueue(TEXT("EventQueue"), &readEventQueueOptions);
    err = GetLastError();
    if (err != ERROR_SUCCESS) {
        NKDbgPrintfW(TEXT("Creating Read Queue Failed\n"));
        return;
    }

    writeEventQueueOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    writeEventQueueOptions.dwFlags = MSGQUEUE_NOPRECOMMIT;
    writeEventQueueOptions.dwMaxMessages = 0;
    writeEventQueueOptions.cbMaxMessage = MAX_MESSAGE_SIZE;
    writeEventQueueOptions.bReadAccess = FALSE;
    /* Write*/
    controlPipe[1] = OpenMsgQueue(GetCurrentProcess(), controlPipe[0], &writeEventQueueOptions);
    err = GetLastError();
    if (err != ERROR_SUCCESS) {
        NKDbgPrintfW(TEXT("Creating Write Queue Failed\n"));
        return;
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
static jfieldID intParam5FieldID;
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
    intParam5FieldID = KNI_GetFieldID(classObj, "intParam5", "I");


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

static void getControlStringField(KNIDECLARGS jobject objectHandle,
                                   jfieldID fieldID, pcsl_string *str) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);

    if (str->length > 0) {
        KNI_NewString(str->data, str->length, stringObj);
        KNI_SetObjectField(objectHandle, fieldID, stringObj);
    }

    KNI_EndHandles();
}

static void setControlStringField(KNIDECLARGS jobject objectHandle,
                                   jobject stringObj, jfieldID fieldID, pcsl_string *str) {

    KNI_GetObjectField(objectHandle, fieldID, stringObj);
    str->length = KNI_GetStringLength(stringObj);
    if (str->length> 0) {
        str->data = (jchar*)midpMalloc(str->length * sizeof(jchar));
        if (str->data == NULL) {
            str->length = 0; // IMPL_NOTE: throw out of memory
        } else {
            KNI_GetStringRegion(stringObj, 0, str->length, str->data);
        }
    } else {
        str->data = NULL;
    }
}

static BOOL sendMidpEvent(MidpEvent* event, int size) {
    if (WriteMsgQueue(controlPipe[1], &size, sizeof(int), INFINITE, 0)) {
        if (WriteMsgQueue(controlPipe[1], event, size, INFINITE, 0)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL readMidpEvent(MidpEvent* event) {
    DWORD size;
    DWORD flags;
    int eventSize = 0;

    if (ReadMsgQueue(controlPipe[0], &eventSize, sizeof(int), &size, INFINITE, &flags)) {
        if (ReadMsgQueue(controlPipe[0], event, eventSize, &size, INFINITE, &flags)) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL sendMidpKeyEvent(MidpEvent* event, int size) {

    event->stringParam1.length = 0;
    event->stringParam2.length = 0;
    event->stringParam3.length = 0;
    event->stringParam4.length = 0;
    event->stringParam5.length = 0;
    event->stringParam6.length = 0;

    return sendMidpEvent(event, size);
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
    DWORD size;
    DWORD flags;
    DWORD ready;
    MidpEvent event;
    MSG msg;
    int eventSize = 0;
    int done = 0;

    do {
        CVMD_gcSafeExec(_ee, {
            ready = WaitForSingleObject(controlPipe[0], 50);
        });

        KNI_StartHandles(3);
        KNI_DeclareHandle(eventObj);
        KNI_DeclareHandle(stringObj);
        KNI_DeclareHandle(classObj);

        KNI_GetParameterAsObject(1, eventObj);
        cacheEventFieldIDs(KNIPASSARGS eventObj, classObj);

        if (ready == WAIT_OBJECT_0) {
            if (readMidpEvent(&event)) {
                KNI_SetIntField(eventObj, typeFieldID, event.type);
                KNI_SetIntField(eventObj, intParam1FieldID, event.intParam1);
                KNI_SetIntField(eventObj, intParam2FieldID, event.intParam2);
                KNI_SetIntField(eventObj, intParam3FieldID, event.intParam3);
                KNI_SetIntField(eventObj, intParam4FieldID, event.intParam4);
                KNI_SetIntField(eventObj, intParam5FieldID, event.intParam5);

                getControlStringField(KNIPASSARGS eventObj, stringParam1FieldID, &event.stringParam1);
                getControlStringField(KNIPASSARGS eventObj, stringParam2FieldID, &event.stringParam2);
                getControlStringField(KNIPASSARGS eventObj, stringParam3FieldID, &event.stringParam3);
                getControlStringField(KNIPASSARGS eventObj, stringParam4FieldID, &event.stringParam4);
                getControlStringField(KNIPASSARGS eventObj, stringParam5FieldID, &event.stringParam5);
                getControlStringField(KNIPASSARGS eventObj, stringParam6FieldID, &event.stringParam6);
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

static const char*  getEventName(KNIDECLARGS jobject objectHandle, jfieldID fieldID)
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
    int eventSize;
    MidpEvent event;

    KNI_StartHandles(3);

    KNI_DeclareHandle(eventObj);
    KNI_DeclareHandle(stringObj);
    KNI_DeclareHandle(classObj);

    KNI_GetParameterAsObject(1, eventObj);
    cacheEventFieldIDs(KNIPASSARGS eventObj, classObj);

    event.type   = KNI_GetIntField(eventObj, typeFieldID);
    event.intParam1 = KNI_GetIntField(eventObj, intParam1FieldID);
    event.intParam2 = KNI_GetIntField(eventObj, intParam2FieldID);
    event.intParam3 = KNI_GetIntField(eventObj, intParam3FieldID);
    event.intParam4 = KNI_GetIntField(eventObj, intParam4FieldID);
    event.intParam5 = KNI_GetIntField(eventObj, intParam5FieldID);


    setControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam1FieldID, &event.stringParam1);

    setControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam2FieldID, &event.stringParam2);

    setControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam3FieldID, &event.stringParam3);

    setControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam4FieldID, &event.stringParam4);

    setControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam5FieldID, &event.stringParam5);

    setControlStringField(KNIPASSARGS eventObj, stringObj,
                           stringParam6FieldID, &event.stringParam6);

    eventSize = sizeof(event);

    sendMidpEvent(&event, eventSize);

    midpFree(event.stringParam1.data);
    midpFree(event.stringParam2.data);
    midpFree(event.stringParam3.data);
    midpFree(event.stringParam4.data);
    midpFree(event.stringParam5.data);
    midpFree(event.stringParam6.data);

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

DUMMY(CNIcom_sun_midp_main_CommandState_exitInternal)
DUMMY(CNIcom_sun_midp_crypto_MD5_nativeUpdate)
DUMMY(CNIcom_sun_midp_crypto_MD5_nativeFinal)
DUMMY(CNIcom_sun_midp_crypto_MD2_nativeUpdate)
DUMMY(CNIcom_sun_midp_crypto_MD2_nativeFinal)
DUMMY(CNIcom_sun_midp_crypto_SHA_nativeUpdate)
DUMMY(CNIcom_sun_midp_crypto_SHA_nativeFinal)
DUMMY(CNIcom_sun_midp_main_MIDletSuiteVerifier_getJarHash)
DUMMY(CNIcom_sun_midp_main_MIDletSuiteVerifier_useClassVerifier)
DUMMY(CNIcom_sun_midp_main_MIDletSuiteVerifier_checkJarHash)
DUMMY(CNIcom_sun_midp_main_MIDletAppImageGenerator_removeAppImage)
DUMMY(CNIcom_sun_midp_pause_PauseSystem_00024MIDPSystem_paused)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_getHandler)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_getMaxByteLength)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_getByteLength)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_byteToChar)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_charToByte)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_sizeOfByteInUnicode)
DUMMY(CNIcom_sun_cdc_i18n_j2me_Conv_sizeOfUnicodeInByte)

void CNIcom_sun_midp_io_j2me_push_PushRegistryImpl_poll0() {
}

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


