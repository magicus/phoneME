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
#include <io.h>
#include <kni.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midpAMS.h>
#include <midpInit.h>
#include <midp_foreground_id.h>
#include <keymap_input.h>


/* IMPL_NOTE - CDC declarations */
CVMInt64 CVMtimeMillis(void);

static int controlPipe[2]; /* [0] for read, [1] for write */

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

    /*    lcdlf_ui_init();*/
    return 0;
}

/**
 * Finalizes the UI.
 */
static void
midpFinalizeUI(void) {
  /*   lcdlf_ui_finalize();*/

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

static void initCDCEvents() {
    if (_pipe(controlPipe, 1024, O_BINARY) != 0) {
        perror("pipe(controlPipe) failed");
    }
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
    for (i=0; i<len; i++) {
        conf_buff[i] = (char)jbuff[i];
    }
    conf_buff[len] = 0;
    len = KNI_GetStringLength(storage);
    if (len > max) {
        len = max;
    }
    KNI_GetStringRegion(storage, 0, len, jbuff);
    for (i=0; i<len; i++) {
        store_buff[i] = (char)jbuff[i];
    }
    store_buff[len] = 0;

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


DUMMY(CNIcom_sun_midp_events_EventQueue_finalize)
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

