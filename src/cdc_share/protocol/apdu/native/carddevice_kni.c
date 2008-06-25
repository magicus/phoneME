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
#include <jsrop_exceptions.h>
#include <javacall_os.h>
#include <javacall_carddevice.h>
#include <internal_props.h>

/** Configuration exception */
static char cardDeviceException[] = 
    "com/sun/cardreader/CardDeviceException";

/** Configuration property name */
static char hostsandports[] = "com.sun.io.j2me.apdu.hostsandports";
static char satselectapdu[] = "com.sun.io.j2me.apdu.satselectapdu";

/** Thread synchronization variables */
static javacall_mutex cardReaderMutex;
static javacall_cond cardReaderCond;

#define PROP_BUF_SIZE 128

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
KNIDECL (com_sun_cardreader_PlatformCardDevice_init0) {
    jboolean retcode = KNI_FALSE;
    javacall_result status;
    char *err_msg;
    char *buffer;
    char prop_buf[PROP_BUF_SIZE];
    const char *prop_value;
    
    prop_value = getInternalProp(hostsandports, prop_buf, PROP_BUF_SIZE);
    if (prop_value != NULL) {
        status = javacall_carddevice_set_property(hostsandports, prop_value);
        if (status != JAVACALL_OK) {
            goto err;
        }

        prop_value = getInternalProp(satselectapdu, prop_buf, PROP_BUF_SIZE);
        status = javacall_carddevice_set_property(satselectapdu, prop_value);
        if (status != JAVACALL_OK) {
            goto err;
        }
    }

    cardReaderMutex = javacall_os_mutex_create();
    if (cardReaderMutex == NULL) {
        goto end;
    }

    cardReaderCond = javacall_os_cond_create(cardReaderMutex);
    if (cardReaderCond == NULL) {
        goto end;
    }

    if ((status = javacall_carddevice_init()) == JAVACALL_OK) {
        javacall_carddevice_clear_error();
        retcode = KNI_TRUE;
    } else
    if (status == JAVACALL_NOT_IMPLEMENTED) {
        
        /* We throw special exception to tell i3tests to skip real testing*/
        KNI_ThrowNew(cardDeviceException, "stub");
        retcode = KNI_TRUE;
    }
    goto end;

err:        
#define BUFFER_SIZE 128
    buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        err_msg = "init0()";
        KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
        goto end;
    }

    switch (status) {
    case JAVACALL_NOT_IMPLEMENTED:
        if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
            err_msg = buffer;
        } else {
            err_msg = "Required property not supported";
        }
        KNI_ThrowNew(cardDeviceException, err_msg);
        break;
    case JAVACALL_OUT_OF_MEMORY:
        if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
            err_msg = buffer;
        } else {
            err_msg = "init0()";
        }
        KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
        break;
    default:
        if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
            err_msg = buffer;
        } else {
            err_msg = "Invalid internal property";
        }
        KNI_ThrowNew(cardDeviceException, err_msg);
        break;
    }
    free(buffer);

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
KNIDECL(com_sun_cardreader_PlatformCardDevice_finalize0) {
    //Global Unlock - if in native
    javacall_os_mutex_destroy(cardReaderMutex);
    javacall_os_cond_destroy(cardReaderCond);
    javacall_carddevice_finalize();
    KNI_ReturnVoid();
}

/**
 * Performs platform lock of the device.
 * <p>Java declaration:
 * <pre>
 * private native boolean lock0();
 * </pre>
 * @return KNI_TRUE in case of success, else KNI_FALSE 
 */
KNIEXPORT KNI_RETURNTYPE_INT 
KNIDECL(com_sun_cardreader_PlatformCardDevice_lock0) {
    javacall_result lock_retcode = JAVACALL_OK;
    jboolean retcode = KNI_FALSE;
    
    // Global Lock if native
    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            goto end;
        }
    });

    do { 
        lock_retcode=javacall_carddevice_lock();
        switch (lock_retcode) {
        case JAVACALL_WOULD_BLOCK:
            CVMD_gcSafeExec(_ee, {
                javacall_os_cond_wait(cardReaderCond, 0);
            });
            break;
        case JAVACALL_OK:
            javacall_carddevice_clear_error();        
            retcode = KNI_TRUE;
            break;
        default:
            break;
        }
    } while (lock_retcode == JAVACALL_WOULD_BLOCK);
    javacall_os_mutex_unlock(cardReaderMutex);

end:
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
KNIDECL(com_sun_cardreader_PlatformCardDevice_unlock0) {
    jboolean retcode = KNI_FALSE;
    
    if (javacall_carddevice_unlock() == JAVACALL_OK) {
        //GLobal Unlock here - if native
        javacall_carddevice_clear_error();
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
KNIDECL(com_sun_cardreader_PlatformCardDevice_selectSlot0) {
    jint slotIndex;
    jboolean retcode;
    
    slotIndex = KNI_GetParameterAsInt(1);
    if (javacall_carddevice_select_slot(slotIndex) != JAVACALL_OK)
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
KNIDECL(com_sun_cardreader_PlatformCardDevice_getSlotCount0) {
    jint retcode;
    javacall_int32 num;
    if (javacall_carddevice_get_slot_count(&num) == JAVACALL_OK) {
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
KNIDECL(com_sun_cardreader_PlatformCardDevice_isSatSlot0) {
    jint retcode = -1;
    javacall_bool result;
    void *context = NULL;
    javacall_result status_code;
    int slotIndex = KNI_GetParameterAsInt(1);

    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            goto end;
        }
    });

    status_code = javacall_carddevice_is_sat_start(slotIndex, &result, &context);
    while (status_code == JAVACALL_WOULD_BLOCK) {
        CVMD_gcSafeExec(_ee, {
            if (javacall_os_cond_wait(cardReaderCond, 0) == JAVACALL_OK) {
                status_code = javacall_carddevice_is_sat_finish(slotIndex, &result, context);                
            }
        });
    }
    javacall_os_mutex_unlock(cardReaderMutex);

    if (status_code == JAVACALL_OK) {
        if (result == JAVACALL_FALSE) {
            retcode = 0;
        } else {
            retcode = 1;
        }
    }

end:
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
KNIDECL(com_sun_cardreader_PlatformCardDevice_reset0) {
    jint retcode = -1;
    javacall_int32 atr_length;
    char *atr_buffer;
    void *context = NULL;
    javacall_result status_code;
    char *err_msg;

    KNI_StartHandles(1);
    KNI_DeclareHandle(atr_handle);

    KNI_GetParameterAsObject(1, atr_handle);
    if (KNI_IsNullHandle(atr_handle)) {
        atr_buffer = NULL;
        atr_length = 0;
    } else {
        atr_length = KNI_GetArrayLength(atr_handle);
        atr_buffer = (char *)malloc(atr_length);

        if (atr_buffer == NULL) {
            err_msg = "reset0()";
            KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
            goto end;
        }

        memset(atr_buffer, 0, atr_length);
        KNI_GetRawArrayRegion(atr_handle, 0, atr_length, (jbyte *)atr_buffer);
    }

    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            goto free_end;
        }
    });

    status_code = javacall_carddevice_reset_start(atr_buffer, &atr_length, &context);
    while (status_code == JAVACALL_WOULD_BLOCK) {
        CVMD_gcSafeExec(_ee, {
            if (javacall_os_cond_wait(cardReaderCond, 0) == JAVACALL_OK) {
                status_code = javacall_carddevice_reset_finish(atr_buffer, &atr_length, context);                
            }
        });        
    }
    javacall_os_mutex_unlock(cardReaderMutex);
    KNI_SetRawArrayRegion(atr_handle, 0, atr_length,(jbyte *)atr_buffer);

    if (status_code == JAVACALL_OK) {
        retcode = atr_length;
    }

free_end:
    free(atr_buffer);

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
KNIDECL(com_sun_cardreader_PlatformCardDevice_cmdXfer0) {
    jint retcode = -1;
    javacall_int32 tx_length, rx_length;
    char *tx_buffer, *rx_buffer;
    void *context = NULL;
    javacall_result status_code;
    char *err_msg;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(request_handle);
    KNI_DeclareHandle(response_handle);
    
    KNI_GetParameterAsObject(1, request_handle);
    if (KNI_IsNullHandle(request_handle)) {
        tx_buffer = NULL;
        tx_length = 0;
        goto end;
    } else {
        tx_length = KNI_GetArrayLength(request_handle);
        tx_buffer = (char *)malloc(tx_length);

        if (tx_buffer == NULL) {
            err_msg = "cmdXfer0()";
            KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
            goto end;
        }

        memset(tx_buffer, 0, tx_length);
        KNI_GetRawArrayRegion(request_handle, 0, tx_length, (jbyte *)tx_buffer);
    }
    
    KNI_GetParameterAsObject(2, response_handle);
    if (KNI_IsNullHandle(response_handle)) {
        rx_buffer = NULL;
        rx_length = 0;
        free(tx_buffer);
        goto end;
    } else {
        rx_length = KNI_GetArrayLength(response_handle);
        rx_buffer = (char *)malloc(rx_length);

        if (rx_buffer == NULL) {
            free(tx_buffer);
            err_msg = "cmdXfer0()";
            KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
            goto end;
        }

        memset(rx_buffer, 0, rx_length);
        KNI_GetRawArrayRegion(response_handle, 0, rx_length, (jbyte *)rx_buffer);
    }
    
    if (tx_length > 5) {
        jsize apdu_len = 5 + (tx_buffer[4]&0xFF) + 1;
        if (tx_length > apdu_len) {
            tx_length = apdu_len;
        }
    }

    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            goto free_end;
        }
    });

    status_code = javacall_carddevice_xfer_data_start(tx_buffer, 
                                                      tx_length, 
                                                      rx_buffer, 
                                                      &rx_length, &context);

    while (status_code == JAVACALL_WOULD_BLOCK) {
        CVMD_gcSafeExec(_ee, {
            if (javacall_os_cond_wait(cardReaderCond, 0) == JAVACALL_OK) {
                status_code = javacall_carddevice_xfer_data_finish(tx_buffer, 
                                                                   tx_length, 
                                                                   rx_buffer, 
                                                                   &rx_length, context);
            }
        });                
    }
    javacall_os_mutex_unlock(cardReaderMutex);
    KNI_SetRawArrayRegion(response_handle, 0, rx_length,(jbyte *)rx_buffer);    

    if (status_code == JAVACALL_OK) {
        retcode = rx_length;
    }

free_end:
    free(tx_buffer);
    free(rx_buffer);

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
KNIDECL(com_sun_cardreader_PlatformCardDevice_getErrorMessage0) {
    char err_msg[1024];
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(err_msg_handle);
    
    if (javacall_carddevice_get_error(err_msg, sizeof err_msg) == JAVACALL_TRUE) {
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
KNIDECL(com_sun_cardreader_PlatformCardDevice_checkCardMovement0) {
    JAVACALL_CARD_MOVEMENT mv = 0;
    jint retcode = javacall_carddevice_card_movement_events(&mv);
    if (retcode == JAVACALL_OK) {
        retcode = (mv & JAVACALL_CARD_MOVEMENT_MASK);
    }
    KNI_ReturnInt(retcode);
}

void javanotify_carddevice_event(javacall_carddevice_event event,
                                 void *context) {
    javacall_os_mutex_lock(cardReaderMutex);
    javacall_os_cond_signal(cardReaderCond);    
    javacall_os_mutex_unlock(cardReaderMutex);
    return;
}
