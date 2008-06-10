/*
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
#include <jsrop_exceptions.h>
#include <javacall_os.h>
#include <internal_props.h>

#include <javacall_carddevice.h>

#include "string.h" /* memcpy() and memset() */

/** Configuration exception */
static char cardDeviceException[] = 
    "com/sun/cardreader/CardDeviceException";

/** Configuration property name */
static char hostsandports[] = "com.sun.io.j2me.apdu.hostsandports";
static char satselectapdu[] = "com.sun.io.j2me.apdu.satselectapdu";

/** Thread synchronization variables */
static javacall_mutex cardReaderMutex;
static javacall_cond cardReaderCond;

#define BUFFER_SIZE 128
#define PROP_BUF_SIZE 128

/**
 * Initializes the device.
 * <p>Java declaration:
 * <pre>
 * private native static int init0() throws IOException;
 * </pre>
 * @return number of supported slots 
 * @exception CardDeviceException If configuration failed.
 * @exception IOException in case of I/O problems.
 */
KNIEXPORT KNI_RETURNTYPE_INT 
KNIDECL (com_sun_cdc_io_j2me_apdu_APDUManager_init0) {
    javacall_int32 retcode = -1;
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
        goto sync_err;
    }

    cardReaderCond = javacall_os_cond_create(cardReaderMutex);
    if (cardReaderCond == NULL) {
sync_err:
        KNI_ThrowNew(cardDeviceException, "Thread synchronization failed");
        goto end;
    }

    status = javacall_carddevice_init();
    if (status == JAVACALL_NOT_IMPLEMENTED) {
        
        /* We throw special exception to tell i3tests to skip real testing*/
        KNI_ThrowNew(cardDeviceException, "stub");
        goto end;
    }
         
    if (status != JAVACALL_OK) {
    err:
#define BUFFER_SIZE 128
        buffer = malloc(BUFFER_SIZE);
        if (buffer == NULL) {
            err_msg = "init0()";
            KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
            goto end;
        }

        if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
            err_msg = buffer;
        } else {
            err_msg = "init0()";
        }

        KNI_ThrowNew(jsropIOException, err_msg);
        free(buffer);
        goto end;
    }
    if (javacall_carddevice_get_slot_count(&retcode) != JAVACALL_OK ||
            retcode < 0) {
        goto err;
    }
    
end:
    KNI_ReturnInt((jint)retcode);
}

/**
 * Checks if this slot is SAT slot. This method is invoked once after a reset of
 * the card.
 * <p>Java declaration:
 * <pre>
 * private native static boolean isSAT(int slotNumber) throws IOException;
 * </pre>
 * @param slotNumber Slot number
 * @return <code>true</code> if the slot is dedicated for SAT,
 *         <code>false</code> if not
 * @exception IOException in case of error
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN 
KNIDECL(com_sun_cdc_io_j2me_apdu_APDUManager_isSAT) {
    javacall_bool result;
    char *err_msg;
    char *buffer;
    javacall_result status_code;
    void *context = NULL;
    jboolean slot_locked = KNI_FALSE;

    // Global lock - if in native!!!    
    int slotIndex = KNI_GetParameterAsInt(1);
    
    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            KNI_ThrowNew(jsropIOException, "Thread synchronization failed");
            goto end;
        }
    });

    do { 
        status_code=javacall_carddevice_lock();
        switch (status_code) {
        case JAVACALL_WOULD_BLOCK:
            CVMD_gcSafeExec(_ee, {
                javacall_os_cond_wait(cardReaderCond, 0);
            });
            break;
        case JAVACALL_OK:
            break;
        default:
            result = JAVACALL_FALSE;
            goto err;
        }
    } while (status_code == JAVACALL_WOULD_BLOCK);

    slot_locked = KNI_TRUE;
    
    status_code = javacall_carddevice_is_sat_start(slotIndex, &result, &context);
    while (status_code == JAVACALL_WOULD_BLOCK) {
        CVMD_gcSafeExec(_ee, {
            if (javacall_os_cond_wait(cardReaderCond, 0) == JAVACALL_OK) {
                status_code = javacall_carddevice_is_sat_finish(slotIndex, &result, context);                
            }
        });
    }

    if (status_code != JAVACALL_OK) {
    err:
        buffer = malloc(BUFFER_SIZE);
        if (buffer == NULL) {
            err_msg = "isSAT()";            
        } else {
            if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
               err_msg = buffer;
            } else {
               err_msg = "isSAT()";
            }

            free(buffer);            
        }        
        KNI_ThrowNew(jsropIOException, err_msg);
    }

    if (slot_locked) {
        javacall_carddevice_unlock();  // ignore status_code
        slot_locked = KNI_FALSE;
    }
 
    //GlobalUnlock - if in native !!!        
    javacall_os_mutex_unlock(cardReaderMutex);

end:
    KNI_ReturnInt(result);
}

/**
 * Performs reset of the card in the slot. This method must be called within
 * <tt>synchronize</tt> block with the Slot object.
 * <p>Java declaration:
 * <pre>
 * public native static byte[] reset0(Slot cardSlot) throws IOException;
 * </pre>
 * @param cardSlot Slot object
 * @return byte array with ATR
 * @exception NullPointerException if parameter is null
 * @exception IOException if any i/o troubles occured
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT 
KNIDECL(com_sun_cdc_io_j2me_apdu_APDUManager_reset0) {
    void *context = NULL;
    javacall_result status_code;
    javacall_int32 atr_length;
    char *err_msg;
    char *buffer;

    jboolean slot_locked = KNI_FALSE;
    jfieldID slot_cardSessionIdID;
    jint slot_cardSessionId;    
    jint slot_slot;    

    // IMPL_NOTE: I assumed that maximum length of ATR is 256 bytes
    char atr_buffer[256];
    
    KNI_StartHandles(3);
    KNI_DeclareHandle(slot_handle);
    KNI_DeclareHandle(atr_handle);
    KNI_DeclareHandle(slot);
    // Global lock - if in native!!!    

    KNI_GetParameterAsObject(1, slot_handle);
    if (!KNI_IsNullHandle(slot_handle)) {
        KNI_GetObjectClass(slot_handle, slot);
    } else {
        KNI_ThrowNew(jsropNullPointerException, "Slot object is null");
        goto end;
    }
    
    atr_length = sizeof atr_buffer;
    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            KNI_ThrowNew(jsropIOException, "Thread synchronization failed");
            goto end;
        }
    });

    do { 
        status_code=javacall_carddevice_lock();
        switch (status_code) {
        case JAVACALL_WOULD_BLOCK:
            CVMD_gcSafeExec(_ee, {
                javacall_os_cond_wait(cardReaderCond, 0);
            });            
            break;
        case JAVACALL_OK:
            break;
        default:
            goto err;
        }
    } while (status_code == JAVACALL_WOULD_BLOCK);

    slot_locked = KNI_TRUE;

// Since this line slot is locked
    slot_slot = KNI_GetIntField(slot_handle, KNI_GetFieldID(slot, "slot", "I"));
    status_code = javacall_carddevice_select_slot(slot_slot);
    if (status_code != JAVACALL_OK) {
        goto err;
    }
    
    status_code = javacall_carddevice_reset_start(atr_buffer, &atr_length, &context);
    
    while (status_code == JAVACALL_WOULD_BLOCK) {
        CVMD_gcSafeExec(_ee, {
            if (javacall_os_cond_wait(cardReaderCond, 0) == JAVACALL_OK) {
                status_code = javacall_carddevice_reset_finish(atr_buffer, &atr_length, context);                
            }
        });        
    }

    if (status_code != JAVACALL_OK) {
    err:
        buffer = malloc(BUFFER_SIZE);
        if (buffer == NULL) {
            err_msg = "reset0()";            
        }
        else {
            if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
               err_msg = buffer;
            } else {
               err_msg = "reset0()";
            }

            free(buffer);            
        }        
        KNI_ThrowNew(jsropIOException, err_msg);

        if (slot_locked) {
            javacall_carddevice_unlock();  // ignore status_code
            slot_locked = KNI_FALSE;
        }
        
        javacall_os_mutex_unlock(cardReaderMutex);
        goto end;
    }
    
    KNI_SetBooleanField(slot_handle, KNI_GetFieldID(slot, "powered", "Z"),
                            KNI_TRUE);
    slot_cardSessionIdID = KNI_GetFieldID(slot, "cardSessionId", "I");
    slot_cardSessionId = KNI_GetIntField(slot_handle, slot_cardSessionIdID) + 1;
    KNI_SetIntField(slot_handle, slot_cardSessionIdID, slot_cardSessionId);
    slot_locked = KNI_FALSE;
    
    status_code = javacall_carddevice_unlock();
    if (status_code != JAVACALL_OK) {
        goto err;
    }

    SNI_NewArray(SNI_BYTE_ARRAY, atr_length, atr_handle);
    KNI_SetRawArrayRegion(atr_handle, 0, atr_length, (jbyte*)atr_buffer);
    
    javacall_os_mutex_unlock(cardReaderMutex);

end:
    //GlobalUnlock - if in native !!!
    KNI_EndHandlesAndReturnObject(atr_handle);
}

/**
 * Performs data transfer to the device. This method must be called within
 * <tt>synchronize</tt> block with the Slot object.
 * <p>Java declaration:
 * <pre>
 * public native static int exchangeAPDU0(Handle h, Slot slot,
                                          byte[] request, byte[] response) 
                            throws IOException;
 * </pre>
 * @param h Connection handle. Can be null for internal purposes
 * @param slot Slot object. Unused when <tt>h</tt> is not null. 
 * Must be provided if <tt>h</tt> is null.
 * @param request Buffer with request data
 * @param response Buffer for response data
 * @return Length of response data
 * @exception NullPointerException if any parameter is null
 * @exception IllegalArgumentException if request does not contain proper APDU
 * @exception InterruptedIOException if the connection handle is suddenly closed
 * in the middle of exchange or the card was removed and inserted again
 * @exception IOException if any I/O troubles occured
 */
KNIEXPORT KNI_RETURNTYPE_INT 
KNIDECL (com_sun_cdc_io_j2me_apdu_APDUManager_exchangeAPDU0) {
    jint retcode = -1;
    void *context = NULL;
    javacall_result status_code;
    JAVACALL_CARD_MOVEMENT movements;
    javacall_int32 tx_length, tx_length_max, rx_length = 0, rx_length_max;
    char *tx_buffer, *rx_buffer;
    char *cur;
    int Lc, Le;
    int cla, channel;
    char *err_msg;
    char *buffer;

    jfieldID cardSlot_poweredID;
    jboolean cardSlot_powered;
    jboolean cardSlot_locked = KNI_FALSE;

    jboolean connection = KNI_FALSE;
    
    KNI_StartHandles(6);
    KNI_DeclareHandle(connection_handle);
    KNI_DeclareHandle(slot_handle);
    KNI_DeclareHandle(request_handle);
    KNI_DeclareHandle(response_handle);
    KNI_DeclareHandle(cardSlot);
    KNI_DeclareHandle(handle);

    //GlobalLock - if in native !!!

    // If the Handle object is provided we get a Slot object from it
    KNI_GetParameterAsObject(1, connection_handle);
    if (!KNI_IsNullHandle(connection_handle)) {
        jboolean handle_opened;
        jint handle_cardSessionId;
        jint cardSlot_cardSessionId;
        
        KNI_GetObjectClass(connection_handle, handle);
        KNI_GetObjectField(connection_handle, 
            KNI_GetFieldID(handle, "cardSlot", "Lcom/sun/cdc/io/j2me/apdu/Slot;"),
            slot_handle);
        
        if (slot_handle == NULL) {
            KNI_ThrowNew(jsropNullPointerException, "Slot object is null");
            goto end;
        }

        handle_opened = KNI_GetBooleanField(connection_handle, 
            KNI_GetFieldID(handle, "opened", "Z"));
        handle_cardSessionId = KNI_GetIntField(connection_handle, 
            KNI_GetFieldID(handle, "cardSessionId", "I"));

        KNI_GetObjectClass(slot_handle, cardSlot); 
        cardSlot_cardSessionId = KNI_GetIntField(slot_handle, 
            KNI_GetFieldID(cardSlot, "cardSessionId", "I"));

        connection = (!handle_opened || handle_cardSessionId != cardSlot_cardSessionId);
    } else {
        handle = NULL;
        KNI_GetParameterAsObject(2, slot_handle);
        if (KNI_IsNullHandle(slot_handle)) {
            KNI_ThrowNew(jsropNullPointerException, "Handle and slot are null");
            goto end;
        }
        KNI_GetObjectClass(slot_handle, cardSlot); 
    }

    cardSlot_poweredID = KNI_GetFieldID(cardSlot, "powered", "Z");
    cardSlot_powered = KNI_GetBooleanField(slot_handle, cardSlot_poweredID);
    
    KNI_GetParameterAsObject(3, request_handle);
    if (KNI_IsNullHandle(request_handle)) {
        KNI_ThrowNew(jsropNullPointerException, "Request APDU is null");
        goto end;
    }
    tx_length = KNI_GetArrayLength(request_handle);

    if (tx_length < 4) { // invalid APDU: too short
        KNI_ThrowNew(jsropIllegalArgumentException, "Invalid APDU");
        goto end;
    }

    if (tx_length == 4) {
        tx_length_max = 5;
    }
    else {
        tx_length_max = tx_length;
    }    
    tx_buffer = (char *)malloc(tx_length_max);
    if (tx_buffer == NULL) {
        err_msg = "exchangeAPDU0()";
        KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
        goto end;
    }
    memset(tx_buffer, 0, tx_length_max);
    KNI_GetRawArrayRegion(request_handle, 0, tx_length, (jbyte *)tx_buffer);
    
    KNI_GetParameterAsObject(4, response_handle);
    if (KNI_IsNullHandle(response_handle)) {
        KNI_ThrowNew(jsropNullPointerException, "Response buffer is null");
        free(tx_buffer);
        goto end;
    }
    rx_length_max = KNI_GetArrayLength(response_handle);
    rx_buffer = (char *)malloc(rx_length_max);
    if (rx_buffer == NULL) {
        free(tx_buffer);
        err_msg = "exchangeAPDU0()";
        KNI_ThrowNew(jsropOutOfMemoryError, err_msg);
        goto end;
    }
    memset(rx_buffer, 0, rx_length_max);
    KNI_GetRawArrayRegion(response_handle, 0, rx_length_max, (jbyte *)rx_buffer);
    
    if (handle != NULL && connection) {
        char *msg = "Connection closed";
        KNI_ThrowNew(jsropIOException, msg);
        goto free_end;
    }

    if (!cardSlot_powered) {
        char *msg = "Card not powered up";
        KNI_ThrowNew(jsropIOException, msg);
        goto free_end;
    }

    // trying to guess the case
    if (tx_length == 4) { // case 1
        Lc = Le = 0;
    } else {
        Lc = (tx_buffer[4]&0xFF);
        if (tx_length == 5) { // case 2
            Le = Lc;
            Lc = 0;
            if (Le == 0) {
                Le = 256;
            }
        } else if (tx_length == 5 + Lc) { // case 3
            Le = 0;
        } else { // case 4
            if (5 + Lc >= tx_length) { // invalid APDU: bad Lc field
                KNI_ThrowNew(jsropIllegalArgumentException, "Invalid APDU");
                goto free_end;
            }
            Le = tx_buffer[5 + Lc] & 0xFF;
            if (Le == 0) {
                Le = 256;
            }
        }
    }

    // if APDU of case 4 has Lc=0 then we transform it to case 2
    if (tx_length > 5 && Lc == 0) {
        tx_buffer[4] = tx_buffer[5];
        tx_length = 5;
    } 
    
    // trimming APDU
    if (tx_length > 5 + Lc + 1) {
        tx_length = 5 + Lc + 1;
    }

    cla = tx_buffer[0] & 0xf8; // mask channel and secure bit
    channel = cla != 0 && (cla < 0x80 || cla > 0xA0) ? 0 : tx_buffer[0] & 3;

    cur = rx_buffer;

    CVMD_gcSafeExec(_ee, {
        if (javacall_os_mutex_lock(cardReaderMutex) != JAVACALL_OK) {
            KNI_ThrowNew(jsropIOException, "Thread synchronization failed");
            goto free_end;
        }
    });

    do { 
        status_code=javacall_carddevice_lock();
        switch (status_code) {
        case JAVACALL_WOULD_BLOCK:
            CVMD_gcSafeExec(_ee, {
                javacall_os_cond_wait(cardReaderCond, 0);
            });            
            break;
        case JAVACALL_OK:
            break;
        default:
            goto err;
        }
    } while (status_code == JAVACALL_WOULD_BLOCK);
    cardSlot_locked = KNI_TRUE;
// Since this line slot is locked
    status_code = javacall_carddevice_select_slot(KNI_GetIntField(slot_handle,
                    KNI_GetFieldID(cardSlot, "slot", "I")));
    if (status_code != JAVACALL_OK) {
        goto err;
    }
    do { // infinite loop
        int sw1, sw2;

        rx_length = rx_length_max - (javacall_int32)(cur - rx_buffer);
        if (rx_length < Le + 2) { 
            err_msg = "Too long response";
            goto err_mess;
        }

        status_code = javacall_carddevice_xfer_data_start(tx_buffer, 
                                                          tx_length, 
                                                          cur, 
                                                          &rx_length, &context);

        if (javacall_carddevice_card_movement_events(&movements) == JAVACALL_OK) {
            if ((movements & JAVACALL_CARD_MOVEMENT_MASK) != 0) {
                err_msg = "Card changed";
                javacall_carddevice_set_error(err_msg);

                buffer = malloc(BUFFER_SIZE);

                if ((buffer != NULL) &&
                    (javacall_carddevice_get_error(buffer, BUFFER_SIZE))) {
                    err_msg = buffer;
                    free(buffer);
                }
                cardSlot_powered = KNI_FALSE;
                KNI_SetBooleanField(slot_handle, cardSlot_poweredID, cardSlot_powered);
                goto interrupted;
            }
        }

        while (status_code == JAVACALL_WOULD_BLOCK) {
            CVMD_gcSafeExec(_ee, {
                if (javacall_os_cond_wait(cardReaderCond, 0) == JAVACALL_OK) {
                    status_code = javacall_carddevice_xfer_data_finish(tx_buffer, 
                                                                       tx_length, 
                                                                       cur, 
                                                                       &rx_length, context);
                }
            });                
        }

        if (status_code != JAVACALL_OK) {
        err:
            buffer = malloc(BUFFER_SIZE);

            if (buffer == NULL) {
                err_msg = "exchangeAPDU0()";
            } else {
                if (javacall_carddevice_get_error(buffer, BUFFER_SIZE)) {
                    err_msg = buffer;                    
                } else {
                    err_msg = "exchangeAPDU0()";
                }
                free(buffer);
            }
        err_mess:
            KNI_ThrowNew(jsropIOException, err_msg);
            if (cardSlot_locked) {
                status_code = javacall_carddevice_unlock(); // ignore status_code
                cardSlot_locked = KNI_FALSE;
            }
            goto destroy_end;
        }
        if (rx_length < 2) {
            err_msg = "Response error";
            goto err_mess;
        }
        
        if (handle != NULL && connection) {
            err_msg = "Handle invalid or closed";
        interrupted:
            KNI_ThrowNew(jsropInterruptedIOException, err_msg);
            goto unlock_end;
        }
        sw1 = cur[rx_length - 2] & 0xFF;
        sw2 = cur[rx_length - 1] & 0xFF;
        
        if (sw1 == 0x6C && sw2 != 0x00 && Le != 0) {
            tx_buffer[tx_length - 1] = sw2;
            Le = sw2;
            continue;
        }
        
        cur += rx_length;
        if (Le == 0 || (sw1 != 0x61 &&
            (channel != 0 ||
            !KNI_GetBooleanField(slot_handle,
                KNI_GetFieldID(cardSlot, "SIMPresent", "Z")) ||
             (sw1 != 0x62 && sw1 != 0x63 &&
              sw1 != 0x9E && sw1 != 0x9F)))) {
            break;
        }
        cur -= 2; // delete last SW1/SW2 from buffer
        Le = sw1 == 0x62 || sw1 == 0x63 ? 0 : sw2;
        
        tx_buffer[0] = channel;
        tx_buffer[1] = 0xC0;
        tx_buffer[2] = 0;
        tx_buffer[3] = 0;
        tx_buffer[4] = Le;
        if (Le == 0) {
            Le = 256;
        }
        tx_length = 5;

    } while(1);
    retcode = (jint)(cur - rx_buffer);

unlock_end:
    cardSlot_locked = KNI_FALSE;
    status_code = javacall_carddevice_unlock(); 
    if (status_code != JAVACALL_OK) {
        goto err;
    }

destroy_end:
    javacall_os_mutex_unlock(cardReaderMutex);

free_end:
    KNI_SetRawArrayRegion(response_handle, 0, retcode,(jbyte *)rx_buffer);    
    free(tx_buffer);
    free(rx_buffer);    
    
end:
    KNI_EndHandles();
    KNI_ReturnInt(retcode);
}

void javanotify_carddevice_event(javacall_carddevice_event event,
                                 void *context) {
    javacall_os_mutex_lock(cardReaderMutex);
    javacall_os_cond_signal(cardReaderCond);    
    javacall_os_mutex_unlock(cardReaderMutex);
    return;
}
