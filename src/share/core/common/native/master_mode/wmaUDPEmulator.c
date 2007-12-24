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

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include <wmaInterface.h>
#include <wmaSocket.h>
#include <midp_properties_port.h>
#include <wmaUDPEmulator.h>
#include <bytePackUnpack.h>
#include <pcsl_memory.h>
#if ENABLE_JSR_205
#include <jsr205_mms_structs.h>
#endif
#if ENABLE_WMA_LOOPBACK
#include <jsr120_sms_structs.h>
#include <jsr120_sms_pool.h>
#include <jsr120_sms_listeners.h>
#if ENABLE_JSR_205
#include <jsr205_mms_pool.h>
#include <jsr205_mms_protocol.h>
#include <jsr205_mms_listeners.h>
#endif
#include <push_server_resource_mgmt.h>
#endif

#include <pcsl_network.h>
#include <pcsl_datagram.h>
#include <pcsl_network_notifier.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#ifndef MAX_DATAGRAM_LENGTH
#define MAX_DATAGRAM_LENGTH 1500
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SMS protocol inbound/outbound physical ports and handles.
 * These are default values. Overriden with
 * com.sun.midp.io.j2me.sms.DatagramPortIn and
 * com.sun.midp.io.j2me.sms.DatagramPortOut properties
 * (see jsr120_wma11-gate/src/config/common/properties_jsr120.xml).
 * smsInPortNumber also can be overriden with JSR_120_SMS_PORT
 * environment variable which takes precedence over value of the
 * property.
 */
static jint smsInPortNumber = 11100;
static jint smsOutPortNumber = 11101;
static void *smsHandle = NULL;

/*
 * CBS protocol inbound physical ports and handles.
 * These are default values. Overriden with
 * com.sun.midp.io.j2me.cbs.DatagramPortIn property
 * (see jsr120_wma11-gate/src/config/common/properties_jsr120.xml).
 * Also can be overriden with JSR_120_CBS_PORT
 * environment variable which takes precedence over value of the
 * property.
 * No outbound CBS port required.
 */
static jint cbsInPortNumber = 22200;
static void *cbsHandle = NULL;

#if ENABLE_JSR_205
/*
 * MMS protocol inbound/outbound physical ports and handles.
 * These are default values. Overriden with
 * com.sun.midp.io.j2me.mms.DatagramPortIn and
 * com.sun.midp.io.j2me.mms.DatagramPortOut properties
 * (see jsr205_wma20-gate/src/config/common/properties_jsr205.xml).
 * mmsInPortNumber also can be overriden with JSR_205_MMS_PORT
 * environment variable which takes precedence over value of the
 * property.
 */
static jint mmsInPortNumber = 33300;
static jint mmsOutPortNumber = 33301;
static void *mmsHandle = NULL;
#endif

/*
 * Target host name to send outbound messages to (typically the host the
 * test suite is running on). Can be overriden with
 * com.sun.midp.io.j2me.wma.DatagramHost property
 * (see jsr120_wma11-gate/src/config/common/properties_jsr120.xml).
 */
static char* targetHost = NULL;
static char* targetHostDefault = "wmadatagramhost";

/** maximum number of bytes per IP address, value per pcl_network.h */
#define MAX_IPADDR_LEN 16

typedef enum {
    JSR120_UDP_EMULATOR_CONTEXT_STATE_INIT = 0,
    JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_INPROGRESS,
    JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_DONE,
    JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_INPROGRESS,
    JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_DONE
} jsr120_udp_emulator_context_state;

typedef struct {
    jsr120_udp_emulator_context_state state;
    unsigned char ipAddr[MAX_IPADDR_LEN];
    /** The buffer for all datagram data. */
    char buffer[MAX_DATAGRAM_LENGTH];
#if ENABLE_JSR_205
    /** The current packet number. */
    jshort packetNumber;
    /** The running pointer to the data to be sent, chunk by chunk. */
    char* p;
    /** The count of bytes for the current chunk of data to be sent. */
    jint count;
    /** Remaining length of a message */
    jint remaining_length;
    /** The total length of all data to be written. */
    jint totalLength;
    /** The buffer that will contain all data to be written. */
    char* msg_buffer;
#endif
    void *pcsl_handle;
    void *pcsl_context;
    void *emulatorSocket;
} jsr120_udp_emulator_context;

/*
 * Helper functions.
 */

#if ENABLE_JSR_205
/**
 * Create an MMS message from the given parameters.
 *
 * @param fromAddress The sender's MMS address.
 * @param appID The application ID corresponding to the message.
 * @param length The total number of bytes in the message.
 * @param buffer A pointer to the message bytes.
 *
 * @return The constructed MMS message.
 */
MmsMessage* createMmsMessage(char* fromAddress, char* appID, char* replyToAppID,
    jint length, char* buffer) {

    MmsMessage* mms = NULL;

    if (buffer != NULL) {

        mms = (MmsMessage*)pcsl_mem_malloc(sizeof(MmsMessage));
        memset(mms, 0, sizeof(MmsMessage));

        /* Copy the "from" address. */
        if (fromAddress == NULL) {
            mms->fromAddress = NULL;
        } else {
            mms->fromAddress = (char*)pcsl_mem_malloc(strlen(fromAddress)+1);
            strcpy(mms->fromAddress, fromAddress);
        }

        /* Copy the application ID string. */
        mms->appID = (char*)pcsl_mem_malloc(strlen(appID)+1);
        strcpy(mms->appID, appID);

        /* Copy the reply-to application ID string. */
        if (replyToAppID == NULL) {
            mms->replyToAppID = NULL;
        } else {
            mms->replyToAppID = (char*)pcsl_mem_malloc(strlen(replyToAppID)+1);
            strcpy(mms->replyToAppID, replyToAppID);
        }

        /* Copy the message length and the message bytes. */
        mms->msgLen = length;

        mms->msgBuffer = (char*)pcsl_mem_malloc(length);
        memcpy(mms->msgBuffer, buffer, length);
    }

    return mms;
}
#endif

/**
 * Open a datagram port for the given protocol in inbound or outbound mode.
 *
 * @param protocol  A protocol type.
 * @param port  The port to be bound to the datagram traffic.
 *
 * @return <code>WMA_NET_SUCCESS<code> if the port could be opened
 *     successfully; <code>WMA_NET_IOERROR</code>, otherwise.
 */
WMA_STATUS jsr120_datagram_open(WMA_PROTOCOLS protocol, jint port) {

    jint fd;
    jint res;
    void *context;

    res = pcsl_datagram_open_start(port, (void**)(void*)&fd, &context);

    /*
     * Need revisit: handle WOULDBLOCK
     */
    if (res == PCSL_NET_SUCCESS) {

        /* Create WMASocket for receiving datagrams. */
        if (protocol == WMA_SMS_PROTOCOL) {
            smsHandle = (void *)wmaCreateSocketHandle(protocol, fd);
        } else if (protocol == WMA_CBS_PROTOCOL) {
            cbsHandle = (void *)wmaCreateSocketHandle(protocol, fd);
        } 
#if ENABLE_JSR_205
        else if (protocol == WMA_MMS_PROTOCOL) {
            mmsHandle = (void *)wmaCreateSocketHandle(protocol, fd);
        }
#endif
        else {
            void *context;

            pcsl_datagram_close_start((void*)fd, &context);

            return WMA_NET_INVALID;
        }

        pcsl_add_network_notifier((void*)fd, PCSL_NET_CHECK_READ);
        
        return WMA_NET_SUCCESS;
    }
    
    return WMA_NET_IOERROR;
}

/**
 * Shut down a protocol's inbound/outbound datagram port.
 *
 * @param protocol  A protocol type.
 *
 * @return  <code>WMA_NET_SUCCESS</code> if the datagram port could be closed
 *          successfully; <code>WMA_NET_IOERROR</code> if the protocol was
 *          unknown, or if there was a problem when closing the port.
 */
WMA_STATUS jsr120_datagram_close(WMA_PROTOCOLS protocol) {
    jint status = 0;

    /* The socket descriptor. */
    jint fd = -1;

    /* Shut down the inbound datagram port. */
    if (protocol == WMA_SMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(smsHandle);
        wmaDestroySocketHandle(smsHandle);
    } else if (protocol == WMA_CBS_PROTOCOL) {
        fd = wmaGetRawSocketFD(cbsHandle);
        wmaDestroySocketHandle(cbsHandle);
    } 
#if ENABLE_JSR_205
      else if (protocol == WMA_MMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(mmsHandle);
        wmaDestroySocketHandle(mmsHandle);
    }
#endif

    if (fd >= 0) {
        void *context;

        status = pcsl_datagram_close_start((void*)fd, &context);

        /*
         * Need revisit: handle WOULDBLOCK
         */
        if (status == PCSL_NET_SUCCESS) {

            return WMA_NET_SUCCESS;
        }
    }

    return WMA_NET_IOERROR;
}

/**
 * Common implementation between pcsl_datagram_read_start()
 * and pcsl_datagram_read_finish().
 *
 * @param protocol     A protocol type (e.g.: <code>WMA_SMS_PROTOCOL</code>).
 * @param pAddress     The network address from the datagram. 
 * @param port         The network port from the datagram.
 * @param buffer       The buffer for the bytes to be read.
 * @param length       The number of bytes to be read.
 * @param pBytesRead   The number of bytes read.
 *
 * @return  <code>WMA_NET_SUCCESS</code> if the read was successful;
 *          <code>WMA_NET_WOULDBLOCK</code> if ??;
 *          <code>WMA_NET_INTERRUPTED</code> if ??;
 *          <code>WMA_NET_IOERROR</code>, otherwise.
 */
WMA_STATUS jsr120_datagram_read(WMA_PROTOCOLS protocol,
                                   unsigned char *pAddress,
                                   jint *port,
                                   char *buffer,
                                   jint length,
                                   jint *pBytesRead)
{
    void *context;
    jint fd = -1;
    jint status;

    /* Pick up the socket descriptor for the given protocol. */
    if (protocol == WMA_SMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(smsHandle);
    } else if (protocol == WMA_CBS_PROTOCOL) {
        fd = wmaGetRawSocketFD(cbsHandle);
    } 
#if ENABLE_JSR_205
      else if (protocol == WMA_MMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(mmsHandle);
    }
#endif

    if (fd < 0) {
        /* Unknown protocol type. */
        return WMA_NET_IOERROR;
    }

    /* Read the datagram packet. */
    status = pcsl_datagram_read_start((void*)fd, pAddress, port, buffer, length, pBytesRead, &context);

    switch (status) {
    case PCSL_NET_SUCCESS:
        return WMA_NET_SUCCESS;
    case PCSL_NET_WOULDBLOCK:
        return WMA_NET_WOULDBLOCK;
    case PCSL_NET_INTERRUPTED:
        return WMA_NET_INTERRUPTED;
    default:
        return WMA_NET_IOERROR;
    }

}

#if ENABLE_JSR_205
/*
 * Write a buffer of bytes using a series of datagram packets. This is
 * MMS-specific at this time, because of the mmsOutPortNumber use.
 *
 * @param protocol The id of protocol this packet is associated with. As currently
 *     this function is MMS specific this should be WMA_MMS_PROTOCOL
 * @param outPort The datagram output port number.
 * @param handle A handle to the open network connection.
 * @param toAddr The address to which the datagrams will be sent (Unused).
 * @param length The number of bytes to be sent.
 * @param buf A pointer to the bytes to be sent.
 * @param bytesWritten A return parameter containing the number of bytes that
 *     were actually written.
 * @param pContext A pointer to preallocated jsr120_udp_emulator_context populated
 *     if the function returns WOULDBLOCK must be provided to sunsequent calls
 *     to this function related to the same operation
 */
#if ENABLE_WMA_LOOPBACK == 0
static WMA_STATUS jsr205_datagram_write(WMA_PROTOCOLS protocol, jint outPort, void* handle, char *toAddr,
    char* fromAddr, jint length, char* buf, jint *bytesWritten, jsr120_udp_emulator_context *context) {

    /** The maximum amount of data allowed within a network packet. */
    jint PACKET_MAX_SIZE = MAX_DATAGRAM_LENGTH - 10;  /* 3 shorts + 1 int */

    /** The socket descriptor that corresponds to the handle. */
    jint socket_fd;

    /** The total number of datagram packets to be sent. */
    jshort totalPackets;

    /** The pointer to buffer for all datagram data. */
    char *dgramBuffer = NULL;

    /** Number of bytes written for given packet. */
    int packetBytesWritten = 0;

    /** Return status. */
    WMA_STATUS status = WMA_NET_IOERROR;

    /** Status of low level operation. */
    int pcsl_status;

    /** The length of the IP address to be retrieved. */
    int plen = 0;

    /** The writing index into the datagram buffer. */
    jint index = 0;

    /*
     * The "to" and "from" addresses are not used in this implementation. In a
     * more sophisticated implementation that may involve more than one
     * recipient, the packets that get broadcast would need to be directed to
     * specific phones. The "to" address would then come into play and would
     * need to be for each specific phone in a phone list.
     */
    (void)toAddr;
    (void)fromAddr;

    /* Pick up the socket descriptor that corresponds to the handle. */
    socket_fd = wmaGetRawSocketFD(handle);

    switch (context->state) {
    case JSR120_UDP_EMULATOR_CONTEXT_STATE_INIT:
        /* First invocation. */
        *bytesWritten = 0;

        context->p = buf;

        context->remaining_length = length;

        /* Resolve host name as a first step. */
        pcsl_status = pcsl_network_gethostbyname_start(
            targetHost, context->ipAddr, sizeof(context->ipAddr), &plen,
            &context->pcsl_handle, &context->pcsl_context);
        switch (pcsl_status) {
        case PCSL_NET_SUCCESS:
            context->state = JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_DONE;
            status = WMA_NET_SUCCESS;
            break;
        case PCSL_NET_WOULDBLOCK:
            context->state = JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_INPROGRESS;
            /* temporarily create entry in emulator-managed sockets list */
            context->emulatorSocket =
                wmaCreateSocketHandle(protocol,
                                      (int)context->pcsl_handle);
            status = WMA_NET_WOULDBLOCK;
            break;
        case PCSL_NET_INTERRUPTED:
            status = WMA_NET_INTERRUPTED;
            break;
        default:
            status = WMA_NET_IOERROR;
        }
        break;

    case JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_INPROGRESS:
        /* Reinvocation after gethostbyname WOULDBLOCK. */
        pcsl_status = pcsl_network_gethostbyname_finish(
            context->ipAddr, sizeof(context->ipAddr), &plen,
            context->pcsl_handle, context->pcsl_context);

        if (pcsl_status != PCSL_NET_WOULDBLOCK)
            wmaDestroySocketHandle(context->emulatorSocket);

        switch (pcsl_status) {
        case PCSL_NET_SUCCESS:
            context->state = JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_DONE;
            status = WMA_NET_SUCCESS;
            break;
        case PCSL_NET_WOULDBLOCK:
            /* still cannot get host name. */
            status = WMA_NET_WOULDBLOCK;
            break;
        case PCSL_NET_INTERRUPTED:
            status = WMA_NET_INTERRUPTED;
            break;
        default:
            status = WMA_NET_IOERROR;
        }
        break;

    default:
        /* Continue with sendto operation. */
        status = WMA_NET_SUCCESS;
        break;
    }


    if (status == WMA_NET_SUCCESS) {
        /*
         * We must be in JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO state if we
         * managed to get here. So start/continue sending data
         */

        /* Compute the total number of datagrams required to send the message. */
        totalPackets =
            (short)((length + PACKET_MAX_SIZE - 1) / PACKET_MAX_SIZE);
    
        dgramBuffer = context->buffer;

        if (context->state == JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_INPROGRESS) {
            /*
             * There were sendto operation resulted in WOULDBLOCK, need to finish it
             * Appropriate data is saved in context
             */
            pcsl_status = pcsl_datagram_write_finish(
                (void*)socket_fd, context->ipAddr, outPort,
                dgramBuffer, sizeof(dgramBuffer),
                &packetBytesWritten, &context->pcsl_context);

        } else {
            pcsl_status = PCSL_NET_SUCCESS;
        }

        /*
         * Perform next and possibly all the rest of writing operations
         *  till WOULDBLOCK or error is returned from network layer
         */
        while (pcsl_status == PCSL_NET_SUCCESS) {

            if (++context->packetNumber > totalPackets)
                break;

            if (context->state == JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_INPROGRESS ||
                context->state == JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_DONE ||
                context->state == JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_DONE) {
            
                *bytesWritten += packetBytesWritten;

                /* Move the pointer past the bytes just sent */
                context->p += context->count;
                context->remaining_length -= context->count;
            }

            /* Prepare next packet and start writing it */

            /** Reset the writing index into the datagram buffer. */
            index = 0;

            /* Initialize the datagram buffer. */
            memset(dgramBuffer, 0, MAX_DATAGRAM_LENGTH);

            /* Compute the number of bytes that can be stuffed into
             * this packet. 
             */
            context->count = context->remaining_length;
            if (context->count > PACKET_MAX_SIZE) {
                context->count = PACKET_MAX_SIZE;
            }

            /* Build the buffer to be written */
            putShort(dgramBuffer, &index, context->packetNumber);
            putShort(dgramBuffer, &index, totalPackets);
            /* Needed for: count < PACKET_MAX_SIZE */
            putShort(dgramBuffer, &index, context->count);
            /* Total length of message. */
            putInt(dgramBuffer, &index, context->remaining_length);
            /* Writes count bytes, starting at p. */
            putBytes(dgramBuffer, &index, context->p, context->count);

            pcsl_status = pcsl_datagram_write_start(
                (void*)socket_fd, context->ipAddr, outPort, dgramBuffer, MAX_DATAGRAM_LENGTH,
                &packetBytesWritten, &context->pcsl_context);

            context->state = pcsl_status != PCSL_NET_WOULDBLOCK ?
                JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_DONE :
                JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_INPROGRESS;
        }
        
        switch (pcsl_status) {
        case PCSL_NET_SUCCESS:
            /* We are done */
            status = WMA_NET_SUCCESS;
            break;
        case PCSL_NET_WOULDBLOCK:
            /* cannot send right now */
            status = WMA_NET_WOULDBLOCK;
            break;
        case PCSL_NET_INTERRUPTED:
            status = WMA_NET_INTERRUPTED;
            break;
        default:
            status = WMA_NET_IOERROR;
        }
    }

    return status;
}
#endif
#endif

/**
 * Initialize string variable from environment or internal properties.
 * 
 * When environment setting is defined, variable receives the value from it.
 * Else it is initialized from property.
 *
 * @param ptrVar The variable address.
 * @param nameEnv The name of environment setting.
 * @param nameProp The name of internal property.
 */
static void load_var_char_env_prop(char **ptrVar, char *nameEnv, char *nameProp) {
    char *charName = getenv(nameEnv);
    if(charName != NULL) {
        *ptrVar = charName;
    } else {
        charName = (char*)getInternalProperty(nameProp);
        if (charName != NULL) {
            *ptrVar = charName;
        }
    }
}

/**
 * Initialize integer variable from environment or internal properties.
 * 
 * When environment setting is defined, variable receives the value from it.
 * Else it is initialized from property.
 *
 * @param ptrVar The variable address.
 * @param nameEnv The name of environment setting.
 * @param nameProp The name of internal property.
 */
static void load_var_jint_env_prop(jint *ptrVar, char *nameEnv, char *nameProp) {
    char *charName = NULL;
    load_var_char_env_prop(&charName, nameEnv, nameProp);
    *ptrVar = atoi(charName);
}

/**
 * Determine whether the given phone number is this device's phone number. 
 *
 * @param phoneList The list of phone numbers to be compared against the device's
 *     phone number. The list is preformatted and must be of the form:
 *     <code>phone1;phone2; phone3; ... ; phone_n</code>
 *
 * @return <code>WMA_NET_IOERROR</code> if the device's phone number cannot be
 *     determined. <code>1</code> if there's a match; <code>0</code>,
 *     otherwise.
 */
WMA_STATUS is_device_phone_number(char* phoneList) {

    /** Device phone number from property list. */
    char *devPhoneNumber = NULL;

    /** Pointer into the phone number list. */
    char* p = phoneList;

    /** Length of the phone list. */
    jint len = 0;

    /** Length of a phone number in the phone number list. */
    jint phoneLen = 0;

    /** Pointer to the phone number separator. */
    char* sep = NULL;

    load_var_char_env_prop(&devPhoneNumber, "JSR_120_PHONE_NUMBER",
        "com.sun.midp.io.j2me.sms.PhoneNumber");
    if (devPhoneNumber == NULL) {
        /* Cannot determine device's phone number. Fatal error. */
        return WMA_NET_IOERROR;
    }

    if (phoneList != NULL) {

        len = strlen(phoneList);
        while (p < phoneList + len) {

            /* Skip blanks. */
            if (*p == ' ') {
                p++;
                continue;
            }

            /* Get length of phone number, assuming this is the last number. */
            phoneLen = len - (p - phoneList);

            /* Extract the phone number between separators. */
            sep = strchr(p, ';');
            if (sep != NULL) {
                phoneLen = sep - p;
            }

            /* If there's a match, exit now. */
            if (strncmp(p, devPhoneNumber, phoneLen) == 0) {
                return WMA_OK;  
            }

            /* Skip this phone number and try the next one. */
            p = p + phoneLen + 1;
        }
    }

    return WMA_ERR;
}

/**
 * Gets the phone number of device
 *
 * @return The phone number of device.
 * This function returns PCSL_STRING_NULL on pcsl error.
 * Note: the caller should free return value of this function 
 * after using.
 */
pcsl_string getInternalPhoneNumber(void) {
    pcsl_string retValue = PCSL_STRING_NULL;
    const char* phoneNumber = NULL;
    pcsl_string_status status = PCSL_STRING_ERR;
    load_var_char_env_prop((char**)&phoneNumber, "JSR_120_PHONE_NUMBER",
        "com.sun.midp.io.j2me.sms.PhoneNumber");
    if (phoneNumber != NULL) {
        status = pcsl_string_convert_from_utf8((const jbyte *)phoneNumber,
            strlen(phoneNumber), &retValue);
        if (status != PCSL_STRING_OK) {
            retValue = PCSL_STRING_NULL;
        }
    }
    return retValue;
}

/**
 * Write an SMS datagram.
 */
WMA_STATUS jsr120_sms_write(jchar msgType,
                               unsigned char address[],
                               unsigned char msgBuffer[],
                               jchar msgLen,
                               jchar sourcePort,
                               jchar destPort,
                               jint *bytesWritten,
                               void **pContext) {
  
    /* Contains context of current operation including
     * the buffer that will contain all data to be written. */
    jsr120_udp_emulator_context *context = NULL;

    /** The writing index. */
    jint index = 0;

    /** Return status */
    WMA_STATUS status = WMA_NET_SUCCESS;

    /** Internal sms phone number property. */
    const char *phNum = NULL;

    /** Physical port to be used to send SMS */
    jint sendPort = smsOutPortNumber; 

    /** Time related structs, variable */
    /* Need revisit: add time code to fetch time */
    jlong timestamp = 0;

    (void)sourcePort;

    if (*pContext == NULL) {
        /* Allocate and initialize datagram buffer */
        context = (jsr120_udp_emulator_context*)pcsl_mem_malloc(sizeof(*context));

        if (context == NULL) {
            return WMA_NET_IOERROR;
        }

        memset(context, 0, sizeof(*context));

        /* Get phone number */
        load_var_char_env_prop((char**)&phNum, "JSR_120_PHONE_NUMBER",
            "com.sun.midp.io.j2me.sms.PhoneNumber");

        if (phNum == NULL) {
            /* Free resources and quit */
            pcsl_mem_free(context);
            /* Cannot determine device's phone number. Fatal error. */
            return WMA_NET_IOERROR;
        }

        *pContext = context;

        /* Check if SMS is being sent to yourself */
        if (strcmp(phNum, (char *)address) == 0) {
            /*
             * Sending SMS to yourself, swicth port to the one you're 
             * listening on.
             */
            sendPort = smsInPortNumber;
        }

        /* Populate the datagram buffer */
        putInt(context->buffer, &index, (int)msgType);
        putInt(context->buffer, &index, (int)destPort);
        putLongLong(context->buffer, &index, timestamp);
        putString(context->buffer, &index, (char *)address);
        putString(context->buffer, &index, (char *)phNum);
        putInt(context->buffer, &index, (int)msgLen);
        if (msgLen > 0) {
            putBytes(context->buffer, &index, (char *)msgBuffer, msgLen);
        }

#if ENABLE_WMA_LOOPBACK
        {
            /*
             * This code is enabled mainly for unit tests
             * that want to test send/receive without the
             * network, i.e. message to be sent is put back
             * into the message pool and received by the sender.
             */
            SmsMessage *sms = (SmsMessage *)pcsl_mem_malloc(sizeof(SmsMessage));

            (void)bytesWritten;

            if (sms != NULL) {
                memset(sms, 0, sizeof(SmsMessage));

                sms->encodingType = msgType;
                sms->destPortNum = (unsigned short)destPort;
                sms->timeStamp = timestamp;
                sms->msgAddr = pcsl_mem_strdup((char *)phNum);
                sms->msgLen = msgLen;
                if (msgLen > 0) {
                    jint i;
                    char *msg = (char*)pcsl_mem_malloc(msgLen + 1);
                    for (i = 0; i < msgLen; i++) {
                        msg[i] = msgBuffer[i];
                    }
                    msg[i] = '\0';
                    sms->msgBuffer = msg;
                }
                /* add message to pool */
                jsr120_sms_pool_add_msg(sms);
                /* Notify all listeners of the new message. */
                jsr120_sms_message_arrival_notifier(sms);

                status = WMA_NET_SUCCESS;
            } else {
                status = WMA_NET_IOERROR;
            }

        }
#else
        {
            /** The status of networking operation. */
            int pcsl_status = PCSL_NET_SUCCESS;

            /** The length of the IP address to be retrieved. */
            int plen;

            pcsl_status = pcsl_network_gethostbyname_start(
                targetHost, context->ipAddr, sizeof(context->ipAddr), &plen,
                &context->pcsl_handle, &context->pcsl_context);

            switch (pcsl_status) {
            case PCSL_NET_SUCCESS:
                context->state = JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_DONE;
                status = WMA_NET_SUCCESS;
                break;
            case PCSL_NET_WOULDBLOCK:
                context->state = JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_INPROGRESS;
                /* temporarily create entry in emulator-managed sockets list */
                context->emulatorSocket =
                    wmaCreateSocketHandle(WMA_SMS_PROTOCOL,
                                          (int)context->pcsl_handle);
                status = WMA_NET_WOULDBLOCK;
                break;
            case PCSL_NET_INTERRUPTED:
                status = WMA_NET_INTERRUPTED;
                break;
            default:
                status = WMA_NET_IOERROR;
            }
        }
#endif
    }

#if ENABLE_WMA_LOOPBACK == 0
    {
        /** The status of networking operation. */
        int pcsl_status = PCSL_NET_SUCCESS;

        /** The length of the IP address to be retrieved. */
        int plen;

        if (status == WMA_NET_SUCCESS && *pContext != NULL) {
            /* handle reinvocation or continuation. RFC: is it needed for loopback mode?  */
            context = (jsr120_udp_emulator_context*)*pContext;

            switch (context->state) {
            case JSR120_UDP_EMULATOR_CONTEXT_STATE_INIT:
                /* Cannot have these states. */
                break;

            case JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_INPROGRESS:
                /* finish gethostbyname operation */

                pcsl_status = pcsl_network_gethostbyname_finish(
                    context->ipAddr, sizeof(context->ipAddr), &plen,
                    context->pcsl_handle, context->pcsl_context);

                if (pcsl_status != PCSL_NET_WOULDBLOCK)
                    wmaDestroySocketHandle(context->emulatorSocket);

                switch (pcsl_status) {
                case PCSL_NET_SUCCESS:
                    status = WMA_NET_SUCCESS;
                    break;
                case PCSL_NET_WOULDBLOCK:
                    /* still cannot get host name. */
                    status = WMA_NET_WOULDBLOCK;
                    break;
                case PCSL_NET_INTERRUPTED:
                    status = WMA_NET_INTERRUPTED;
                    break;
                default:
                    status = WMA_NET_IOERROR;
                }

                if (status != WMA_NET_SUCCESS)
                    break;
                /* Intentional fallthrough to continue with sendto. */
            case JSR120_UDP_EMULATOR_CONTEXT_STATE_GETHOSTBYNAME_DONE:
                context->state = JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_INPROGRESS;
                pcsl_status = pcsl_datagram_write_start(
                    (void*)wmaGetRawSocketFD(smsHandle), context->ipAddr, sendPort,
                    context->buffer, sizeof(context->buffer),
                    bytesWritten, &context->pcsl_context);

                switch (pcsl_status) {
                case PCSL_NET_SUCCESS:
                    /* finish */
                    status = WMA_NET_SUCCESS;
                    break;
                case PCSL_NET_WOULDBLOCK:
                    status = WMA_NET_WOULDBLOCK;
                    break;
                case PCSL_NET_INTERRUPTED:
                    status = WMA_NET_INTERRUPTED;
                    break;
                default:
                    status = WMA_NET_IOERROR;
                }
                break;

            case JSR120_UDP_EMULATOR_CONTEXT_STATE_SENDTO_INPROGRESS:
                pcsl_status = pcsl_datagram_write_finish(
                    (void*)wmaGetRawSocketFD(smsHandle), context->ipAddr, sendPort,
                    context->buffer, sizeof(context->buffer),
                    bytesWritten, &context->pcsl_context);

                switch (pcsl_status) {
                case PCSL_NET_SUCCESS:
                    /* finish */
                    status = WMA_NET_SUCCESS;
                    break;
                case PCSL_NET_WOULDBLOCK:
                    /* still cannot send */
                    status = WMA_NET_WOULDBLOCK;
                    break;
                case PCSL_NET_INTERRUPTED:
                    status = WMA_NET_INTERRUPTED;
                    break;
                default:
                    status = WMA_NET_IOERROR;
                }
                break;

            default:
                status = WMA_NET_IOERROR;
            }
        }
    }
#endif
    if (status != WMA_NET_WOULDBLOCK) {
        /* Message was sent, so free the buffer. */
        pcsl_mem_free(context);
        *pContext = NULL;
    }
    
    return status;
}

#if ENABLE_JSR_205
/**
 * Send an MMS message.
 *
 * @param sendingToSelf <code>1</code> if sending the message to this device;
 *     <code>0</code>, otherwise.
 * @param toAddr The recipient's MMS address.
 * @param fromAddr The sender's MMS address.
 * @param appID The application ID string associated with this message.
 * @param replyToAppID The reply-to application ID string associated with this
 *     message.
 * @param msgLen The total length, in bytes, of the MMS message.
 * @param msg A pointer to the MMS message, which contains both the message
 *     header and message body structures.
 * @param bytesWritten Returns the number of bytes written after successful
 *     write operation. This is only set if this function returns
 *     WMA_NET_SUCCESS.
 * @param pContext pointer to location to save operation context for asynchronous
 *     operations
 *
 * @return WMA_NET_SUCCESS for successful write operation;\n
 *	 WMA_NET_WOULDBLOCK if the operation would block,\n
 *	 WMA_NET_INTERRUPTED for an Interrupted IO Exception\n
 *	 WMA_NET_IOERROR for all other errors.
 */
WMA_STATUS jsr205_mms_write(jint sendingToSelf, char *toAddr, char* fromAddr, 
                               char* appID, char* replyToAppID, jint msgLen, char* msg, 
                               jint *bytesWritten, void **pContext) {

    /** The writing index. */
    int index = 0;

    /** The return status. */
    WMA_STATUS status;

    jsr120_udp_emulator_context *context;

    char *buffer;

#if ENABLE_WMA_LOOPBACK
    MmsMessage* mms;

    (void)sendingToSelf;
    (void)toAddr;
#else
    if (sendingToSelf == WMA_NET_IOERROR) {
        return WMA_NET_IOERROR;
    }
#endif

    if (appID == NULL) appID = "";
    if (replyToAppID == NULL) replyToAppID = "";

    if (*pContext == NULL) {
        context = (jsr120_udp_emulator_context *)pcsl_mem_malloc(sizeof(*context));
        if (context == NULL)
            return WMA_NET_IOERROR;

        memset(context, 0, sizeof(*context));

        /*
         * Create storage for the following:
         * - Application ID and its terminator (1 byte)
         * - The 1-byte terminator for the replyToAppID, or the 1-byte
         *   special flag when replyToAppID is NULL.
         * - The message length data
         * - The message data.
         */
        context->totalLength = strlen(fromAddr) + 1 +
            strlen(appID) + 1 +
            strlen(replyToAppID) + 1 +
            sizeof(int) + msgLen;
        buffer = (char*)pcsl_mem_malloc(context->totalLength);
        if (buffer == NULL) {
            pcsl_mem_free(context);
            return WMA_NET_IOERROR;
        }

        *pContext = context;

        memset(buffer, 0, context->totalLength);
        context->msg_buffer = buffer;

        /* Populate the buffer to be written. */
        putString(buffer, &index, (char*)fromAddr);
        putString(buffer, &index, (char*)appID);
        putString(buffer, &index, (char*)replyToAppID);
        putInt(buffer, &index, msgLen);
        putBytes(buffer, &index, (char *)msg, msgLen);
    }
    else {
        context = (jsr120_udp_emulator_context*)*pContext;

        buffer = context->msg_buffer;
    }

#if ENABLE_WMA_LOOPBACK
    mms = createMmsMessage(fromAddr, appID, replyToAppID, msgLen, msg);

    /* Add the message to the pool. */
    jsr205_mms_pool_add_msg(mms);
    /* Notify all listeners of the new message. */
    jsr205_mms_message_arrival_notifier(mms);

    /* Fake the written count as well as the "sending" status. */
    *bytesWritten = context->totalLength;
    status = WMA_NET_SUCCESS;

#else
    {
        int outputPort = mmsOutPortNumber;

        /* The buffer to be written would have the reply-to address. */
        (void)replyToAppID;

        /* If talking to ourselves, redirect output to the in-port. */
        if (sendingToSelf != 0) {
            outputPort = mmsInPortNumber;
        }
        status = jsr205_datagram_write(WMA_MMS_PROTOCOL, outputPort, mmsHandle,
                                    toAddr, fromAddr, context->totalLength,
                                    buffer, bytesWritten, context);
    }
#endif

    if (status != WMA_NET_WOULDBLOCK) {
        /*
         * Message was sent or operation aborted, so free the buffer and
         * context.
         */
        pcsl_mem_free(buffer);
        pcsl_mem_free(context);
        *pContext = NULL;
    }

    return status;
}
#endif

/**
 * Initialize datagram support for the given protocol.
 */
WMA_STATUS init_jsr120() {

    WMA_STATUS status;

    /* Initialize networking required for UDP emulator */
    if (pcsl_network_init() != PCSL_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

    /* Initialize  WMA_SMS_PROTOCOL */

    /* Check for the SMS-configured datagram input port number. */
    load_var_jint_env_prop(&smsInPortNumber, "JSR_120_SMS_PORT", 
        "com.sun.midp.io.j2me.sms.DatagramPortIn");

    status = jsr120_datagram_open(WMA_SMS_PROTOCOL, smsInPortNumber);
    if (status != WMA_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

    /* Check for the SMS-configured datagram output port number. */
    load_var_jint_env_prop(&smsOutPortNumber, "JSR_120_SMS_OUT_PORT", 
        "com.sun.midp.io.j2me.sms.DatagramPortOut");

    /* Get WMA host name */
    load_var_char_env_prop(&targetHost, "JSR_120_DATAGRAM_HOST",
        "com.sun.midp.io.j2me.wma.DatagramHost");
    if (targetHost == NULL) {
        targetHost = targetHostDefault;
    }

    /* Initialize WMA_CBS_PROTOCOL */

    /* Check for the CBS-configured datagram input port number. */
    load_var_jint_env_prop(&cbsInPortNumber, "JSR_120_CBS_PORT", 
        "com.sun.midp.io.j2me.cbs.DatagramPortIn");

    status = jsr120_datagram_open(WMA_CBS_PROTOCOL, cbsInPortNumber);
    if (status != WMA_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

#if ENABLE_JSR_205
    /* Initialize WMA_MMS_PROTOCOL */

    /* Check for the MMS-configured datagram input port number. */
    load_var_jint_env_prop(&mmsInPortNumber, "JSR_205_MMS_PORT", 
        "com.sun.midp.io.j2me.mms.DatagramPortIn");

    status = jsr120_datagram_open(WMA_MMS_PROTOCOL, mmsInPortNumber);
    if (status != WMA_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

    /* Check for the MMS-configured datagram output port number. */
    load_var_jint_env_prop(&mmsOutPortNumber, "JSR_205_MMS_OUT_PORT", 
        "com.sun.midp.io.j2me.mms.DatagramPortOut");
#endif

    return WMA_NET_SUCCESS;
}

/**
 * Shuts down 
 */
void finalize_jsr120() {

    WMA_STATUS status;

    /* Shut down the inbound datagram ports. */
    status = jsr120_datagram_close(WMA_SMS_PROTOCOL);
    status = jsr120_datagram_close(WMA_CBS_PROTOCOL);
#if ENABLE_JSR_205
    status = jsr120_datagram_close(WMA_MMS_PROTOCOL);
#endif
}

#ifdef __cplusplus
}
#endif
