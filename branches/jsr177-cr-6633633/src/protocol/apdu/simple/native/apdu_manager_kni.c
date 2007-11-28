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

#include "string.h" /* memcpy() and memset() */

/** This structure represents the connection handle object. */
typedef 
    struct Java_com_sun_midp_io_j2me_apdu_Handle ConnectionHandle;

/** This structure represents the slot object. */
typedef 
    struct Java_com_sun_midp_io_j2me_apdu_Slot CardSlot;

/** Configuration exception */
static char midpCardDeviceException[] = 
    "com/sun/cardreader/CardDeviceException";

/** Configuration property name */
static char hostsandports[] = "com.sun.midp.io.j2me.apdu.hostsandports";

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
Java_com_sun_midp_io_j2me_apdu_APDUManager_init0() {
    jint retcode;
    JSR177_STATUSCODE status;
    char *err_msg;
    const char *prop_value;

    prop_value = getInternalProperty(hostsandports);
    if (prop_value != NULL) {
        status = jsr177_set_property((jbyte*)hostsandports, (jbyte*)prop_value);
        if (status == JSR177_STATUSCODE_NOT_IMPLEMENTED) {
            if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "Required property not supported";
            }
            KNI_ThrowNew(midpCardDeviceException, err_msg);
            goto end;
        }
        if (status == JSR177_STATUSCODE_OUT_OF_MEMORY) {
            if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "init0()";
            }
            KNI_ThrowNew(midpOutOfMemoryError, err_msg);
            goto end;
        }
        if (status != JSR177_STATUSCODE_OK) {
            if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "Invalid 'hostsandports' property";
            }
            KNI_ThrowNew(midpCardDeviceException, err_msg);
            goto end;
        }
    }
    status = jsr177_init();
    if (status == JSR177_STATUSCODE_NOT_IMPLEMENTED) {
        
        /* We throw special exception to tell i3tests to skip real testing */
        KNI_ThrowNew(midpCardDeviceException, "stub");
        goto end;
    }
    if (status != JSR177_STATUSCODE_OK) {
    err:
        if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
            err_msg = gKNIBuffer;
        } else {
            err_msg = "init0()";
        }
        KNI_ThrowNew(midpIOException, err_msg);
        goto end;
    }
    if (jsr177_get_slot_count(&retcode) != JSR177_STATUSCODE_OK ||
            retcode < 0) {
        goto err;
    }
    
end:
    KNI_ReturnInt(retcode);
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
Java_com_sun_midp_io_j2me_apdu_APDUManager_isSAT() {
    jboolean result;
    char *err_msg;
    int slotIndex = KNI_GetParameterAsInt(1);
    
    if (jsr177_is_sat(slotIndex, &result) != JSR177_STATUSCODE_OK) {
        if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
            err_msg = gKNIBuffer;
        } else {
            err_msg = "isSAT()";
        }
        KNI_ThrowNew(midpIOException, err_msg);
    }
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
Java_com_sun_midp_io_j2me_apdu_APDUManager_reset0() {
    MidpReentryData* info;
    void *context = NULL;
    JSR177_STATUSCODE status_code;
    CardSlot *card_slot;
    jsize atr_length;
    char *err_msg;
    // IMPL_NOTE: I assumed that maximum length of ATR is 256 bytes
    jbyte atr_buffer[256]; 
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(slot_handle);
    KNI_DeclareHandle(atr_handle);
    
    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    
    KNI_GetParameterAsObject(1, slot_handle);
    if (!KNI_IsNullHandle(slot_handle)) {
        card_slot = unhand(CardSlot,(slot_handle));
    } else {
        KNI_ThrowNew(midpNullPointerException, "Slot object is null");
        goto end;
    }
    
    atr_length = sizeof atr_buffer;
    
    if (info == NULL || info->status == SIGNAL_LOCK) {
        if (!card_slot->locked) {
            status_code = jsr177_lock();
            if (status_code == JSR177_STATUSCODE_WOULD_BLOCK) {
                midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, NULL);
                goto end;
            }
            if (status_code != JSR177_STATUSCODE_OK) {
                goto err;
            }
            card_slot->locked = KNI_TRUE;
// Since this line slot is locked
            status_code = jsr177_select_slot(card_slot->slot);
            if (status_code != JSR177_STATUSCODE_OK) {
                goto err;
            }
        }
        status_code = jsr177_reset_start(atr_buffer, &atr_length, &context);
    } else {
        context = info->pResult;
        status_code = 
            jsr177_reset_finish(atr_buffer, &atr_length, context);
    }
    if (status_code == JSR177_STATUSCODE_WOULD_BLOCK) {
        midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_RESET, context);
        goto end;
    }

    if (status_code != JSR177_STATUSCODE_OK) {
    err:
        if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
            err_msg = gKNIBuffer;
        } else {
            err_msg = "reset0()";
        }
        KNI_ThrowNew(midpIOException, err_msg);
        if (card_slot->locked) {
            status_code = jsr177_unlock(); // ignore status_code
            card_slot->locked = KNI_FALSE;
            midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, SIGNAL_LOCK);
        }
        goto end;
    }
    status_code = jsr177_is_sat(card_slot->slot, &card_slot->SIMPresent);
    if (status_code != JSR177_STATUSCODE_OK) {
        goto err;
    }
    
    card_slot->cardSessionId++;
    card_slot->powered = KNI_TRUE;
    card_slot->locked = KNI_FALSE;
    midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, SIGNAL_LOCK);
    status_code = jsr177_unlock();
    if (status_code != JSR177_STATUSCODE_OK) {
        goto err;
    }
    SNI_NewArray(SNI_BYTE_ARRAY, atr_length, atr_handle);
    memcpy(JavaByteArray(atr_handle), atr_buffer, atr_length);
end:
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
Java_com_sun_midp_io_j2me_apdu_APDUManager_exchangeAPDU0() {
    jint retcode = -1;
    MidpReentryData* info;
    void *context = NULL;
    JSR177_STATUSCODE status_code;
    JSR177_CARD_MOVEMENT movements;
    jsize tx_length, rx_length, rx_length_max;
    jbyte *tx_buffer, *rx_buffer;
    jbyte *cur;
    jbyte case_2[5];
    int Lc, Le;
    int cla, channel;
    ConnectionHandle *h;
    CardSlot *card_slot;
    char *err_msg;
    jbyte getResponseAPDU[5];
    
    KNI_StartHandles(4);
    KNI_DeclareHandle(connection_handle);
    KNI_DeclareHandle(slot_handle);
    KNI_DeclareHandle(request_handle);
    KNI_DeclareHandle(response_handle);
    
    info = (MidpReentryData*)SNI_GetReentryData(NULL);

    // If the Handle object is provided we get a Slot object from it
    KNI_GetParameterAsObject(1, connection_handle);
    if (!KNI_IsNullHandle(connection_handle)) {
        h = unhand(ConnectionHandle,(connection_handle));
        card_slot = h->cardSlot;
        if (card_slot == NULL) {
            KNI_ThrowNew(midpNullPointerException, "Slot object is null");
            goto end;
        }
    } else {
        h = NULL;
        KNI_GetParameterAsObject(2, slot_handle);
        if (KNI_IsNullHandle(slot_handle)) {
            KNI_ThrowNew(midpNullPointerException, "Handle and slot are null");
            goto end;
        }
        card_slot = unhand(CardSlot,(slot_handle));
    }
    
    KNI_GetParameterAsObject(3, request_handle);
    if (KNI_IsNullHandle(request_handle)) {
        KNI_ThrowNew(midpNullPointerException, "Request APDU is null");
        goto end;
    }
    tx_length = KNI_GetArrayLength(request_handle);
    tx_buffer = SNI_GetRawArrayPointer(request_handle);
    
    KNI_GetParameterAsObject(4, response_handle);
    if (KNI_IsNullHandle(response_handle)) {
        KNI_ThrowNew(midpNullPointerException, "Response buffer is null");
        goto end;
    }
    rx_length_max = KNI_GetArrayLength(response_handle);
    rx_buffer = SNI_GetRawArrayPointer(response_handle);
    
    if (h != NULL && 
            (!h->opened || h->cardSessionId != card_slot->cardSessionId)) {
        char *msg = "Connection closed";
        if (!card_slot->locked) {
            KNI_ThrowNew(midpIOException, msg);
            goto end;
        } else {
            KNI_ThrowNew(midpInterruptedIOException, msg);
            goto unlock_end;
        }
    }

    if (!card_slot->powered) {
        char *msg = "Card not powered up";
        if (!card_slot->locked) {
            KNI_ThrowNew(midpIOException, msg);
            goto end;
        } else {
            KNI_ThrowNew(midpInterruptedIOException, msg);
            goto unlock_end;
        }
    }
    
    if (tx_length < 4) { // invalid APDU: too short
    invalid_apdu:
        KNI_ThrowNew(midpIllegalArgumentException, "Invalid APDU");
        goto end;
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
                goto invalid_apdu;
            }
            Le = tx_buffer[5 + Lc] & 0xFF;
            if (Le == 0) {
                Le = 256;
            }
        }
    }
    
    // if APDU of case 4 has Lc=0 then we transform it to case 2
    if (tx_length > 5 && Lc == 0) {
        memcpy(case_2, tx_buffer, 4);
        case_2[4] = tx_buffer[5];
        tx_buffer = case_2;
        tx_length = 5;
    } 
    
    // trimming APDU
    if (tx_length > 5 + Lc + 1) {
        tx_length = 5 + Lc + 1;
    }

    cla = tx_buffer[0] & 0xf8; // mask channel and secure bit
    channel = cla != 0 && (cla < 0x80 || cla > 0xA0) ? 0 : tx_buffer[0] & 3;
    
    // locked slot means that we are in the middle of an exchange, 
    // otherwise we should start a data transfer
    if (!card_slot->locked) {
        card_slot->received = 0;
        cur = rx_buffer;
    } else {
        cur = rx_buffer + card_slot->received;
    }
    
    do { // infinite loop
        int sw1, sw2;

        rx_length = rx_length_max - (jint)(cur - rx_buffer);
        if (rx_length < Le + 2) { 
            err_msg = "Too long response";
            goto err_mess;
        }
        if (info == NULL || info->status == SIGNAL_LOCK) {
            if (!card_slot->locked) {
                status_code = jsr177_lock();
                if (status_code == JSR177_STATUSCODE_WOULD_BLOCK) {
                    midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, NULL);
                    goto end;
                }
                if (status_code != JSR177_STATUSCODE_OK) {
                    goto err;
                }
                card_slot->locked = KNI_TRUE;

// Since this line slot is locked
                status_code = jsr177_select_slot(card_slot->slot);
                if (status_code != JSR177_STATUSCODE_OK) {
                    goto err;
                }
            }
            status_code = 
                jsr177_xfer_data_start(tx_buffer, tx_length, 
                    cur, &rx_length, &context);
        } else {
            context = info->pResult;
            status_code = 
                jsr177_xfer_data_finish(tx_buffer, tx_length, 
                    cur, &rx_length, context);
        }
        if (jsr177_card_movement_events(&movements) == JSR177_STATUSCODE_OK) {
            if ((movements & JSR177_CARD_MOVEMENT_MASK) != 0) {
                err_msg = "Card changed";
                jsr177_set_error(err_msg);
                if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
                    err_msg = gKNIBuffer;
                }
                card_slot->powered = KNI_FALSE;
                goto interrupted;
            }
        }
        if (status_code == JSR177_STATUSCODE_WOULD_BLOCK) {
            midp_thread_wait(CARD_READER_DATA_SIGNAL, SIGNAL_XFER, context);
            card_slot->received = (jint)(cur - rx_buffer);
            goto end;
        }
        
        if (status_code != JSR177_STATUSCODE_OK) {
        err:
            if (jsr177_get_error((jbyte*)gKNIBuffer, KNI_BUFFER_SIZE)) {
                err_msg = gKNIBuffer;
            } else {
                err_msg = "exchangeAPDU0()";
            }
        err_mess:
            KNI_ThrowNew(midpIOException, err_msg);
            if (card_slot->locked) {
                status_code = jsr177_unlock(); // ignore status_code
                card_slot->locked = KNI_FALSE;
                midp_thread_signal(CARD_READER_DATA_SIGNAL, 
                                               SIGNAL_LOCK, SIGNAL_LOCK);
            }
            goto end;
        }
        if (rx_length < 2) {
            err_msg = "Response error";
            goto err_mess;
        }
        
        if (h != NULL && 
                (!h->opened || h->cardSessionId != card_slot->cardSessionId)) {
            err_msg = "Handle invalid or closed";
        interrupted:
            KNI_ThrowNew(midpInterruptedIOException, err_msg);
            goto unlock_end;
        }
        sw1 = cur[rx_length - 2] & 0xFF;
        sw2 = cur[rx_length - 1] & 0xFF;
        
        if (sw1 == 0x6C && sw2 != 0x00 && Le != 0) {
            tx_buffer[tx_length - 1] = sw2;
            Le = sw2;
            info = NULL;
            continue;
        }
        
        cur += rx_length;
        if (Le == 0 || (sw1 != 0x61 &&
            (channel != 0 || !card_slot->SIMPresent ||
             (sw1 != 0x62 && sw1 != 0x63 &&
              sw1 != 0x9E && sw1 != 0x9F)))) {
            break;
        }
        cur -= 2; // delete last SW1/SW2 from buffer
        Le = sw1 == 0x62 || sw1 == 0x63 ? 0 : sw2;
        
        memset(getResponseAPDU, 0, sizeof getResponseAPDU);
        tx_buffer = getResponseAPDU;
        tx_buffer[0] = channel;
        tx_buffer[1] = 0xC0;
        tx_buffer[4] = Le;
        if (Le == 0) {
            Le = 256;
        }
        tx_length = 5;
        info = NULL;
    } while(1);
    retcode = (jint)(cur - rx_buffer);

unlock_end:
    card_slot->locked = KNI_FALSE;
    midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_LOCK, SIGNAL_LOCK);
    status_code = jsr177_unlock(); 
    if (status_code != JSR177_STATUSCODE_OK) {
        goto err;
    }

end:
    KNI_EndHandles();
    KNI_ReturnInt(retcode);
}

