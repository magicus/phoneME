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
#include <kni_globals.h>
#include <midpServices.h>
#include <midp_thread.h>
#include <midpError.h>
#include <midp_properties_port.h>
#include <midpMalloc.h>

#include "carddevice.h"


/**
 * Device locking flag
 */
static jboolean locked = KNI_FALSE;

/** Configuration exception */
static char midpCardDeviceException[] = 
    "com/sun/cardreader/CardDeviceException";

/** Configuration property name */
static char hostsandports[] = "com.sun.midp.io.j2me.apdu.hostsandports";

/**
 * Initializes the device.
 * <p>Java declaration:
 * <pre>
 * private native boolean init0();
 * </pre>
 * @return KNI_TRUE in case of success, else KNI_FALSE 
 * @throws CardDeviceException If configuration failed.
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_init0() {
    jboolean retcode = KNI_FALSE;
    JSR177_STATUSCODE status;
    jbyte *err_msg;
    const jbyte *prop_value;
    
    prop_value = getInternalProperty(hostsandports);
    if (prop_value != NULL) {
        status = jsr177_set_property(hostsandports, prop_value);
        if (status == JSR177_STATUSCODE_NOT_IMPLEMENTED) {
            if (jsr177_get_error(gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "Required property not supported";
            }
            KNI_ThrowNew(midpCardDeviceException, err_msg);
            goto end;
        }
        if (status == JSR177_STATUSCODE_OUT_OF_MEMORY) {
            if (jsr177_get_error(gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "init0()";
            }
            KNI_ThrowNew(midpOutOfMemoryError, err_msg);
            goto end;
        }
        if (status != JSR177_STATUSCODE_OK) {
            if (jsr177_get_error(gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "Invalid 'hostsandports' property";
            }
            KNI_ThrowNew(midpCardDeviceException, err_msg);
            goto end;
        }
    }
    if ((status = jsr177_init()) == JSR177_STATUSCODE_OK) {
        locked = KNI_FALSE;
        jsr177_clear_error();
        retcode = KNI_TRUE;
    } else
    if (status == JSR177_STATUSCODE_NOT_IMPLEMENTED) {
        
        /* We throw special exception to tell i3tests to skip real testing*/
        KNI_ThrowNew(midpCardDeviceException, "stub");
        retcode = KNI_TRUE;
    }
end:
    KNI_ReturnInt(retcode);
}

/**
 * Finalizes the device.
 * <p>Java declaration:
 * <pre>
 * private native void finalize0();
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_cardreader_PlatformCardDevice_finalize0() {
    locked = KNI_FALSE;
    jsr177_finalize();
    KNI_ReturnVoid();
}

/* This constant is used only in jsr177_lock and jsr177_unlock functions. */
#define SIGNAL_LOCK     0x7783

/**
 * Performs platform lock of the device.
 * <p>Java declaration:
 * <pre>
 * private native boolean lock0();
 * </pre>
 * @return KNI_TRUE in case of success, else KNI_FALSE 
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_lock0() {
    JSR177_STATUSCODE lock_retcode = JSR177_STATUSCODE_OK;
    jboolean retcode = KNI_FALSE;
    
    if (!locked  &&  ((lock_retcode=jsr177_lock()) == JSR177_STATUSCODE_OK)) {
        locked = KNI_TRUE;
        jsr177_clear_error();
        retcode = KNI_TRUE;
    } else 
    if (!locked  &&  (lock_retcode != JSR177_STATUSCODE_WOULD_BLOCK)) {
        retcode = KNI_FALSE;
    } else {
        midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, NULL); 
    }
    KNI_ReturnInt(retcode);
}

/**
 * Unlocks the device.
 * <p>Java declaration:
 * <pre>
 * private native boolean unlock0();
 * </pre>
 * @return KNI_TRUE in case of success, else KNI_FALSE 
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_unlock0() {
    jboolean retcode = KNI_FALSE;
    
    if (locked  &&  (jsr177_unlock() == JSR177_STATUSCODE_OK)) {
        locked = KNI_FALSE;
        midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, 0);
        jsr177_clear_error();
        retcode = KNI_TRUE;
    }
    
    KNI_ReturnInt(retcode);
}

/**
 * Selects the current slot for the subsequent transfer operations.
 * <p>Java declaration:
 * <pre>
 * private native boolean selectSlot0(int slotIndex);
 * </pre>
 * @param slotIndex Slot number
 * @return KNI_TRUE in case of success, else KNI_FALSE 
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_selectSlot0() {
    jint slotIndex;
    jboolean retcode;
    
    if (!locked) {
        jsr177_set_error("Device not locked");
        KNI_ReturnInt(KNI_FALSE);
    }
    slotIndex = KNI_GetParameterAsInt(1);
    if (jsr177_select_slot(slotIndex) != JSR177_STATUSCODE_OK)
        retcode = KNI_FALSE;
    else
        retcode = KNI_TRUE;
    KNI_ReturnInt(retcode);
}

/**
 * Gets number of slots on a device. 
 * <p>Java declaration:
 * <pre>
 * private native int getSlotCount0();
 * </pre>
 * @return Number of slots on a device. In case of error
 * returns 0.
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_getSlotCount0() {
    jint retcode;
    jint num;
    if (jsr177_get_slot_count(&num) == JSR177_STATUSCODE_OK) {
        if (num > 0) {
            retcode = num;
        } else {
            retcode = 0;
        }
    } else {
        retcode = 0;
    }
    KNI_ReturnInt(retcode);
}

/**
 * Checks if this slot is SAT slot. 
 * <p>Java declaration:
 * <pre>
 * private native int isSatSlot0(int slotNumber);
 * </pre>
 * @param slotNumber Slot number
 * @return <code> 1</code> if the slot is dedicated for SAT,
 *         <code> 0</code> if not,
 *         <code>-1</code> if any error occured
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_isSatSlot0() {
    jint retcode;
    jboolean result;
    int slotIndex = KNI_GetParameterAsInt(1);
    if (jsr177_is_sat(slotIndex, &result) == JSR177_STATUSCODE_OK) {
        if (result == PCSL_FALSE) {
            retcode = 0;
        } else {
            retcode = 1;
        }
    } else {
        retcode = -1;
    }
    KNI_ReturnInt(retcode);
}

/**
 * Performs reset of the device.
 * <p>Java declaration:
 * <pre>
 * private native int reset0(byte[] atr);
 * </pre>
 * @param atr ATR bytes
 * @return Length of ATR in case of success, else -1 
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_reset0() {
    jint retcode;
    jsize atr_length;
    jbyte *atr_buffer;
    MidpReentryData* info;
    void *context = NULL;
    JSR177_STATUSCODE status_code;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(atr_handle);
    
    if (!locked) {
        jsr177_set_error("Device not locked");
        retcode = -1;
        goto end;
    }
    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    
    KNI_GetParameterAsObject(1, atr_handle);
    if (KNI_IsNullHandle(atr_handle)) {
        atr_buffer = NULL;
        atr_length = 0;
    } else {
        atr_length = KNI_GetArrayLength(atr_handle);
        atr_buffer = SNI_GetRawArrayPointer(atr_handle);
    }
    
    if (info == NULL) {
        status_code = jsr177_reset_start(atr_buffer, &atr_length, &context);
    } else {
        context = info->pResult;
        status_code = jsr177_reset_finish(atr_buffer, &atr_length, context);
    }
    if (status_code == JSR177_STATUSCODE_WOULD_BLOCK) {
        midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_RESET, context);
        goto end;
    }
    if (status_code != JSR177_STATUSCODE_OK) {
        retcode = -1;
    } else {
        retcode = atr_length;
    }
end:
    KNI_EndHandles();
    KNI_ReturnInt(retcode);
}

/**
 * Performs data transfer to the device.
 * <p>Java declaration:
 * <pre>
 * private native int cmdXfer0(byte[] request, byte[] response);
 * </pre>
 * @param request Buffer with request data
 * @param response Buffer for response data
 * @return Length of response in case of success, else -1
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_cardreader_PlatformCardDevice_cmdXfer0() {
    jint retcode;
    jsize tx_length, rx_length;
    jbyte *tx_buffer, *rx_buffer;
    MidpReentryData* info;
    void *context = NULL;
    JSR177_STATUSCODE status_code;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(request_handle);
    KNI_DeclareHandle(response_handle);
    
    if (!locked) {
        jsr177_set_error("Device not locked");
        retcode = -1;
        goto end;
    }
    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    
    KNI_GetParameterAsObject(1, request_handle);
    if (KNI_IsNullHandle(request_handle)) {
        tx_buffer = NULL;
        tx_length = 0;
        retcode = -1;
        goto end;
    } else {
        tx_length = KNI_GetArrayLength(request_handle);
        tx_buffer = SNI_GetRawArrayPointer(request_handle);
    }
    
    KNI_GetParameterAsObject(2, response_handle);
    if (KNI_IsNullHandle(response_handle)) {
        rx_buffer = NULL;
        rx_buffer = 0;
        retcode = -1;
        goto end;
    } else {
        rx_length = KNI_GetArrayLength(response_handle);
        rx_buffer = SNI_GetRawArrayPointer(response_handle);
    }
    
    if (tx_length > 5) {
        jsize apdu_len = 5 + (tx_buffer[4]&0xFF) + 1;
        if (tx_length > apdu_len) {
            tx_length = apdu_len;
        }
    }

    if (info == NULL) {
        status_code = jsr177_xfer_data_start(tx_buffer, 
                                             tx_length, 
                                             rx_buffer, 
                                             &rx_length, &context);
    } else {
        context = info->pResult;
        status_code = jsr177_xfer_data_finish(tx_buffer, 
                                              tx_length, 
                                              rx_buffer, 
                                              &rx_length, context);
    }
    if (status_code == JSR177_STATUSCODE_WOULD_BLOCK) {
        midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_XFER, context);
        goto end;
    }
    if (status_code != JSR177_STATUSCODE_OK) {
        retcode = -1;
    } else {
        retcode = rx_length;
    }

end:
    KNI_EndHandles();
    KNI_ReturnInt(retcode);
}

/**
 * Retrives error message clears error state.
 * <p>Java declaration:
 * <pre>
 * private native String getErrorMessage0();
 * </pre>
 * @return Error message string
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_cardreader_PlatformCardDevice_getErrorMessage0() {
    jbyte err_msg[1024];
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(err_msg_handle);
    
    if (jsr177_get_error(err_msg, (jsize)sizeof err_msg) == PCSL_TRUE) {
        KNI_NewStringUTF(err_msg, err_msg_handle);
    }
    
    KNI_EndHandlesAndReturnObject(err_msg_handle);
}

/**
 * Checks if the card in the selected slot was changed 
 * since last call or since last reset.
 * <p>Java declaration:
 * <pre>
 * private native int checkCardMovement0();
 * </pre>
 * @return 0 if no movements has happened, 
 *         > 0 if the card was changed,
 *         < 0 if an error occured.
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cardreader_PlatformCardDevice_checkCardMovement0() {
    JSR177_CARD_MOVEMENT mv = 0;
    jint retcode = jsr177_card_movement_events(&mv);
    if (retcode == JSR177_STATUSCODE_OK) {
        retcode = (mv & JSR177_CARD_MOVEMENT_MASK);
    }
    KNI_ReturnInt(retcode);
}
